/*
 * Teradesk. Copyright (c)       1993, 1994, 2002  W. Klaren,
 *                                     2002, 2003  H. Robbers,
 *                         2003, 2004, 2005, 2006  Dj. Vukovic
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
#include <library.h>
#include <limits.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "lists.h"
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"
#include "filetype.h"
#include "font.h"
#include "window.h"
#include "xscncode.h"

void log_shortname( char *dest, char* appname );

LSTYPE *selitem;


/*
 * Fill-in and redraw contents of scrolled fields FTYPE1... FTYPEn
 * in the list-manipulation dialog. See lists.c 
 */

static void set_lselector(SLIDER *slider, boolean draw, XDINFO *info) 
{
	int i;
	LSTYPE *f;
	OBJECT *ob;

	for (i = 0; i < NLINES; i++)
	{
		ob = &setmask[FTYPE1 + i];

		if ((f = get_item( slider->list, i + slider->line)) == NULL)
			*ob->ob_spec.tedinfo->te_ptext = 0;
		else
			cv_fntoform(ob, 0, f->filetype );
	}

	/* Note: this will redraw the slider and also FTYPE1...FTYPEn */

	if (draw && info)
	{
		xd_drawdeep(info, FTPARENT);
		xd_drawdeep(info, FTSPAR);
	}
}



/*
 * Initiate a (FTYPE, PRGTYPE, ICONTYPE...) list-scroller slider
 */

static void ls_sl_init ( int n, SLIDER *sl, LSTYPE **list )
{
	sl->type = 1;
	sl->tree = setmask;			/* root object */
	sl->up_arrow = FTUP;		/* up-arrow object   */
	sl->down_arrow = FTDOWN;	/* down-arrow object */
	sl->slider = FTSLIDER;		/* slider object     */
	sl->sparent = FTSPAR;		/* slider parent (background) object    */
	sl->lines = NLINES;			/* number of visible lines in the box   */
	sl->n = n;					/* number of items in the list          */
	sl->line = 0;				/* index of first item shown in the box */
	sl->list = list;			/* pointer to list of items to be shown */
	sl->set_selector = set_lselector;	/* selector routine */
	sl->first = FTYPE1;			/* first object in the scrolled box     */
	sl->findsel = find_selected;

	xd_set_rbutton(setmask, FTPARENT, FTYPE1 );
	sl_init ( sl );
}


/* 
 * Find in the list an item presented by a name possibly containing
 * cwildcards; copy the name (if given) and other data (if match found)
 * to work area. If a name is given, it is stripped off its path component,
 * if there is any.
 */

boolean find_wild 
( 
	LSTYPE **list,		/* list to be searched in */ 
	char *name,			/* name to match */ 
	LSTYPE *work,		/* work area where data is copied */ 
	void *copy_func 	/* pointer to a functon to copy data */
)
{
	char 
		*filename;		/* local: pointer to name without path */

	LSTYPE 
		*p = *list;	/* start from the beginning of the list */

	void 
		(*copyf)( LSTYPE *t, LSTYPE *s ) = copy_func;

	boolean
		result = FALSE;


	if ( name )
	{					
		/* Find the last occurence of "\" and the name after it */

		filename = fn_get_name((const char *)name);

		/* Search in the list */

		while (p)
		{
			/* If match found (match name only, no path!), then */

			if (cmp_wildcard(filename, p->filetype))
			{
				/* Copy all data, using appropriate routine */

				copyf( work, p );
				result = TRUE; /* Found in the list! */
				break;
			}
			p = p->next;
		}

		/* Copy the name in any case, but beware of the length */

		log_shortname(work->filetype, filename);
	}
	else
		work->filetype[0] = 0;	/* if nonexistent, clear the name */

	return result;
}


/*
 * Find which of the scrolled item fields (FTYPE1... FTYPEn) in the
 * filetype dialog is selected. Routine returns ordinal-1 of this field
 * ( i.e. 0 for FTYPE1, 1 for FTYPE2, etc.)
 */

int find_selected(void)
{
	int object;		/* object index of filetype field */

	return ((object = xd_get_rbutton(setmask, FTPARENT)) < 0) ? 0 : object - FTYPE1;
}


/* 
 * Get from a list the pointer to an item specified by its ordinal number 
 */

