/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                                     2003  Dj. Vukovic
 *
 * This file is part of Teradesk.
 *
 * Teradesk is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Teradesk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Teradesk; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <np_aes.h>
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <xdialog.h>
#include <mint.h>

#include "desk.h"
#include "error.h"
#include "resource.h"
#include "lists.h"
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"
#include "filetype.h"
#include "font.h"
#include "window.h"
#include "xscncode.h"



/* 
 * Free memory allocated to a list item and set pointer to NULL 
 * (because in several places action is dependent on whether 
 * the pointer is NULL)
 */

void free_item( void **ptr )
{
	free(*ptr);
	*ptr = NULL;
}


/* 
 * Find in the list an item presented by a name possibly
 * containing wildcards; copy name (if given) and other data 
 * (if match found) to work area. If a name is given, 
 * it is stripped off its path component, if there is any
 */

boolean find_wild 
( 
	LSTYPE **list,		/* list to be searched in */ 
	char *name,			/* name to match */ 
	LSTYPE *work,		/* work area where data is copied */ 
	void *copy_func 	/* pointer to functon to copy data */
)
{
	char 
		*filename;		/* local: pointer to name without path */

	LSTYPE 
		*p = *list;	/* start from the beginning of the list */

	void 
		(*copyf)( LSTYPE *t, LSTYPE *s ) = copy_func;


	if ( name != NULL )
	{					
		/* Find last occurence of "\" */
	  
		if ((filename = strrchr(name, '\\')) == NULL)
			filename = name;
		else
			filename++;

		/* Search in the list */

		while (p != NULL)
		{
			/* If match found (match name only, no path!): */

			if (cmp_wildcard(filename, p->filetype) == TRUE)
			{
				/* Copy all data, using appropriate routine, then copy name */

				if ( copy_func != NULL )
					copyf( work, p );
				strsncpy (work->filetype, filename, sizeof(work->filetype));
				return TRUE;
			}
			p = p->next;
		}

		/* Not found in the list; just copy the name */

		strsncpy (work->filetype, filename, sizeof(work->filetype));
	}
	else
		work->filetype[0] = 0;

	return FALSE;
}


/*
 * Find which of the scrolled item fields (FTYPE1... FTYPEn) in the
 * filetype dialog is selected. Routine returns ordinal-1 of this field
 * ( i.e. 0 for FTYPE1, 1 for FTYPE2, etc.)
 */

int find_selected(void)
{
	int 
		object;		/* object index of filetype field */

	return ((object = xd_get_rbutton(setmask, FTPARENT)) < 0) ? 0 : object - FTYPE1;
}


/* Get from a list pointer to an item specified by its ordinal number */

LSTYPE *get_item
( 
	LSTYPE **list,		/* list to be searched */ 
	int item			/* item # */
)
{
	int 
		i = 0;			/* item counter */

	LSTYPE 
		*f = *list;		/* pointer to current list item */   


	if (item < 0 )
		return NULL;

	while ((f != NULL) && (i != item))
	{
		i++;
		f = f->next;
	}

	return f;
}


/*
 * Get from a list pointer to an item specified by its name or name+path,
 * name possibly containing wildcards; for the search, name string is
 * stripped of its path component; this routine is NOT usable 
 * for applications list, only for filetypes, programtypes 
 * and icontypes (applications are searched for by their full name)
 * in parameter "pos" return also the index of this item in the list
 * or -1 if not found
 */

LSTYPE *find_lsitem(LSTYPE **list, char *name, int *pos)
{
	LSTYPE 
		*f = *list;		/* pointer to current list item */   

	*pos = -1;

	if (name == NULL)
		return NULL;

	while (f)
	{
		*pos = *pos + 1;
		if ( cmp_wildcard( f->filetype, fn_get_name(name) ) )
			return f;
		f = f->next;
	}

	return NULL;
}


/*
 * Remove an item from the list- but this is NOT usable for the list of
 * installed applications, because in that particular case other lists 
 * must be updated too;
 * this routine is for filetypes, programtypes and icontypes
 */

