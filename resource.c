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
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <boolean.h>
#include <library.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "resource.h"
#include "version.h"
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h"
#include "lists.h" 
#include "internal.h"

OBJECT *menu,
	   *setprefs,
	   *addprgtype,
	   *newfolder,
	   *fileinfo,
	   *infobox,
	   *addicon,
	   *getcml,
	   *nameconflict,
	   *copyinfo,
	   *setmask,
	   *applikation,
	   *loadmods,
	   *viewmenu,
	   *stabsize,
	   *wdoptions,
	   *wdfont,
	   *helpno1,
	   *helpno2,
	   *fmtfloppy,
	   *vidoptions, 
	   *copyoptions,
	   *ftydialog,
	   *searching;

char
	 *dirname, /* should not be removed for the time being */
	 *openline,/* specification for open- must not be reused */
	 *oldname, /* must stay */
	 *newname, /* must stay */
	 *finame,
	 *flname,
	 *cmdline,	
	 *applcmdline,
	 *spattfld,
	 *drvid,
	 *iconlabel,
	 *cpfile,
	 *cpfolder;

char 
	dirnametxt[132],
     flnametxt[132],
     opentxt[132],
     cmdlinetxt[132];

extern int xd_aes4_0; /* show if it is a 3D aes */
extern int tos_version, aes_version;


/*
 * Move a menu box left until it fits the screen, and a litle more.
 */

static void set_menubox(int box)
{
	int x, dummy, offset;

	objc_offset(menu, box, &x, &dummy);

	if ((offset = x + menu[box].r.w + 5 * screen_info.fnt_w - max_w) > 0)
		menu[box].r.x -= offset;
}


/*
 * horizontal and vertical correction of object position to compensate
 * change of object size in 3D AESses, so that objects in sliders 
 * and similar arrangements do not overlap.
 */

extern int aes_hor3d, aes_ver3d;

static void rsc_xalign(OBJECT *tree, int left, int right, int object)
{
	tree[object].r.x = tree[left ].r.x + tree[left  ].r.w + (2 * aes_hor3d + 1);
	tree[object].r.w = tree[right].r.x - tree[object].r.x - (2 * aes_hor3d + 1);
}


/*
 * Note: rsc_yalign is currently applied -only- to slider background objects;
 * therefore the calculation is slightly different than in rsc_xalign,
 * because these objects are -not- marked to be set in 3D.
 * Note: this routine also changes the size of the slider itself.
 */

static void rsc_yalign(OBJECT *tree, int up, int down, int object)
{
	tree[object].r.x = tree[object].r.x - aes_hor3d;
	tree[object].r.w = tree[object].r.w + 2 * aes_hor3d - ((aes_hor3d) ? 1 : 0 );
	tree[tree[object].ob_head].r.x += aes_hor3d;

	tree[object].r.y = tree[up  ].r.y + tree[up    ].r.h + aes_ver3d + 1;
	tree[object].r.h = tree[down].r.y - tree[object].r.y - aes_ver3d - 1;

	/* Change fill pattern in sliders to dark gray when appropriate */

	if ( xd_aes4_0 && ncolors > 4 )
		tree[object].ob_spec.obspec.fillpattern = 7;
}


/*
 * Set object vertical position to n * 1/2 char vertical distance 
 * from reference object; in low-res, objects positioned at non-integer
 * vertical positions (in char height units) get badly placed, this
 * is an attempt to fix the flaw. Seems to work ok. 
 */

void rsc_yfix 
( 
	OBJECT *tree,	/* object tree which is fixed */ 
	int refobject,	/* index of object which is a reference for moved object */ 
	int object,		/* index of repositioned- moved object */ 
	int chalfs		/* vertical distance from ref. object - in char half-heights */ 
)
{
	tree[object].r.y = tree[refobject].r.y + screen_info.fnt_h * chalfs / 2;
}


/* 
 * Funktie voor het verwijderen van een menupunt uit een menu 
 * Delete a menu item
 */

#if _MENUDEL

static void mn_del(int box, int item)
{

	int i, y, ch_h = screen_info.fnt_h;
	OBJECT *tree = menu;


	tree[box].r.h -= ch_h;
	y = tree[item].r.y;
	i = tree[box].ob_head;

	while (i != box)
	{
		if (tree[i].r.y > y)
			tree[i].r.y -= ch_h;
		i = tree[i].ob_next;
	}

	objc_delete(menu, item);
}
#endif


/* 
 * Fix position and contents of the main menu, if necessary
 */

#define MFILEBX MOPEN-1 /* until name set in the resource definition */