LSTYPE *get_item
( 
	LSTYPE **list,		/* pointer to the pointer to list to be searched */ 
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
 * Get from a list the pointer to an item specified by its name or name+path,
 * for the search, name string is stripped of its path component;
 * this routine is NOT usable for applications list, only for filetypes, 
 * programtypes  and icontypes (applications are searched for by their 
 * full name). Also used for the list of AV-protocol clients.
 * In parameter "pos" return also the index of this item in the list
 * or -1 if not found
 */

LSTYPE *find_lsitem
(
	LSTYPE **list,	/* pointer to the pointer to list to be searched */
	char *name,		/* item name to be found */
	int *pos		/* position where item was found, or -1 */
)
{
	LSTYPE 
		*f = *list;		/* pointer to current list item */   

	char
		*n;

	*pos = -1;


	if (name)
	{
		n = fn_get_name(name);

		while (f)
		{
			*pos = *pos + 1;

			if(strcmp(n, f->filetype) == 0)
				return f;

			f = f->next;
		}
	}

	return NULL;
}


/*
 * Remove an item from the list- but this is NOT usable for the list of
 * installed applications, because in that particular case other lists 
 * must be updated too;
 * this routine is for filetypes, programtypes and icontypes
 */

void lsrem
( 
	LSTYPE **list, 		/* pointer to pointer to list of filetype items */
	LSTYPE *item		/* item to be removed */
)
{

	LSTYPE 
		*f = *list,		/* pointer to current item in the list */ 
		*prev = NULL;	/* pointer to previous item in the list */


	while (f && (f != item))
	{
		prev = f;
		f = f->next;
	}

	if ( f )
	{
		if (prev == NULL)
			*list = f->next;
		else
			prev->next = f->next;

		free(f);
	}
}


/* 
 * Clear a list of FTYPE, PRGTYPE, ICONTYPE or APPLINFO items;
 * list is cleared by always deleting the currently first item.
 * Note: beware of recursive call: when applications lists are
 * cleared, then: rem_all-->rem_appl-->rem_all-->rem in order to
 * clear documentype lists.
 * If the list is empty, nothing is done.
 */

void lsrem_all
(
	LSTYPE **list,	/* pointer to pointer to list to work on */ 
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


/*
 * A shorter form of the above, uses lsrem() only
 */

void lsrem_all_one
(
	LSTYPE **list	/* pointer to list to work on */ 
)
{
	lsrem_all(list, lsrem);
}


/* 
 * Add an item into a list; return pointer to this item 
 */

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

	if ((n = malloc_chk(size)) != NULL) 
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


/*
 * A shorter form of the above; add at end of the list only
 */

LSTYPE *lsadd_end 
( 
	LSTYPE **list,	/* pointer to the list into which the item is added */ 
	size_t size,	/* size of item */ 
	LSTYPE *pt, 	/* pointer to work (edit) area of item data to be added */
	void *copy_func	/* pointer to a function which copies the data of specific kind */ 
)
{
	return lsadd(list, size, pt, INT_MAX, copy_func);
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

	while (p)  
	{
		if ( lsadd_end( copy, size, p, copyf ) == NULL )
			return FALSE; 
		p = p->next;
	}

	return TRUE; 
}


/*  
 * Count items in a list; return number of items;
 * hopefully there will never be more than 32767 (INT_MAX) items :)
 * (there is no check for overflow anywhere)
 */

int cnt_types
(
	LSTYPE **list	/* list to be counted */
)
{
	int n = 0;
	LSTYPE *f = *list;  

	while (f)
	{
		n++;
		f = f->next;
	}

	return n;
}


/*
 * Check a list for duplicate entries, return true if OK 
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

	if ( list )
	{
		f = *list;

		/* while there is something to do... */

		while (f)
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
	}

	return TRUE;
}


/*
 * A size-saving aux. function; remove three lists
 */

void lsrem_three
(
	LSTYPE **clist,	/* pointer to array of pointers to lists */ 
	void *remfunc	/* pointer to function that removes list member */
)
{
	/* Note: this is shorter than a loop */

	lsrem_all(&clist[0], remfunc);
	lsrem_all(&clist[1], remfunc);
	lsrem_all(&clist[2], remfunc);
}


/*
 * Aux. routine for resizing the setmask dialog when convenient
 */

