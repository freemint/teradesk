/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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
#include <library.h>
#include <mint.h>
#include <xdialog.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "version.h"
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h"
#include "lists.h" 
#include "stringf.h"

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
	   *fmtfloppy,
	   *vidoptions, 
	   *copyoptions,
	   *ftydialog,
	   *searching,
	   *specapp,
	   *openw,
	   *compare;

char
	*oldname, /* must stay */
	*newname, /* must stay */
	*cfile1,
	*cfile2,
	*tgname,
	*drvid,
	*iconlabel
#if _EDITLABELS
#if _MINT_
	,
	*lblvalid,
	*lbltmplt
#endif
#endif
	;

VLNAME				/* scrolled text fields for file/folder/path names */
	openline,       /* specification for open- must not be reused */
	flname,
	cmdline,
	ttpline,
	dirname,
	envline;


extern int 
	tos_version,	/* tos version, hex encoded */ 
	aes_version;	/* aes version, hex encoded */


/*
 * Hide some dialog objects
 * List of items must terminate with a 0
 */

void rsc_hidemany(OBJECT *tree, int *items)
{
	while(*items)
	{
		obj_hide(tree[*items++]);
	}
}


/*
 * Move a menu box left until it fits the screen, and a litle more.
 */

static void set_menubox(int box)
{
	int
		x,
		dummy,
		offset;

	objc_offset(menu, box, &x, &dummy);

	if ((offset = x + menu[box].r.w + 5 * screen_info.fnt_w - max_w) > 0)
		menu[box].r.x -= offset;
}


/*
 * horizontal and vertical correction of object position to compensate
 * change of object size in 3D AESses, so that objects in sliders 
 * and similar arrangements do not overlap.
 * Object width and position is adjusted to fit between a left and
 * a right object.
 */


static void rsc_xalign(OBJECT *tree, int left, int right, int object)
{
	OBJECT
		*ob = &tree[object];

	int
		h3d2 = 2 * aes_hor3d + 1;


	ob->r.x = tree[left ].r.x + tree[left].r.w + h3d2;
	ob->r.w = tree[right].r.x - ob->r.x - h3d2;
}


/*
 * rsc_yalign is currently applied -only- to slider background objects;
 * therefore the calculation is slightly different than in rsc_xalign,
 * because these objects are -not- marked to be set in 3D.
 * Note2: this routine also changes the size of the slider itself.
 */

static void rsc_yalign(OBJECT *tree, int up, int down, int object)
{
	OBJECT
		*ob = &tree[object];

	int
		v3d1 = aes_ver3d + 1;

	if(aes_hor3d)
	{
		ob->r.x -= aes_hor3d;
		ob->r.w += 2 * aes_hor3d - 1;
		tree[ob->ob_head].r.x += aes_hor3d;
	}

	ob->r.y = tree[up  ].r.y + tree[up].r.h + v3d1;
	ob->r.h = tree[down].r.y - ob->r.y - v3d1;

	/* Change fill pattern in sliders to full dark gray when appropriate */

	if ( xd_aes4_0 && xd_colaes )
		ob->ob_spec.obspec.fillpattern = 7; 
}


/*
 * Set object vertical position to chalfs * 1/2 char vertical distance 
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
 * Delete a menu item when the screen buffer is too small
 * for the complete menu.
 */

#if _MENUDEL

