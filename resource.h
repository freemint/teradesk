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

#include "desktop.h"

extern	OBJECT 
	*menu,
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
	*helpno3,
	*fmtfloppy, 
	*vidoptions,
	*copyoptions,
	*ftydialog,
	*searching,
	*specapp;

extern char 
	*dirname,
	*openline,
	*oldname,
	*newname,
	*tgname,
	*envline,
	*finame,
	*flname,
	*cmdline,
	*applcmdline,
	*spattfld,	
	*drvid,
	*iconlabel,
	*cpfile,
	*cpfolder;


void rsc_init(void);
void rsc_title(OBJECT *tree, int object, int title);
void rsc_ltoftext(OBJECT *tree, int object, long value);
