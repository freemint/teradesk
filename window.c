/* 
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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
#include <vdi.h>
#include <tos.h>
#include <stdlib.h>
#include <string.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>
#include <library.h>

#include "desk.h"
#include "error.h"
#include "events.h"
#include "resource.h"
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h"	/* before dir.h and viewer.h */
#include "dir.h"
#include "file.h"
#include "lists.h"
#include "slider.h"
#include "filetype.h"
#include "icon.h"
#include "icontype.h"
#include "copy.h"
#include "prgtype.h"
#include "viewer.h"
#include "applik.h"
#include "floppy.h"   
#include "showinfo.h"
#include "printer.h"
#include "open.h"	
#include "dragdrop.h"
#include "screen.h"
#include "xscncode.h"
#include "va.h"


static boolean in_window(WINDOW *w, int x, int y);
static boolean itm_nlist(WINDOW *w, int *n, int **list);
extern int appl_control(int ap_id, int what, void *out);

void wd_printtext(WINDOW *w);

extern boolean wd_sel_enable;
extern int aes_ctrl; /* this should probably be in xdialog.h */

char floppy;

SEL_INFO selection;

extern FONT txt_font, dir_font;

extern WINFO 
	textwindows[MAXWINDOWS],
	dirwindows[MAXWINDOWS];

extern RECT 
	dmax,
	tmax;

#if _OVSCAN	
extern long
	over;		/* identification of overscan type */
extern int
	ovrstat;	/* state of overscan */
#endif

static void desel_old(void);

extern OBJECT *icons;
void icw_draw(WINDOW *w);
RECT icwsize;
extern int aes_wfunc;

boolean 
	can_touch,
	can_iconify; 


NEWSINFO1 thisw;
FONT *cfg_font;


/*
 * Window font configuration table
 */

CfgEntry fnt_table[] =
{
	{CFG_HDR, 0, "font" },
	{CFG_BEG},
	{CFG_D,   0, "iden", &thisw.font.id	},
	{CFG_D,   0, "size", &thisw.font.size	},
	{CFG_END},
	{CFG_LAST}
};


/*
 * Window position configuration table
 */

CfgEntry positions_table[] =
{
	{CFG_HDR, 0, "pos"	},
	{CFG_BEG},
	{CFG_D, CFG_INHIB, "indx", &thisw.i }, /* window index is not essential, but accept if specified */
	{CFG_D,   0,       "xpos", &thisw.x	}, /* note: can't go off the left edge */
	{CFG_D,   0,       "ypos", &thisw.y	},
	{CFG_D,   0,       "winw", &thisw.ww	},
	{CFG_D,   0,       "winh", &thisw.wh	},
	{CFG_D,   0,       "xicw", &thisw.ix	},
	{CFG_D,   0,       "yicw", &thisw.iy	},
	{CFG_D,   0,       "wicw", &thisw.iw	},
	{CFG_D,   0,       "hicw", &thisw.ih	},
	{CFG_X,   0,       "flag", &thisw.flags	},
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configurtion of window font
 * Note: this is initialized to zero in wd_config
 */

CfgNest cfg_wdfont
{
	if ( io == CFG_SAVE )
	{
		thisw.font.id = cfg_font->id;
		thisw.font.size = cfg_font->size;
	}

	*error = handle_cfg(file, fnt_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, NULL, NULL );

	if ( io == CFG_LOAD )
	{
		if ( (*error == 0) && thisw.font.size )
			fnt_setfont(thisw.font.id, thisw.font.size, cfg_font);
	}
}


/* 
 * Ensure that a loaded window is in the screen in case of a resolution change 
 * (or if these data were loaded from a human-edited configuration file)
 * and that it has a certain minimum size.
 * Note: windows can't go off the left and upper edges.
 */

static void wrect_in_screen(RECT *info, boolean normalsize)
{
	info->x = max(0, min( info->x, screen_info.dsk.w - 32)); 
	info->y = max(0, min( info->y, screen_info.dsk.h - 32)); /* maybe better 16 pxl instead of 32 ?*/
	info->w = min( info->w, screen_info.dsk.w / screen_info.fnt_w);
	info->h = min( info->h, screen_info.dsk.h / screen_info.fnt_h);

	if (normalsize )
	{	
		info->w = max(info->w, 14);
		info->h = max(info->h, 5);
	}
}


void wd_in_screen ( WINFO *info ) 
{
	boolean normalsize = (info->flags.iconified) ? FALSE : TRUE;

	wrect_in_screen( (RECT *)(&(info->x)), normalsize);
	wrect_in_screen( (RECT *)(&(info->ix)), TRUE );
}


/*
 * Configuration of windows positions
 */

CfgNest positions
{
	WINFO *w = thisw.windows;
	int i;

	if (io == CFG_SAVE)
	{
		for (i = 0; i < MAXWINDOWS; i++)
		{
			thisw.i = i;
/*
			thisw.x = w->x;
			thisw.y = w->y;
			thisw.ww = w->w;
			thisw.wh = w->h;
*/
			memcpy(&thisw.x, &(w->x), sizeof(RECT));

			thisw.flags = w->flags;

			if ( w->flags.iconified )
			{
/*
				thisw.ix = w->ix;
				thisw.iy = w->iy;
				thisw.iw = w->iw;
				thisw.ih = w->ih;
*/
				memcpy(&thisw.ix, &(w->ix), sizeof(RECT));
			}
			else
				memset( &thisw.ix, 0, sizeof(RECT) );
	
			*error = CfgSave(file, positions_table, lvl + 1, CFGEMP);

			w++;

			if ( *error != 0 )
				break;
		}
	}
	else
	{
		memset( &thisw.x, 0, 2*sizeof(RECT) ); /* for x,y,ww,wh and ix,iy,iw,ih */
		memset( &thisw.flags, 0, sizeof(WDFLAGS) );

		*error = CfgLoad(file, positions_table, MAX_KEYLEN, lvl + 1);
		
		if ( (*error == 0) && (thisw.i < MAXWINDOWS) )
		{
			w += thisw.i;
/*
			w->x = thisw.x;
			w->y = thisw.y;
			w->w = thisw.ww;
			w->h = thisw.wh;
			w->ix = thisw.ix;
			w->iy = thisw.iy;
			w->iw = thisw.iw;
			w->ih = thisw.ih;
*/
			memcpy(&(w->x), &thisw.x, sizeof(RECT) );
			memcpy(&(w->ix), &thisw.ix, sizeof(RECT) );

			thisw.flags.resvd = 0; /* block potential junk from config file */
			w->flags = thisw.flags; 

			thisw.i++;
		}
	}
}


/*
 * Configuration table for one window type
 */

CfgEntry wtype_table[] =
{
	{CFG_HDR, 0, "*" },
	{CFG_BEG},
	{CFG_NEST,0, "font",	 cfg_wdfont },
	{CFG_NEST,0, "pos", positions	},		/* Repeating group */
	{CFG_END},
	{CFG_LAST}
};


/********************************************************************
 *																	*
 * Functions for changing the View menu.							*
 *																	*
 ********************************************************************/


/*
 * Check (mark) only one of several menu items which are in sequence;
 * n=number of items, obj = first item, i= item which is to be checked
 */

static void menu_checkone(int n, int obj, int i)
{
	int j;
	
	for ( j = 0; j < n; j++ )
		menu_icheck(menu, j + obj, (i == j) ? 1 : 0);
}


/* 
 * Set sorting order.
 * Note: it is tacitly assumed here that "sorting" 
 * menu items follow in a fixed sequence after "sort by name" and
 * that there are exactly five sorting options. 
 */


static void wd_set_sort(int type)
{
	menu_checkone( 5, MSNAME, type );
}


/* 
 * Check/mark (or otherwise) menu items for showing directory info fields 
 * It is assumed that these are in fixed order: MSHSIZ, MSHDAT, MSHTIM, MSHATT;
 * check it with order of WD_SHSIZ, WD_SHDAT, WD_SHTIM, WD_SHATT
 */

static void wd_set_fields( int fields )
{
	int i;

	for ( i = 0; i < 4; i++ )
		menu_icheck(menu, MSHSIZ + i, ( (fields & (WD_SHSIZ << i) ) ? 1 : 0));

}


/* 
 * Check/mark menu item for text/icon directory display mode.
 * It is assumed that MSHOWTXT and MSHOWICN are in sequence
 * (mode: 0=text, 1=icon) 
 */

static void wd_set_mode(int mode)
{
	menu_checkone( 2, MSHOWTXT, mode );
}



/*
 * Count open windows
 */

int wd_wcount(void)
{
	WINDOW *h = xw_first();
	int n = 0;

	while (h != NULL)
	{
		n++;
		h = h->xw_next;
	}

	return n;
}


/********************************************************************
 *																	*
 * Funkties voor het enablen en disablen van menupunten.			*
 *																	*
 ********************************************************************/

/* 
 * Funktie die menupunten enabled en disabled, die met objecten te
 * maken hebben. 
 * *w: pointer to the window in which the selection is made
 */

void itm_set_menu(WINDOW *w)
{

	WINDOW
		*wtop;				/* pointer to top window */

	int 
		n = 0,				/* number of selected items */
	    *list = NULL,		/* pointer to a list of selected items */
		wtype = 0,			/* type of selection window */
		wtoptype = 0,		/* type of the topped window */
		nwin = 0,			/* number of open windows */
	    i = 0;

	boolean 
		nonsel,				/* true if nothing selected and dir/text topped */
		topicf = FALSE,		/* true if top window is iconified */
		showinfo = FALSE,	/* true if show info is enabled */
	    ch_icon = FALSE,	/* true if chenge icon is enabled */
	    enab = FALSE,
	    enab2 = FALSE;

	char 
		drive[8];

	ITMTYPE 
		type,					/* type of an item in the list */
		type2 = ITM_FILE;		/* same */

	/*
	 * Does this (selection) window exist at all, and is there a list 
	 * of selected items? Beware: it is possible that n and list do -not- 
	 * be initialized here at all! (if the condition becomes false sooner)
	 */

	if (   w == NULL
	    || xw_exist(w) == FALSE
	    || itm_list(w, &n, &list) == FALSE
	   )
	{
		w = NULL;
		n = 0;
	}

	/* 
	 * Find out the types of this (selection) window and the top window.
	 * Also find if these windows are iconified and how many exist
	 */

	if (w)
	{
		wtype = xw_type(w);
	}

	wtop = xw_top();

	if ( wtop )
	{
		wtoptype = xw_type(wtop);
		topicf = (can_iconify) ? wtop->xw_iflag : FALSE;
	}

	nwin = wd_wcount();
	nonsel = ( !n && (wtoptype == DIR_WIND || wtoptype == TEXT_WIND) );

	/*
	 * Scan the list of selected items. If any of them is NOT a printer,
	 * trashcan or parent dir, enable show info and change icon.
	 * In fact this enables change icon always?
	 */

	while ( (i < n) &&  !( showinfo || ch_icon ) )
	{
		type = itm_type(w, list[i++]);
		if 
		(
			(type != ITM_PRINTER) && 
		    (type != ITM_TRASH) && 
		    (type != ITM_PREVDIR)
		)
			showinfo = TRUE;

		ch_icon = TRUE;
	}

	/* 
	 * Enable show info even if there is no selection, but top window
	 * is a directory window (show info of that drive then)
	 * or a text window (show info on the displayed file then)
	 */

	can_touch = showinfo;

	if ( nonsel )
	{
		can_touch = FALSE;
		showinfo = TRUE;
	}

	menu_ienable(menu, MSHOWINF, showinfo);

	/* Enable search only if show info is enabled and something is selected */

	menu_ienable(menu, MSEARCH, (showinfo == TRUE && n > 0 ) ? 1 : 0);

	/* 
	 * New dir is OK only if a dir window is topped and not iconified, 
	 * and either nothing is selected or a single item is selected and 
	 * a link can be created (assuming that mint +non-tos filesystem
	 * always mean that a link can be created; it should be better
	 * checked with dpathconf!!! 
	 */

	if 
	( 
		(wtoptype == DIR_WIND && !topicf)  &&
		( 
			n == 0
#if _MINT_
 			|| (mint && n == 1 /*  better without this! && ((DIR_WINDOW *)wtop)->fs_type == 0 */ )  
#endif
		)
	)
		enab = TRUE;

	menu_ienable(menu, MNEWDIR, enab);

	/*
	 * "enab" will be set if there are selected items and
	 * all of the selected items are files, folders or programs.
	 * "enab2" (for printing) will be reset if any item is not a file.
	 */

	enab = (n > 0); 
	enab2 = enab;		/* will always become false sooner then enab */

	i = 0;
	while ( (i < n) && enab )
	{
		type = itm_type(w, list[i++]);
		enab = (   type == ITM_FILE
		       || type == ITM_FOLDER
		       || type == ITM_PROGRAM
		       );  
		enab2 = (type == ITM_FILE);
	}	

	/*
 	 * enab2 (for printing) will also be set if a dir or text window is topped 
	 * (and not iconified) and nothing is selected
	 */

	if ( nonsel && !topicf )
		enab2 = 1;
  
	/*
	 * Enable delete only if there are only files, programs and 
	 * folders among selected items;
	 * Enable print only if there are only files among selected items
	 * or a window is open (to print directory or text)
	 */

	menu_ienable(menu, MDELETE, enab);
	menu_ienable(menu, MPRINT, enab2);

	/* Item type is the type of the first item in the list of selected ones */

	if (n >= 1)
		type = itm_type(w, list[0]);
	if (n >= 2)
		type2 = itm_type(w, list[1]);

	/* Compare will be enabled only if there are only one or two files selected */

	if ( (n == 1 || n == 2) &&
	     (type == ITM_FILE || type == ITM_PROGRAM) &&
	     (type2 == ITM_FILE || type2 == ITM_PROGRAM) 
	   ) 
	{
		enab = TRUE;
		if ( type == ITM_PROGRAM )
			enab2 = TRUE;
	}
	else
	{
		enab = FALSE;
		enab2 = FALSE;
	}

	menu_ienable(menu, MCOMPARE, enab);

	/* Select all is possible only on directory or desk window */

	menu_ienable(menu, MSELALL, (int)( (wtoptype == DIR_WIND && !topicf) || wtoptype == DESK_WIND ));

	/* Enable window closing if there are open windows */

	menu_ienable(menu, MCLOSE, nwin);
	menu_ienable(menu, MCLOSEW, nwin);

	/* Enable duplication of dir and text windows */

	menu_ienable(menu, MDUPLIC, (int)(wtoptype == DIR_WIND || wtoptype == TEXT_WIND) );

	/* Enable window cycling if there is more than one window open */

	menu_ienable(menu, MCYCLE, (nwin > 1) ? 1 : 0 );

	/* 
	 * Enable setting of applications and program types only for
	 * appropriate selected item types (see above)
	 */

	if ( n > 1 )
	{
		enab = FALSE;
		enab2 = FALSE;
	}
	else if ( n == 0 )
	{
		enab = TRUE;
		enab2 = TRUE;
	}

	menu_ienable(menu, MPRGOPT, enab);
	menu_ienable(menu, MAPPLIK, enab2);

	/* Enable setting of window icons */

	menu_ienable(menu, MIWDICN, (n < 2) ? 1 : 0 );

  	/*
	 * enable disk formatting and disk copying
	 * only if single drive is selected and it is A or B;
	 * use the opportunity to say which drive is to be used 
	 * (is it always uppercase A or B, even in other filesystems ?)
	 */

	enab=FALSE;
	if ( (n == 1) && (type == ITM_DRIVE) )
	{
		char *fullname = itm_fullname ( w, list[0] );
		strsncpy ( drive, fullname , sizeof(drive) );
		free(fullname);
		drive[0] &= 0xDF; /* to uppercase */
		if (   ( drive[0] >= 'A' )
		    && ( drive[0] <= 'B' )
		    && ( drive[1] == ':' )
		   )
		{  
			floppy = drive[0];      /* i.e. floppy= 65dec or 66dec */
			enab = TRUE;
		}  
	}

/* By removing floppy formatting/copying from the resource, this will be deleted */

#if MFFORMAT
	menu_ienable(menu, MFCOPY, enab );
	menu_ienable(menu, MFFORMAT, enab );
#endif


	/* Enable removal of desk icons */

/*
	if ((ch_icon == TRUE) && (wtype == DESK_WIND))
		menu_ienable(menu, MREMICON, 1);
	else
		menu_ienable(menu, MREMICON, 0);
*/
	menu_ienable(menu, MREMICON, ((ch_icon == TRUE) && (wtype == DESK_WIND)) ? 1 : 0 );


	/* How many items were seelcted */

	selection.w = (n > 0) ? w : NULL;
	selection.selected = (n == 1) ? list[0] : -1;
	selection.n = n;

	free(list);
}


/*
 * DjV 111: What this routine does is also set in itm_set_menu,
 * but below is faster
 */

void wd_setselection(WINDOW *w)
{
	int
		n,
		*list = NULL;

	if (   w == NULL
	    || xw_exist(w) == FALSE
	    || itm_list(w, &n, &list) == FALSE
	   )
	{
		n = 0;
	}

	selection.w = (n > 0) ? w : NULL;
	selection.selected = (n == 1) ? list[0] : -1;
	selection.n = n;
	free(list);
}


/********************************************************************
 *																	*
 * Funkties voor het deselecteren van objecten in windows.			*
 *																	*
 ********************************************************************/

void wd_deselect_all(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->itm_select(w, -1, 0, FALSE);
		w = w->xw_next;
	}
	((ITM_WINDOW *) desk_window)->itm_func->itm_select(desk_window, -1, 0, TRUE);
	wd_setselection(NULL); 
}


