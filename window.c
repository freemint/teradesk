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
#include <vdi.h>
#include <tos.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>
#include <library.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "events.h"
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


#define WMINW 14 /* minimum window width (in character cell units) */
#define WMINH 4	 /* minimum window height (in character cell units) */

extern int 
	aes_wfunc,	/* result appl_getinfo(11, ...) */
	aes_ctrl;	/* this should probably be in xdialog.h */

extern FONT 
	txt_font,	/* font for directory windows */ 
	dir_font;	/* font for text viewer windows */

extern WINFO 
	textwindows[MAXWINDOWS],	/* some information about open windows */
	dirwindows[MAXWINDOWS];		/* some information about open windows */

extern RECT 
	dmax,		/* maximum window size */
	tmax;		/* maximum window size */

extern OBJECT 
	*icons;		/* icons from the resource file */

extern char
	*infname;	/* name of the current configuration fule */

#if _OVSCAN	
extern long
	over;		/* identification of overscan type */
extern int
	ovrstat;	/* state of overscan */
#endif

extern boolean
	onekey_shorts;	/* true if single-key menu shortcuts exist */

extern FONT 
	dir_font,
	txt_font;

char 
	floppy;

RECT 
	icwsize;	/* size of the iconified window */

boolean 
	wclose = FALSE,	/* true during windows being temporarily closed */
	can_touch,
	can_iconify;	/* true if current AES supports iconification */ 


SEL_INFO
	selection;

NEWSINFO1 
	thisw;		/* structure used when saving/loading window data */

SINFO2 
	that;		/* structure used when saving/loading window data */

FONT 
	*cfg_font;

boolean
	autoloc = FALSE;	/* true if autolocator is in effect */

#if _MINT_
LNAME automask = {0};	/* to compose the autolocator mask */
#else
SNAME automask = {0};	/* no need for long names in single-TOS */
#endif

static size_t 
	aml = 0;			/* length of string in automask */

static XFILE 
	*mem_file;			/* for saving temporarily closed windows */


int appl_control(int ap_id, int what, void *out);
void load_settings(char *newinfname);
static void desel_old(void);
static void itm_selall(WINDOW *w);
static void wd_type_hndlmenu(WINDOW *w, int title, int item);
static void icw_draw(WINDOW *w);
static boolean in_window(WINDOW *w, int x, int y);

WD_FUNC wd_type_functions =
{
	wd_type_hndlkey,
	wd_type_hndlbutton,
	wd_type_redraw,
	wd_type_topped,
	wd_type_bottomed,
	wd_type_topped,
	wd_type_close,	
	wd_type_fulled, 
	wd_type_arrowed,
	wd_type_hslider,
	wd_type_vslider,
	wd_type_sized,
	wd_type_moved,
	wd_type_hndlmenu,
	0L,
	wd_type_iconify,
	wd_type_uniconify
};


/*
 * Window font configuration table
 */

CfgEntry fnt_table[] =
{
	{CFG_HDR, 0, "font" },
	{CFG_BEG},
	{CFG_D,   0, "iden", &thisw.font.id		},
	{CFG_D,   0, "size", &thisw.font.size	},
	{CFG_D,   0, "fcol", &thisw.font.colour	},
	{CFG_D,   0, "feff", &thisw.font.effects},
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
	{CFG_D,   0, "xpos", &thisw.x	}, /* note: can't go off the left edge */
	{CFG_D,   0, "ypos", &thisw.y	},
	{CFG_D,   0, "winw", &thisw.ww	},
	{CFG_D,   0, "winh", &thisw.wh	},
	{CFG_D,   0, "xicw", &thisw.ix	},
	{CFG_D,   0, "yicw", &thisw.iy	},
	{CFG_D,   0, "wicw", &thisw.iw	},	/* Is this needed? */
	{CFG_D,   0, "hicw", &thisw.ih	},
	{CFG_X,   0, "flag", &thisw.flags	},
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configurtion of window font
 * Note: this is initialized to zero in wd_config.
 * There are no initialization or default routines for this
 */

CfgNest cfg_wdfont
{
	thisw.font.size = 0;

	if ( io == CFG_SAVE )
		thisw.font = *cfg_font;

	*error = handle_cfg(file, fnt_table, lvl, CFGEMP, io, NULL, NULL );

	if ( (io == CFG_LOAD) && (*error == 0) && thisw.font.size ) 
	{
		*cfg_font = thisw.font;
			fnt_setfont(thisw.font.id, thisw.font.size, cfg_font);
	}
}


/*
 * Turn off and reset autolocator and its name mask in the top window
 */

void autoloc_off(void)
{
	WINDOW *w = xw_top();
	autoloc = FALSE;
	aml = 0;
	*automask = 0;
	if(w && xw_type(w) == DIR_WIND && *(((DIR_WINDOW *)w)->info + 1) == '(' )
		dir_info((DIR_WINDOW *)w);
}


/* 
 * Some size-optimization mouse-related functions (reduced number of params)
 */

void arrow_mouse(void)
{
	graf_mouse(ARROW, NULL);
}


void hourglass_mouse(void)
{
	graf_mouse(HOURGLASS, NULL);
}


void mon_mouse(void)
{
	graf_mouse(M_ON, NULL);
}


void moff_mouse(void)
{
	graf_mouse(M_OFF, NULL);
}


/*
 * Write window text in transparent mode at location (x,y)
 */

void w_transptext( int x, int y, char *text)
{
	xd_vswr_trans_mode(); 
	v_gtext(vdi_handle, x, y, text);
}


/*
 * Is a window item some kind of a file (i.e. a file or a program?)
 */

boolean isfileprog(ITMTYPE type)
{
	if (type == ITM_FILE || type == ITM_PROGRAM)
		return TRUE;
	
	return FALSE;
}


/* 
 * Ensure that a loaded window is in the screen in case of a resolution change 
 * (or if these data were loaded from a human-edited configuration file)
 * and that it has a certain minimum size.
 * Note 1: windows can't go off the left and upper edges.
 * Note 2: width and height are in character cell units, x and y in pixels!
 */

static void wrect_in_screen(RECT *info, boolean normalsize)
{
	/* Window should be at least 32 pixels in the screen */

	info->x = minmax(0, info->x, screen_info.dsk.w - 32);
	info->y = minmax(0, info->y, screen_info.dsk.h - 32);

	info->w = min( info->w, screen_info.dsk.w / screen_info.fnt_w );
	info->h = min( info->h, screen_info.dsk.h / screen_info.fnt_h );

	/* Below is an arbitrary minimum window size (14 char x 5 lines) */

	if (normalsize )
	{	
		/* Note: do not use max() here, code would be longer */
		
		if (info->w < WMINW)
			info->w = WMINW;
		if (info->h < WMINH)
			info->h = WMINH;
	}
}


/*
 * Check if a window is in the screen 
 */

void wd_in_screen( WINFO *info ) 
{
	wrect_in_screen( (RECT *)(&(info->x)), (info->flags.iconified) ? FALSE : TRUE);
	wrect_in_screen( (RECT *)(&(info->ix)), TRUE );
}


/*
 * Determine unit cell size in a window.
 * It will be equal to character size (text font or directory font)
 * or icon size, depending on window type and display mode.
 */

void wd_cellsize(TYP_WINDOW *w, int *cw, int *ch)
{
	if (xw_type(w) == TEXT_WIND)
	{
		*cw = txt_font.cw;
		*ch = txt_font.ch;
	}
	else /* assumed to be dir window */
	{
		if (options.mode == TEXTMODE)
		{
			*cw = dir_font.cw;
			*ch = dir_font.ch + DELTA;
		}
		else
		{
			*cw = ICON_W;
			*ch = ICON_H;
		}
	}
} 


/*
 * Configuration of windows positions
 * Note: in this routine it is assumed that "iconified size" rectangle data
 * immedately follows "window size" rectangle data, and that "flags"
 * follow immediately after them in NEWSINFO1 thisw and also in WINFO.
 * If NEWSINFO1 or WINFO is modified in this aspect, code below is fail!
 */

CfgNest positions
{
	WINFO *w = thisw.windows;
	int i;

	size_t s = 2 * sizeof(RECT) + sizeof(thisw.flags); /* data size */

	if (io == CFG_SAVE)
	{
		/* Save data... */

		for (i = 0; i < MAXWINDOWS; i++)
		{
			thisw.i = i;

			memcpy(&thisw.x, &(w->x), s); /* copy all in one row */
	
			/* 
			 * In order to save on size of configuration file,
			 * don't save iconified size of noniconified windows 
			 */

			if (!(w->flags.iconified))
				memclr(&(thisw.ix), sizeof(RECT)); 

			*error = CfgSave(file, positions_table, lvl, CFGEMP);

			w++;

			if ( *error != 0 )
				break;
		}
	}
	else
	{
		/* Load data... */

		memclr(&thisw.ix, s);

		*error = CfgLoad(file, positions_table, MAX_KEYLEN, lvl);
		
		if ( (*error == 0) && (thisw.i < MAXWINDOWS) )
		{
			w += thisw.i;
			memcpy(&(w->x), &thisw.x, s); /* copy all in one row */
			thisw.i++;
		}
	}
}


/*
 * Configuration table for one window type
 */

CfgEntry wtype_table[] =
{
	{CFG_HDR, 0, NULL }, /* keyword will be substituted */
	{CFG_BEG},
	{CFG_NEST,0, "font", cfg_wdfont },
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
 * unmark all others.
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
	int i = 1 - (type >> 4);
	menu_checkone( 5, MSNAME, (type & ~WD_REVSORT) );
	menu_checkone( 1, MREVS,  i);

}


/* 
 * Check/mark (or the opposite) menu items for showing directory info fields 
 * It is assumed that these are in fixed order: MSHSIZ, MSHDAT, MSHTIM, MSHATT, MSHOWN;
 * check it with order of WD_SHSIZ, WD_SHDAT, WD_SHTIM, WD_SHATT, WD_SHOWN
 */

static void wd_set_fields( int fields )
{
	int i;

	for ( i = 0; i < 5; i++ )
		menu_icheck(menu, MSHSIZ + i, ( (fields & (WD_SHSIZ << i) ) ? 1 : 0));
}


/* 
 * Check/mark menu item for text/icon directory display mode.
 * It is assumed that MSHOWTXT and MSHOWICN are in sequence
 * (mode: 0=text, 1=icon).
 * Note: since TeraDesk V3.40 MSHOWTXT has been removed; 
 * MSHOWICN now toggles the mode 
 */

static void wd_set_mode(int mode)
{
#ifdef MSHOWTXT
	menu_checkone( 2, MSHOWTXT, mode );
#else
	menu_icheck(menu, MSHOWICN, mode ? 1 : 0 );	
#endif
}


/*
 * Count open windows (text, directory or accessory type)
 * Note: this routine relies on definitions of DIR_WIND, TEXT_WIND and 
 * ACC_WIND being in a sequence.
 */

int wd_wcount(void)
{
	WINDOW *h = xw_first();
	int n = 0;

	while (h)
	{
		if (xw_type(h) >= DIR_WIND && xw_type(h) <= ACC_WIND)
			n++;
		h = h->xw_next;
	}

	return n;
}


/*
 * Reset "selection"
 */

void wd_noselection(void)
{
	selection.w = NULL;
	selection.selected = -1;
	selection.n = 0;
}


/*
 * Note the first selected item in a window.
 * Attention: the item list is deallocated here!
 */

void wd_iselection(WINDOW *w, int n, int *list)
{
	wd_noselection();

	if(w && n > 0)
	{
		selection.n = n;
		selection.w = w;
		if (n == 1)
			selection.selected = list[0];
	}

	free(list);
}


#if _MINT_
/* 
 * Top the specified application
 * This works without any open windows, but unfortunately
 * not in all AESses. Will be ok in N.AES, XaAES and Magic 
 */

void wd_top_app(int apid)
{
	if(naes || aes_ctrl) 
		appl_control(apid, 12, NULL);	/* N.AES, XaAES */
	else 
		wind_set(-1, WF_TOP, apid);		/* Magic */
}
#endif


/*
 * Save the id of the top window (code=0) or restore it (code=1).
 * To be used when opening dialogs asked for by the clients of the 
 * AV or the FONT protocol. This should prevent TeraDesk from remaining
 * topped in a multitasking environment, after closing the dialog.
 * Probably relevant only in multitasking.
 * Note: this is not exactly right- if there are no open windows,
 * problems may be encountered.
 *
 */

#if _MINT_
void wd_restoretop(int code, int *whandle, int *wap_id)
{
	int p2, p3, p4;

	if(mint || geneva)
	{
		if(code == 0)
		{
			/* 
			 * Memorize which window is currently topped 
			 * Note: wind_get(, WF_TOP,...) is supposed to
			 * return ap_id of the owner in p2, but this does not
			 * seem to work, thence another call to get it.
			 * If failed, set *wap_id to -1; Data in p2 to p4 is
			 * returned but currently ignored.
			 */

			*wap_id = -1;
/* maybe there is no need ???
			if((aes_wfunc & 17) == 17) /* WF_TOP and WF_OWNER supported ? */ 
*/
				if(wind_get(0, WF_TOP, whandle, &p2, &p3, &p4))
					if(!wind_get( *whandle, WF_OWNER, wap_id, &p2, &p3, &p4))
						*wap_id = -1;
		}
		else
		{
			/* 
			 * Send a message to top the memorized window, but 
			 * do not send to itself- TeraDesk is already topped.
			 * Do this only if data are valid.
			 */

			if (*wap_id >= 0 && *wap_id != ap_id)
			{
				WINDOW w;
				w.xw_handle = *whandle;
				w.xw_ap_id = *wap_id;
				xw_send(&w, WM_TOPPED);
			}

			/* Just in case... */

			wd_top_app(av_current);
		}
	}
}
#endif


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
		topicf = 0,			/* nonzero if top window is iconified */
		n = 0,				/* number of selected items */
	    *list = NULL,		/* pointer to a list of selected items */
		wtoptype = 0,		/* type of the topped window */
		nwin = 0,			/* number of open windows */
	    i = 0;				/* counter */

	boolean 
		nonsel,				/* true if nothing selected and dir/text topped */
		showinfo = FALSE,	/* true if show info is enabled */
	    enab = FALSE,		/* flag for enabling diverse options */
	    enab2 = FALSE;		/* flag for enabling diverse options */

	char 
		drive[8];			/* disk drive name string */

	ITMTYPE 
		type,				/* type of an item in the list */
		type2 = ITM_FILE;	/* same */


	/*
	 * Does this (selection) window exist at all, and is there a list 
	 * of selected items? If the window does not exist, locally zero
	 * the pointer to it.
	 */

	if(!itm_list(w, &n, &list))
		w = NULL;

	/* 
	 * Find out the type of the top window.
	 * Also find if windows are iconified and how many exist
	 */

	wtop = xw_top();

	if ( wtop )
	{
		wtoptype = xw_type(wtop);
		topicf =  (can_iconify) ? (wtop->xw_xflags & XWF_ICN) : 0; 
	}

	nwin = wd_wcount();
	nonsel = ( !n && (wtoptype == DIR_WIND || wtoptype == TEXT_WIND) );

	if(n > 0)
		showinfo = TRUE;

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
	menu_ienable(menu, MSEARCH, showinfo);

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
 			|| (mint && (n == 1))  
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
		enab = ( isfile(type) );  
		enab2 = ( enab2 && isfileprog(type) );
	}	

	/*
 	 * enab2 (for printing) will also be set if a dir or text window 
	 * is topped  (and not iconified) and nothing is selected
	 */

	if ( nonsel && !topicf )
		enab2 = TRUE;
  
	/*
	 * Enable delete only if there are only files, programs and 
	 * folders among selected items;
	 * Enable print only if there are only files among selected items
	 * or a window is open (to print directory or text)
	 */

	menu_ienable(menu, MDELETE, enab && (n > 0));
	menu_ienable(menu, MPRINT, enab2);

	/* Determine enabling states for Compare, Programtype and App menu items */

	if (n == 0)
	{
		enab = TRUE;
		enab2 = TRUE;
	}
	else /* n > 0 */
	{
		enab = FALSE;
		enab2 = FALSE;

		type = itm_type(w, list[0]);

		if (n >= 2)
			type2 = itm_type(w, list[1]);

		if ( (n < 3) && isfileprog(type) && isfileprog(type2) )
		{
			enab = TRUE;
			if ( type == ITM_PROGRAM )
				enab2 = TRUE;
		}
	}

	/* Compare will be enabled only if there are not more than two files selected */

	menu_ienable(menu, MCOMPARE, enab );

	/* 'Select all' is possible only on directory or desk window */

	menu_ienable(menu, MSELALL, (int)( (wtoptype == DIR_WIND && !topicf) || wtoptype == DESK_WIND ));

	/* Enable window closing if there are open windows */

	menu_ienable(menu, MCLOSE, nwin);
	menu_ienable(menu, MCLOSEW, nwin);
	menu_ienable(menu, MCLOSALL, nwin);

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

	menu_ienable(menu, MPRGOPT, enab);
	menu_ienable(menu, MAPPLIK, enab2); 

	/* Enable setting of window icons */

	menu_ienable(menu, MIWDICN, (n < 2) );

  	/*
	 * enable disk formatting and disk copying
	 * only if a single drive is selected and it is A or B;
	 * use the opportunity to say which drive is to be used 
	 * (is it always uppercase A or B, even in other filesystems ?)
	 */

	enab=FALSE;
	if ( (n == 1) && (type == ITM_DRIVE) )
	{
		char *fullname = itm_fullname ( w, list[0] );
		strsncpy ( drive, fullname , sizeof(drive) );
		free(fullname);
		drive[0] &= 0x5F; /* to uppercase */
		if 
		(   
			   ( drive[0] >= 'A' )
			&& ( drive[0] <= 'B' )
			&& ( drive[1] == ':' )
		)
		{  
			floppy = drive[0];      /* i.e. floppy= 65dec (A) or 66dec (B) */
			enab = TRUE;
		}  
	}