static void resize_dialog
(
	int dh	/* vertical size change */
)
{
	setmask[0].r.h += dh;
	setmask[FTOK].r.y += dh;
	setmask[FTCANCEL].r.y += dh;
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
 *       LS_APPL : use to set applications
 *
 * Function returns identification of pressed exit button
 *
 * Note: in "Install Application" this routine may be called recursively:
 * app_install-->list_edit-->app-dialog-->ft_dialog-->list_edit; 
 * therefore some care is taken in various places to recreate what is 
 * needed after this call. 
 */

int list_edit
(
	LS_FUNC *lsfunc,	/* pointers to item-type-specific functions used */
	LSTYPE **lists,		/* addresses of pointers to lists of items */
	int nl,				/* number of lists */
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
		*clist[3] = {NULL, NULL, NULL},	 /* pointer to copies of list1 */
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
		i,					/* counter */
		dh = 0,				/* amount of dialog resizing */
		pos = -1,			/* index of item in the list */
		pos0,				/* similar to that */
		luse,				/* local copy of use parameter */
		button = FTCANCEL,	/* index of pressed button */
		wsel,				/* true if item selected in a dir window */
		item;				/* index of item selected in a dir window */

	char
		*pname;				/* pointer to path+name of item in a dir window */

	ITMTYPE
		itype;				/* item type */


	/* In some cases make the dialog a bit smaller */

	if(use & LS_APPL)
	{
		dh = setmask[FTCHANGE].r.y - setmask[FTADD].r.y;
		resize_dialog(-dh);
	}

	/* 
	 * Has this routine been called to operate on a selection 
	 * in a window (or desktop), or just called from the menu?
	 */

	if
	( 
		( (w = selection.w) != NULL ) 			&& 
		( (item = selection.selected) >= 0 )    &&
		( ( use & (LS_DOCT | LS_SELA) ) == 0 ) 	&&
		( ( itype = itm_tgttype(w, item) ) != ITM_DRIVE )
	)
	{
		/* 
		 * Yes, item(s?) selected from a directory window. Then,
		 * get this item's path+name and type (file/folder or else).
		 * check that this is a file or a folder 
		 */

		wsel = TRUE;

		if ( use & LS_ICNT )
		{
			switch( itype )
			{
				case ITM_FILE:
					luse = use | LS_FIIC;
					list = &lists[0];
					break;
				case ITM_PREVDIR:
				case ITM_FOLDER:
					luse = use | LS_FOIC;
					list = &lists[1];
					break;
				case ITM_PROGRAM:
					luse = use | LS_PRIC;
					list = &lists[2];
					break;
				default:
					goto exitnow;
			}
		}
		else
		{
			list = &lists[0];
			luse = use;
		}

		luse = luse | LS_WSEL;

		/*
		 * Create full name; pname is allocated here, has to be free'd later.
		 * Some special considerations so that icons can be assigned to ".."
		 */

		if( ( (luse & LS_ICNT) != 0) && (itype == ITM_PREVDIR) )
			pname = strdup(prevdir);
		else
			pname = itm_fullname(w,item);

		if(pname == NULL)
			goto exitnow;

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

		copyok = TRUE;

		for ( i = 0; i < nl; i++)
		{
			if(copyok)
				copyok = copy_all( &clist[i], &lists[i], size, lsfunc->lscopy );
		}

		if ( !copyok )
		{
			lsrem_three(clist, lsfunc->lsrem);
			goto exitnow;
		}

		list = &clist[0]; /* always start from the first list */
		luse = use | LS_FIIC;

		ls_sl_init( cnt_types(list), &sl_info, list); 

		if(chk_xd_open( setmask, &info ) < 0)
		{
			lsrem_three(clist, lsfunc->lsrem);
			return FTCANCEL;
		}
	}

	/* Loop until told otherwise */

	while ( !stop )
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
			/* 
			 * dc marks a double click; relevant for filemasks and
			 * selected applications only
			 */
			button = sl_form_do(ROOT, &sl_info, &info);
 			dc = (button & 0x8000) ? TRUE : FALSE; 
			button &= 0x7FFF;
		}

		/*
		 * Note: all references to pos0 further down in the code
		 * were earlier replaced by find_selected() + sl_info.line
		 */

		pos0 = find_selected() + sl_info.line;

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

				if (lsfunc->ls_dialog(list, INT_MAX, lwork, luse | LS_ADD ) == TRUE)
				{
					/* Addition accepted */

					if ( wsel )
					{
						/* if from a window, exit after finished */
						pos = INT_MAX;
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

						pos = pos0;
						sl_info.n = cnt_types(list) + 1;
						redraw = TRUE;
						sl_set_slider(&sl_info, &info); 
					}

					/* Add an item to the temporary list here */

					curitm = lsadd( list, size, lwork, pos, lsfunc->lscopy );

					/* Maybe it didn't succeed ? */

					if(curitm == NULL)
						button = FTCANCEL;

				} /* ls_dialog ? */
				break;

			case FTDELETE: 

				/* 
		 	 	 * Delete an item from the list
		 	 	 * Note: currently this operation can't be accessed 
		 	 	 * if item is selected in a window 
		 		 */

				pos = pos0;
				if ((curitm = get_item(list, pos)) != NULL)
				{
					lsfunc->lsrem(list, curitm);
					sl_info.n = cnt_types(list);
					redraw = TRUE;
					sl_set_slider(&sl_info, &info); 
				}
				break;

			case FTCHANGE:
			
				/* Edit an existing item in the list */

				if (!wsel)
				{
					/* if from the menu */
					pos = pos0;
					curitm = get_item(list, pos);
				}

				/* 
				 * curitm is either found above or else selected from
				 * a window, copy this data to work area and edit:
				 */

				if ( curitm )
				{
					lsfunc->lscopy( lwork, curitm );

					if ( lsfunc->ls_dialog(list, pos, lwork, luse | LS_EDIT) )
					{
						lsfunc->lscopy( curitm, lwork );
						redraw = TRUE;

						if(wsel)
							button = FTOK;
					}
				}
				break;

			case FTMOVEUP:

				/* Move an item up in the list, unless it is the first one */

				pos = pos0;

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

				/* Move an item down in the list, unless it is the last one */

				pos = pos0;

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
			
				/* Select one of the items in the listbox */
							
				keep = TRUE;
				redraw = TRUE;

				if ( (use & LS_FMSK) || (use & LS_SELA) )
				{
					strcpy(setmask[FILETYPE].ob_spec.tedinfo->te_ptext, 
					       setmask[button].ob_spec.tedinfo->te_ptext);
					xd_drawthis(&info, FILETYPE);

					if (dc)
					{
						stop = TRUE;
						button = FTOK;
					}

					pos = pos0;
				}

				break;

			case ITFILES:
			case ITFOLDER:
			case ITPROGRA:
				/*
				 * The following two linew will work only if object indices
				 * for ITFILES to ITPROGRA are in sequence.
				 */
				luse = luse | (LS_FIIC << (button - ITFILES)); /* assuming buttons in a sequence */
				newlist = &clist[button - ITFILES];

				keep = TRUE;

				if ( newlist != list )
				{
					redraw = TRUE;
					list = newlist;
					ls_sl_init( cnt_types(list), &sl_info, list); 
				}
				break;

			default:
				break;

		}		/* add/delete/change or else */

		if ( !wsel )
		{
			/* Redraw fields FTYPE1... FTYPEn */

			if ( redraw || (use & LS_APPL) )
			{
				set_lselector(&sl_info, TRUE, &info);
				sl_set_slider(&sl_info, &info); 
			}

			/* Reset buttons in some cases */

			if (!keep)
				xd_drawbuttnorm(&info, button);

			/* Sometimes must redraw add/delete/change buttons */

			if ( (use & LS_APPL) && (button >= FTADD) && (button <= FTCHANGE) )
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

		free(pname);
		pname = NULL;
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
			for(i = 0; i < nl; i++)
			{
				lsrem_all(&lists[i], lsfunc->lsrem);
				lists[i] = clist[i];
			}

			/* This is specifically for selecting an app to open a file */

			if ( (use & LS_SELA) && (pos >= 0) )
				selitem = get_item(&lists[0], pos);
		}
		else
		{
			selitem = NULL;
			lsrem_three(clist, lsfunc->lsrem);
		}

		/* Close the dialog */

		xd_close(&info);
	}

	exitnow:;
	resize_dialog(dh);

	/* Return last (exit) button code */

	return button;
}