/* 
 * Funktie die aangeroepen moet worden, als door een andere oorzaak
 * dan het met de muis selekteren of deselekteren van objecten,
 * objecten gedeselekteerd worden. 
 */

void wd_reset(WINDOW *w)
{
	if (w)
	{
		if (selection.w == w)
			wd_setselection(w); 
	}
	else
	{
		if (xw_exist(selection.w) == 0)
			wd_setselection(NULL); 
	}
}


/********************************************************************
 *																	*
 * Funktie voor het opvragen van het pad van een window.			*
 *																	*
 ********************************************************************/


/*
 * Return pointer to the string containing path of the topped directory window ?
 */

const char *wd_toppath(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			return ((ITM_WINDOW *) w)->itm_func->wd_path(w);
		w = w->xw_next;
	}

	return NULL;
}


/* 
 * Retur pointer to the string containing path of a directory window
 */

const char *wd_path(WINDOW *w)
{
	if (xw_type(w) == DIR_WIND)
		return (((ITM_WINDOW *) w)->itm_func->wd_path) (w);
	else
		return NULL;
}


/********************************************************************
 *																	*
 * Funkties voor het verversen van windows als er een file gewist	*
 * of gekopieerd is.												*
 *																	*
 ********************************************************************/


void wd_set_update(wd_upd_type type, const char *fname1, const char *fname2)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_set_update(w, type, fname1, fname2);
		w = w->xw_next;
	}

	((ITM_WINDOW *) desk_window)->itm_func->wd_set_update(desk_window, type, fname1, fname2);

	app_update(type, fname1, fname2);
}


void wd_do_update(void)
{
	WINDOW *w = xw_first();


	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_do_update(w);
		w = w->xw_next;
	}

	((ITM_WINDOW *) desk_window)->itm_func->wd_do_update(desk_window);
}