/* By removing floppy formatting/copying from the resource, this will be deleted */

#if MFFORMAT
	menu_ienable(menu, MFCOPY, enab );
	menu_ienable(menu, MFFORMAT, enab );
#endif

	/* Notify the first selected item */

	wd_iselection(w, n, list); /* attention: list deallocated */
}


/*
 * What this routine does is also set in itm_set_menu,
 * but below is smaller/faster.
 */

void wd_setselection(WINDOW *w)
{
	int n, *list;

	itm_list(w, &n, &list);		/* note: allocate list, or set to null */
	wd_iselection(w, n, list);	/* note: also deallocates list */
}


/********************************************************************
 *																	*
 * Funkties voor het deselecteren van objecten in windows.			*
 *																	*
 ********************************************************************/

/*
 * Deselect all selected objects in directory windows or
 * on the disktop. This function also turns off the autolocator
 */

void wd_deselect_all(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->itm_select(w, -1, 0, /* FALSE */ TRUE);
		w = w->xw_next;
	}

	((ITM_WINDOW *) desk_window)->itm_func->itm_select(desk_window, -1, 0, TRUE);

	wd_setselection(NULL);

/* no need?
	selection.w = NULL;
	selection.n = 0;
*/
	autoloc_off();
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
	
	autoloc_off();
}


/********************************************************************
 *																	*
 * Funktie voor het opvragen van het pad van een window.			*
 *																	*
 ********************************************************************/

/*
 * Return a pointer to the string containing the path 
 * of the uppermost directory window
 */

const char *wd_toppath(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			return ((DIR_WINDOW *)w)->path; 
		w = w->xw_next;
	}

	return NULL;
}


/* 
 * Return a pointer to the string containing path of a directory window
 * or path+filename of a text window. Otherwise return NULL.
 */

const char *wd_path(WINDOW *w)
{
	if(xw_type(w) == DIR_WIND || xw_type(w) == TEXT_WIND)
		return ((TYP_WINDOW *)w)->path; 

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
	ITMFUNC *thefunc;

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
		{
			thefunc = ((ITM_WINDOW *)w)->itm_func;

			if (drive == -1)
				/* update all */
				thefunc->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
			else
			{			
				const char *path = thefunc->wd_path(w);

				boolean goodpath = (((path[0] & 0x5F - 'A') == drive) && (path[1] == ':')) ? TRUE : FALSE;
#if _MINT_
				if (mint)
				{
					if ((drive == ('U' - 'A')) && goodpath ) /* this is for U drive */
						thefunc->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
					else
					{
						XATTR attr;

						if ((x_attr(0, FS_LFN, path, &attr) == 0) && (attr.dev == drive)) /* follow the link */
							thefunc->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
					}
				}
				else
#endif
				{
					if ( goodpath )
						thefunc->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
				}
			}

		}
		w = w->xw_next;
	}

	wd_do_update();
}


/*
 * Close a directory, text or accessory window.
 * Note: take care not to attempt to use it on desk window
 */

void wd_type_close( WINDOW *w, int mode)
{
	int wt = xw_type(w);

	/* Act according to window type */

	if (wt == ACC_WIND)
		va_close(w);
	else
	{
		/* fulled and setmask flags are preserved for temporary close */

		if (!wclose)
		{
			((TYP_WINDOW *)w)->winfo->flags.fulled = 0;
			((TYP_WINDOW *)w)->winfo->flags.setmask = 0;
		}

		if (wt == TEXT_WIND)
			txt_closed(w);
		else
			dir_close(w, mode);
	}		
}


/*
 * Handle menus in windows (currently in text window only)
 */

static void wd_type_hndlmenu(WINDOW *w, int title, int item)
{
	if (xw_type(w) == TEXT_WIND)
		txt_hndlmenu(w, title, item);
}


/*
 * Delete all windows EXCEPT pseudowindows opened through AV-protocol
 */

void wd_del_all(void)
{
	WINDOW *prev, *w = xw_last();

	while (w)
	{
		prev = w->xw_prev;
		if ( xw_type(w) != ACC_WIND )
			wd_type_close(w, 1);		
		w = prev;
	}
}


/*
 * Check if a window was successfully opened. If so, return TRUE.
 * Note: this routine resets error to 0!
 * Used for checking opening of saved windows.
 * Currently, return status is, in fact, irelevant.
 */