static void rsc_fixmenus(void)
{
	RECT desk;

	/* 
	 * Some modifications may be needed for older TOS (1.0, 1.2, 1.4, 1.6)
	 * Certain menu items will have to be deleted, in order
	 * to fit the menu into a limited screen buffer size (?).
	 * It is assumed that an alternative AES loaded over an old TOS
	 * will always be an AES4, capable of answering the query
	 * about buffer size
	 */

#if _MENUDEL

	if ( tos_version < 0x200 )
	{
		int dummy;
		long mnsize;
		RECT boxrect;
		MFDB mfdb;
		union
		{
			long size;
			struct
			{
				int high;
				int low;
			} words;
		} buffer;

		xd_objrect(menu, MNOPTBOX, &boxrect);
		mnsize = xd_initmfdb(&boxrect, &mfdb);

		/*
		 * TOS 1.4 and higher can answer an inquiry about screen buffer size.
		 * Older TOSses (in fact older AESses ?)  can not.
		 */

		if ( /* tos1_4() */  aes_version >= 0x140 )
			wind_get(0, WF_SCREEN, &dummy, &dummy, &buffer.words.high, &buffer.words.low);
		else
			buffer.size = 8000L;

		/*
		 * Only the options menu is tested, but items from all boxes
		 * are removed. Not very logical, but so it is for the time being
		 * (a test showed that all menu boxes are of the approximately 
		 * same size, "Options" being slightly larger than the others.
		 * (If buffer size is sufficient, nothing will happen)
		 */

		if (mnsize > buffer.size)
		{
			/* First remove the (non-essential) separator lines */

			mn_del(MFILEBX, SEP1);
			mn_del(MFILEBX, SEP2);
			mn_del(MFILEBX, SEP3);
			mn_del(MNVIEWBX,SEP5);
			mn_del(MNVIEWBX,SEP6);
			mn_del(MNOPTBOX, SEP7);
			mn_del(MNOPTBOX, SEP8);

			/* Even more must be deleted for oldest TOS (AES?) versions */

			if ( /* !(tos1_4() */ aes_version < 0x140 )
			{
				mn_del(MFILEBX, MDELETE);
				mn_del(MFILEBX, MPRINT);
				mn_del(MFILEBX, MFCOPY);

				mn_del(MNVIEWBX,MSHSIZ);
				mn_del(MNVIEWBX,MSHDAT);
				mn_del(MNVIEWBX,MSHTIM);
				mn_del(MNVIEWBX,MSHATT);
				mn_del(MNVIEWBX,MSUNSORT);

				mn_del(MNOPTBOX, MPRGOPT);
				mn_del(MNOPTBOX, SEP9);
				mn_del(MNOPTBOX, MWDOPT);
				mn_del(MNOPTBOX, MVOPTS);
				mn_del(MNOPTBOX, MSAVEAS);
			}
		}
	}

#endif

	/* Move two rightmost menu boxes left to fit the screen (if needed) */

	set_menubox(MNOPTBOX);
	set_menubox(MNVIEWBX);

	/* Adapt menu bar width to be the same as screen width */

	xw_get(NULL, WF_WORKXYWH, &desk);
	menu[menu->ob_head].r.w = desk.w;
}


/*
 * Convert a scrolled text field to a G_FTEXT suitable for a filename 
 * in 8+3 format (always exactly 12 characters long). The routine is 
 * to be used when the OS does not support long filenames. 
 * Object changes position depending on the value of "just":
 *
 *  just = 0: no change of position
 *  just > 0: move left edge by "just" characters to the right
 *  just < 0: move right edge by "just"-1 characters to the left
 * 
 * See also (changes in) routines cv_fntoform and cv_formtofn in main.c
 *
 */
 
static char *tos_fnform( OBJECT *obj, int just )
{
	int
		dx = 0;

	TEDINFO 
		*ted = xd_get_obspec(obj).tedinfo;

	/* Fix horizontal position and size */ 

	if ( just < 0 )
		dx = ted->te_tmplen - 12  + just;
	else
		dx = just;

	obj->r.x += dx * screen_info.fnt_w;
	obj->r.w = 12 * screen_info.fnt_w;

	/* Change type */

	obj->ob_type = G_FTEXT; /* no extended type */
	obj->ob_spec.tedinfo = ted;
	obj->ob_flags |= 0x400; /* set to background for gray text background */

	/* 
	 * Change validation and template strings; 
	 * It is assumed that the original (scrolled text) strings are 
	 * never shorter than the new ones, so the same locations are used
	 */

	strcpy(ted->te_pvalid, "FFFFFFFFFFF");
	strcpy(ted->te_ptmplt, "________.___");

	ted->te_tmplen = 12;
	ted->te_txtlen = 12; /* includes zero term. byte */

	return ted->te_ptext;
}


/*
 * Decode (hexadecimal) information on OS / AES version for the infobox
 */