void wd_update_drv(int drive)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
		{

			if (drive == -1)
				/* update all */
				((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
			else
			{			
				const char *path = ((ITM_WINDOW *) w)->itm_func->wd_path(w);

				boolean goodpath = (((path[0] & 0xDF - 'A') == drive) && (path[1] == ':')) ? TRUE : FALSE;
#if _MINT_
				if (mint)
				{
					if ((drive == ('U' - 'A')) && goodpath )
						((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
					else
					{
						XATTR attr;
	
						if ((x_attr(0, path, &attr) == 0) && (attr.dev == drive)) /* follow the link */
							((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
					}
				}
				else
#endif
				{
					if ( goodpath )
						((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
				}
			}

		}
		w = w->xw_next;
	}

	wd_do_update();
}


/*
 * Close a directory or a text wndow
 */

static void wd_type_close( WINDOW *w, int mode)
{
	switch(xw_type(w))
	{
		case DIR_WIND :
			dir_close(w, mode);
			break;
		case TEXT_WIND :
			txt_closed(w);
			break;
		case ACC_WIND:
			va_close(w);
			break;
	}
}


/*
 * Delete all windows EXCEPT pseudowindows opened through AV-protocol
 */

void wd_del_all(void)
{
	WINDOW *w = xw_last();

	while (w)
	{
		if ( w->xw_type != ACC_WIND )
		{
			wd_type_close(w, 1);
			w = w->xw_prev;
		}
	}
}


/********************************************************************
 *																	*
 * Funktie voor het afhandelen van menu events.						*
 *																	*
 ********************************************************************/


/* 
 * Set icon or text display of directory windows 
 */

static void wd_mode(int mode)
{
	WINDOW *w = xw_first();
	DIR_WINDOW *dw;	
	int i;

	if ( mode != options.mode )
	{
		options.mode = mode;

		while (w)
		{
			if (xw_type(w) == DIR_WIND)	
			{								
				if ( mode )			
				{
					/* This can take a long time, change mouse form to busy */

					graf_mouse(HOURGLASS, NULL);
					dw = (DIR_WINDOW *)w;
			
					for ( i = 0; i < dw->nfiles; i++ )
					{
						NDTA *d = (*dw->buffer)[i];
/*
						d->icon = icnt_geticon( d->name, d->item_type, d->link );
*/
						d->icon = icnt_geticon( d->name, d->tgt_type, d->link );
					}
					graf_mouse(ARROW, NULL);
				}

				((ITM_WINDOW *) w)->itm_func->wd_disp_mode(w, mode);
			}
			w = w->xw_next;
		}

		/* mark (check) appropriate menu entry */

		wd_set_mode(mode);
	}
}


/* 
 * Sort a directory according to selected key (name, size, date...) 
 */

static void wd_sort(int sort)
{
	WINDOW *w = xw_first();

	options.sort = sort; /* save this in options */

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_sort(w, sort);
		w = w->xw_next;
	}

	wd_set_sort(sort);	/* mark a menu item */
}


/* 
 * Set visible directory fields in a window 
 */

void wd_fields(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_fields(w, options.V2_2.fields);
		w = w->xw_next;
	}
	wd_set_fields(options.V2_2.fields);
}


void wd_seticons(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_seticons(w);
		w = w->xw_next;
	}
}



/*
 * Print contents of fhe file which is in the topped text window "w". 
 * Simulate enough of a dir window to use existing routine(s)
 * in order to manage printing complete with print-confirmation
 * dialog, etc.
 */

static void wd_printtext(WINDOW *w)
{

	DIR_WINDOW 
		*dw;		/* simulated dir window with one file selected */

	TXT_WINDOW 
		*tw	= (TXT_WINDOW *)w; /* cast of the topped window */ 

	LNAME
		filename,	/* name of the text file in wndow  */ 
		path;		/* path of the text file in window */

	int 
		list = 0; 	/* list of the ordinals of files to print- just one file */


	/* Determine name into filename proper and path */

	split_path(path, filename, tw->name); 

	/* Simulate that this file is selected in a dir window */

	dir_simw(&dw, path, filename, ITM_FILE, tw->size, FA_ARCHIVE );

	/* Use an existing routine to print a file selected in dir window */

	itmlist_op( (WINDOW *)dw, dw->nfiles, &list, NULL, CMD_PRINT);
}



/* 
 * Handle some of the main menu activities 
 * (the rest is covered in main.c).
 * Checking of window existence is not needed in some cases, because
 * thos menu items would have been already disabled otherwise 
 */

void wd_hndlmenu(int item, int keystate)
{
	WINDOW 
		*w, 		/* window in which a selection is made */
		*wtop;		/* top wiondow */

	int 
		n = 0,		/* number of items selected */ 
		*list,		/* pointer to list of selected items indexes */
		wtoptype = 0, 
		object;

	char
		*toppath = NULL;

	w = selection.w;

	wtop = xw_top();
	if ( wtop )
		wtoptype = xw_type(wtop);

	switch (item)
	{
	case MOPEN:
		object = selection.selected;
		if ((w != NULL) && (object >= 0))
		{
			if (itm_open(w, object, keystate) == TRUE)
				itm_select(w, object, 2, TRUE);
			wd_setselection(w); 
		}
		else
			item_open( NULL,  0, 0, NULL, NULL );	/* If nothing else is selected, open form to enter item specification */
		break;
	case MSHOWINF:
	{
		WINDOW *ww = NULL;
		char *thepath = NULL, *thename; 

		if ( w )
			ww = w;
		else
		{
			ITMTYPE itype = 0;
			const char *thisname;

 			if ( wtoptype == DIR_WIND )
			{
				thepath = strdup(wd_toppath());
				thepath[3] = 0;
				thename = ".";
				itype = ITM_DRIVE;
			}
			else if ( wtoptype == TEXT_WIND )
			{
				thisname = ((TXT_WINDOW *)wtop)->name;
				thepath = fn_get_path( thisname );
				thename = fn_get_name( thisname );
				itype = ITM_FILE;
			}

			if ( itype )
				dir_simw(&((DIR_WINDOW *)ww), thepath, thename, itype, 0, 0);
		}
		
		if ( ww && (itm_nlist(ww, &n, &list) == TRUE) )
		{
			((ITM_WINDOW *)ww)->itm_func->itm_showinfo(ww, n, list, NULL);
			free(list);
		}
		free(thepath);
	}
	break;
	case MNEWDIR:

#if _MINT_
		if (
				mint && 
				(w != NULL) && 
				(itm_list(w, &n, &list) == TRUE) && 
				(n == 1) &&
				(wtoptype == DIR_WIND) &&
				(((DIR_WINDOW *)wtop)->fs_type == 0)
			)
		{
			/*
			 * It is assumed here that if fs is not tos, links can always
			 * be created in it- which in fact may not always be true!!!
			 */
			char *target = itm_fullname(w, list[0]);

			dir_newlink(wtop, target);
			free(list);
			free(target);
		}
		else
#endif
			if ( n == 0 && wtoptype == DIR_WIND )
				((ITM_WINDOW *) wtop)->itm_func->wd_newfolder(wtop);
		break;
	case MSEARCH:
		if ((itm_nlist(w, &n, &list) == TRUE))
		{
			if ( !app_specstart( AT_SRCH, w, list, n, 0 ) )
				((ITM_WINDOW *) w)->itm_func->itm_showinfo(w, n, list, TRUE);
			free(list);
		}
		break;
	case MCOMPARE:
		if ((itm_nlist(w, &n, &list) == TRUE) )
		{
			compare_files(w, n, list);
			free(list);
		}
		break;
	case MDELETE:
		if ((itm_nlist(w, &n, &list) == TRUE) )
		{
			itmlist_op(w, n, list, NULL, CMD_DELETE);
			free(list);
		}
		break;
	case MPRINT:
		if (w)
		{
			/* Print selected items */

			if ((itm_nlist(w, &n, &list) == TRUE) )
			{
				itmlist_op(w, n, list, NULL, CMD_PRINT);
				free(list);
			}
		}
		else if ( wtop )
		{
			/* Nothing selected, print directory or contents of text window */

			if ( wtoptype == DIR_WIND )
			{
				/* Select everything in this window */

				itm_select(wtop, 0, 4, TRUE);
				if ( itm_list(wtop, &n, &list) )
				{
					itmlist_op( wtop, n, list, NULL, CMD_PRINTDIR); 
					free(list);
				}
				wd_deselect_all();
				dir_always_update(wtop);
			}
			else if ( wtoptype == TEXT_WIND )
				wd_printtext(wtop);
		}
		break;
#if MFFORMAT
	case MFCOPY:
		formatfloppy(floppy, FALSE);
		break;
	case MFFORMAT:
		if ( !app_specstart(AT_FFMT, w, list, n, 0) )
			formatfloppy(floppy, TRUE);
		break;
#endif
	case MSETMASK:
		if ( wtoptype == DIR_WIND )
			wd_set_filemask(wtop);
		else
			wd_filemask(NULL);
		break;
	case MSHOWTXT:
	case MSHOWICN:
		wd_mode(item - MSHOWTXT);
		break;
	case MSNAME:
	case MSEXT:
	case MSDATE:
	case MSSIZE:
	case MSUNSORT:
		wd_sort(item - MSNAME);
		break;
	case MSHSIZ:
		options.V2_2.fields ^= WD_SHSIZ;
		wd_fields();
		break;
	case MSHDAT:
		options.V2_2.fields ^= WD_SHDAT;
		wd_fields();
		break;
	case MSHTIM:
		options.V2_2.fields ^= WD_SHTIM;
		wd_fields();
		break;
	case MSHATT:
		options.V2_2.fields ^= WD_SHATT;
		wd_fields();	  
		break;	 
	case MCLOSE:
	case MCLOSEW:
		wd_type_close(wtop, (item == MCLOSEW) ? 1 : 0 );
		break;
	case MDUPLIC:
		if ( wtoptype == DIR_WIND )
		{
			if ( ( toppath = strdup(wd_path(wtop)) ) != NULL   )
				dir_add_window(toppath, NULL, NULL);
		}
		else if ( wtoptype == TEXT_WIND )
		{
			if ( (toppath = strdup( ((TXT_WINDOW *)wtop)->name) ) != NULL )
				txt_add_window( NULL, 0, 0, toppath );
		}
		break;
	case MSELALL:
		if ( wtoptype == DIR_WIND || wtoptype == DESK_WIND )
		{
			if (w != wtop)
				desel_old();
			itm_select(wtop, 0, 4, TRUE);
/* DjV 111
			itm_set_menu(wtop);
*/
		}
		break;
	case MCYCLE:
		xw_cycle();
		break;         
	}
}


/********************************************************************
 *																	*
 * Functions for initialisation of the windows modules and loading	*
 * and saving about windows.										*
 *																	*
 ********************************************************************/



void wd_default(void)
{
	wd_deselect_all();
	wd_del_all();

	wd_set_mode(options.mode);
	wd_set_sort(options.sort);
	wd_set_fields(options.V2_2.fields);

	dir_default();
	txt_default();
}


void wd_init(void)
{
	selection.w = NULL;
	selection.selected = -1;
	selection.n = 0;

	dir_init();
	txt_init();

	calc_icwsize();
	can_iconify = aes_wfunc & 128; 

	wd_default();
}


/* 
 * Functies voor het veranderen van het window font.
 * Note: this is only for dir windows and text windows 
 */

boolean wd_type_setfont(int title)
{
	int 
		oldid,		/* id. of font before change */
		oldsize,	/* size of font before change */
		i;

	RECT 
		work;

	WINFO 
		*wd;
 
	FONT 
		*the_font;

	if ( title == DTVFONT )
	{
		the_font = &txt_font;
		wd = &textwindows[0];
	}
	else
	{
		the_font = &dir_font;
		wd = &dirwindows[0];
	}

	oldid = the_font->id;
	oldsize = the_font->size;

	if ( (fnt_dialog(title, the_font, FALSE) == TRUE) && (the_font->id != oldid || the_font->size != oldsize) )
	{
		for (i = 0; i < MAXWINDOWS; i++)
		{
			if ( wd->used != FALSE)
			{
				xw_get((WINDOW *) wd->typ_window, WF_WORKXYWH, &work); 
				calc_rc(wd->typ_window, &work); 
				set_sliders(wd->typ_window);
				wd_type_draw((TYP_WINDOW *)(wd->typ_window), TRUE); 
/*
				if ( title == DTDFONT )
				{
					/* 
				 	 * change of directory font 
				 	 * should perhaps affect size of a fulled window.
					 * No code for the time being. 
				 	 */

				}
*/
			}
			wd++;
		}

#if _MORE_AV
		/* Tell av-clients that directory font has changed */

		va_fontreply( VA_FONTCHANGED, 0 );
#endif

		return TRUE;
	}
	else
		return FALSE;
}


/*
 * Note: in the routines below it is assumed that if window
 * type is NOT TXT_WINDOW than it is DIR_WINDOW, because there is no
 * third type used in this context (i.e. DIR_WINDOW not explicitely checked). 
 * If a third window type be required, if-then-else constructs
 * should be replaced by switch(...){...}
 */

/* 
 * Funktie voor het berekenen van het aantal rijen en kolommen in
 * een window. 
 */

void calc_rc(TYP_WINDOW *w, RECT *work)
{
	FONT *the_font;
	int d;


	if (xw_type((WINDOW *)w) == TEXT_WIND || options.mode == TEXTMODE)
	{
		/* Paremeters for a text window or a dir window in text mode */

		if ( xw_type((WINDOW *)w) == TEXT_WIND )
		{
			the_font = &txt_font;
			d = 0;
		}
		else
		{
			the_font = &dir_font;
			d = DELTA;
		}

		w->rows     = (work->h +  the_font->ch - 1) / (the_font->ch + d);
		w->columns  = (work->w +  the_font->cw - 1) / the_font->cw;
		w->nrows    =  work->h / (the_font->ch + d); 
		w->ncolumns =  work->w /  the_font->cw;
	}
	else
	{
		/* Parameters for directory window in icon mode */

		w->rows     = (work->h + ICON_H - 1 /* - YOFFSET*/ ) / ICON_H;
		w->columns = ( work->w + ICON_W / 3 ) / ICON_W;

		w->nrows    = (work->h - YOFFSET) / ICON_H; 
		w->ncolumns =  w->columns;
	}

	w->scolumns = work->w / screen_info.fnt_w;
}


/* 
 * Funktie die uit opgegeven grootte de werkelijke grootte van het
 * window berekent.
 */

void wd_wsize(TYP_WINDOW *w, RECT *input, RECT *output)
{
	RECT 
		work, 
		*dtmax;

	int 
		fw, 
		fh, 
		d, 
		wflags;

	OBJECT 
		*menu;

	/* Normal window size */

	if ( xw_type((WINDOW *)w) == TEXT_WIND )
	{
		menu = viewmenu;
		d = 0;
		wflags = TFLAGS;
		dtmax = &tmax;
	}
	else
	{
		menu = NULL;
		d = DELTA;
		wflags = DFLAGS;
		dtmax = &dmax;
	}

	fw = screen_info.fnt_w;
	fh = screen_info.fnt_h + d;

	xw_calc(WC_WORK, wflags, input, &work, menu);
	work.x += fw / 2;
	work.w += fw / 2;
	work.h += fh / 2 - d;
	work.x -= (work.x % fw);
	work.w -= (work.w % fw);
	work.h -= (work.h % fh) - d;

	work.w = min(work.w, dtmax->w);
	work.h = min(work.h, dtmax->h);

	xw_calc(WC_BORDER, wflags, &work, output, menu);

	calc_rc((TYP_WINDOW *)w, &work);

	if ( xw_type((WINDOW *)w) != TEXT_WIND )
		calc_nlines((DIR_WINDOW *)w);
	
	if ( ( (w->winfo)->flags.iconified == 1 ) && can_iconify )
	{

		/* 
		 * If this is an iconified window, override window size
		 * (size of a dir window is calculated differently than that
		 * of a text window, which is iconvenient here)
		 */

		output->w = icwsize.w;
		output->h = icwsize.h;
	}
}


/* 
 * Calculating window size; merge of txt_calcsize and dir_calcsize 
 */

void wd_calcsize(WINFO *w, RECT *size)
{
	OBJECT 
		*menu;

	RECT 
		def, 
		border,
		high = screen_info.dsk;

	int 
		d, 
		wflags,
		wtype = xw_type((WINDOW *)(w->typ_window));

	if ( wtype == TEXT_WIND )
	{
		menu = viewmenu;
		d = 0;
		wflags = TFLAGS;
	}
	else
	{
		menu = NULL;
		d = DELTA;
		wflags = DFLAGS;
	}

	if (w->flags.fulled == 1)
	{

		if (options.mode == TEXTMODE && wtype == DIR_WIND )
		{
			high.x = w->x + screen_info.dsk.x;

			/*
			 * Calculate length of directory line 
			 * taking into account directory font size;
			 * calculate width of "fulled" window so as to display all fields
			 * (seems that left BORDERS=2*2 is -not- in directory font units 
			 * but in default system font units- why so?)
			 * For some unclear reason it looks better if 
			 * two char widths more are added (vert.slider width?)
			 */
     	
			high.w = linelength((DIR_WINDOW *)(w->typ_window)) * dir_font.cw + (BORDERS + 2) * screen_info.fnt_w;

			/* Move and resize to fit on the screen */
			
			high.w = min( high.w, screen_info.dsk.w );
			high.x = min( high.x, screen_info.dsk.w - high.w - 8); 

			/*
			 * Calculate vertical size of the fulled directory window;
			 * if there are not many items, add two empty lines at the end;
			 * otherwise, clipping will be performed elsewhere;
			 * below: 5 = 2 empty lines + 3 lines for win.header and slider?
			 */
/* not useful
			high.h = ((WINFO *)w->typ_window->nlines + 5 ) * ( dir_font.ch + DELTA );
*/
		}
		else if ( wtype == TEXT_WIND && ( ((TXT_WINDOW *)(w->typ_window))->hexmode != 0 ) )
		{
			/*
			 * Note: contrary to dir window, margins here are in the same
			 * units as the text itself- i.e. text font,  not screen font.
			 * Perhaps margins in a dir window should be done the same way-
			 * seems more appropriate if font size is changed
			 */
 
			high.x = w->x + screen_info.dsk.x;
			high.w = min( (HEXLEN + MARGIN + 1) * txt_font.cw, screen_info.dsk.w );
			high.x = min( high.x, screen_info.dsk.w - high.w - 8);
		} 

		wd_wsize(w->typ_window, &high, size);
	}
	else
	{
		/* 
		 * Window work area size in WINFO is in character units; therefore
		 * a conversion is needed here (def = window work area );
		 */

		def.x = w->x + screen_info.dsk.x;
		def.y = w->y + screen_info.dsk.y;
		def.w = w->w * screen_info.fnt_w;	/* hoogte en breedte van het werkgebied. */
		def.h = w->h * (screen_info.fnt_h + d) + d;

		/* Bereken hoogte en breedte van het window */

		xw_calc(WC_BORDER, wflags, &def, &border, menu);

		border.x = def.x;
		border.y = def.y;

		wd_wsize(w->typ_window, &border, size); 
	}
}


/*
 * Calculate width of a window line 
 */

int wd_width(TYP_WINDOW *w)
{
	if ( xw_type((WINDOW *)w) == TEXT_WIND )
		return txt_width((TXT_WINDOW *)w);
	else
		return linelength((DIR_WINDOW *)w) + BORDERS;	
}


/* 
 * Merge of txt_redraw and do_redraw functions for text and dir windows 
 */

void do_redraw(WINDOW *w, RECT *r1, boolean clear)
{
	RECT 
		r2, 
		in, 
		work;			/* window work area */

	boolean 
		text,			/* flag that directory window is in text mode */ 
		icf;			/* flag that the window is iconified */

	OBJECT 
		*obj = NULL;	/* window root object in icon mode */


	if (clip_desk(r1) == FALSE)
		return;

	xw_get(w, WF_WORKXYWH, &work);

	icf = ( (((TYP_WINDOW *)w)->winfo)->flags.iconified == 1 ) ? TRUE : FALSE;

	if ( icf && can_iconify )
		icw_draw( w );
	else
	{
		if ( xw_type(w) == DIR_WIND )
		{
			if (options.mode == TEXTMODE)
				text = TRUE;
			else 
			{
				if ((obj = make_tree((DIR_WINDOW *)w, (int)(((DIR_WINDOW *)w)->py), ((DIR_WINDOW *)w)->rows, FALSE, &work)) == NULL) 
					return;
				text = FALSE;
			}
		}

		xd_wdupdate(BEG_UPDATE);
		graf_mouse(M_OFF, NULL);
		xw_get(w, WF_FIRSTXYWH, &r2);

		while ((r2.w != 0) && (r2.h != 0))
		{
			if (xd_rcintersect(r1, &r2, &in) == TRUE)
			{
				xd_clip_on(&in);
				if ( xw_type(w) == TEXT_WIND )
					txt_prtlines((TXT_WINDOW *) w, &in);
				else
					do_draw((DIR_WINDOW *)w, &in, obj, clear, text, &work);
				xd_clip_off();
			}

			xw_get(w, WF_NEXTXYWH, &r2);
		}

		graf_mouse(M_ON, NULL);
		xd_wdupdate(END_UPDATE);

		free(obj);
	}	
}


/* 
 * Merge of txt_redraw and dir_redraw 
 */

void wd_type_redraw(WINDOW *w, RECT *area)
{
	RECT r;

	r = *area;
	do_redraw(w, &r, TRUE);
}


/* 
 * Merge of txt_draw and dir_draw 
 */

void wd_type_draw(TYP_WINDOW *w, boolean message) 
{
	RECT area;

	xw_get((WINDOW *) w, WF_CURRXYWH, &area);
	if (message)
		xw_send_redraw((WINDOW *) w, &area);
	else
		wd_type_redraw((WINDOW *) w, &area);
}


/*
 * Redraw all windows
 */

void wd_drawall(void)
{
	int wtype;
	
	WINDOW *w = xw_first();

	while (w)
	{
		wtype = xw_type(w);
		if (wtype == DIR_WIND || wtype == TEXT_WIND)
			wd_type_draw((TYP_WINDOW *)w, FALSE);
		w = w->xw_next;
	}
}

/*
 * Button event handler voor tekst & dir windows.
 *
 * Parameters:
 *
 * w			- Pointer naar window
 * x			- x positie muis
 * y			- y positie muis
 * n			- aantal muisklikken
 * button_state	- Toestand van de muisknoppen
 * keystate		- Toestand van de SHIFT, CONTROL en ALTERNATE toets
 */

void wd_type_hndlbutton(WINDOW *w, int x, int y, int n, int button_state, int keystate)
{
	int type = xw_type(w);

	if ( type == TEXT_WIND )
		return;
	wd_hndlbutton(w, x, y, n, button_state, keystate);
}


/* 
 * Window topped handler for dir and textwindow 
 */

void wd_type_topped (WINDOW *w)
{
	xw_set(w, WF_TOP);
}


/* DjV 111 Not needed anymore, itm_set_menu does it all

/*
 * Enable/disable some menu items when a window is topped
 */

void wd_type_top(WINDOW *w)
{
}

*/

void wd_type_fulled(WINDOW *w)
{
	WINFO *winfo = ((TYP_WINDOW *)w)->winfo; 

	winfo->flags.fulled = (winfo->flags.fulled == 1) ? 0 : 1;
	wd_adapt(w);
}


void wd_type_arrowed(WINDOW *w, int arrows)
{
	switch (arrows)
	{
	case WA_UPLINE:
	case WA_DNLINE:
	case WA_LFLINE:
	case WA_RTLINE:
		w_scroll((TYP_WINDOW *) w, arrows);
		break;
	case WA_UPPAGE:
		w_pageup((TYP_WINDOW *) w); 
		break;
	case WA_DNPAGE:
		w_pagedown((TYP_WINDOW *) w); 
		break;
	case WA_LFPAGE:
		w_pageleft((TYP_WINDOW *) w); 
		break;
	case WA_RTPAGE:
		w_pageright((TYP_WINDOW *) w); 
		break;
	}
}


void wd_type_hslider(WINDOW *w, int newpos)
{
	long h;
	int oldx;

	h = (long) (wd_width((TYP_WINDOW *) w) - ((TYP_WINDOW *) w)->ncolumns);

	/* Save old position and calculate new one */

	oldx = ((TYP_WINDOW *) w)->px;
	((TYP_WINDOW *) w)->px = (int) (((long) newpos * h) / 1000L);

	/* Set only if different from the old one */

	if (oldx != ((TYP_WINDOW *) w)->px)
		w_page((TYP_WINDOW *)w, HORIZ );
}


void wd_type_vslider(WINDOW *w, int newpos)
{
	long h, oldy;

	h = (long) (((TYP_WINDOW *) w)->nlines - ((TYP_WINDOW *) w)->nrows);

	/* Save old position and calculate new one */

	oldy = ((TYP_WINDOW *) w)->py;
	((TYP_WINDOW *) w)->py = ((long)newpos * h )/ 1000L;

	/* Set only if different from the old one */

	if (oldy != ((TYP_WINDOW *) w)->py)
		w_page((TYP_WINDOW *)w, VERTI );
}


void wd_set_defsize(WINFO *w) 
{
	RECT border, work;

	xw_get((WINDOW *) w->typ_window, WF_CURRXYWH, &border);
	xw_get((WINDOW *) w->typ_window, WF_WORKXYWH, &work);

	w->x = border.x - screen_info.dsk.x;
	w->y = border.y - screen_info.dsk.y;
	w->w = work.w / screen_info.fnt_w;
	w->h = work.h / (screen_info.fnt_h + ((xw_type((WINDOW *)(w->typ_window)) == TEXT_WIND) ? 0 : DELTA) );
}


/* 
 * Handle moving of a window.
 * "fulled" flag is reset if a window is moved
 */

void wd_type_moved(WINDOW *w, RECT *newpos)
{
	WINFO *wd = ((TYP_WINDOW *) w)->winfo; 
	RECT size;

	wd_wsize((TYP_WINDOW *)w, newpos, &size);
	wd->flags.fulled = 0;

#if _OVSCAN

	/*
	 * If overscan hack exists and it is turned OFF there is a problem
	 * with redrawing of a moved window, if initial position of the window
	 * was such that the lower portion of the window was off the screen.
	 * It seems that any attempt, message etc. to redraw the window
	 * does not do that completely (the "sizer" widget is not redrawn)
	 * unless its horizontal size is actually changed.
	 * So, a brute hack was made here to amend this:
	 * Window width is briefly reduced by one pixel below, then returned
	 * to normal. "Overscan off" state is irregular for many programs anyway,
	 * and this would probably be used only rarely.
	 */

	if ( over != 0xffffffffL && ovrstat == 0 ) /* overscan exists and is OFF */
	{
		size.w -= 1;	
		xw_set(w, WF_CURRXYWH, &size);
		size.w += 1;
	}
#endif

	xw_set(w, WF_CURRXYWH, &size);
	wd_set_defsize(wd);
}


void wd_type_sized(WINDOW *w, RECT *newsize)
{
	RECT area;
	int 
		old_w, 
		old_h;

	xw_get(w, WF_WORKXYWH, &area);

	old_w = area.w;
	old_h = area.h;

	wd_type_moved(w, newsize);

	xw_get(w, WF_WORKXYWH, &area);

	if ( xw_type(w) == TEXT_WIND )
	{
		if ((area.w > old_w) || (area.h > old_h))
			wd_type_draw((TYP_WINDOW *) w, TRUE);
		txt_title((TXT_WINDOW *)w);
	}
	else
	{
/* this was not good enough for XaAES  
		if ((area.w < old_w || area.h < old_h) && options.mode != TEXTMODE)
*/
		if ((area.w != old_w || area.h != old_h) && options.mode != TEXTMODE)
			wd_type_draw((TYP_WINDOW *) w, TRUE);
		dir_title((DIR_WINDOW *) w);
	}

	set_sliders((TYP_WINDOW *)w);
}


/********************************************************************
 *																	*
 * Funkties voor het zetten van de window sliders.					*
 *																	*
 ********************************************************************/


/* 
 * Calculate size and position of window slider
 * (later beware of int/long difference in hor/vert applicaton)
 */

void calc_wslider
(
	long wlines, 	/* in-     w->ncolumns or w->nrows; visible width */
	long nlines,	/* in-     wd_width or w->nlines; total width */
	long *wpxy,		/* in/out- w->px or w->py; current position */
	int *p,			/* out-    calculated slider size        */
	int *pos		/* out-    calculated slider position */
)
{
	long h;

	h = nlines - wlines;
	*wpxy = (h < 0) ? 0 : lmin(*wpxy, h);

	if ( h > 0 )
	{
		*p = (int)((wlines * 1000L) / nlines);
		*pos = (int) ( (1000L * (*wpxy)) / h);
	}
	else
	{
		*p = 1000;
		*pos = 0;
	}
}


/*
 * Calculate and set size and position of horizontal window slider;
 * note: this is somewhat slower than sometimes necessary, 
 * because both size and position are always set;
 */

void set_hslsize_pos(TYP_WINDOW *w)
{
	int 
		p,		/* calculated slider size     */ 
		pos;	/* calculated slider position */

	if (options.mode == TEXTMODE || xw_type((WINDOW *)w) == TEXT_WIND )
	{
		/* This is a dir window in text mode or a text window */

		long wpx;

		wpx = (long)w->px;
		calc_wslider( (long)w->ncolumns, (long)wd_width(w), &wpx, &p, &pos );
		w->px = (int)wpx;
	}
	else
	{
		/* 
		 * This is a dir window in icon mode, always full width,
		 * because the icons are always rearranged to fit width
		 */

		p = 1000;
		pos = 0;
	}

	/* both size and position of the slider are always set; */

	xw_set((WINDOW *) w, WF_HSLIDE, pos);
	xw_set((WINDOW *) w, WF_HSLSIZE, p);
}


/*
 * Calculate and set size and position of vertical window slider.
 * Note that some variables are long here, different 
 * than in set_hslsize_pos()
 */

void set_vslsize_pos(TYP_WINDOW *w)
{
	int 
		p,		/* slider size     */ 
		pos;	/* slider position */

	calc_wslider( w->nrows, w->nlines, &(w->py), &p, &pos );
	xw_set((WINDOW *)w, WF_VSLIDE, pos);
 	xw_set((WINDOW *)w, WF_VSLSIZE, p);
}


/*
 * Set both window sliders (sizes and positions)
 */

void set_sliders(TYP_WINDOW *w)
{
	set_hslsize_pos(w);
	set_vslsize_pos(w);
}


/*
 *  Set sliders and redraw; direct=1: horizontal; 2: vertical; 3: both 
 */

void w_page(TYP_WINDOW *w, int direct )
{
	if ( (direct & HORIZ) != 0 )
		set_hslsize_pos((TYP_WINDOW *)w); 
	if ( (direct & VERTI) != 0 )
		set_vslsize_pos((TYP_WINDOW *)w); 
	wd_type_draw(w, FALSE);
}


/*
 * Window page scrolling routines
 */

void w_pageup(TYP_WINDOW *w)
{
	long oldy;

	oldy = w->py;
	w->py -= w->nrows;

	if (w->py < 0)
		w->py = 0;

	if (w->py != oldy)
		w_page( w, VERTI );
}


void w_pagedown(TYP_WINDOW *w)
{
	long oldy;

	oldy = w->py;								/* remember old position */
	w->py += w->nrows;							/* calculate new one */

	if ((w->py + w->nrows) > w->nlines)			/* check if end reached */
		w->py = lmax(w->nlines - (long)w->nrows, 0);

	if (w->py != oldy)							/* move only if really changed */
		w_page (w, VERTI );
}


void w_pageleft(TYP_WINDOW *w)
{
	/*
	 * this operation is not active in dir window in icon mode because 
	 * icons are always rearranged to fit current window width 	
	 */

	if (options.mode == TEXTMODE || xw_type((WINDOW *)w) == TEXT_WIND )
	{
		int oldx = w->px;
		w->px -= w->ncolumns;
		if (w->px < 0)
			w->px = 0;

		if (w->px != oldx)
			w_page(w, HORIZ);
	}
}


void w_pageright(TYP_WINDOW *w)
{
	/*
	 * this operation is not active in dir window in icon mode because
	 * icons are always rearranged to fit current window width 	
	 */

	if (options.mode == TEXTMODE || xw_type((WINDOW *)w) == TEXT_WIND )
	{
		int oldx, lwidth;

		lwidth = wd_width((TYP_WINDOW *)w);

		oldx = w->px;
		w->px += w->ncolumns;

		if ((w->px + w->ncolumns) > lwidth)
			w->px = max(lwidth - w->ncolumns, 0);

		if (w->px != oldx)
			w_page ( w, HORIZ );
	}
}


/*
 * More window scrolling routines ;
 * for text window: ch= txt_font.ch;
 * for dir window: ch is either dir_font.ch + DELTA or ICON_H
 */

/*
 * Zoek de eerste regel van een window, die zichtbaar is binnen
 * een rechthoek.
 *
 * Parameters:
 *
 * wx,wy	- x,y coordinaat werkgebied window
 * area		- rechthoek
 * prev		- pointer naar een boolean, die aangeeft of een of twee
 *			  regels opnieuw getekend moeten worden.
 *
 * ch  		- char height (or char height + DELTA or icon height)
 * cw		- char width
 * 
 */


/*
 * note: find_firstline and find_lastline
 * in viewer.c were declared as long, and in dir.c as int.
 * as line is computed from int vars it can't be longer than
 * max int, but just in case they are here declared as long.
 */

static long find_firstline(int wy, RECT *area, boolean *prev, int ch)
{
	long line;

	line = (area->y - wy);
	*prev = ((line % ch) == 0) ? FALSE : TRUE;
	return (line / ch);
}


static long find_lastline(int wy, RECT *area, boolean *prev, int ch)
{
	long line;

	line = (area->y + area->h - wy);
	*prev = ((line % ch) == 0) ? FALSE : TRUE;
	return ((line - 1) / ch);
}


static int find_leftcolumn(int wx, RECT *area, boolean *prev, int cw)
{
	int column;

	column = (area->x - wx);
	*prev = ((column % cw) == 0) ? FALSE : TRUE;
	return (column / cw);
}


static int find_rightcolumn(int wx, RECT *area, boolean *prev, int cw)
{
	int column;

	column = (area->x + area->w - wx);
	*prev = ((column % cw) == 0) ? FALSE : TRUE;
	return ((column - 1) / cw);
}


/*
 * Functie om een text/dir window een regel te scrollen.
 *
 * Parameters:
 *
 * w		- Pointer naar window
 * type		- geeft richting van scrollen aan
 */

void w_scroll(TYP_WINDOW *w, int type) 
{
	RECT work, area, r, in, src, dest;
	long line;
	int column, wx, wy, ch;
	boolean prev, iconwind;
	FONT *the_font;
	int wtype = xw_type((WINDOW *)w);

	iconwind = (wtype == DIR_WIND) && (options.mode !=TEXTMODE);

	/* Change position in the window, i.e. w->px, w->py */

	switch (type)
	{
	case WA_UPLINE:
		if (w->py <= 0)
			return;
		w->py -= 1;
		break;
	case WA_DNLINE:
		if ((w->py + w->nrows) >= w->nlines)
			return;
		w->py += 1;
		break;
	case WA_LFLINE:
		if ((w->px <= 0) || iconwind  )
			return;
		w->px -= 1;
		break;
	case WA_RTLINE:
		if (((w->px + w->ncolumns) >= wd_width(w)) || iconwind )
			return;
		w->px += 1;
		break;
	default:
		return;
	}

	xw_get((WINDOW *) w, WF_WORKXYWH, &work);

	if ( wtype == TEXT_WIND )
	{
		the_font = &txt_font;
		ch = txt_font.ch;
	}
	else
	{
		the_font = &dir_font;
		if ( options.mode == TEXTMODE )
			ch = dir_font.ch + DELTA;
		else
			ch = ICON_H;
	}

	wx = work.x;
	wy = work.y;
	area = work;

	if ( iconwind )
	{
		area.x += XOFFSET;
		area.w -= XOFFSET;
		area.y += YOFFSET;
		area.h -= YOFFSET;
	}

	if (clip_desk(&area) == FALSE) /* for text window this was &work ? */
		return;

	xd_wdupdate(BEG_UPDATE);

	if ((type == WA_UPLINE) || (type == WA_DNLINE))
		set_vslsize_pos(w);
	else
		set_hslsize_pos(w);

	graf_mouse(M_OFF, NULL);
	xw_get((WINDOW *) w, WF_FIRSTXYWH, &r);

	set_txt_default(the_font->id, the_font->size);

	while ((r.w != 0) && (r.h != 0))
	{
		if (xd_rcintersect(&r, &area, &in) == TRUE) /* for text window this was &work ? */
		{
			xd_clip_on(&in);

			src = in;
			dest = in;

			if ((type == WA_UPLINE) || (type == WA_DNLINE))
			{

				if (type == WA_UPLINE)
				{
					dest.y += ch;
					line = find_firstline(wy, &in, &prev, ch);
				}
				else
				{
					src.y += ch;
					line = find_lastline(wy, &in, &prev, ch);
				}
				line += w->py;
				dest.h -= ch;
				src.h -= ch;
			}
			else
			{
				if (type == WA_LFLINE)
				{
					dest.x += the_font->cw;
					column = find_leftcolumn(wx, &in, &prev, the_font->cw);
				}
				else
				{
					src.x += the_font->cw;
					column = find_rightcolumn(wx, &in, &prev, the_font->cw);
				}
				column += w->px;
				dest.w -= the_font->cw;
				src.w -= the_font->cw;
			}

			if ((src.h > 0) && (src.w > 0))
				move_screen(&dest, &src);

			if ((type == WA_UPLINE) || (type == WA_DNLINE))
			{

				if ( wtype == TEXT_WIND )
					txt_prtline((TXT_WINDOW *)w, line, &in, &work);
				else
					dir_prtline((DIR_WINDOW *)w, (int)line, &in, &work);


				if (prev == TRUE)
				{
					line += (type == WA_UPLINE) ? 1 : -1;

					if ( wtype == TEXT_WIND )	
						txt_prtline((TXT_WINDOW *)w, line, &in, &work);
					else				
						dir_prtline((DIR_WINDOW *)w, (int)line, &in, &work);
				}
			}
			else
			{
				if ( wtype == TEXT_WIND )
					txt_prtcolumn((TXT_WINDOW *)w, column, &in, &work);
				else
					dir_prtcolumn((DIR_WINDOW *)w, column, &in, &work);

				if (prev == TRUE)
				{
					column += (type == WA_LFLINE) ? 1 : -1;
					if ( wtype == TEXT_WIND )
						txt_prtcolumn((TXT_WINDOW *)w, column, &in, &work);
					else
						dir_prtcolumn((DIR_WINDOW *)w, column, &in, &work);
				}
			}

			xd_clip_off();
		}
		xw_get((WINDOW *) w, WF_NEXTXYWH, &r);
	}

	graf_mouse(M_ON, NULL);

	xd_wdupdate(END_UPDATE);
}


/*
 * Adapt window size with resolution change
 */

void wd_adapt(WINDOW *w)
{
	RECT size;
	if (xw_type(w) != ACC_WIND )
	{
		wd_calcsize(((TYP_WINDOW *) w)->winfo, &size);
		xw_set(w, WF_CURRXYWH, &size);
		set_sliders((TYP_WINDOW *)w); 
	}
}


/* 
 *
 * Keyboard event handler for text & dir windows.
 *
 * Parameters:
 *
 * w		- Pointer naar window
 * scancode	- Scancode van de ingedrukte toets
 * keystate	- Toestand van de SHIFT, CONTROL en ALTERNATE toets
 *
 * Resultaat: 0 - toets kon niet verwerkt worden
 *			  1 - toets kon verwerkt worden
 */

int wd_type_hndlkey(WINDOW *w, int scancode, int keystate)
{
	char 
		*newpath;

	long 
		lines;

	int 
		i, 
		key,
		wt,
		result = 1;

	wt = xw_type(w);

	switch (scancode)
	{
	case RETURN:
		if ( wt != TEXT_WIND )
			break;
	case CURDOWN:
		w_scroll( (TYP_WINDOW *)w, WA_DNLINE);
		break;
	case CURUP:
		w_scroll( (TYP_WINDOW *)w, WA_UPLINE);
		break;
	case CURLEFT:
		w_scroll( (TYP_WINDOW *)w, WA_LFLINE);
		break;
	case CURRIGHT:
		w_scroll( (TYP_WINDOW *)w, WA_RTLINE);
		break;
	case SPACE:
		if ( wt != TEXT_WIND )
			break;
	case PAGE_DOWN:				/* PgUp/PgDn keys on PC keyboards (Emulators and MILAN) */
	case SHFT_CURDOWN:
		w_pagedown((TYP_WINDOW *)w);
		break;
	case PAGE_UP:				/* PgUp/PgDn keys on PC keyboards (Emulators and MILAN) */
	case SHFT_CURUP:
		w_pageup((TYP_WINDOW *)w);
		break;
	case SHFT_CURLEFT:
		w_pageleft((TYP_WINDOW *)w);
		break;
	case SHFT_CURRIGHT:
		w_pageright((TYP_WINDOW *)w);
		break;
	case HOME:
		((TYP_WINDOW *) w)->px = 0;
		((TYP_WINDOW *) w)->py = 0;
		w_page ((TYP_WINDOW *)w, VERTI | HORIZ ); 
		break;
	case SHFT_HOME:
		((TYP_WINDOW *) w)->px = 0;
		lines = ((TYP_WINDOW *) w)->nlines;
		if (((TYP_WINDOW *) w)->nrows < lines)
		{
			((TYP_WINDOW *) w)->py = lines - ((TYP_WINDOW *) w)->nrows;
			w_page ((TYP_WINDOW *)w, VERTI | HORIZ ); 
		}
		break;
	case ESCAPE:
		if ( wt != TEXT_WIND )
		{
			force_mediach(((DIR_WINDOW *) w)->path);
			dir_refresh_wd((DIR_WINDOW *) w);
		}
		break;
	default:
		if ( scansh( scancode, keystate ) == options.V2_2.kbshort[MCLOSEW-MFIRST] )
			wd_type_close(w, 1);
		else
		{
			if ( wt == TEXT_WIND )
				result = 0;
			else
			{
				key = scancode & ~XD_SHIFT;

				if ((scancode & XD_SHIFT) && (key >= ALT_A) && (key <= ALT_Z))
				{
					i = key - (XD_ALT | 'A');

					if (check_drive(i))
					{
						if ((newpath = strdup("A:\\")) != NULL)
						{
							newpath[0] = (char) i + 'A';
							free(((DIR_WINDOW *) w)->path);
							((DIR_WINDOW *) w)->path = newpath;
							dir_readnew((DIR_WINDOW *) w);
						}
					}
				}
				else
					result = 0;
			}
		}
		break;
	}
	return result;
}


CfgNest 
	dir_config, 	/* configuration table for directory windows */
	view_config,	/* Configuration table for text windows */ 
	open_config,	/* configuration table for open windows */
	dir_one,		/* configuration table for one dir window */
	text_one;		/* configuration table for one text window */


/*
 * Configuration table for all windows
 */

CfgEntry window_table[]= 
{
	{CFG_HDR, 0, "windows" },
	{CFG_BEG},
	{CFG_NEST,0, "directories",	dir_config	},	/* directory windows */
	{CFG_NEST,0, "views",		view_config	},	/* text windows */
	{CFG_NEST,0, "open",		open_config	},	/* open windows (any type */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Load or save all windows information
 */

CfgNest wd_config
{
	*error = handle_cfg(file, window_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, wd_init, wd_default);
}


/* 
 * Configuration tables for start/end of group for open windows
 */

static CfgEntry open_start_table[] =
{
	{CFG_HDR, 0, "open" },
	{CFG_BEG},
	{CFG_LAST}
};

static CfgEntry open_end_table[] =
{
	{CFG_END},
	{CFG_LAST}
};


/*
 * Read-only configuration table for all open windows
 */

static CfgEntry open_table[] =
{
	{CFG_HDR, 0, "open" },
	{CFG_BEG},
	{CFG_NEST,0, "dir",	 dir_one  },
	{CFG_NEST,0, "text", text_one },
	{CFG_END},
	{CFG_LAST}
};


/*
 * Configuration table for temporary saving and reopening windows 
 */

CfgEntry reopen_table[]=
{
	{CFG_NEST, 0, "open", open_config},	/* open windows (any type) */
	{CFG_FINAL}, 				       	/* file completness check  */
	{CFG_LAST}
};


/* 
 * Save or load configuration of open windows.
 * The construction is needed to preserve window stack order:
 * all windows must be saved intermingled on stack order,
 * hence the single loop.
 * during saving, windows may be closed, depending on "wclose"
 * Loading is straightforward on keyword occurrence.
 */

boolean wclose = FALSE;

CfgNest open_config
{
	if (io == CFG_SAVE)
	{
		WINDOW *w;

		*error = CfgSave(file, open_start_table, lvl + 1, CFGEMP);

		w = xw_last();

		while ( (*error >= 0) && (w != NULL) )
		{
			switch(xw_type(w))
			{
				case DIR_WIND :
					*error = dir_save(file, w, lvl);
					if (wclose)
						dir_close(w, 1);
					break;

				case TEXT_WIND :
					*error = text_save(file, w, lvl);
					if (wclose)
						txt_closed(w);
					break;
				default:
					break;
			}
	
			w = w->xw_prev;
		}

		*error = CfgSave(file, open_end_table, lvl + 1, CFGEMP); 
	}
	else
	{
		/* Load configuration of open windows */

		*error = CfgLoad(file, open_table, MAX_KEYLEN, lvl + 1); 

	}
}


/********************************************************************
 *																	*
 * Functies voor het tijdelijk sluiten en weer openen van windows.	*
 *																	*
 ********************************************************************/

static XFILE *mem_file;

/*
 * Save current positions of the open windows into a (file-like) buffer 
 * (a "memory file") and close the windows. This routine also opens 
 * this "file".
 */

boolean wd_tmpcls(void)
{
	int error;

	wd_deselect_all();

	if ((mem_file = x_fmemopen(O_DENYRW | O_RDWR, &error)) != NULL)
	{
		wclose = TRUE;
		error = CfgSave(mem_file, reopen_table, 0, CFGEMP); 
		wclose = FALSE;
	}

	/* 
	 * Close the "file" only in case of error, otherwise keep it
	 * open for later rereading
	 */

	if (error != 0) 
	{
		if (mem_file != NULL)
			x_fclose(mem_file);
/* why ?
		if (error != 1)
*/
			xform_error(error);
		return FALSE;
	}
	else
		return TRUE;
}


/*
 * Load the positions of windows from a (file-like) memory buffer 
 * ("the memory file") and open them.  * This routine closes the "file"
 * after reading
 */

extern int chklevel;

void wd_reopen(void)
{
	int error;
	long offset;

	chklevel = 0;

	/* Rewind the "file", then read from it */

	if ((offset = x_fseek(mem_file, 0L, 0)) >= 0)
		error = CfgLoad(mem_file, reopen_table, MAX_KEYLEN, CFGEMP); 
	else
		error = (int) offset;

	x_fclose(mem_file);

	if (error != 0)
		xform_error(error);
}


/********************************************************************
 *																	*
 * Functions for handling items in a window.						*
 *																	*
 ********************************************************************/

int itm_find(WINDOW *w, int x, int y)
{
	if (in_window(w, x, y) == TRUE)
		return (((ITM_WINDOW *) w)->itm_func->itm_find) (w, x, y);
	else
		return -1;
}


boolean itm_state(WINDOW *w, int item)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_state) (w, item);
}


/*
 * Return type of an item in a window. Must be disabled for text window
 * (because this routine may be called for text windows in itm_set_menu)
 */

ITMTYPE itm_type(WINDOW *w, int item)
{
	ITMTYPE type;

	if ( xw_type(w) == TEXT_WIND )
		return ITM_NOTUSED;

	type = (((ITM_WINDOW *) w)->itm_func->itm_type) (w, item);

	if ((type == ITM_FILE) && (prg_isprogram(itm_name(w, item)) == TRUE))
		type = ITM_PROGRAM;

	return type;
}


ITMTYPE itm_tgttype(WINDOW *w, int item)
{
	ITMTYPE 
		type;
	
	char
		*name, 
		*tgtname = NULL;

	if ( xw_type(w) == TEXT_WIND )
		return ITM_NOTUSED;

	type = (((ITM_WINDOW *) w)->itm_func->itm_tgttype) (w, item);

	if ( itm_islink(w, item, FALSE) )
	{
		tgtname = itm_tgtname(w, item);
		name = fn_get_name((const char *)tgtname);
	}
	else
		name = (char *)itm_name(w, item);

	if ((type == ITM_FILE) && (prg_isprogram((const char *)name) == TRUE))
		type = ITM_PROGRAM;

	free(tgtname);
	return type;
}



int itm_icon(WINDOW *w, int item)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_icon) (w, item);
}