boolean wd_checkopen(int *error)
{
	if (*error)
	{
		alert_printf(1, AWOPERR, get_message(*error));
		*error = 0;
		return FALSE;
	}

	return TRUE;
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

#ifdef MSHOWTXT
	/* Do something only if mode has changed */

	if ( mode != options.mode )
	{
#endif
		options.mode = mode;

		while (w)
		{
			if (xw_type(w) == DIR_WIND)	
			{
				wd_type_nofull(w);
								
				if ( mode )	
				{
					/* 
					 * Icon mode; find appropriate icons.
					 * This can take a long time, change mouse form to busy 
					 */

					hourglass_mouse();
					dw = (DIR_WINDOW *)w;
			
					for ( i = 0; i < dw->nfiles; i++ )
					{
						NDTA *d = (*dw->buffer)[i];
						d->icon = icnt_geticon( d->name, d->item_type, d->tgt_type);
					}

					arrow_mouse();
				}

				/* Now really make the change */

				dir_disp_mode(w);
			}
			w = w->xw_next;
		}

		/* mark (check) appropriate menu entry */

		wd_set_mode(mode);

#ifdef MSHOWTXT
	}
#endif

}


/*
 * Set auto-arrangement of items in directory windows
 */

static void wd_arr(void)
{
	WINDOW *w = xw_first();

	options.aarr = !options.aarr; 					/* toggle option */
	menu_icheck(menu, MAARNG, (int)options.aarr);	/* set menu item */

	while(w)
	{
		wd_type_nofull(w);

		if ( xw_type(w) == DIR_WIND )
			dir_disp_mode(w);

		w = w->xw_next;
	}
}


/* 
 * Sort directory windows according to the selected key 
 * (name, size, date...) 
 */

static void wd_sort(int sort) 
{
	WINDOW *w = xw_first();

	options.sort = sort; /* save this in options */

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			dir_sort(w, sort);
		w = w->xw_next;
	}

	wd_set_sort(sort);	/* mark a menu item */
}


/* 
 * Set visible directory fields in all directory windows 
 */

void wd_fields(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
		{
			wd_type_nofull(w);
			dir_newdir((DIR_WINDOW *)w);
		}
		w = w->xw_next;
	}
	wd_set_fields(options.fields);
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
 * Sometimes it is convenient to fake that the file in a topped text window
 * is selected in a directory window. Path must be kept to be deallocated
 * after later operations.
 */

static void wd_ontext(WINDOW *ww, char **thepath)
{
	char
		*thisname = ((TXT_WINDOW *)xw_top())->path,
		*thename;

	*thepath = fn_get_path( thisname );

	if (*thepath)
	{
		thename = fn_get_name( thisname );
		dir_simw((DIR_WINDOW *)ww, *thepath, thename, ITM_FILE, (size_t)1, 0);
	}
	else
		xform_error(ENSMEM);
}


/* 
 * Handle some of the main menu activities 
 * (the rest is covered in main.c).
 * Checking of window existence is not needed in some cases, because
 * those menu items would have been already disabled otherwise.
 * See itm_set_menu()
 */

void wd_hndlmenu(int item, int keystate)
{
	DIR_WINDOW
		simw;			/* space for a simulated dir window */

	WINDOW 
		*w, 			/* pointer to the window in which a selection is made */
		*ww,			/* pointer a real or simulated window to search in or show info of */
		*wtop;			/* pointer to the top window */

	int 
		i,				/* aux. counter */
		n = 0,			/* number of items selected */ 
		*list = NULL,	/* pointer to a list of selected items indices */
		wtoptype = 0; 	/* type of the top window */

	char			
		*thepath = NULL,/* aux. path */
		*toppath = NULL;/* path of the topped window */


	w = selection.w;
	ww = w;

	if(w && !itm_list(w, &n, &list))
		n = 0;	/* if a list can not be allocated */

	/* Take the opportunity to update window order */

	xw_bottom();	
	wtop = xw_top();

	if ( wtop )
		wtoptype = xw_type(wtop);

	switch (item)
	{
	case MOPEN:
		if(n)
		{
			int k, delayt;
			ITMTYPE it;

			for(i = 0; i < n; i++ )
			{
				delayt = 0;
				k = keystate;
				it = itm_type(w, list[i]);

				/* Provision for multiple openings... */

				if (n > 1)
				{
					wd_drawall();

					if(it == ITM_FOLDER || it == ITM_DRIVE || it == ITM_PREVDIR)
						k |= K_ALT; /* open directories in new windows */
					else if (it != ITM_FILE || app_find(itm_name(w, list[i]), TRUE) != NULL )
						delayt = 2000; /* a program or a file assigned to a program */
				}

				if (itm_open(w, list[i], k))
				{
					/* 
					 * In single-TOS, can't deselect after a program
					 * because windows will have been reopened
					 */

					if(selection.w)
						itm_select(w, list[i], 2, TRUE); /* deselect this item */
				}

				/* Wait some time for programs to settle */

				wait(delayt);
			}
		}
		else
			/* 
			 * If nothing is selected, open the dialog to enter 
			 * item specification 
			 */
			item_open( NULL, 0, 0, NULL, NULL );	
		break;
	case MSHOWINF:
	case MSEARCH:
	{
		*dirname = 0; /* this is the buffer for the search pattern */

		if (w == NULL && wtop)
		{
			/* Nothing is selected, info on the disk of the top window  */

 			if ( wtoptype == DIR_WIND )
			{			
				if ( (thepath = strdup(wd_toppath()) ) != NULL )
				{
					/* 
					 * Note: if the next line is removed, and
					 * item type is set to ITM_FOLDER, then
					 * info on the current directory will be shown
					 * instead on the drive
					 */

					if(item == MSHOWINF)
						thepath[3] = 0;

					ww = (WINDOW *)&simw;
					dir_simw(&simw, thepath, ".", ITM_DRIVE, 0, 0);
				}
			}
			else if ( wtoptype == TEXT_WIND )
			{
				ww = (WINDOW *)&simw;
				wd_ontext(ww, &thepath);
			}
		}

		/* Perform search and/or show information */

		if(n || (thepath && itm_list(ww, &n, &list))) 
		{
			if(n == 1 && itm_type(ww, list[0]) == ITM_FILE)
				cv_fntoform(searching, SMASK, itm_name(ww, list[0]));

			if ( item == MSHOWINF || !app_specstart( AT_SRCH, ww, list, n, 0 ) )
				item_showinfo(ww, n, list, (item == MSHOWINF) ? FALSE : TRUE);

			if(w == NULL) /* deselect all in a simulated window */
				wd_noselection();
		}

		free(thepath);
	}
	break;
	case MNEWDIR:

#if _MINT_
		if 
		(
			mint && 
			(n == 1) &&
			(wtoptype == DIR_WIND) &&
			((( (DIR_WINDOW *)wtop)->fs_type & FS_LNK) != 0)
		)
		{
			/*
			 * Create a symbolic link.
			 * Note: with a change in dir_newlink and the resource file
			 * (add Abort & Skip buttons) this can easily be modified
			 * to handle more than one item at a time.
			 */

			if ( (thepath = itm_fullname(w, list[0])) != NULL)
			{
				dir_newlink(wtop, thepath);
				free(thepath);
			}
			else
				xform_error(ENSMEM);
		}
		else
#endif
			if ( n == 0 && wtoptype == DIR_WIND )
				dir_newfolder(wtop);
		break;
	case MCOMPARE:
		if(w == NULL && wtoptype == TEXT_WIND)
		{
			ww = (WINDOW *)&simw;
			wd_ontext(ww, &thepath);
		}

		itm_list(ww, &n, &list);

		if ( !app_specstart( AT_COMP, ww, list, n, 0 ) )
			compare_files(ww, n, list); /* Only items #0 and #1 from the list */

		if(w == NULL) /* deselect all in a simulated window */
			wd_noselection();

		free(thepath);
		break;
	case MDELETE:
		if(n)
			itmlist_wop(w, n, list, CMD_DELETE);
		break;
	case MPRINT:
	{
		int cmd = CMD_PRINT;

		printmode = PM_TXT;

		if(w == NULL && wtop)
		{
			/* Nothing selected, print directory or contents of text window */

			if ( wtoptype == DIR_WIND )
			{
					/* Select everything in this window */

				cmd = CMD_PRINTDIR;
				ww = wtop;
				itm_selall(ww);
			}
			else if ( wtoptype == TEXT_WIND )
			{
				if ( ((TXT_WINDOW *)wtop)->hexmode )
					printmode = PM_HEX;
				ww = (WINDOW *)&simw;
				wd_ontext(ww, &thepath);
			}
		}


		if(n || ((thepath || (wtoptype == DIR_WIND)) && itm_list(ww, &n, &list))) 
		{
			itmlist_wop( ww, n, list, cmd); 
			if(cmd == CMD_PRINTDIR || w == NULL)
				wd_deselect_all();
/* no need?
			if(w == NULL) /* deselect all in a simulated window */
				wd_noselection();
*/
		}

		free(thepath);
		break;
	}
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
			dir_filemask((DIR_WINDOW *)wtop);
		else
			wd_filemask(NULL);	/* the dialog */
		break;
	case MSHOWICN:
#ifdef MSHOWTXT
	case MSHOWTXT:
		wd_mode(item - MSHOWTXT); 
#else
		wd_mode(!options.mode);	
#endif
		break;
	case MAARNG:
		wd_arr();
		break;
	case MSNAME:
	case MSEXT:
	case MSDATE:
	case MSSIZE:
	case MSUNSORT:
		wd_sort( (item - MSNAME) | (options.sort & WD_REVSORT) );
		break;
	case MREVS:
		options.sort ^= WD_REVSORT;
		wd_sort(options.sort);
		break;
	case MSHSIZ: /* beware of the order of cases vs flag values */
	case MSHDAT:
	case MSHTIM:
	case MSHATT:
	case MSHOWN:
		options.fields ^= (WD_SHSIZ << (item - MSHSIZ));
		wd_fields();
		break;
	case MCLOSE:
		i = 0;
		goto closeit;
	case MCLOSEW:
	case MCLOSALL:
		i = 1;
		closeit:;
		while(wtop && (xw_type(wtop) != DESK_WIND))
		{
			wd_type_close(wtop, i);
			if (item != MCLOSALL)
				break;
			wtop = xw_top();
		}
		break;
	case MDUPLIC:
		toppath = strdup(wd_path(wtop));
		if ( toppath )
		{
			if ( wtoptype == DIR_WIND )
				dir_add_dwindow(toppath);
			else if (wtoptype == TEXT_WIND )
				txt_add_window(NULL, 0, 0, toppath);
		}
		break;
	case MSELALL:
		if ( wtoptype == DIR_WIND || wtoptype == DESK_WIND )
		{
			if (w != wtop)
				desel_old();
			itm_selall(wtop);
		}
		break;
	case MCYCLE:
		xw_cycle();
		break;     
	case MIDSKICN:
		if(n > 0 && w->xw_type == DESK_WIND)
			dsk_chngicon(n, list, TRUE);
		else 
			dsk_insticon(w, n, list);
		break;
	case MIWDICN:
		icnt_settypes();
		break;
	case MLOADOPT:
		load_settings((n == 1 && itm_type(w, list[0]) == ITM_FILE) ? itm_fullname(w, list[0]) : locate(infname, L_LOADCFG));
		break;
	}

	free(list);
}


/********************************************************************
 *																	*
 * Functions for initialisation of the windows modules and loading	*
 * and saving about windows.										*
 *																	*
 ********************************************************************/