static void show_os( OBJECT *obj, int v )
{
	int 
		i, 
		o;

	char 
		tmp[5],
		*tosversion = obj->ob_spec.tedinfo->te_ptext;

	o = (int) strlen(itoa(v, tmp, 16)) - 4;

	for (i = 0; i < 4; i++)
		tosversion[i] = ((i + o) >= 0) ? tmp[i + o] : ' ';
}


/* 
 * Initialize the resource structures 
 */

void rsc_init(void)
{
	int i;

	xd_gaddr(R_TREE, MENU, &menu);
	xd_gaddr(R_TREE, OPTIONS, &setprefs);
	xd_gaddr(R_TREE, ADDPTYPE, &addprgtype);
	xd_gaddr(R_TREE, NEWDIR, &newfolder);
	xd_gaddr(R_TREE, GETCML, &getcml);
	xd_gaddr(R_TREE, FILEINFO, &fileinfo);
	xd_gaddr(R_TREE, INFOBOX, &infobox);
	xd_gaddr(R_TREE, ADDICON, &addicon);
	xd_gaddr(R_TREE, NAMECONF, &nameconflict);
	xd_gaddr(R_TREE, COPYINFO, &copyinfo);
	xd_gaddr(R_TREE, SETMASK, &setmask);
	xd_gaddr(R_TREE, APPLIKAT, &applikation);
	xd_gaddr(R_TREE, VIEWMENU, &viewmenu);
	xd_gaddr(R_TREE, STABSIZE, &stabsize);
	xd_gaddr(R_TREE, WOPTIONS, &wdoptions);
	xd_gaddr(R_TREE, WDFONT, &wdfont);
	xd_gaddr(R_TREE, HELP1, &helpno1);
	xd_gaddr(R_TREE, HELP2, &helpno2);
	xd_gaddr(R_TREE, FLOPPY, &fmtfloppy);
	xd_gaddr(R_TREE, VOPTIONS, &vidoptions);
	xd_gaddr(R_TREE, COPTIONS, &copyoptions);
	xd_gaddr(R_TREE, ADDFTYPE, &ftydialog);	
	xd_gaddr(R_TREE, SEARCH, &searching);

	/*  
	 * Handle pointers for scrolling editable texts. 
	 * Some strings are reused to save space.
	 */

	dirname = xd_set_srcl_text(newfolder,    DIRNAME,  dirnametxt );
	openline = xd_set_srcl_text(newfolder,   OPENNAME, opentxt );
	oldname = xd_set_srcl_text(nameconflict, OLDNAME,  dirnametxt );
	newname = xd_set_srcl_text(nameconflict, NEWNAME,  flnametxt );
	flname  = xd_set_srcl_text(fileinfo,     FLNAME,   flnametxt  );
	cmdline = xd_set_srcl_text(getcml,       CMDLINE,  cmdlinetxt );
	          xd_set_srcl_text(applikation,  APNAME,   flnametxt );	
	          xd_set_srcl_text(applikation,  APPATH,   dirnametxt );	
	applcmdline = xd_set_srcl_text(applikation,  APCMLINE, cmdlinetxt);
	spattfld = xd_set_srcl_text(searching, SMASK, dirnametxt ); 

	/* Pointers to some other texts */

	cpfile = copyinfo[CPFILE].ob_spec.tedinfo->te_ptext;
	cpfolder = copyinfo[CPFOLDER].ob_spec.tedinfo->te_ptext;
	drvid = addicon[DRIVEID].ob_spec.tedinfo->te_ptext;
	iconlabel = addicon[ICNLABEL].ob_spec.tedinfo->te_ptext;

	/* 
	 * Section immediately following fixes some characteristics of
	 * rsc objects depending on AES, screen resolution, etc.
	 */

	/* 
	 * DjV note: rsc_xalign and rsc_yalign below change the dimensions
	 * of dialog items so that they do not overlap with adjacent items;
	 * this is supposed to prevent overlapping of scrolling arrow buttons
	 * with scrolled-item fields.
	 */

	rsc_xalign(wdoptions, DSKPDOWN, DSKPUP, DSKPAT);
	rsc_xalign(wdoptions, DSKCDOWN, DSKCUP, DSKCOLOR);
	rsc_xalign(wdoptions, WINPDOWN, WINPUP, WINPAT);
	rsc_xalign(wdoptions, WINCDOWN, WINCUP, WINCOLOR);
	rsc_xalign(wdfont, WDFSUP, WDFSDOWN, WDFSIZE);
	rsc_xalign(setprefs, OPTMPREV, OPTMNEXT, OPTMTEXT);

	rsc_yalign(addicon, ICNUP, ICNDWN, ICPARENT);
	rsc_yalign(setmask, FTUP, FTDOWN, FTSPAR);
	rsc_yalign(wdfont, WDFUP, WDFDOWN, FSPARENT);

	/* 
	 * Fix position of some objects, which are placed at n * 1/2 char
	 * vertical distance from other objects, for the sake of low-res display
	 * (in low and medium resolution such objects get VERY badly placed) 
	 * note: this enlarged the size of the program noticeably,
	 * perhaps a more compact solution should be used.
	 */  

	rsc_yfix( copyinfo, CMK1, CPT4, 3 );		/* copy info dialog     */
	rsc_yfix( fmtfloppy, FLABEL, FLOT1, 3 );	/* floppy format dialog */		


	/* Assuming that filemask dialog and fonts dialog listboxes are the same size... */

	for ( i = 0; i < NLINES; i++)				/* setmask & font dialogs */
	{
		rsc_yfix( setmask, FTPARENT, FTYPE1 + i, 2 * i + 1 );
		rsc_yfix( wdfont, WDPARENT, WDFONT1 + i, 2 * i + 1 ); 
	}		

	/*
	 * If mint (or magic) is not present, set validation strings for
	 * all file/filetype input fields to (automatically convert to) uppercase
	 * and modify their length to 12 characters (8+3 format).
	 * note: for scrolled text fields it is currently sufficient to put just
	 * the first validation character to lowercase "x" (see xdialog.c)   
	 */	

#if _MINT_
	if ( !mint )
#endif
	{
		tos_fnform( &addicon[ICNTYPE], 3 );
		tos_fnform( &addprgtype[PRGNAME], 3 );
		tos_fnform( &ftydialog[FTYPE0], -1);
		tos_fnform( &setmask[FILETYPE], 3);
	
		for ( i = 0; i < NLINES; i++ )
			tos_fnform( &setmask[FTYPE1 + i], 3 );

		tos_fnform(&fileinfo[FLNAME], -1);
		tos_fnform(&copyinfo[CPFILE], -1);
		tos_fnform(&applikation[APNAME], -1);
		tos_fnform(&nameconflict[NEWNAME], -1);
		tos_fnform(&nameconflict[OLDNAME], -1);
		tos_fnform(&searching[SMASK], -1);
		tos_fnform(&newfolder[DIRNAME], -1);
		xd_get_obspec(&applikation[APPATH]).tedinfo->te_pvalid[0] = 'x';
	}

	*drvid = 0;
	*iconlabel = 0;

	/* Fill-in constant part of info-box dialog */

	infobox[INFOVERS].ob_spec.tedinfo->te_ptext = INFO_VERSION;
	infobox[COPYRGHT].ob_spec.tedinfo->te_ptext = INFO_COPYRIGHT;
	infobox[INFOSYS ].ob_spec.tedinfo->te_ptext = INFO_SYSTEMS; 
	infobox[OTHRINFO].ob_spec.tedinfo->te_ptext = INFO_OTHER;

	show_os( &infobox[INFOTV], tos_version );
	show_os( &infobox[INFOAV], aes_version );

	/*
	 * As SETMASK dialog is now to be reused for several purposes, some
	 * use-dependent items are here made invisible. For each use (when needed)
	 * they are to be made visible before opening the dialog and then
	 * invisible after closing it.
	 * (this could be handled in the resource file as well- hide them there)
	 */

	setmask[FILETYPE].ob_flags |= HIDETREE; 
	setmask[FTTEXT].ob_flags |= HIDETREE;
	setmask[MSKATT].ob_flags |= HIDETREE; 
	setmask[PGATT].ob_flags |= HIDETREE; 
	setmask[ICATT].ob_flags |= HIDETREE;

	rsc_fixmenus();
}