const char *itm_name(WINDOW *w, int item)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_name) (w, item);
}


char *itm_fullname(WINDOW *w, int item)
{
	return (((ITM_WINDOW *)w)->itm_func->itm_fullname) (w, item);
}


/*
 * Get target name of an object which is a link;
 * If the object is not a link, return name of the object.
 * This routine allocates space for the name obtained.
 */

char *itm_tgtname(WINDOW *w, int item)
{
	char 
		*thename,
		*fullname = (((ITM_WINDOW *)w)->itm_func->itm_fullname) (w, item);

#if _MINT_
	if ( itm_islink(w, item, FALSE) )
	{
		thename = x_fllink(fullname);
		free(fullname);
	}
	else
#endif
		thename = fullname;

	return thename;	
}


int itm_attrib(WINDOW *w, int item, int mode, XATTR *attrib)
{
	int result;
	result = (((ITM_WINDOW *) w)->itm_func->itm_attrib) (w, item, mode, attrib);

	wd_setselection(w);
	return result;
}


/*
 * Is the object a link? Return TRUE if it is.
 * BUT: if cond (i.e. contiditonal) is FALSE, return real result;
 * if cond is TRUE, return TRUE only if copy option "follow link"
 * is not set. (This means that if "follow link" is set, links
 * are not treated as separate entities, but instead are followed
 * to their targets)
 */