/*
 * Create some reasonable default window sizes and positions.
 * Each new window of a type will generally be located a little bit
 * to the right and down.
 */

static void wd_defsize(int type)
{
	WINFO *wi;
	int i;

	for (i = 0; i < MAXWINDOWS; i++)
	{
		int x0, x1, y0, y1, w1, w2, w3, h1, h2;

		if (type == TEXT_WIND)
		{
			wi = &textwindows[i];
			txt_font = def_font;
			x0 = 16;
			x1 = i;
			y0 = 8;
			y1 = i * 2;
			w1 = 7;
			w3 = i + i % 2;
			h1 = 4;
		}
		else
		{
			wi = &dirwindows[i];
			dir_font = def_font;
			x0 = 64;
			x1 = i * 2;
			y0 = 20;
			y1 = i; 
			w1 = 5;
			w3 = i * 2;
			h1 = 5;
		}

		w2 = 10;
		h2 = 10;

		wi->x = x0 + x1 * screen_info.fnt_w + screen_info.dsk.x;
		wi->y = y0 + y1 * screen_info.fnt_h + screen_info.dsk.y;
		wi->w = screen_info.dsk.w * w1 / (w2 * screen_info.fnt_w) + w3; 	/* in char cell units */
		wi->h = screen_info.dsk.h  * h1 / (h2 * screen_info.fnt_h);			/* in char cell units */
	}
}


/* 
 * Determine maximum permitted window sizes, mostly depending on screen area
 */

void wd_sizes(void)
{
	RECT work;

	/* Maximum possible sizes of directory and text windows */

	xw_calc(WC_WORK, DFLAGS, &screen_info.dsk, &work, NULL);
	dmax.x = screen_info.dsk.x;
	dmax.y = screen_info.dsk.y;
	dmax.w = work.w - (work.w % screen_info.fnt_w);
	dmax.h = work.h + DELTA - (work.h % (screen_info.fnt_h + DELTA));

	xw_calc(WC_WORK, TFLAGS, &screen_info.dsk, &work, viewmenu);
	tmax.x = screen_info.dsk.x;
	tmax.y = screen_info.dsk.y;
	tmax.w = work.w - (work.w % screen_info.fnt_w);
	tmax.h = work.h - (work.h % screen_info.fnt_h);

	/* 
	 * How big should an iconified window be for an icon to fit 
	 * into work area? Iconified window has only the title bar (i.e. NAME flag);
	 * remember iconified size in icwsize- it is the same for all windows.
	 * Use this routine just once to determine the dimensions.
	 * Note: this works perfectly with most AESses, but with some (NAES 1.1 ?)
	 * it seems that size is not calculated exactly as it should be 
	 * (by a couple of pixels)
	 */

    work.x = 0;
	work.y = 0;
	work.w = ICON_W;
	work.h = ICON_H;
	xw_calc(WC_BORDER, NAME, &work, &icwsize, NULL);
	can_iconify = aes_wfunc & 128; 
}


void wd_default(void)
{
	wd_deselect_all();
	wd_del_all();
	wd_set_mode(options.mode);
	wd_set_sort(options.sort);
	menu_icheck(menu, MAARNG, (int)options.aarr);
	wd_set_fields(options.fields);
	wd_sizes();
	wd_defsize(TEXT_WIND);
	wd_defsize(DIR_WIND);
}


#if !__USE_MACROS

/*
 * Initialize windows data
 */

void wd_init(void)
{
	/* Nothing is selected */

/* no need?
	wd_noselection();

	/* Determine default windows sizes */

	wd_sizes();
*/

	wd_default();
}

#endif


/* 
 * Functies voor het veranderen van het window font.
 * Note: this is only for dir windows and text windows 
 * Return TRUE if OK selected.
 */

boolean wd_type_setfont(int title)
{
	int 
		i;			/* counter */

	RECT 
		work;

	WINFO 
		*wd = &dirwindows[0];
 
	FONT 
		*the_font = &dir_font;

	TYP_WINDOW 
		*tw;

	if ( title == DTVFONT )
	{
		the_font = &txt_font;
		wd = &textwindows[0];
	}

	if ( fnt_dialog(title, the_font, FALSE) )
	{
		for (i = 0; i < MAXWINDOWS; i++)
		{
			tw = wd->typ_window;

			if ( wd->used && tw )
			{
				wd_type_nofull((WINDOW *)tw);
				xw_getwork((WINDOW *)tw, &work); 
				calc_rc(tw, &work); 

				/* no need to set sliders here, see wd_drawall */
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
 * Calculate number of lines and columns, total and visible, in a window.
 * 'work' is input parameter denoting the size of the rectangle
 * the calculation should apply to.
 */

void calc_rc(TYP_WINDOW *w, RECT *work)
{
	int cw, ch;

	wd_cellsize(w, &cw, &ch);

	/* Note: do not set w->columns here */

	w->rows     = (work->h +  ch - 1) / ch;
	w->nrows    =  work->h / ch; 
	w->ncolumns =  work->w /  cw;
	w->scolumns = work->w / screen_info.fnt_w;

	if (xw_type(w) == DIR_WIND)
	{
		w->xw_work.w = work->w;
		calc_nlines((DIR_WINDOW *)w);
	}
}


/* 
 * Funktie die uit opgegeven grootte de werkelijke grootte van het
 * window berekent.
 * If iswork == TRUE, input->w and input->h are assumed to be equal to 
 * area (but -not- input->x and input->y).
 * This routine also limits window size so that it does not
 * exceed the screen.
 */

void wd_wsize(TYP_WINDOW *w, RECT *input, RECT *output, boolean iswork)
{
	RECT 
		work, 
		*dtmax = &dmax;

	int 
		fw, 
		fh, 
		d = DELTA, 
		wflags = DFLAGS;

	OBJECT 
		*menu = NULL;

	/* Normal window size */

	if ( xw_type((WINDOW *)w) == TEXT_WIND )
	{
		menu = viewmenu;
		d = 0;
		wflags = TFLAGS;
		dtmax = &tmax;
	}

	/* Calculate work area from overall size */

	xw_calc(WC_WORK, wflags, input, &work, menu);

	if (iswork)
	{
		work.w = input->w;
		work.h = input->h;
	}

	/* Round-up position and dimensions */

	fw = screen_info.fnt_w;
	fh = screen_info.fnt_h + d;

	work.x += fw / 2;
	work.w += fw / 2;
	work.h += fh / 2 - d;
	work.x -= (work.x % fw);
	work.w -= (work.w % fw);
	work.h -= (work.h % fh) - d;

	/* Note: do not use min() here */

	if ( work.w > dtmax->w)
		work.w = dtmax->w;
	if ( work.h > dtmax->h)
		work.h = dtmax->h;

	if ( (w->xw_xflags & XWF_ICN) == 0 )
	{
		work.w = max(work.w, WMINW * fw);
		work.h = max(work.h, WMINH * fh);
	}

	xw_calc(WC_BORDER, wflags, &work, output, menu);
	calc_rc((TYP_WINDOW *)w, &work);
	
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
 * Calculate window size; merge of txt_calcsize and dir_calcsize.
 * Special considerations when the window is fulled.
 */

void wd_calcsize(WINFO *w, RECT *size)
{
	TYP_WINDOW
		*tw = (TYP_WINDOW *)(w->typ_window);

	OBJECT 
		*menu = NULL;

	RECT 
		def = screen_info.dsk, 
		border;

	int 
		d =  DELTA,
		wflags = DFLAGS,
		wtype = xw_type((WINDOW *)(tw));

	boolean
		iswork = FALSE;

	int 
		ch, cw;


	if ( wtype == TEXT_WIND )
	{
		menu = viewmenu;
		wflags = TFLAGS;
		d = 0;
	}

	/* Find position of the window on screen */

	def.x = w->x + screen_info.dsk.x;
	def.y = w->y + screen_info.dsk.y;

	/* If the window is fulled, calculate its new size, etc. */

	if (w->flags.fulled == 1)
	{
		/* Adjust size of a fulled window- no more than needed */

		long ll = tw->nlines;

		iswork = TRUE;

		/* 
		 * Calculation will depend on whether this is a shift-full,
		 * and also on whether auto-arrange is set
		 */

		if (w->flags.fullfull)
		{
			/* Full to the whole screen */

			def.w = max_w - (tw->xw_size.w - tw->xw_work.w);
			def.h = max_h - (tw->xw_size.h - tw->xw_work.h);
		}
		else
		{
			/* Full only as much as needed */

			wd_cellsize(tw, &cw, &ch);
			def.w = tw->columns * cw;

			/* 
			 * Add some empty lines to fulled directory directory windows
			 * This will make draging of items into windows more comfortable. 
			 * Add two lines in text mode, or one row in icons mode.
			 */

			if (wtype == DIR_WIND)
			{
				ll += 2;	/* two lines in text mode */
				if (options.mode != TEXTMODE)
				{
					ll -= 1; /* one line is enough in icon mode */
					def.w += XOFFSET;
				}
			}

			/* Window outside size must not exceed max integer */

			def.h = lmin(32700, ll * ch);
		}

		wd_wsize(tw, &def, size, iswork);

		size->x = max(0, min(size->x, screen_info.dsk.w - size->w - 4)); 
		size->y = max(screen_info.dsk.y, min(size->y, screen_info.dsk.h - size->h));
	}
	else
	{
		/* 
		 * Window is not fulled;
		 * Window work area size in WINFO is in screen-font character units; 
		 * therefore a conversion is needed here (def = window work area);
		 */

		def.w = w->w * screen_info.fnt_w;	/* hoogte en breedte van het werkgebied. */
		def.h = w->h * (screen_info.fnt_h + d) + d;

		/* Calculate window width and height */

		xw_calc(WC_BORDER, wflags, &def, &border, menu);

		border.x = def.x;
		border.y = def.y;

		wd_wsize(tw, &border, size, FALSE); 
	}
}


/* 
 * Merge of txt_redraw and do_redraw functions for text and dir windows 
 */

void wd_type_redraw(WINDOW *w, RECT *r1)
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

	xw_getwork(w, &work);

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
				int nc = ((DIR_WINDOW *)w)->ncolumns;

				if ( work.w % ICON_W != XOFFSET)
					nc++;

				if ((obj = make_tree
				(
					(DIR_WINDOW *)w,
					((DIR_WINDOW *)w)->px,
					nc,
					(int)(((DIR_WINDOW *)w)->py), 
					((DIR_WINDOW *)w)->rows, 
					FALSE, 
					&work
				)) == NULL) 
					return;
				text = FALSE;
			}
		}

		xd_wdupdate(BEG_UPDATE);
		moff_mouse();

		xw_getfirst(w, &r2);
		while (r2.w != 0 && r2.h != 0)
		{
			if (xd_rcintersect(r1, &r2, &in))
			{
				xd_clip_on(&in);
				if ( xw_type(w) == TEXT_WIND )
					txt_prtlines((TXT_WINDOW *) w, &in);
				else
					do_draw((DIR_WINDOW *)w, &in, obj, text, &work);
				xd_clip_off();
			}

			xw_getnext(w, &r2);
		}

		mon_mouse();
		xd_wdupdate(END_UPDATE);

		free(obj);
	}	
}


/* 
 * Merge of txt_draw and dir_draw 
 */

void wd_type_draw(TYP_WINDOW *w, boolean message) 
{
	RECT area;

	xw_getsize((WINDOW *)w, &area);
	if (message)
		xw_send_redraw((WINDOW *)w, &area);
	else
		wd_type_redraw((WINDOW *) w, &area);
}


/* 
 * Draw a window but set sliders first
 */

void wd_type_sldraw(TYP_WINDOW *w, boolean message)
{
	set_sliders(w);
	wd_type_draw(w, message);
}


/*
 * Redraw all windows
 */

void wd_drawall(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		/* 
		 * Note: windows below dialogs do not get redrawn nicely
		 * if wd_type_draw() is called with message=FALSE
		 * (as in earlier versions of this file)
		 */
/* below is shorter
		switch( xw_type(w))
		{
			case DIR_WIND:
			case TEXT_WIND:
				set_sliders((TYP_WINDOW *)w);
			case DESK_WIND:
			case ACC_WIND:
				wd_type_draw((TYP_WINDOW *)w, TRUE);
		}
*/
		if(xw_type(w) == DIR_WIND || xw_type(w) == TEXT_WIND)
			set_sliders((TYP_WINDOW *)w);
		wd_type_draw((TYP_WINDOW *)w, TRUE);

		w = w->xw_next;
	}
}