void rem
( 
	LSTYPE **list, 		/* list of filetype items */
	LSTYPE *item		/* item to be removed */
)
{

	LSTYPE 
		*f = *list,		/* pointer to current item in the list */ 
		*prev;			/* pointer to previous item in the list */

	prev = NULL;

	while ((f != NULL) && (f != item))
	{
		prev = f;
		f = f->next;
	}

	if ( (f == item) && (f != NULL) )
	{
		if (prev == NULL)
			*list = f->next;
		else
			prev->next = f->next;
		free_item(&(void *)f);
	}
}


/* 
 * Clear a list of FTYPE, PRGTYPE, ICONTYPE or APPLINFO items;
 * list is cleared by always deleting the currently first item.
 * Note: beware of recursive call: when applications lists are
 * cleared, then: rem_all-->rem_appl-->rem_all-->rem in order to
 * clear documentype lists
 */

void rem_all
(
	LSTYPE **list,	/* pointer to list to work on */ 
	void *rem_func	/* pointer to function to remove specific item kind */
)
{
	LSTYPE *p;

	void (*remf)(LSTYPE **list, LSTYPE *item) = rem_func;

	if ( list != NULL )
	{
		while ( (p = *list) != NULL )
		{
	 		remf(list, p);	
		}
	}
}


/* Add an item into a list; return pointer to this item */

LSTYPE *lsadd 
( 
	LSTYPE **list,	/* pointer to the list into which the item is added */ 
	size_t size,	/* size of item */ 
	LSTYPE *pt, 	/* pointer to work (edit) area of item data to be added */
	int pos,		/* position (ordinal) in the list: where to add */ 
	void *copy_func	/* pointer to a function which copies the data of specific kind */ 
)
{
	void (*copyf)( LSTYPE *t, LSTYPE *s) = copy_func;

	LSTYPE 
		*p = *list, 	/* pointer to current item   */
		*prev,			/* pointer to previous item  */ 
		*n;				/* pointer to new-added item */

	int 
		i = 0;			/* position counter          */


	prev = NULL;

	/* Find where to insert new data */

	while ((p != NULL) && (i != pos))
	{
		prev = p;		/* remember this as previous */
		p = p->next;	/* take the next one         */
		i++;			/* count position as well    */
	}

	/* 
	 * Allocate memory and copy data 
	 * Note: as "copyf" may copy complete structure,
	 * set n->next always after copyf
	 */

	if ((n = malloc(size)) == NULL) 
		xform_error(ENSMEM);
	else
	{
		copyf( n, pt );			/* copy data from pt to n */

		if (prev == NULL)		/* if there is no previous item */
			*list = n;			/* this is the first item in the list */
		else
			prev->next = n;		/* not first, previous item points to new as next */

		n->next = p; 			/* will be NULL if there is no next entry */
	}

	/* Return pointer to added entry */

	return n;
}


/* Copy a filetype, programtype, icontype or applications list into another */

boolean copy_all
(
	LSTYPE **copy, 	/* target list to which data is copied */
	LSTYPE **list,	/* source list from which data is copied */ 
	size_t size, 	/* size of data item */
	void *copy_func	/* pointer to function to copy data of specific kind */ 
) 
{
	LSTYPE 
		*p = *list;	/* start from the beginning of the list */

	void (*copyf)(LSTYPE *target, LSTYPE *source) = copy_func;


	*copy = NULL;	/* new list isn't anywhere yet */

	/* Add to new list while there is something in the original list... */

	while (p != NULL)  
	{
		if ( lsadd( copy, size, p, END, copyf ) == NULL )
			return FALSE; 
		p = p->next;
	}

	return TRUE; 
}


/*  
 * Count items in a list; return number of items;
 * hopefully there will never be more than 32767 items :)
 * (there is no check for overflow anywhere)
 */

int cnt_types (LSTYPE **list)
{
	int n = 0;
	LSTYPE *f = *list;  

	while (f != NULL)
	{
		n++;
		f = f->next;
	}

	return n;
}


/*
 * Check list for duplicate entries, return true if OK 
 * (i.e. duplicate not found)
 * If list = NULL, do not check but always return true 
 *
 *  name = pointer to a name which is checked if it is already in the list,
 *         in a position other than "pos"
 *  pos  = position of "name" item in the list 
 */