boolean itm_islink(WINDOW *w, int item, boolean cond)
{
	boolean itis;

	itis = (((ITM_WINDOW *) w)->itm_func->itm_islink) (w, item);

	if ( cond )
		return ( itis && ( (options.cprefs & CF_FOLL) == 0) );
	else
		return itis;
}


boolean itm_open(WINDOW *w, int item, int kstate)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_open) (w, item, kstate);
}


void itm_select(WINDOW *w, int selected, int mode, boolean draw)
{
	if (xw_exist(w))
		(((ITM_WINDOW *) w)->itm_func->itm_select) (w, selected, mode, draw);

	wd_setselection(w);
}


void itm_rselect(WINDOW *w, int x, int y)
{
	(((ITM_WINDOW *) w)->itm_func->itm_rselect) (w, x, y);
}


boolean itm_xlist(WINDOW *w, int *ns, int *nv, int **list, ICND **icns, int mx, int my)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_xlist) (w, ns, nv, list, icns, mx, my);
}


/*
 * Return a list of items selected in a window
 */

boolean itm_list(WINDOW *w, int *n, int **list)
{
	if ( xw_type(w) == TEXT_WIND )
		return FALSE;

	return (((ITM_WINDOW *) w)->itm_func->itm_list) (w, n, list);
}