/*
 * Button event handler for text & dir windows.
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
	if ( xw_type(w) == DIR_WIND )
		wd_hndlbutton(w, x, y, n, button_state, keystate);
}


/* 
 * Window topped handler for dir and text window 
 */

void wd_type_topped (WINDOW *w)
{
	autoloc_off();
	xw_set(w, WF_TOP);
}


void wd_type_bottomed(WINDOW *w)
{
	autoloc_off();
	xw_set(w, WF_BOTTOM);
}


/*
 * Set window title. For the sake of size optimization, 
 * also set window sliders, because that routine was always 
 * (except in one place once) called immediately after setting the title.
 */

void wd_type_title(TYP_WINDOW *w)
{
	int 
		d = 3,		/* Default single-TOS value */ 
		columns;

	char 
		*fulltitle;

	/* 
	 * aes_hor3d is usually either 0 or 2 pixels;
	 * line below increases d by about 1 character 
	 * if there is nonzero aes_hor3d
	 */

	if(xd_aes4_0)
		d = 5 + aes_hor3d / 2;

	/* Calculate available title width (in characters) */

	columns = min( w->scolumns - d, (int)sizeof(w->title) );

	if(xw_type(w) == TEXT_WIND)
		fulltitle = strdup(((TXT_WINDOW *)w)->path);
	else
		fulltitle = fn_make_path(((DIR_WINDOW *)w)->path, ((DIR_WINDOW *)w)->fspec);

	/* Set window */

	if(fulltitle)
		cramped_name(fulltitle, w->title, columns);

	free(fulltitle);
	xw_set((WINDOW *)w, WF_NAME, w->title);
	set_sliders(w);
}


/*
 * Window fuller handler.
 * Two fulled states are recognized: normal and fullscreen.
 * The second one is obtained by pressing the shift key while
 * clicking on the fuller window gadget.
 * Alternate clicks on the fuller gadget will toggle the window
 * between the fulled and the normal state.
 */

void wd_type_fulled(WINDOW *w, int mbshift)
{
	WINFO *winfo = ((TYP_WINDOW *)w)->winfo; 
	boolean f = (( mbshift & ( 0x03 ) ) != 0 );


	if ( (winfo->flags.fulled == 0) || (f && (winfo->flags.fullfull == 0)) )
	{
		winfo->flags.fulled = 1;
		if (mbshift)
			winfo->flags.fullfull = 1;
	}
	else
	{
		winfo->flags.fulled = 0;
		winfo->flags.fullfull = 0;
	}

	/* Change window size depending on flags state */

	wd_adapt(w);

	/* Set window title (and sliders), depending on window size */

	wd_type_title((TYP_WINDOW *)w);
}


/*
 * Reset a fulled directory or text window to nonfulled state
 * without changing its size.
 */

void wd_type_nofull(WINDOW *w)
{
	WINFO *wi;

	if ( xw_type(w) == DIR_WIND || xw_type(w) == TEXT_WIND )
	{
 		wi = ((TYP_WINDOW *)w)->winfo;
		wi->flags.fulled = 0;
		wi->flags.fullfull = 0;
		wd_set_defsize(wi);
	}
}


/*
 * Handle scrolling window gadgets 
 */

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
	long h = (long) ( ((TYP_WINDOW *)w)->columns - ((TYP_WINDOW *) w)->ncolumns);
	w_page((TYP_WINDOW *)w, (int)calc_slpos(newpos, h), ((TYP_WINDOW *)w)->py);
}


void wd_type_vslider(WINDOW *w, int newpos)
{
	long h = (long) (((TYP_WINDOW *) w)->nlines - ((TYP_WINDOW *) w)->nrows);
	w_page((TYP_WINDOW *)w, ((TYP_WINDOW *)w)->px, calc_slpos(newpos, h));
}


void wd_set_defsize(WINFO *w) 
{
	RECT border, work;

	xw_getsize((WINDOW *)w->typ_window, &border);
	xw_getwork((WINDOW *)w->typ_window, &work);

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

	wd_wsize((TYP_WINDOW *)w, newpos, &size, FALSE);
	
	/* Note: at least a minimum window size should be set here */ 

	wd->flags.fulled = 0;
	wd->flags.fullfull = 0;

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
		xw_setsize(w, &size);
		size.w += 1;
	}
#endif

	xw_setsize(w, &size);
	wd_set_defsize(wd);
}


/*
 * Handle resizing of a window
 */

void wd_type_sized(WINDOW *w, RECT *newsize)
{
	RECT 
		area;

	int 
		dc,
		old_w, 
		old_h;

	if ( xw_type(w) == DIR_WIND)
	{
		dc = ((DIR_WINDOW *)w)->dcolumns;

		if(((DIR_WINDOW *)w)->par_itm > 0)
			((DIR_WINDOW *)w)->par_itm = -((DIR_WINDOW *)w)->par_itm;
	}


	xw_getwork(w, &area);

	old_w = area.w;
	old_h = area.h;

	wd_type_moved(w, newsize);
	xw_getwork(w, &area);

	if ( xw_type(w) == TEXT_WIND )
	{
		if ((area.w > old_w) || (area.h > old_h))
			wd_type_draw((TYP_WINDOW *) w, TRUE);
	}
	else
	{
		calc_nlines((DIR_WINDOW *)w);

		if ((area.w != old_w || area.h != old_h) && (options.mode != TEXTMODE || dc != ((DIR_WINDOW *)w)->dcolumns) )
			wd_type_draw((TYP_WINDOW *) w, TRUE);
	}

	wd_type_title((TYP_WINDOW *)w);
}


/********************************************************************
 *																	*
 * Funkties voor het zetten van de window sliders.					*
 *																	*
 ********************************************************************/


/* 
 * Calculate size and position of a window slider
 * (later beware of int/long difference in hor/vert applicaton)
 */

void calc_wslider
(
	long wlines, 	/* in-     w->ncolumns or w->nrows; visible width */
	long nlines,	/* in-     w->columns or w->nlines; total width */
	long *wpxy,		/* in/out- w->px or w->py; current position */
	int *p,			/* out-    calculated slider size */
	int *pos		/* out-    calculated slider position */
)
{
	long h = nlines - wlines;

	*wpxy = lminmax(0, h, *wpxy);

	if ( h > 0 )
	{
		*p = calc_slmill(wlines, nlines);
		*pos = calc_slmill(*wpxy, h);
	}
	else
	{
		*p = 1000;
		*pos = 0;
	}
}


/*
 * Calculate and set size and position of horizontal window slider;
 * note: this is somewhat slower than may be necessary, 
 * because both size and position are always set;
 */