/* 
 * Modify a R_STRING-type text in a dialog (i.e. a dialog title) 
 * by pointing to another string in the resource tree
 * Note: is something wrong here: Structure passed by value?
 */

void rsc_title(OBJECT *tree, int object, int title)
{
	OBSPEC s;

	xd_gaddr(R_STRING, title, &s);
	xd_set_obspec(tree + object, s);
}


/*
 * Write a long integer into a formatted text field. 
 * Text is right justified and padded with spaces on the left side.
 * Maximum length is 16 digits.
 *
 * Parameters:
 *
 * tree		- object tree.
 * object	- index of formatted text field.
 * value	- value to convert.
 *
 * Note: a good validation string must exist for the field, 
 * it is used to determine field length
 */

void rsc_ltoftext(OBJECT *tree, int object, long value)
{
	long l1, l2, i;
	char s[16], *p;
	TEDINFO *ti;

	ti = xd_get_obspec(tree + object).tedinfo;
	p = ti->te_ptext;
	l1 = strlen(ti->te_pvalid);	/* Length of the text field.        */
	ltoa(value, s, 10);			/* Convert value to ASCII, decimal. */
	l2 = strlen(s);				/* Length of the number as string.  */

	i = 0;

	while (i < (l1 - l2))
	{
		*p++ = ' ';						/* Fill with spaces. */
		i++;
	}

	strsncpy(p, s, l2 + 1);						/* Copy number. */
}