/*
 * Similar to the above, but FALSE if no items selected
 */

static boolean itm_nlist(WINDOW *w, int *n, int **list)
{
	boolean result = itm_list(w, n, list);
	if (*n == 0 ) 
		result = FALSE;
	return result;
}

/********************************************************************
 *																	*
 * Functies voor het afhandelen van muisklikken in een window.		*
 *																	*
 ********************************************************************/

static boolean itm_copy(WINDOW *sw, int n, int *list, WINDOW *dw,
					 int dobject, int kstate, ICND *icnlist, int x, int y)
{
	if (dw != NULL)
	{
		/* Can't copy over/to a file */

		if ((dobject != -1) && (itm_tgttype(dw, dobject) == ITM_FILE))
			dobject = -1;

		/* Perform window-type-specific item-copy operation */

		if (((ITM_WINDOW *)dw)->itm_func->itm_copy(dw, dobject, sw, n, list, icnlist, x, y, kstate) == TRUE)
			itm_select(sw, -1, 0, va_reply ? FALSE : TRUE);

		return TRUE;
	}
	else
	{
		alert_printf(1, AILLDEST);
		return FALSE;
	}
}


/* 
 * Drag & drop items from a list onto a window at position x,y.
 */

static boolean itm_drop
(
	WINDOW *w, 			/* source window */
	int n,				/* number of selected items */ 
	int *list, 			/* list of selected items' indices */
	int kstate, 		/* keyboard state */
	int x, 				/* position of destination window */
	int y				/* positon of destination window  */
)
{
#if _MINT_

	int 
		fd,			/* pipe handle */ 
		i, 			/* item counter */
		item, 		/* index of a selected item */
		apid = -1,	/* destination app id */ 
		hdl;		/* destination window handle */
	
	const char 
		*path;		/* full name of the dragged item */


	/* Find the owner of the window at x,y */

	if ( (hdl = wind_find(x,y)) > 0)
		wind_get(hdl, WF_OWNER, &apid); 

	/* Drag & drop is possible only in mint or magic (what about geneva 6?) */

	if (mint && apid > 0)
	{
		char ddsexts[34];
		long nsize = 0;

		/* Create a string with filenames to be sent */

		*global_memory = 0;

		for ( i = 0; i < n; i++ )
		{
			if ( (item = list[i]) == -1 )
				continue;

			path=itm_fullname(w, item);

			/* Concatenate filename(s) */

			if (!va_add_name(itm_type(w, item), path))
			{
				free(path);
				return FALSE;
			}
			free (path);
		}

		nsize = strlen(global_memory);

		/* Create a drag&drop pipe; if successful, continue */

		fd = ddcreate(apid, ap_id, hdl, x, y, kstate, ddsexts);

		if (fd > 0)
		{
			int reply;

			/* Will the recepient accept data ? */

			reply = ddstry(fd, "ARGS", "", nsize);

			/* 
			 * Do whatever is needed, depending on what is on destination.
			 * Note: there is a warning in the docs NOT to do any
			 * wind_update during the operation, so itmlist_op
		 	 * below may do it wrong if wind_update is called somewhere
			 * from them ?
			 */

			switch(reply)
			{
				case DD_OK:
					/* Write the name(s) down the pipe, then close the pipe */
					Fwrite(fd, nsize, global_memory);
					break;
				case DD_PRINTER:
					/* dropped on a printer, so print items */
					itmlist_op(w, n, list, NULL, CMD_PRINT);
					break;
				case DD_TRASH:
					/* dropped on a trashcan, so delete these items */
					itmlist_op(w, n, list, NULL, CMD_DELETE);
					break;
				case DD_CLIPBOARD:	/* app should put it in clipboard */
				case DD_NAK:		/* app refuses the drop */
				case DD_EXT:		/* app doesn't understand this type */
				case DD_LEN:		/* app can't recive so much data */
				default:	
					/* These drag & drop operations are not (yet) supported */
					alert_iprint(APPNOEXT); 
					ddclose(fd);
					return FALSE;
			}
			ddclose(fd);
			itm_select(w, -1, 0, TRUE);	/* deselect items */
			return TRUE;

		}
		else
		{
			/* Destination app does not know D&D or won't accept data */

			alert_iprint(APPNODD); 
		}

		return FALSE;
	}

#endif

	alert_printf(1, AILLDEST);
	return FALSE;
}