void set_hslsize_pos(TYP_WINDOW *w)
{
	int 
		p,		/* calculated slider size     */ 
		pos;	/* calculated slider position */

	long 
		ww = w->columns,
		wpx;

	wpx = (long)w->px;
	calc_wslider( (long)w->ncolumns, ww, &wpx, &p, &pos );
	w->px = (int)wpx;

	/* both size and position of the slider are always set; */

	xw_set((WINDOW *) w, WF_HSLSIZE, p);
	xw_set((WINDOW *) w, WF_HSLIDE, pos);
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

 	xw_set((WINDOW *)w, WF_VSLSIZE, p);
	xw_set((WINDOW *)w, WF_VSLIDE, pos);
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
 * Page window. If autolocator is turned on, enable
 * action even if positions have not changed.
 */

void w_page(TYP_WINDOW *w, int newpx, long newpy)
{
	boolean doit = FALSE;

	if ( newpx != w->px )
	{
		w->px = newpx;
		set_hslsize_pos((TYP_WINDOW *)w); 
		doit = TRUE;
	}

	if ( newpy != w->py )
	{
		w->py = newpy;
		set_vslsize_pos((TYP_WINDOW *)w);
		doit = TRUE;
	} 

	if (autoloc || doit)
		wd_type_draw(w, FALSE);
}


/*
 * Window page scrolling routines
 */

void w_pageup(TYP_WINDOW *w)
{
	long newy = w->py - w->nrows;

	if (newy < 0)
		newy = 0;

	w_page( w, w->px, newy);
}


void w_pagedown(TYP_WINDOW *w)
{
	long 
		newy = w->py + w->nrows;						

	if ((newy + w->nrows) > w->nlines)	
		newy = lmax(w->nlines - (long)w->nrows, 0);

	w_page (w, w->px, newy);
}


void w_pageleft(TYP_WINDOW *w)
{
	int newx = w->px - w->ncolumns;

	if (newx < 0)
		newx = 0;

	w_page(w, newx, w->py);
}


void w_pageright(TYP_WINDOW *w)
{
	int 
		nc = w->ncolumns,
		lwidth = w->columns,
		newx;

	newx = w->px + nc;
	if ((newx + nc) > lwidth)
		newx = max(lwidth - nc, 0);

	w_page ( w, newx, w->py);
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
 * Find the first or the last column or line in a window
 * If last=1 find the last one; if last=0 find the first one.
 */

static long find_firstlast(int wy, int ay, int ah, boolean *prev, int ch, int last)
{
	long line;

	line = ay - wy;

	*prev = FALSE;

	if(last)
	{
		line += ah;
		if((line % ch) != 0)
			*prev = TRUE;

		line--;
	}

	return (long)(line/ ch);
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
	RECT 
		work, 
		area, 
		r, 
		in, 
		src, 	/* screen area bein copied from */
		dest;	/* screen area being copied to */


	long 
		line;

	int 
		last,
		dl = -1,
		column,
		nc,
		wx, 
		wy, 
		cw,
		ch,
		wtype = xw_type((WINDOW *)w);

	boolean 
		prev, 
		iconwind = (wtype == DIR_WIND) && (options.mode != TEXTMODE);

	/* Change position in the window, i.e. w->px, w->py */

	switch (type)
	{
	case WA_UPLINE:
		if (w->py <= 0)
			return;
		w->py -= 1;
		dl = 1;
		break;
	case WA_DNLINE:
		if ((w->py + w->nrows) >= w->nlines)
			return;
		w->py += 1;
		break;
	case WA_LFLINE:
		if ((w->px <= 0))
			return;
		w->px -= 1;
		dl = 0;
		break;
	case WA_RTLINE:
			if ((w->px + w->ncolumns) >= w->columns) 
			return;
		w->px += 1;
		break;
	default:
		return;
	}

	xw_getwork((WINDOW *) w, &work);

	/* Determine increments for scrolling- in char cell or icon cell units */

	wd_cellsize(w, &cw, &ch);

	wx = work.x;
	wy = work.y;
	area = work;

	/* 
	 * In an icon window icons do not start from the very corner
	 * of the window, so calculate another rectangle, to be used
	 * from now on instead of window work area
	 */

	if ( iconwind )
	{
		area.x += XOFFSET;
		area.w -= XOFFSET;
		area.y += YOFFSET;
		area.h -= YOFFSET;
	}

	if (!clip_desk(&area)) /* for text window this was &work ? */
		return;

	xd_wdupdate(BEG_UPDATE);

	/*  Calculate and set window slider(s) size(s) and position(s) */

	if ((type == WA_UPLINE) || (type == WA_DNLINE))
		set_vslsize_pos(w);
	else
		set_hslsize_pos(w);

	moff_mouse();
	set_txt_default(( wtype == TEXT_WIND ) ? &txt_font : &dir_font);

	/* Traverse all rectangles */

	xw_getfirst((WINDOW *) w, &r);
	while (r.w != 0 && r.h != 0)
	{
		if (xd_rcintersect(&r, &area, &in)) /* for text window this was &work ? */
		{
			xd_clip_on(&in);

			src = in;
			dest = in;

			if ((type == WA_UPLINE) || (type == WA_DNLINE))
			{
				if (type == WA_UPLINE)
				{
					dest.y += ch;
					last = 0;
				}
				else
				{
					src.y += ch;
					last = 1;
				}

				line = find_firstlast(wy, in.y, in.h, &prev, ch, last);					

				line += w->py;
				dest.h -= ch;
				src.h -= ch;
			}
			else
			{
				nc = 1;

				if (type == WA_LFLINE)
				{
					dest.x += cw;
					last = 0;
				}
				else
				{
					src.x += cw;
					last = 1;
				}

				column = (int)find_firstlast(wx, in.x, in.w, &prev, cw, last);

				column += w->px;
				dest.w -=cw;
				src.w -=cw;

				if ( prev )
					nc++;
			}

			/* 
			 * Part of the window which is not scrolled out
			 * is moved by the right amount in the appropriate direction
			 */

			if ((src.h > 0) && (src.w > 0))
				move_screen(&dest, &src);

			if ((type == WA_UPLINE) || (type == WA_DNLINE))
			{
				again:;

				if ( wtype == TEXT_WIND )
					txt_prtline((TXT_WINDOW *)w, line, &in, &work);
				else
					dir_prtcolumns((DIR_WINDOW *)w, line, &in, &work);

				if (prev)
				{
					line += dl;
					prev = FALSE;
					goto again;
				}
			}
			else	/* left/right */
			{
				if ( prev )	
					column += dl;

				if ( wtype == TEXT_WIND )
					txt_prtcolumn((TXT_WINDOW *)w, column, nc, &in, &work);
				else
					dir_prtcolumn((DIR_WINDOW *)w, column, nc, &in, &work);
			}

			xd_clip_off();
		}
		xw_getnext((WINDOW *)w, &r);
	}

	mon_mouse();
	xd_wdupdate(END_UPDATE);
}


/*
 * Adapt window size, e.g. when fulled or with resolution change.
 * Return TRUE if adaptation performed (correct window type)
 * if returned TRUE, routine set_sliders() should be called next.
 */

boolean wd_adapt(WINDOW *w)
{
	RECT size;

	if (xw_type(w) == DIR_WIND || xw_type(w) == TEXT_WIND)
	{
		wd_calcsize(((TYP_WINDOW *) w)->winfo, &size);
		xw_setsize(w, &size);
		return TRUE;
	}
	
	return FALSE;
}


/* 
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
	int 
		act,
		key,
		wt = xw_type(w),
		result = 1;

	TYP_WINDOW 
		*tyw = (TYP_WINDOW *)w;


	if(!onekey_shorts)
		autoloc = TRUE;

	switch (scancode)
	{
	case RETURN:
		if ( wt != TEXT_WIND )
		{	
			int n, *list;
			if(autoloc && itm_list(w, &n, &list) && n)
			{
				free(list);
				wd_hndlmenu(MOPEN, keystate);
			}
			else 
				result = 0;
			break;
		}
	case CURDOWN:
		act = WA_DNLINE;
		goto scroll;
	case CURUP:
		act = WA_UPLINE;
		goto scroll;
	case CURLEFT:
		act = WA_LFLINE;	
		goto scroll;
	case CURRIGHT:
		act = WA_RTLINE;
		scroll:;
		w_scroll(tyw, act);
		break;
	case SPACE:
		if ( wt == DIR_WIND )
		{
			if(autoloc)
				goto thedefault;
			result = 0;
			break;
		}
	case PAGE_DOWN:				/* PgUp/PgDn keys on PC keyboards (Emulators and MILAN) */
	case SHFT_CURDOWN:
		w_pagedown(tyw);
		break;
	case PAGE_UP:				/* PgUp/PgDn keys on PC keyboards (Emulators and MILAN) */
	case SHFT_CURUP:
		w_pageup(tyw);
		break;
	case SHFT_CURLEFT:
		w_pageleft(tyw);
		break;
	case SHFT_CURRIGHT:
		w_pageright(tyw);
		break;
	case HOME:					/* Reset sliders to window top */
		w_page (tyw, 0, 0L); 
		break;
	case SHFT_HOME:				/* Set sliders to window bottom */
		if ( tyw->nrows < tyw->nlines )
			w_page (tyw, 0, (long)(tyw->nlines - tyw->nrows)); 
		break;
	case ESCAPE:
		if ( wt == DIR_WIND )
		{
			force_mediach(((DIR_WINDOW *) w)->path);
			dir_refresh_wd((DIR_WINDOW *)w);
		}
		else
		{
			int wpx = tyw->px;
			long wpy = tyw->py;
			txt_reread((TXT_WINDOW *)w, NULL, wpx, wpy);
		}
		break;
	case INSERT:
		if( wt != DIR_WIND )
			break;
		if(autoloc)
			autoloc_off();
		else
		{
			autoloc = TRUE;
			aml = 0;
			*automask = 0;
		}
	default:
		thedefault:;
		if ( !(autoloc && onekey_shorts) && scansh( scancode, keystate ) == options.kbshort[MCLOSEW-MFIRST] )
			wd_type_close(w, 1);
		else
		{
			if ( wt == TEXT_WIND )
				result = 0;
			else 
			{
				key = scancode & ~XD_SHIFT;
				if(!((scancode & XD_SHIFT) && dir_onalt(key, w)))
				{
					if((((TYP_WINDOW *)w)->winfo)->flags.iconified != 0)
						autoloc_off();

					/* 
					 * Name mask for the autoselector has to be composed,
					 * taking into account limitations of the filesystem;
					 * last character entered can be deleted by [Backspace]
					 */

					if (autoloc && (key == BACKSPC || (unsigned int)key == INSERT || (key >= ' ' && key <= '~'))  )
					{
						/* key is [Backspace] or [Insert] or a printable character */

						char *ast, *dot;  /* pointer to "." in the name */
						int ei = 0; /*  index of "*"  */
						size_t lm = x_pathconf(((DIR_WINDOW *)w)->path, DP_NAMEMAX);

						/* [Insert] and wildcard should have no effect */

						if((unsigned int)key == INSERT)
							key = 0;

						/* If [Backspace], delete last character */

						if(key ==  BACKSPC)
						{
							key = 0;
							if(aml)
								aml--;
							if(aml && automask[aml - 1] == '*')
								aml--;
						}
						
						automask[aml] = 0;

						/* Are long names possible or is this a 8+3 FAT filesystem? */
#if _MINT_
						if((x_inq_xfs(((DIR_WINDOW *)w)->path) & FS_LFN) == 0)
#endif
						{
							/* 
							 * Compose a mask obeying the 8+3 rule 
							 * (append either "*.*" or ".*" or "*")
							 */

							if(key == SPACE)
								key = 0;
							key = touppc(key);
							dot = strchr(automask,'.');

							if(dot)
							{
								lm = dot - automask + 4;
								if(key == '.')
									key = 0;
							}
							else if (key != '.')
							{ 
#if _MINT_
								if(!mint)
#endif
									ei = 10;	/* Index of ".*" */

								if(!((aml == 7) && key))
								{
									if(aml == 8)
										key = 0;
									else
									{
#if _MINT_ 
										if(!mint) 
#endif
												ei = 1;	/* index of *.*  */
									}
								}
							}							
						}

						/* This should prevent too long namemasks */

						if(key && (aml == lm - 1) )
							ei = 11;				/* index of "\0" */
	
						if(aml == lm)
						{
							key = 0;
							ei = 11;	/* index of "\0" */
						}
						
						/* Append the character and wildcards to namemask */

						if(key)
							automask[aml++] = key;

						automask[aml] = 0;
						strcat(automask, presets[ei]);

						/* Consecutive wildcards are not needed */

						if((ast = strstr(automask, "**")) != NULL)
							strcpy(ast, ast + 1);

						/* Select items in the window */

						dir_autoselect((DIR_WINDOW *)w);
					}
					else
						result = 0;
				}
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
	*error = handle_cfg(file, window_table, lvl, CFGEMP, io, wd_init, wd_default);
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

CfgNest open_config
{
	WINDOW *w;

	w = xw_last();
	*error = 0;

	/* Save open windows data only if there are any */

	if (io == CFG_SAVE)
	{
		if(w && (((options.sexit & SAVE_WIN) != 0) || wclose) )
		{
			*error = CfgSave(file, open_start_table, lvl, CFGEMP);

			while ( (*error >= 0) && w )
			{
				that.w = w;

				switch(xw_type(w))
				{
					case DIR_WIND :
 						dir_one(file, lvl + 1, CFG_SAVE, error); 
						break;
					case TEXT_WIND :
						text_one(file, lvl + 1, CFG_SAVE, error);
						break;
					default:
						break;
				}

				if (wclose)
					wd_type_close(w, 1);
	
				w = w->xw_prev;
			}

			*error = CfgSave(file, open_end_table, lvl, CFGEMP); 
		}
	}
	else
	{
		/* Load configuration of open windows */

		*error = CfgLoad(file, open_table, MAX_KEYLEN, lvl); 
	}
}


/********************************************************************
 *																	*
 * Functies voor het tijdelijk sluiten en weer openen van windows.	*
 *																	*
 ********************************************************************/

/*
 * Save current positions of the open windows into a (file-like) buffer 
 * (a "memory file") and close the windows. This routine also opens 
 * this "file".
 */

boolean wd_tmpcls(void)
{
	int error;

	/* 
	 * Deselect anything that was selected in windows. 
	 * This will also set selection.w to NULL and selection.n to 0
	 */

	wd_deselect_all();

	/* Attempt to open a 'memory file' (set level-1 here) */

	if ((mem_file = x_fmemopen(O_DENYRW | O_RDWR, &error)) != NULL)
	{
		wclose = TRUE;
		error = CfgSave(mem_file, reopen_table, -1, CFGEMP); 
		wclose = FALSE;
	}

	/* 
	 * Close the 'file' only in case of error, otherwise keep it
	 * open for later rereading
	 */

	if (error < 0) 
	{
		if (mem_file != NULL)
			x_fclose(mem_file);

		xform_error(error);
		return FALSE;
	}
	else
		return TRUE;
}


/*
 * Load the positions of windows from a (file-like) memory buffer 
 * ("the memory file") and open them.  This routine closes the "file"
 * after reading
 */

extern int chklevel;

void wd_reopen(void)
{
	int error = EINVFN;

	chklevel = 0;

	/* Rewind the "file", then read from it (use level -1 here) */

	if(mem_file)
	{
		mem_file->read = 0;
		error = CfgLoad(mem_file, reopen_table, MAX_KEYLEN, -1); 

		x_fclose(mem_file);
	}

	xform_error(error); /* will ignore error >= 0 */
}


/********************************************************************
 *																	*
 * Functions for handling items in a window.						*
 *																	*
 ********************************************************************/

/*
 * Find an object in a window. 
 * Attention: Must be used only on directory and desktop windows
 */

int itm_find(WINDOW *w, int x, int y)
{
	if (in_window(w, x, y))
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
	if ( xw_type(w) == TEXT_WIND )
		return ITM_NOTUSED;

	return (((ITM_WINDOW *) w)->itm_func->itm_type) (w, item);
}


/*
 * Determine the type of an item in a window, but if
 * the item is a link, follow it to the target
 */

ITMTYPE itm_tgttype(WINDOW *w, int item)
{
	if ( xw_type(w) == TEXT_WIND )
		return ITM_NOTUSED;

	return (((ITM_WINDOW *) w)->itm_func->itm_tgttype) (w, item);
}


/*
 * Get the icon associated to an item in a dir window
 */

int itm_icon(WINDOW *w, int item)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_icon) (w, item);
}


/*
 * Get the name of an item in a directory window
 */

const char *itm_name(WINDOW *w, int item)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_name) (w, item);
}