boolean check_dup
( 
	LSTYPE **list,	/* list of filetypes */ 
	char *name,		/* name to check */ 
	int pos 		/* position of "name" in the list (i.e. skip that one) */
)
{
	int 
		i = 0;			/* item counter */

	LSTYPE 
		*f;				/* pointer to current item in the list */

	/* If there is no list to check, result is always TRUE */

	if ( list == NULL )
		return TRUE;

	f = *list;

	/* while there is something to do... */

	while (f != NULL)
	{
		if ( i != pos )
		{
			/* compare names... */

			if ( strcmp( f->filetype, name ) == 0 )
			{
				alert_printf( 1, AFILEDEF, f->filetype );
				return FALSE;
			}
		}

		i++;
		f = f->next;
	}

	return TRUE;
}


/* 
 * Manipulate dialog(s) for handling filetype, icon, programtype, 
 * and applications lists (add/delete/change assignments) 
 *
 * Some use-specific operations are handled by argument "use":
 *
 * use = LS_FMSK : use to set filemask
 *       LS_DOCT : use to set documenttypes
 *       LS_PRGT : use to set programtypes
 *       LS_ICNT : use to set icontypes
 *       LS_APPS : use to set applications
 *
 * Function returns identification of pressed exit button
 *
 * Note: in "Install Application" this routine may be called recursively:
 * app_install-->list_edit-->app-dialog-->ft_dialog-->list_edit; 
 * therefore some care is taken in various places to recreate what is 
 * needed after this call. 
 *
 */