/* 
 * Routines voor het tekenen van de omhullende van een icoon. 
 */

static void get_minmax(ICND *icns, int n, int *clip)
{
	int i, j;

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < icns[i].np; j++)
		{
			if ((i == 0) && (j == 0))
			{
				clip[0] = icns[i].coords[0];
				clip[1] = icns[i].coords[1];
				clip[2] = clip[0];
				clip[3] = clip[1];
			}
			else
			{
				clip[0] = min(clip[0], icns[i].coords[j * 2]);
				clip[1] = min(clip[1], icns[i].coords[j * 2 + 1]);
				clip[2] = max(clip[2], icns[i].coords[j * 2]);
				clip[3] = max(clip[3], icns[i].coords[j * 2 + 1]);;
			}
		}
	}
}


static void clip_coords(int *clip, int x, int y, int *nx, int *ny)
{
	int h;

	*nx = x;
	*ny = y;

	h = screen_info.dsk.x - clip[0];
	if (x < h)
		*nx = h;

	h = screen_info.dsk.y - clip[1];
	if (y < h)
		*ny = h;

	h = screen_info.dsk.x + screen_info.dsk.w - 1 - clip[2];
	if (x > h)
		*nx = h;

	h = screen_info.dsk.y + screen_info.dsk.h - 1 - clip[3];
	if (y > h)
		*ny = h;
}


static void draw_icns(ICND *icns, int n, int mx, int my, int *clip)
{
	int i, j, c[18], x, y;

	clip_coords(clip, mx, my, &x, &y);

	vswr_mode(vdi_handle, MD_XOR);

	vsl_color(vdi_handle, 1);
	vsl_ends(vdi_handle, 0, 0);
	vsl_type(vdi_handle, 7);
	vsl_udsty(vdi_handle, 0xCCCC);
	vsl_width(vdi_handle, 1);

	graf_mouse(M_OFF, NULL);

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < icns[i].np; j++)
		{
			c[j * 2] = icns[i].coords[j * 2] + x;
			c[j * 2 + 1] = icns[i].coords[j * 2 + 1] + y;
		}
		v_pline(vdi_handle, icns[i].np, c);
	}

	graf_mouse(M_ON, NULL);
}


WINDOW *wd_selected_wd(void)
{
	return selection.w;
}


int wd_selected(void)
{
	return selection.selected;
}


int wd_nselected(void)
{
	return selection.n;
}


static void desel_old(void)
{
	if (selection.w != NULL)
		itm_select(selection.w, -1, 0, TRUE);
}


/* mode = 2 - deselecteren, mode = 3 - selecteren */

static void select_object(WINDOW *w, int object, int mode)
{
	if (object < 0)
		return;

	if (itm_tgttype(w, object) != ITM_FILE)
		itm_select(w, object, mode, TRUE);
}


/*
 * Find the window and the object at position x,y 
 */

static void find_newobj(int x, int y, WINDOW **wd, int *object, boolean *state)
{
	WINDOW *w = xw_find(x, y);

	*wd = w;

	if (w != NULL)
	{
		if ((xw_type(w) == DIR_WIND) || (xw_type(w) == DESK_WIND))
		{
			if ((*object = itm_find(w, x, y)) >= 0)
				*state = itm_state(w, *object);
		}
		else
			*object = -1;
	}
	else
	{
		*wd = NULL;
		*object = -1;
	}
}


/*
 * Copy objects from an object (window) to another
 */

boolean itm_move
(
	WINDOW *src_wd,	/* source window */ 
	int src_object,	/* source object */ 
	int old_x,		/* initial position */ 
	int old_y,		/* initial position */ 
	int avkstate 	/* key state only in av protocol */
)
{
	int 
		x = old_x, 
		y = old_y,
		ox, 
		oy, 
		kstate, 
		*list, 
		n, 
		nv = 0, 
		i,
		cur_object = src_object, 
		new_object,
		clip[4] = {0, 0, 0, 0};

	WINDOW 
		*cur_wd = src_wd, 
		*new_wd;

	boolean 
		result = FALSE,
		cur_state = TRUE, 
		new_state, 
		mreleased;

	ICND 
		*icnlist = NULL;

	if (itm_type(src_wd, src_object) == ITM_PREVDIR )
	{
		if (!va_reply)
			wait_button();
		return result;
	}

	/* Create a list of selected objects in the source window */

	if ((itm_list(src_wd, &n, &list) == FALSE) || (n == 0))
		return result;

	if ( !va_reply )
	{
		for (i = 0; i < n; i++)
		{
			if (itm_type(src_wd, list[i]) == ITM_PREVDIR)
				itm_select(src_wd, list[i], 2, TRUE);
		}

		free(list); 
		if (itm_xlist(src_wd, &n, &nv, &list, &icnlist, old_x, old_y) == FALSE)
			return result;
	}

	get_minmax(icnlist, nv, clip);

	wind_update(BEG_MCTRL);
	graf_mouse(FLAT_HAND, NULL);

	draw_icns(icnlist, nv, x, y, clip);

	do
	{
		if ( va_reply )
		{
			x = old_x;
			y = old_y;
			mreleased = TRUE; /* position is supplied, so as if released immediately */
			kstate = avkstate;
		}
		else
		{
			ox = x;
			oy = y;
			mreleased = xe_mouse_event(0, &x, &y, &kstate);
		}

		if ( va_reply || (x != ox) || (y != oy))
		{
			/* Safe, becuse if va_reply, nv=0, so undefined ox,oy do not matter */

			draw_icns(icnlist, nv, ox, oy, clip);

			/* Find the window and the object at destination */

			find_newobj(x, y, &new_wd, &new_object, &new_state);

			if ((cur_wd != new_wd) || (cur_object != new_object))
			{
				if ((cur_state == FALSE) && (cur_object >= 0))
					select_object(cur_wd, cur_object, 2);

				cur_wd = new_wd;
				cur_object = new_object;
				cur_state = new_state;

				if ((cur_object >= 0) && (cur_state == FALSE))
					select_object(cur_wd, cur_object, 3);
			}

			if (mreleased == FALSE)
				draw_icns(icnlist, nv, x, y, clip);
		}
		else if (mreleased == TRUE)
			draw_icns(icnlist, nv, x, y, clip);
	}
	while (mreleased == FALSE);

	graf_mouse(ARROW, NULL);
	wind_update(END_MCTRL);

	if ((cur_state == FALSE) && (cur_object >= 0))
		select_object(cur_wd, cur_object, 2);

	if ((cur_wd != src_wd) || (cur_object != src_object))
	{
		if (cur_wd != NULL)
		{
			int cur_type = xw_type(cur_wd);

			if ((cur_type == DIR_WIND) || (cur_type == DESK_WIND))
			{
				/* 
				 * Test if destination window is the desktop and if the
				 * destination object is -1 (no object). 
				 */

				if ((xw_type(cur_wd) == DESK_WIND) && (cur_object == -1) && (xw_type(src_wd) == DESK_WIND))
					clip_coords(clip, x, y, &x, &y);

				result = itm_copy(src_wd, n, list, cur_wd, cur_object, kstate, icnlist, x, y);

			}
			else if (cur_type == ACC_WIND )
				result = va_accdrop(cur_wd, src_wd, list, n, kstate, x, y );
			else
				alert_printf(1, AILLCOPY);
		}
		else
			result = itm_drop(src_wd, n, list, kstate, x, y); /* drag & drop */
	}
	free(list);
	free(icnlist);
	return result;
}