static void mn_del(int box, int item)
{
	OBJECT
		*tree = menu;

	int 
		i,
	 	y,
		ch_h = screen_info.fnt_h;


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

static void rsc_fixmenus(void)
{
	/* 
	 * Some modifications may be needed for older TOS (1.0, 1.2, 1.4, 1.6)
	 * Certain menu items will have to be deleted, in order
	 * to fit the menu into a limited screen buffer size (?).
	 * It is assumed that an alternative AES loaded over an old TOS
	 * will always be an AES4, capable of answering the query
	 * about buffer size
	 */

	RECT
		desk,
		boxrect;

	MFDB
		mfdb;

	union
	{
		long size;
		struct
		{
			int high;
			int low;
		} words;
	} buffer;

	long
		mnsize = 0;

	int
		i,
		n,
		dummy;


#if _MENUDEL

	static const char maxbox[] = {MNVIEWBX, MNOPTBOX};

	/* List of menuboxes and items in them to be deleted (maybe) */

	static const char mnbx[] = 
	{
		MNFILEBX,MNFILEBX,MNFILEBX,
		MNVIEWBX,MNVIEWBX,MNVIEWBX,
		MNOPTBOX,MNOPTBOX,MNOPTBOX,
		MNFILEBX,MNFILEBX,
		MNVIEWBX,MNVIEWBX,

		MNVIEWBX,MNVIEWBX,MNVIEWBX,MNVIEWBX, /* TOS < 1.04 only */
		MNOPTBOX,MNOPTBOX,MNOPTBOX,MNOPTBOX
	};

	static const char mnit[] = 
	{
		SEP1,SEP2,SEP3,
		SEP4,SEP5,SEP6,
		SEP7,SEP8,SEP9,
		MDELETE,MFCOPY,
		MSHSIZ,MREVS,

		MSHDAT,MSHTIM,MSHATT,MSUNSORT,		/* TOS < 1.04 only */
		MPRGOPT,MWDOPT,MVOPTS,MSAVEAS
	};

	/* 
	 * memory needed for the menu is estimated from the
	 * size of the View and Options menu boxes (the largest ones)
	 */

	for( i = 0; i < 2; i++ )
	{
		xd_objrect(menu, (int)maxbox[i], &boxrect);
		mnsize = lmax(mnsize, xd_initmfdb(&boxrect, &mfdb));
	}

	/*
	 * TOS 1.4 and higher can answer an inquiry about screen buffer size.
	 * Older TOSses (in fact older AESses ?)  can not.
	 */

	if ( aes_version >= 0x140 )
	{
		wind_get(0, WF_SCREEN, &dummy, &dummy, &buffer.words.high, &buffer.words.low);
		n = 13;
	}
	else
	{
		buffer.size = 8000L;
		n = 21;
	}

	if (mnsize >= buffer.size)
	{
		for ( i = 0; i < n; i++ )
			mn_del((int)mnbx[i], (int)mnit[i]);
	}
#endif

	/* Move some menu boxes left to fit the screen (if needed) */

	set_menubox(MNVIEWBX);
	set_menubox(MNWINBOX);
	set_menubox(MNOPTBOX);

	/* Adapt menu bar width to have the same width as screen */

	xw_getwork(NULL, &desk);
	menu[menu->ob_head].r.w = desk.w;
}


/*
 * Change validation and template strings. Space for both has
 * already to be allocated and of sufficient size.
 * Note: it seems that new strings must actually be copied to the old
 * locations, if just pointers are changed it won't work correctly.
 */

void rsc_fixtmplt(TEDINFO *ted, char *valid, char *tmplt)
{
	strcpy(ted->te_pvalid, valid);
	strcpy(ted->te_ptmplt, tmplt);
	ted->te_tmplen = (int)strlen(tmplt) + 1;
	ted->te_txtlen = (int)strlen(valid) + 1;
	ted->te_ptext[ted->te_txtlen] = 0;
}



#if _EDITLABELS

/*
 * Fix template and validation string for a field converted
 * to 8+3 filename form, validating only characters permitted in TOS
 * filenames. New validation and template are taken from the resource.
 */

void rsc_tostmplt(TEDINFO *ted)
{
	rsc_fixtmplt(ted, get_freestring(TFNVALID), get_freestring(TFNTMPLT));
}

#endif


/*
 * Convert a scrolled text field to a normal G_FTEXT suitable for a filename 
 * in 8+3 format (always exactly 12 characters long). The routine is to be 
 * used when the OS does not support long filenames. 
 * Object changes position depending on the value of "just":
 *
 *  just = 0: no change of position
 *  just > 0: move left edge by "just" characters to the right
 *  just < 0: move right edge by "just"-1 characters to the left
 *
 * id parameter "extra" is true, validation string suitable for masks is used;
 * otherwise, validation permits only characters valid in TOS filenames.
 * 
 * See also routines cv_fntoform() and cv_formtofn() 
 */
 
static void tos_fnform( OBJECT *tree, int object, int just, bool extra )
{
	OBJECT
		*obj = tree + object;

	TEDINFO 
		*ted = xd_get_obspecp(obj)->tedinfo; 

	int
		dx = 0;


	/* Fix horizontal position and size */ 

	if ( just < 0 )
		dx = ted->te_tmplen - 12  + just;
	else
		dx = just;

	obj->r.x += dx * screen_info.fnt_w;
	obj->r.w = 12 * screen_info.fnt_w;

	/* Change object type */

	obj->ob_type = G_FTEXT; /* no extended type */
	obj->ob_spec.tedinfo = ted;
	obj->ob_flags |= 0x400; /* set to background for gray text background */

	/* Change validation and template strings */

	rsc_fixtmplt(ted, get_freestring((extra) ? XFNVALID : TFNVALID), get_freestring(TFNTMPLT));
}


/*
 * A briefer form of the above, used several times for a saving in size
 */

static void tos_bform(OBJECT *tree, int object)
{
	tos_fnform(tree, object, -1, FALSE);
}


/*
 * Decode (hexadecimal) information on OS / AES version for the infobox
 * This will work correctly only on a three-digit hexadecimal number.
 */

static void show_os( OBJECT *obj, int v )
{
	sprintf(obj->ob_spec.tedinfo->te_ptext, " %3x", v);
}


/* 
 * Initialize the resource structures. All used dialogs should be set here. 
 * Also fix positions, object types, object sizes and whatever, depending
 * on the capabilities of the AES and the screen resolution.
 */

void rsc_init(void)
{
	int 
		i, 
		v3d2 = 2 * aes_ver3d + 1;

	static const char /* Beware: all these indices must be below 127 */ 
		xleft[] = {DSKPDOWN, DSKCDOWN, WINPDOWN, WINCDOWN},
		xitem[] = {DSKPUP, DSKCUP, WINPUP, WINCUP},
		xright[] = {DSKPAT, DSKPAT, WINPAT, WINPAT},
		xv[] = {DSKCDOWN, DSKCUP, WINCDOWN, WINCUP};

	/* Get pointers to dialog trees in the resource */

	xd_gaddr(MENU, &menu);
	xd_gaddr(OPTIONS, &setprefs);
	xd_gaddr(ADDPTYPE, &addprgtype);
	xd_gaddr(NEWDIR, &newfolder);
	xd_gaddr(GETCML, &getcml);
	xd_gaddr(FILEINFO, &fileinfo);
	xd_gaddr(INFOBOX, &infobox);
	xd_gaddr(ADDICON, &addicon);
	xd_gaddr(NAMECONF, &nameconflict);
	xd_gaddr(COPYINFO, &copyinfo);
	xd_gaddr(SETMASK, &setmask);
	xd_gaddr(APPLIKAT, &applikation);
	xd_gaddr(VIEWMENU, &viewmenu);
	xd_gaddr(STABSIZE, &stabsize);
	xd_gaddr(WOPTIONS, &wdoptions);
	xd_gaddr(WDFONT, &wdfont);
	xd_gaddr(HELP1, &helpno1);
	xd_gaddr(FLOPPY, &fmtfloppy);
	xd_gaddr(VOPTIONS, &vidoptions);
	xd_gaddr(COPTIONS, &copyoptions);
	xd_gaddr(ADDFTYPE, &ftydialog);	
	xd_gaddr(SEARCH, &searching);
	xd_gaddr(SPECAPP, &specapp);
	xd_gaddr(OPENW, &openw);
	xd_gaddr(COMPARE, &compare);

	/*  
	 * Define buffers for scrolling editable texts in dialogs. 
	 * Some strings are reused to save space. 
	 */ 

	oldname = xd_set_srcl_text(nameconflict, OLDNAME,  dirname  );	/* may be converted to 8+3 */
	newname = xd_set_srcl_text(nameconflict, NEWNAME,  flname   );	/* may be converted to 8+3 */
	tgname =  xd_set_srcl_text(fileinfo,     FLTGNAME, dirname  );	/* always a long name */
	          xd_set_srcl_text(fileinfo,     FLNAME,   flname   );	/* may be converted to 8+3 */
	          xd_set_srcl_text(newfolder,    OPENNAME, openline );	/* a command, i.e. very long */
	          xd_set_srcl_text(newfolder,    DIRNAME,  dirname  );
	          xd_set_srcl_text(getcml,       CMDLINE,  ttpline  );	/* a command, i.e. very long */
			  xd_set_srcl_text(applikation,  APCMLINE, cmdline  );	/* a command, i.e. long */
	          xd_set_srcl_text(applikation,  APNAME,   flname   );	/* may be converted to 8+3 */	
	          xd_set_srcl_text(applikation,  APPATH,   dirname  );
	          xd_set_srcl_text(applikation,  APLENV,   envline  );	/* always long */
	          xd_set_srcl_text(searching,    SMASK,    dirname  ); 	/* may be converted to 8+3 */
	          xd_set_srcl_text(addicon,      IFNAME,   dirname  );
	cfile1 =  xd_set_srcl_text(compare,      CFILE1,   dirname  );	/* always complete path, i.e. long */
	cfile2 =  xd_set_srcl_text(compare,      CFILE2,   flname   );	/* always cmplete path, i.e. long */	

	/* Pointers to some other often-used texts */

	drvid = addicon[DRIVEID].ob_spec.tedinfo->te_ptext;
	iconlabel = addicon[ICNLABEL].ob_spec.tedinfo->te_ptext;

	/* 
	 * Section immediately following fixes some characteristics of
	 * rsc objects depending on AES, screen resolution, etc.
	 *
	 * Note: rsc_xalign and rsc_yalign below change the dimensions
	 * of dialog items so that they do not overlap with adjacent items;
	 * this is supposed to prevent overlapping of scrolling arrow buttons
	 * with scrolled-item fields when 3D-enlargements exist.
	 */

	rsc_xalign(wdfont, WDFSUP, WDFSDOWN, WDFSIZE);
	rsc_xalign(wdfont, WDFCDOWN, WDFCUP, WDFCOL);
	rsc_xalign(vidoptions, VNCOLDN, VNCOLUP, VNCOL);
	rsc_xalign(setprefs, OPTMPREV, OPTMNEXT, OPTMTEXT);

	for(i = 0; i < 4; i++)
	{
		rsc_xalign(wdoptions, (int)xleft[i], (int)xitem[i], (int)xright[i]);
		wdoptions[xv[i]].r.y += v3d2;
	}

	wdoptions[DSKPAT].r.h += v3d2;
	wdoptions[WINPAT].r.h += v3d2;

	wdoptions[WDDCOLT].r.y = wdoptions[DSKCUP].r.y;
	wdoptions[WDWCOLT].r.y = wdoptions[WINCUP].r.y;

	addicon[ICONBACK].r.w = addicon[ICSELBOX].r.w - addicon[ICNUP].r.w - 2 * aes_hor3d - 1;

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

	rsc_yfix( copyinfo, CPT4, CPT4, 1 );		/* copy-info dialog     */
	rsc_yfix( fmtfloppy, FLABEL, FLOT1, 3 );	/* floppy-format dialog */		
	rsc_yfix( infobox, INFOVERS, INFOSYS, 3 );	/* info box */

	/*
	 * Assuming that filemask dialog and fonts dialog listboxes
	 * both have NLINES elements...
	 */

	for ( i = 0; i < NLINES; i++)				/* setmask & font dialogs */
	{
		int i2 = i * 2 + 1;
		rsc_yfix( setmask, FTPARENT, FTYPE1 + i, i2 );
		rsc_yfix( wdfont, WDPARENT, WDFONT1 + i, i2 ); 
	}		

	/*
	 * If neither Mint nor Magic is present, set validation strings for
	 * all file/filetype input fields to (automatically convert to) uppercase
	 * and modify their length to 12 characters (8+3 format).
	 * note: for scrolled text fields it is currently sufficient to put just
	 * the first validation character to lowercase "x" (see xdialog.c)   
	 */	

#if _MINT_
	if ( !mint )
#endif
	{
		char *s;

		/* These are SNAMEs */

		tos_fnform( addicon, ICNTYPE, 3, TRUE );
		tos_fnform( addprgtype, PRGNAME, 3, TRUE );
		tos_fnform( ftydialog, FTYPE0, -1, TRUE);
		tos_fnform( setmask, FILETYPE, 3, TRUE);
	
		for ( i = 0; i < NLINES; i++ )
			tos_fnform( setmask, FTYPE1 + i, 3, FALSE );

		/* These are VLNAMEs */

		tos_bform(fileinfo, FLNAME);
		tos_bform(copyinfo, CPFILE);
		tos_bform(applikation, APNAME);
		tos_bform(nameconflict, NEWNAME);
		tos_bform(nameconflict, OLDNAME);
		tos_bform(newfolder, DIRNAME);

		tos_fnform(searching, SMASK, -1, TRUE);

		/* Paths should be upppercase  and contain valid characters only */

		*xd_pvalid(&applikation[APPATH]) = 'p';
		*xd_pvalid(&compare[CFILE1]) = 'p';
		*xd_pvalid(&compare[CFILE2]) = 'p';
		*xd_pvalid(&addicon[IFNAME]) = 'p';

 		/* Icon namemask must allow ".." */

		xd_pvalid(&addicon[ICNTYPE])[8] = 'p';

		/* 
		 * Below is substituted "Files and links" with "Files" if no mint.
		 * If sometime links get into dialogs separately, this should be removed
		 */

		rsc_title(copyinfo, CIFILES, TFILES);
		s = get_freestring(TNFILES);
		memcpy(fileinfo[FLFILES].ob_spec.tedinfo->te_ptmplt, s, strlen(s));

		/* Modify size, position or visibility of some objects */

		addicon[CHNBUTT].r.y -= addicon[CHNBUTT].r.h;
		addicon[ADDBUTT].r.y -= addicon[CHNBUTT].r.h;
		addicon[0].r.h -= addicon[CHNBUTT].r.h;

		mn_del(MNVIEWBX, MSHOWN);
	}

#if _MINT_
	if(!xd_aes4_0)
#endif
		mn_del(MNWINBOX, MICONIF); /* can iconify only in AES 4 */

#if _EDITLABELS
#if _MINT_

	/* 
	 * Copy original validation strings for volume labels
	 * No action is taken if memory allocation fails here;
	 * hopefully failure will not occur, as this is done 
	 * only at startup of TeraDesk
	 */

	lblvalid = strdup(xd_pvalid(&fileinfo[FLLABEL]));
	lbltmplt = strdup((char *)fileinfo[FLLABEL].ob_spec.tedinfo->te_ptmplt);
#endif
#else
	tos_bform(fileinfo, FLLABEL);
#endif

	*drvid = 0;
	*iconlabel = 0;

	/* Fill-in the constant part of the info-box dialog */

#if _MINT_
	infobox[INFOSYS].ob_spec.tedinfo->te_ptext = get_freestring(TMULTI);
#endif
	infobox[INFOVERS].ob_spec.tedinfo->te_ptext = INFO_VERSION;
	infobox[COPYRGHT].ob_spec.tedinfo->te_ptext = INFO_COPYRIGHT;
	infobox[OTHRINFO].ob_spec.tedinfo->te_ptext = INFO_OTHER;

	show_os( &infobox[INFOTV], tos_version );
	show_os( &infobox[INFOAV], aes_version );

	rsc_fixmenus();
}


/* 
 * Modify a R_STRING-type text in a dialog (i.e. a dialog title) 
 * by pointing to another string in the resource tree
 * Note: see also routine get_freestring() in error.c
 */

void rsc_title(OBJECT *tree, int object, int title)
{
	OBSPEC s;

	rsrc_gaddr(R_STRING, title, &s);
	xd_set_obspec(tree + object, &s); 
}


/*
 * Write a long integer into a formatted text field. 
 * Text is right-justified and padded with spaces on the left side.
 * Maximum length is 14 decimal digits (no checking done).
 *
 * Parameters:
 *
 * tree		- object tree.
 * object	- index of formatted text field.
 * value	- value to convert.
 * This routine works with G_TEXT and G_FTEXT objects.
 * Note1: a good validation string must exist for the G_FTEXT field, 
 * it is used to determine field length
 * Length of a G_TEXT field is ignored. Care should be taken not to
 * try to write too many bytes.
 * Note2: it would be possible to use strcpyj() but there would be
 * almost no gain in size.
 * If a negative number is entered, a 'K' will be appended to the
 * absolute value of the number.
 */

void rsc_ltoftext(OBJECT *tree, int object, long value)
{
	OBJECT 
		*ob = tree + object;

	TEDINFO 
		*ti = xd_get_obspecp(ob)->tedinfo;

	int 
		l2;					/* length of the string representing the number */

	char 
		s[16], 				/* temporary buffer for the string */
		*s1 = s,			/* pointer to real beginning of s */
		*p = ti->te_ptext;	/* pointer to a location in the destination */


	ltoa(value, s, 10);		/* Convert value to ASCII, decimal */
	l2 = (int)strlen(s);	/* Length of this string  */

 	/* 
	 * If value is negative, '-' will be ignored, but 'K' will be added,
	 * so that length will remain the same
	 */

	if(value < 0)
	{
		s1++;			/* ignore the '-' in the string */
		s[l2++] = 'K';	/* append 'K' to ASCII presentation of number */
		s[l2] = 0;		/* and a termination 0 after the 'K' */
	}

   if((ob->ob_type & 0xFF) == G_FTEXT)
	{
		int l1 = (int)strlen(ti->te_pvalid) - l2;

		while(l1 > 0)
		{
			*p++ = ' ';
			l1--;
		}
	}

	strcpy(p, s1);	/* copy the number. Beware: no checking of overrun */
}