/*
 * Get full name (i.e. path + name) of an item in a dir window 
 * item identified by its ordinal in the list.
 */

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
	if ( itm_islink(w, item) )
	{
		thename = x_fllink(fullname); 	/* allocate new space */
		free(fullname);					/* free old space */
	}
	else
#endif
		thename = fullname;

	return thename;	
}


/*
 * Get iem attributes; on mode=0 follow link, on mode=1 don't
 */

int itm_attrib(WINDOW *w, int item, int mode, XATTR *attrib)
{
	int result;
	result = (((ITM_WINDOW *) w)->itm_func->itm_attrib) (w, item, mode, attrib);

	wd_setselection(w);
	return result;
}


/*
 * Is the window object a link? Return TRUE if it is.
 */

boolean itm_islink(WINDOW *w, int item)
{
#if _MINT_
	return (((ITM_WINDOW *) w)->itm_func->itm_islink) (w, item);
#else
	return FALSE;
#endif
}


/*
 * Is the object a link and should it be followed?
 * If so, this routine resets 'link' to FALSE.
 * Use the opportunity to find target object's fullname.
 */

boolean itm_follow(WINDOW *w, int item, boolean *link, char **name, ITMTYPE *type)
{
#if _MINT_
	boolean ll =  itm_islink(w, item);

	if (ll && ((options.cprefs & CF_FOLL) != 0))
	{
		*link = FALSE;
		*type = itm_tgttype(w, item);
		*name = itm_tgtname(w, item);

		if(*type != ITM_NETOB)	/* Don't follow to net objects */
			return TRUE;
	}
	else
#else
	boolean ll = FALSE;
#endif
	{
		*link = ll;
		*name = itm_fullname(w, item);
		*type = itm_type(w, item);
	}

	return FALSE;
}


boolean itm_open(WINDOW *w, int item, int kstate)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_open) (w, item, kstate);
}

/*
 * Funktie voor het selecteren van een item in een window.
 * mode = 0: selecteer selected en deselecteer de rest.
 * mode = 1: inverteer de status van selected.
 * mode = 2: deselecteer selected.
 * mode = 3: selecteer selected.
 * mode = 4: select all  
 */

void itm_select(WINDOW *w, int selected, int mode, boolean draw)
{
	if (xw_exist(w))
		(((ITM_WINDOW *) w)->itm_func->itm_select) (w, selected, mode, draw);

	wd_setselection(w);
}


/* 
 * A shorter form of the above
 */