/*
 * Check whether position (x,y) is inside the work area of window *w);
 */

static boolean in_window(WINDOW *w, int x, int y)
{
	RECT work;

	xw_get(w, WF_WORKXYWH, &work);

	if (   (x >= work.x) && (x < (work.x + work.w))
	    && (y >= work.y) && (y < (work.y + work.h))
	   )
		return TRUE;
	else
		return FALSE;
}


/* 
 * Note: bstate never used in this routine ? 
 */

void wd_hndlbutton(WINDOW *w, int x, int y, int n, int bstate, int kstate)
{
	int item, m_state;

	if (selection.w != w)
		desel_old();

	m_state = xe_button_state();
	item = itm_find(w, x, y);

	if (item >= 0)
	{
		if (n == 2)
		{
			itm_select(w, item, 0, TRUE);
			wait_button();

			if (itm_open(w, item, kstate) == TRUE)
				itm_select(w, item, 2, TRUE);
		}
		else
		{
			if ((m_state == 0) || (itm_state(w, item) == FALSE))
			{
				itm_select(w, item, (kstate & 3) ? 1 : 0, TRUE);
			}

			if ((m_state != 0) && (itm_state(w, item) == TRUE))
				itm_move(w, item, x, y, 0);
		}

		wd_setselection(selection.w); 
	}
	else if (in_window(w, x, y) == TRUE)
	{

		/* Top TeraDesk by clicking on desktop area */

#if _MINT_
		if ( (mint || geneva) && (xw_type(w) == DESK_WIND) )
		{
			/*  This works only if TeraDesk has open windows ! */

			WINDOW *ww = xw_first();

			while (ww)
			{
				if (xw_type(ww) == DIR_WIND || xw_type(ww) == TEXT_WIND )
				{
					wd_type_topped(ww);	
					break;
				}
				ww = ww->xw_next;
			}

			/* 
			 * This works without any open windows, but unfortunately
			 * not in all AESses. Will be ok in N.AES, XaAES, Magic 
			 */
 
			if (!ww)
			{
				if(naes || aes_ctrl) 
					appl_control(ap_id, 12, NULL);
				else 
					wind_set(-1, WF_TOP, ap_id);
			}
		}

#endif

		if (((m_state == 0) || ((kstate & 3) == 0)) && (selection.w == w))
			itm_select(w, -1, 0, TRUE);
		if (m_state)
			itm_rselect(w, x, y);

		wd_setselection(w); 
	}
}



/********************************************************************
 *																	*
 * Functions for window iconify/uniconify.							*
 *																	*
 ********************************************************************/


/*
 * Set-up background object for icon(s) in a window;
 * this routine is used for an iconified window and for
 * directory window in icon mode 
 */

void wd_set_obj0( OBJECT *obj, boolean smode, int row, int lines, int yoffset, RECT *work )
{
	/* Note: when object type is I_BOX it will not be redrawn */
	 
	obj[0].ob_next = -1;
	obj[0].ob_head = -1;
	obj[0].ob_tail = -1;
	obj[0].ob_type = (smode == FALSE) ? G_BOX : G_IBOX;
	obj[0].ob_flags = NORMAL;
	obj[0].ob_state = NORMAL;
	obj[0].ob_spec.obspec.framesize = 0;
	obj[0].ob_spec.obspec.interiorcol = options.V2_2.win_color;
	obj[0].ob_spec.obspec.fillpattern = options.V2_2.win_pattern;
	obj[0].ob_spec.obspec.textmode = 1;
	obj[0].r.x = work->x;
	obj[0].r.y = row + work->y + YOFFSET - yoffset;
	obj[0].r.w = work->w;
	obj[0].r.h = lines * ICON_H;
}


/* 
 * Set one icon object 
 */

void set_obji( OBJECT *obj, long i, long n, boolean selected, int icon_no, int obj_x, int obj_y, char *name )
{
	CICONBLK *cicnblk;		/* icon block */

	cicnblk = (CICONBLK *) & obj[n + 1];

	obj[i + 1].ob_head = -1;
	obj[i + 1].ob_tail = -1;
	obj[i + 1].ob_type = icons[icon_no].ob_type;
	obj[i + 1].ob_flags = NONE;
	obj[i + 1].ob_state = (selected == FALSE) ? NORMAL : SELECTED;
	obj[i + 1].ob_spec.ciconblk = &cicnblk[i];
	obj[i + 1].r.x = obj_x;
	obj[i + 1].r.y = obj_y;
	obj[i + 1].r.w = ICON_W;
	obj[i + 1].r.h = ICON_H;

	cicnblk[i] = *icons[icon_no].ob_spec.ciconblk;
	cicnblk[i].monoblk.ib_ptext = name;
	cicnblk[i].monoblk.ib_char &= 0xFF00;
	cicnblk[i].monoblk.ib_char |= 0x20;

	objc_add(obj, 0, (int) i + 1);
}


/* 
 * Draw contents of an iconified window 
 */

void icw_draw(WINDOW *w)
{

	OBJECT *obj;			/* background and icon objects */
	RECT where;	 			/* location ans size of the background object */
	RECT r2;				/* rectangle for redraw test */
	INAME icname;			/* name of icon in desktop.rsc */
	int icon_no, icon_ind;	/* icon indices in resource files */

	/* allocate memory for objects: background + 1 icon */

	if ((obj = malloc( 2 * sizeof(OBJECT) + sizeof(CICONBLK))) == NULL)
	{
		xform_error(ENSMEM);
		return;
	}

	/* Set background object. It is needed for background colour / pattern */

	icwsize.x = w->xw_size.x;
	icwsize.y = w->xw_size.y;
	xw_calc(WC_WORK, NAME, &icwsize, &where, NULL);
	wd_set_obj0(obj, FALSE, 0, 1, YOFFSET, &where);

	/* determine which icon to use: file, floppy, disk or folder */

	if ( xw_type(w) == TEXT_WIND )
		icon_ind = FIINAME; 			/* file icon */
	else
	{
		if ( isroot( wd_path(w) ) )
		{
			if ( wd_path(w)[0] <= 'B' ) /* funny, this syntax works ! */
				icon_ind = FLINAME; 	/* floppy icon */
			else
				icon_ind = HDINAME;		/* hard disk icon */
		}
		else
			icon_ind = FOINAME;			/* folder icon */
	}

	icon_no = rsrc_icon_rscid( icon_ind, icname );

	/* Set icon */

	set_obji( obj, 0L, 1L, FALSE, icon_no, XOFFSET/2, YOFFSET, icname );

	/* (re)draw it */

	wind_update(BEG_UPDATE);

	xw_get((WINDOW *)w, WF_FIRSTXYWH, &r2);
	while ((r2.w != 0) && (r2.h != 0))
	{
 		objc_draw(obj, ROOT, MAX_DEPTH, r2.x, r2.y, r2.w, r2.h);
		xw_get((WINDOW *) w, WF_NEXTXYWH, &r2);
	}

	wind_update(END_UPDATE);

	free(obj);
}



/* 
 * How big should an iconified window be for an icon to fit 
 * into work area? Iconified window has only the title bar (i.e. NAME flag);
 * remember iconified size in icwsize- it is same for all windows.
 * Use this routine just once to determine the dimensions.
 * Note: this works perfectly with most AESses, but with some (NAES 1.1 ?)
 * it seems that size is not calculated exactly as it should be 
 * (by a couple of pixels)
 */

static void calc_icwsize(void)
{
	RECT 
		work;	  /* size of work area of iconified window */

    work.x = 0;
	work.y = 0;
	work.w = ICON_W;
	work.h = ICON_H;
	xw_calc(WC_BORDER, NAME, &work, &icwsize, NULL);
}


/* 
 * Routine for text/dir window iconification 
 */

void wd_type_iconify(WINDOW *w)
{

	/* Can this be done at all ? */

	if ( !can_iconify )
		return;

	/* Remember size of noniconified window */

	memcpy ( &( ((TYP_WINDOW *)w)->winfo->ix), &( ((TYP_WINDOW *)w)->winfo->x), sizeof(RECT) );

	/* Call function to iconify window */

	xw_iconify(w, icwsize.w, icwsize.h);

	/* If window is not open, return */

	if ( w->xw_opened == FALSE )
		return;

	/* Draw contents of iconified window */

	icw_draw(w);

	/* 
	 * Set a flag that this is an iconified window;
	 * note: this information is duplicated by xw_iconify
	 * in w->xw_iflag, so that xdialog routines know of
	 * iconified state. flags.iconified is used for saving
	 * in .cfg file
	 */

	(((TYP_WINDOW *)w)->winfo)->flags.iconified = 1;
	(((TYP_WINDOW *)w)->winfo)->flags.fulled = 0;
}


/* 
 * Routine for text/dir window deiconification 
 */

void wd_type_uniconify(WINDOW *w)
{
	/* Reset a flag that this is an iconified window */

	(((TYP_WINDOW *)w)->winfo)->flags.iconified = 0;

	/* Call uniconify function */

	xw_uniconify(w);

	/*
	 * XaAES (V0.963 and 0.970 at least) does not redraw uniconifed windows well. 
	 * The area occupied by the iconified window does not get redrawn.
	 * Below is a fix for that. This line of code is not needed
	 * with other AESses (like: AES4.1, Geneva4. Geneva6, Magic, N_AES1.1).
	 * In order to avoid needless redraw, redraw is restricted to mint only
	 * (XaAES is not explicitely detected) so that only AES4.1 and N_AES will
	 * suffer.
	 */

#if _MINT_
	if (mint && !magx && !naes)
		wd_type_draw ((TYP_WINDOW *)w, TRUE );
#endif

}


/* 
 * Open a window, normal size or iconified, depending on flag state.
 * Note that this routine is only used on directory windows and
 * text windows, NOT av-client windows (which do not have a WINFO pointer)
 */

void wd_iopen ( WINDOW *w, RECT *size )
{
	boolean icf = can_iconify && ((TYP_WINDOW *)w)->winfo->flags.iconified;
	WINFO *wi = ((TYP_WINDOW *)w)->winfo;
	RECT nsize;

#if _MINT_
	/* 
	 * In magic the window should be opened -after- iconifying,
	 * otherwise it will get the wrong title bar???
	 */

	if (icf && magx)
		xw_iconify(w, icwsize.h, icwsize.h );
#endif
		xw_open( w, size );


	wi->flags.iconified = 0; /* so that wd_calcsize would be correctly done */

	if ( icf )   
	{		
		memcpy(&( ((TYP_WINDOW *)w)->winfo->x), &( ((TYP_WINDOW *)w)->winfo->ix), sizeof(RECT) );
 		wd_calcsize ( wi, &nsize );
		wd_type_iconify(w);
		memcpy(&(w->xw_nsize), &nsize, sizeof(RECT) );
	}

}