int list_edit
(
	LS_FUNC *lsfunc,	/* pointers to item-type-specific functions used */
	LSTYPE **list1,		/* list of items */
	LSTYPE **list2,		/* another list of items (only for folders icons) */
	size_t size,		/* item size */
	LSTYPE *lwork,		/* work area for editing */
	int use				/* use identifier */
)
{
	WINDOW 
		*w;					/* Pointer to window where item is selected */

	XDINFO 
		info;				/* dialog info structure */

	SLIDER 
		sl_info;			/* sliderinfo structure */

	LSTYPE
		*clist1 = NULL,		/* pointer to a copy of list1 */
		*clist2 = NULL,		/* pointer to a copy of clist2 */
		**list = NULL,		/* pointer to currently active list of items */
		**newlist,			/* pointer to a new list (icons only) */
		*curitm,			/* pointer to current item in the list */
		*anitem;			/* temp. pointer storage for swapping places */

	boolean
		keep = FALSE,		/* true if not needed to set buttons to normal */
		stop = FALSE,		/* true to exit from the main loop */
		dc = FALSE,			/* true for a double click */
		copyok = FALSE,		/* true if copies of lists made ok */
		redraw;				/* true if need to redraw dialog */

	int 
		pos,				/* index of item in the list */
		luse,				/* local copy of use parameter */
		button,				/* index of pressed button */
		wsel,				/* true if item selected in a dir window */
		item;				/* index of item selected in a dir window */

	char
		*pname;				/* pointer to path+name of item in a dir window */

	/* 
	 * Has this routine been called to operate on a selection 
	 * in a window (or desktop), or just called from the menu?
	 */

	if ( ((w = wd_selected_wd()) != NULL) && 
	     ((item = wd_selected()) >= 0)    &&
	      !( use & LS_DOCT )
	   )
	{
		/* 
		 * Yes, item(s?) selected from a directory window. Then,
		 * get this item's path+name and type (file/folder or else).
		 * check that this is a file or a folder 
		 */
 
		wsel = TRUE;
		item = wd_selected();

		if ( use & LS_ICNT )
		{
			switch( itm_type(w, item) )
			{
				case ITM_FILE:
				case ITM_PROGRAM:
					luse = use | LS_FIIC;
					list = list1;
					break;
				case ITM_PREVDIR:
				case ITM_FOLDER:
					luse = use | LS_FOIC;
					list = list2;
					break;
				default:
					return FALSE;
			}
		}
		else
		{
			list = list1;
			luse = use;
		}

		luse = luse | LS_WSEL;

		/* Create full name; pname is allocated here, has to be free'd later */

		if ((pname = itm_fullname(w, item)) == NULL)
			return FTCANCEL;

		/* Is this item already in the list? */

		curitm = lsfunc->lsfinditem(list, pname, &pos);
	}
	else
	{
		/* 
		 * No, just activated from the menu; 
		 * in this case, a filetypes-selector dialog will be opened.
		 */

		wsel = FALSE;
		curitm = NULL;
		pname = NULL;

		/*
		 * It is needed now to make copies of list(s), because it might be
		 * possible to work on several items before accepting/quitting.
		 * If copying fails, return with a cancel
		 */

		copyok = copy_all( &clist1, list1, size, lsfunc->lscopy );
		if ( copyok && (list2 != NULL) )
			copyok = copy_all( &clist2, list2, size, lsfunc->lscopy );

		if ( !copyok )
		{
			rem_all(&clist1, lsfunc->lsrem);
			rem_all(&clist2, lsfunc->lsrem);
			return FTCANCEL;
		}

		list = &clist1; /* always start from the first list */
		luse = use | LS_FIIC;

		ls_sl_init( cnt_types(list), set_selector, &sl_info, list); 
		xd_open( setmask, &info );
	}

	/* Loop until told otherwise */

	while ( stop == FALSE )
	{
		redraw = FALSE;
		keep = FALSE;

		/* 
		 * If there has been a selection in a window, 
		 * simulate Add or Change button, depending on whether this item
		 * already exists in the list; otherwise (just activated a menu item)
		 * get real button pressed
		 */

		if ( wsel )
		{
			if ( curitm == NULL )
				button = FTADD;		/* list item not found, install new */
			else
				button = FTCHANGE;	/* list item found, edit existing */
		}
		else
		{
			button = sl_form_do(setmask, ROOT, &sl_info, &info);
 			dc = (button & 0x8000) ? TRUE : FALSE; /* relevant for filemasks only */
			button &= 0x7FFF;
		}

		/* Act upon a pressed button */

		switch (button)
		{
			case FTOK:
			case FTCANCEL:

				stop = TRUE;
				break;

			case FTADD:
	
				/*
			 	 * Add (install) a new item in the list;
			 	 * First, set appropriate default values, if needed
				 * i.e. set information existing for the list entry
				 * matching the name, possibly using wildcards
			 	 * (note: pname=NULL if not selected from a window) 
			 	 */

				lsfunc->lsitem(list, pname, luse, lwork);  /* default */

				/* If a window selection, always exit after editing */

				/* Open the appropriate dialog */

				if (lsfunc->ls_dialog(list, END, lwork, luse | LS_ADD ) == TRUE)
				{
					/* Addition accepted */

					if ( wsel )
					{
						/* if from a window, exit after finished */
						pos = END;
						button = FTOK;
					}
					else
					{
						/* 
						 * if from the menu, show list, slider, etc.
						 * note: new item is not yet added, and
						 * it can't be counted, but need to set slider
						 * properly, therefore cnt_types()+1 below
						 */
						pos = find_selected() + sl_info.line;
						sl_info.n = cnt_types(list) + 1;
						redraw = TRUE;
						sl_set_slider(setmask, &sl_info, &info); 
					}

					/* Add an item to the temporary list here */

					curitm = lsadd( list, size, lwork, pos, lsfunc->lscopy );

				} /* ls_dialog ? */
				break;

			case FTDELETE: 

				/* 
		 	 	 * Delete an item from the list
		 	 	 * Note: currently this operation can't be accessed 
		 	 	 * if item is selected in a window 
		 		 */

				pos = find_selected() + sl_info.line;

				if ((curitm = get_item(list, pos)) != NULL)
				{
					lsfunc->lsrem(list, curitm);
					sl_info.n = cnt_types(list);
					redraw = TRUE;
					sl_set_slider(setmask, &sl_info, &info); 
				}
				break;

			case FTCHANGE:
			
				/*
		 	 	 * Edit an existing item in the list
		 	 	 */

				if (!wsel)
				{
					/* if from the menu */
					pos = find_selected() + sl_info.line;
					curitm = get_item(list, pos);
				}

				/* 
				 * curitm is either found above or else selected from
				 * a window, copy this data to work area and edit:
				 */

				if ( curitm != NULL )
				{
					lsfunc->lscopy( lwork, curitm );
					if ( lsfunc->ls_dialog(list, pos, lwork, luse | LS_EDIT) )
					{
						lsfunc->lscopy( curitm, lwork );
						redraw = TRUE;
					}
				}
				break;

			case FTMOVEUP:

				/* 
				 * Move an item up in the list, unless it is the first one 
				 */

				pos = find_selected() + sl_info.line;

				if (pos > 0 && (curitm = get_item(list, pos)) != NULL)
				{
					anitem = get_item(list, pos - 1);
					anitem->next = curitm->next;
					curitm->next = anitem;

					if ( pos > 1 )
					{
						anitem = get_item(list, pos - 2);
						anitem->next = curitm;
					}
					else
						*list = curitm;

					keyfunc( &info, &sl_info, CTL_CURUP );
				}

				break;

			case FTMOVEDN:

				/*
				 * Move an item down in the list, unless it is the last one
				 */

				pos = find_selected() + sl_info.line;

				if ( (pos < (sl_info.n - 1)) && (curitm = get_item(list, pos)) != NULL)
				{

					anitem = get_item(list, pos + 1);

					curitm->next = anitem->next;
					anitem->next = curitm;

					if ( pos > 0 )
					{
						curitm = get_item(list, pos - 1);
						curitm->next = anitem;
					}
					else
						*list = anitem;

					keyfunc( &info, &sl_info, CTL_CURDOWN );

				}

				break;

			case FTYPE1:
			case FTYPE2:
			case FTYPE3:
			case FTYPE4:
							
				keep = TRUE;
				redraw = TRUE;

				if ( use & LS_FMSK )
				{
					strcpy(setmask[FILETYPE].ob_spec.tedinfo->te_ptext, 
					       setmask[button].ob_spec.tedinfo->te_ptext);
					xd_draw(&info, FILETYPE, 0);

					if (dc == TRUE)
					{
						stop = TRUE;
						button = FTOK;
					}
				}

				break;

			case ITFOLDER:
			case ITFILES:

				keep = TRUE;
				if ( button == ITFOLDER )
				{
					luse = use | LS_FOIC;
					newlist = &clist2;
				}
				else
				{
					luse = use | LS_FIIC;
					newlist = &clist1;
				}

				if ( newlist != list )
				{
					redraw = TRUE;
					list = newlist;
					ls_sl_init( cnt_types(list), set_selector, &sl_info, list); 
				}
				break;

			default:
				break;

		}		/* add/delete/change or else */

		if ( !wsel )
		{
			/* Redraw fields FTYPE1... FTYPEn */

			if ( redraw == TRUE || (use & LS_APPL) )
			{
				set_selector(&sl_info, TRUE, &info);
				sl_set_slider(setmask, &sl_info, &info); 
			}

			/* Reset buttons in some cases */

			if (!keep)
				xd_change(&info, button, NORMAL, TRUE);

			/* Sometimes must redraw add/delete/change buttons */

			if ( (use & LS_APPL) && button >= FTADD && button <= FTCHANGE )
				xd_draw( &info, TSBUTT, 1);
		}
		else			/* if selected from a window, stop immediately */
			stop = TRUE;

	}		/* loop while stop != TRUE */

	/* Final activities before exit... */

	if ( wsel )
	{
		/* 
		 * List item was selected from the window, so no lists have been 
		 * copied, just the path+name allocated has to be free'd
		 * and pointer set to NULL.
		 */

		free_item(&(void *)pname);
	}
	else
	{
		/* 
		 * Dialog has just been activated from the menu, so:
		 * if changes have been accepted, original lists will be cleared
		 * and copies used instead. If changes have been canceled
		 * copies of lists will be cleared and forgotten
		 */

		if ( button == FTOK )
		{
			rem_all(list1, lsfunc->lsrem);
			*list1 = clist1;

			if ( list2 != NULL )
			{
				rem_all(list2, lsfunc->lsrem);
				*list2 = clist2;
			}
		}
		else
		{
			rem_all(&clist1, lsfunc->lsrem);
			rem_all(&clist2, lsfunc->lsrem);
		}

		/* Close the dialog */

		xd_close(&info);
	}

	/* Return last (exit) button code */

	return button;

}