static void itm_selall(WINDOW *w)
{
	itm_select(w, 0, 4, TRUE);
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
 * Return a list of items selected in a window.
 * This routine will return FALSE and a NULL list pointer
 * if no list has been created.
 * If list allocation fails, an alert will be generated.
 * Note: XWF_SIM flag is set in dir_simw() to mark a simulated window;
 * it is checked here because xw_exist does not recognize simulated windows
 */

boolean itm_list(WINDOW *w, int *n, int **list)
{
	if
	(
		w == NULL || 
		(!xw_exist(w) && (w->xw_xflags & XWF_SIM) == 0) || 
		xw_type(w) == TEXT_WIND
	)
	{
		*list = NULL;
		*n = 0;
		return FALSE;
	}

	return (((ITM_WINDOW *) w)->itm_func->itm_list)(w, n, list);
}


/********************************************************************
 *																	*
 * Functies voor het afhandelen van muisklikken in een window.		*
 *																	*
 ********************************************************************/

static boolean itm_copy(WINDOW *sw, int n, int *list, WINDOW *dw,
					 int dobject, int kstate, ICND *icnlist, int x, int y)
{
	if (dw)
	{
		/* Can't copy over/to a file */

		if ((dobject != -1) && (itm_tgttype(dw, dobject) == ITM_FILE))
			dobject = -1;

		/* 
		 * Perform window-type-specific item-copy operation.
		 * Beware that a deselection after the operation can not be done
		 * if the windows have been closed meanwhile, as in single-TOS
		 * when starting a program by copying/dragging objects to it.
		 * If windows have been closed, selection.w is NULL
		 */

		if (((ITM_WINDOW *)dw)->itm_func->itm_copy(dw, dobject, sw, n, list, icnlist, x, y, kstate))
		{
			if(selection.w && (va_reply || ((options.cprefs & CF_KEEPS) == 0) ) )
				itm_select(sw, -1, 0, va_reply ? FALSE : TRUE);
		}

		return TRUE;
	}
	else
	{
		alert_printf(1, AILLDEST);
		return FALSE;
	}
}


/* 
 * Drag & drop items from a list onto a window at position x,y,
 * using the drag & drop protocol. This routine does something useful
 * only in the multitasking version; otherwise, it just returns 
 * an error. It should be called only if destination window is not TeraDesk's
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

	/* Drag & drop is possible only in Mint or Magic (what about geneva 6?) */

	if (mint && apid >= 0) /* was 'apid > 0' earlier */
	{
		char ddsexts[34];
		long nsize = 0;

		/* Create a string with filenames to be sent */

		*global_memory = 0;

		for ( i = 0; i < n; i++ )
		{
			if ( (item = list[i]) < 0 )
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

		nsize = (long)strlen(global_memory);

		/* Create a drag&drop pipe; if successful, continue */

		fd = ddcreate(apid, ap_id, hdl, x, y, kstate, ddsexts);

		if (fd > 0)
		{
			int reply;

			/* Will the recepient accept data ? */

			reply = ddstry(fd, "ARGS", (char *)empty, nsize);

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
					itmlist_wop(w, n, list, CMD_PRINT);
					break;
				case DD_TRASH:
					/* dropped on a trashcan, so delete these items */
					itmlist_wop(w, n, list, CMD_DELETE);
					break;
				case DD_CLIPBOARD:	/* app should put it in clipboard */
				case DD_NAK:		/* app refuses the drop */
				case DD_EXT:		/* app doesn't understand this type */
				case DD_LEN:		/* app can't receive so much data */
				default:	
					/* These drag & drop operations are not (yet) supported */
					alert_iprint(APPNOEXT); 
					ddclose(fd);
					return FALSE;
			}
			ddclose(fd);
			itm_select(w, -1, 0, TRUE);	
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
	int i, j, j2;
	ICND *icnd;

	for (i = 0; i < n; i++)
	{
		icnd = &icns[i];

		for (j = 0; j < icnd->np; j++)
		{
			if ((i == 0) && (j == 0))
			{
				int *cp = clip;

				*cp++ = icnd->coords[0];
				*cp++ = icnd->coords[1];
				*cp++ = clip[0];
				*cp   = clip[1];
			}
			else
			{
				j2 = j * 2;
				if (clip[0] > icnd->coords[j2]) /* Do not use min() here */
					clip[0] = icnd->coords[j2];
				clip[1] = min(clip[1], icnd->coords[j2 + 1]);
				if ( clip[2] < icnd->coords[j2])
					clip[2] = icnd->coords[j2]; /* Do not use max() here */
				clip[3] = max(clip[3], icnd->coords[j2 + 1]);
			}
		}
	}
}


/* 
 * Ensure that coordinates are within ceratin limits;
 * In this particular case thers would be no gain in size when using minmax()
 * but there would be a loss in speed
 */

static void clip_coords(int *clip, int *nx, int *ny)
{
	int h, *cp = clip;

	h = screen_info.dsk.x - *cp++;
	if (*nx < h)
		*nx = h;

	h = screen_info.dsk.y - *cp++;
	if (*ny < h)
		*ny = h;

	h = screen_info.dsk.x + screen_info.dsk.w - 1 - *cp++;
	if (*nx > h)
		*nx = h;

	h = screen_info.dsk.y + screen_info.dsk.h - 1 - *cp;
	if (*ny > h)
		*ny = h;
}


static void draw_icns(ICND *icns, int n, int mx, int my, int *clip)
{
	int i, j, j2, c[18], x = mx, y = my;
	ICND *icnd;


	clip_coords(clip, &x, &y);
	set_rect_default();
	moff_mouse();

	for (i = 0; i < n; i++)
	{
		icnd = &icns[i];

		for (j = 0; j < icnd->np; j++)
		{
			j2 = j * 2;
			c[j2] = icnd->coords[j2] + x;
			c[j2 + 1] = icnd->coords[j2 + 1] + y;
		}
		v_pline(vdi_handle, icnd->np, c);
	}

	mon_mouse();
}


static void desel_old(void)
{
	if (selection.w)
		itm_select(selection.w, -1, 0, TRUE);
}


/*
 * mode = 2 - deselecteren, mode = 3 - selecteren 
 */

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

	SEL_INFO
		keepsel = selection;	/* to restore corrupted "selection" */


	if ( itm_type(src_wd, src_object) == ITM_PREVDIR )
	{
		if (!va_reply)
		{
			/* Wait for a button click */
			while
				(xe_button_state());
		}

		return result;
	}

	/* Create a list of selected objects in the source window */

	if ( (itm_list(src_wd, &n, &list) == FALSE) )
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

	/* Loop until object(s) released at a positon */

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
				if (!cur_state && (cur_object >= 0))
					select_object(cur_wd, cur_object, 2);

				cur_wd = new_wd;
				cur_object = new_object;
				cur_state = new_state;

				if ((cur_object >= 0) && (cur_state == FALSE))
					select_object(cur_wd, cur_object, 3);
			}

			if (!mreleased)
				draw_icns(icnlist, nv, x, y, clip);
		}
		else if (mreleased)
			draw_icns(icnlist, nv, x, y, clip);
	}
	while (mreleased == FALSE);

	arrow_mouse();
	wind_update(END_MCTRL);

	/* 
	 * During moving of the cursor over objects, current selection may
	 * have changed; now restore it
	 */

	selection = keepsel;

	if (!cur_state && (cur_object >= 0))
		select_object(cur_wd, cur_object, 2);

	if ((cur_wd != src_wd) || (cur_object != src_object))
	{
		if (cur_wd != NULL)
		{
			int cur_type = xw_type(cur_wd);
			ITMTYPE itype;

			switch(cur_type)
			{
			case DIR_WIND:
			case DESK_WIND:
				/* 
				 * Test if destination window is the desktop and if the
				 * destination object is -1 (no object). 
				 */

				if ((xw_type(cur_wd) == DESK_WIND) && (cur_object == -1) && (xw_type(src_wd) == DESK_WIND))
					clip_coords(clip, &x, &y);

				result = itm_copy(src_wd, n, list, cur_wd, cur_object, kstate, icnlist, x, y);

				break;
			case ACC_WIND:
				result = va_accdrop(cur_wd, src_wd, list, n, kstate, x, y );
				break;
			case TEXT_WIND:			
				if (n == 1 && ((itype = itm_tgttype(src_wd, list[0])) == ITM_FILE || itype == ITM_PROGRAM) )
				{
					char *path = itm_fullname(src_wd, list[0]);
					result = txt_reread((TXT_WINDOW *)cur_wd, x_fllink(path), 0, 0L);
					free(path);
					break;
				}
			default:
				alert_printf(1, AILLCOPY);
			}
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

	xw_getwork(w, &work);

	if (   (x >= work.x) && (x < (work.x + work.w))
	    && (y >= work.y) && (y < (work.y + work.h))
	   )
		return TRUE;
	else
		return FALSE;
}


/* 
 * Button handling routine. 
 * This is called in desktop,  directory and text windows.
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

			/* Wait for a button click */

			while 
				(xe_button_state());

			/* 
			 * Note: if itm_open() starts a program in single-tos,
			 * windows will be closed and reopened, and "w" will no
			 * longer will be valid. In this case selection.w will be NULL;
			 * deselect object only if this is not the case
			 */

			if (itm_open(w, item, kstate))
			{
				w = selection.w;
				if(w)
					itm_select(w, item, 2, TRUE);
			}
		}
		else
		{
			if ((m_state == 0) || (itm_state(w, item) == FALSE))
			{
				itm_select(w, item, (kstate & (K_RSHIFT | K_LSHIFT)) ? 1 : 0, TRUE);
			}

			if ((m_state != 0) && itm_state(w, item))
				itm_move(w, item, x, y, 0);
		}
		wd_setselection(selection.w);
	}
	else if (in_window(w, x, y))
	{
		autoloc_off();

		/* Top TeraDesk by clicking on desktop area */

#if _MINT_
		if ( (mint || geneva) && (xw_type(w) == DESK_WIND) )
		{
			/*  This works only if TeraDesk has open windows ! */

			WINDOW *ww = xw_first();
			boolean nowin = TRUE;

			while (ww)
			{
				if (xw_type(ww) == DIR_WIND || xw_type(ww) == TEXT_WIND )
				{
					wd_type_topped(ww);	
					nowin = FALSE;
					break;
				}
				ww = ww->xw_next;
			}

			/* 
			 * This works without any open windows, but unfortunately
			 * not in all AESses. Will be ok in N.AES, XaAES, Magic 
			 */
 
			if (nowin)
				wd_top_app(ap_id);
		}
#endif

		if (((m_state == 0) || ((kstate & (K_RSHIFT | K_LSHIFT)) == 0)) && (selection.w == w))
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
 * this routine is used for iconified windows and for
 * directory windows in icon mode 
 */

void wd_set_obj0( OBJECT *obj, boolean smode, int row, int lines, int yoffset, RECT *work )
{
	/* Note: when object type is I_BOX it will not be redrawn */

	init_obj(&obj[0], (smode) ? G_IBOX : G_BOX);

	obj[0].ob_spec.obspec.framesize = 0;
	obj[0].ob_spec.obspec.interiorcol = options.win_color;
	obj[0].ob_spec.obspec.fillpattern = options.win_pattern;
	obj[0].ob_spec.obspec.textmode = 1;
	obj[0].r.x = work->x;
	obj[0].r.y = row + work->y + YOFFSET - yoffset;
	obj[0].r.w = work->w;
	obj[0].r.h = lines * ICON_H;
}


/* 
 * Set one icon object.
 * Note similar object-setting code in add_icon() in icon.c
 */

void set_obji( OBJECT *obj, long i, long n, boolean selected, boolean hidden, boolean link, int icon_no, int obj_x, int obj_y, char *name )
{
	OBJECT *obji = &obj[i + 1];

	CICONBLK *cicnblk = (CICONBLK *) &obj[n + 1];

	init_obj(obji, icons[icon_no].ob_type);

	if(selected)
		obji->ob_state = SELECTED;

	if (hidden && (obj[0].ob_spec.obspec.interiorcol == 0) )
		obji->ob_state |= DISABLED;	/* will this work in all AESes ? */
	if (link)
		obji->ob_state |= CHECKED;

	obji->r.x = obj_x;
	obji->r.y = obj_y;
	obji->ob_spec.ciconblk = &cicnblk[i];
	cicnblk[i] = *icons[icon_no].ob_spec.ciconblk;

	cicnblk[i].monoblk.ib_ptext = name;
	cicnblk[i].monoblk.ib_char &= 0xFF00;
	cicnblk[i].monoblk.ib_char |= 0x20;

	objc_add(obj, 0, (int)i + 1);
}


/* 
 * Draw contents of an iconified window 
 */

static void icw_draw(WINDOW *w)
{

	OBJECT *obj;			/* background and icon objects */
	RECT where;	 			/* location ans size of the background object */
	RECT r2;				/* rectangle for redraw test */
	INAME icname;			/* name of icon in desktop.rsc */
	int icon_no, icon_ind;	/* icon indices in resource files */
	const char *wpath = wd_path(w);


	/* allocate memory for objects: background + 1 icon */

	if ((obj = malloc_chk( 2 * sizeof(OBJECT) + sizeof(CICONBLK))) == NULL)
		return;

	/* Set background object. It is needed for background colour / pattern */

	icwsize.x = w->xw_size.x;
	icwsize.y = w->xw_size.y;
	xw_calc(WC_WORK, NAME, &icwsize, &where, NULL);
	wd_set_obj0(obj, FALSE, 0, 1, YOFFSET, &where);

	/* determine which icon to use: file, floppy, disk or folder */

	if ( xw_type(w) == TEXT_WIND )
	{
		icon_ind = FIINAME; 			/* file icon */
	}
	else
	{
		if ( wpath && isroot( wpath ) )
		{
			if ( wpath[0] <= 'B' ) 
				icon_ind = FLINAME; 	/* floppy icon */
			else
				icon_ind = HDINAME;		/* hard disk icon */
		}
		else
			icon_ind = FOINAME;			/* folder icon */
	}

	icon_no = rsrc_icon_rscid( icon_ind, icname );
	cramped_name(wpath ,icname, sizeof(INAME));

	/* Set icon */

	set_obji( obj, 0L, 1L, FALSE, FALSE, FALSE, icon_no, XOFFSET/2, YOFFSET, icname );

	/* (re)draw it */

	wind_update(BEG_UPDATE);

	xw_getfirst((WINDOW *)w, &r2);
	while (r2.w != 0 && r2.h != 0)
	{
 		objc_draw(obj, ROOT, MAX_DEPTH, r2.x, r2.y, r2.w, r2.h);
		xw_getnext((WINDOW *)w, &r2);
	}

	wind_update(END_UPDATE);

	free(obj);
}


/* 
 * Routine for text/dir window iconification 
 */

void wd_type_iconify(WINDOW *w)
{
	WINFO *wi = ((TYP_WINDOW *)w)->winfo;

	/* Can this be done at all ? */

	if ( !can_iconify )
		return;

	/* Change window title */

	xw_set((WINDOW *)w, WF_NAME, nonwhite(get_freestring(MENUREG)));

	/* Remember size of noniconified window */

	*(RECT *)(&(wi->ix)) = *(RECT *)(&(wi->x));

	/* Call function to iconify window */

	xw_iconify(w, icwsize.w, icwsize.h);

	/* If window is not open, return */

	if ( (w->xw_xflags & XWF_OPN) == 0 )
		return;

	/* Draw contents of iconified window */

	icw_draw(w);

	/* 
	 * Set a flag that this is an iconified window;
	 * note: this information is duplicated by xw_iconify
	 * in w->xw_xflags, so that xdialog routines know of
	 * iconified state. flags.iconified is used for saving
	 * in .cfg file
	 */

	wi->flags.iconified = 1;
	wi->flags.fulled = 0;
	wi->flags.fullfull = 0;
}


/* 
 * Routine for text/dir window deiconification 
 */

void wd_type_uniconify(WINDOW *w)
{
	/* Call uniconify function */

	xw_uniconify(w);

	/* Reset a flag that this is an iconified window */

	(((TYP_WINDOW *)w)->winfo)->flags.iconified = 0;

	/* Calculate and draw the window */

	wd_type_sized( w, &(w->xw_nsize) );

	/*
	 * XaAES (V0.963 and 0.970 at least) does not redraw uniconifed windows 
	 * well. The area occupied by the iconified window does not get redrawn.
	 * Below is a fix for that. This block of code is not needed
	 * with other AESses (like: AES4.1, Geneva4. Geneva6, Magic, N_AES1.1).
	 * In order to avoid needless redraw, it is restricted to mint only
	 * (XaAES is not explicitely detected) so that only AES4.1 and MyAES
	 * will suffer.
	 */

#if _MINT_
	if (mint && !(magx || naes))
	{
		RECT area;
		xw_getsize(w, &area);
		area.w = ICON_W + 16;	/* a window will always be wider than this */
		area.h = 2 * ICON_H;	/* and higher than this, so it is safe */
		xw_send_redraw(w, &area);
	}
#endif

}


/* 
 * Open a window, normal size or iconified, depending on flag state.
 * Note that this routine is only used on directory windows and
 * text windows, NOT av-client windows (which do not have a WINFO pointer)
 */

void wd_iopen ( WINDOW *w, RECT *size )
{
	WINFO *wi = ((TYP_WINDOW *)w)->winfo;
	boolean icf = can_iconify && wi->flags.iconified;
	RECT nsize;

#if _MINT_
	/* 
	 * In Magic the window should be opened -after- iconifying,
	 * otherwise it will get a wrong title bar???
	 */

	if (icf && magx)
		xw_iconify(w, icwsize.h, icwsize.h );
#endif
		xw_open( w, size );

	wi->flags.iconified = 0; /* so that wd_calcsize would be correctly done */

	if ( icf )   
	{	
		*(RECT *)(&(wi->x)) = *(RECT *)(&(wi->ix)); 
		wd_calcsize ( wi, &nsize );
		wd_type_iconify(w);
		*(RECT *)(&(w->xw_nsize)) = *(RECT *)(&nsize);
	}

	wi->used = TRUE;
}


