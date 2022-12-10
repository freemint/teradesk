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

/*
 * If KBMB is defined as 1048576 instead of 1024, 'M' and 'G'
 * suffixes will be used to display large file sizes
 * instead of current 'K' and 'M'
 */

#define DISP_KBMB 9999999L 	/* sizes larger than this to be shown in KB or MB */
#define KBMB 1024			/* kilo or mega */

#include "desktop.h"


extern OBJECT *menu;
extern OBJECT *setprefs;
extern OBJECT *addprgtype;
extern OBJECT *newfolder;
extern OBJECT *fileinfo;
extern OBJECT *infobox;
extern OBJECT *addicon;
extern OBJECT *getcml;
extern OBJECT *nameconflict;
extern OBJECT *copyinfo;
extern OBJECT *setmask;
extern OBJECT *applikation;
extern OBJECT *loadmods;
extern OBJECT *viewmenu;
extern OBJECT *stabsize;
extern OBJECT *wdoptions;
extern OBJECT *wdfont;
extern OBJECT *helpno1;
extern OBJECT *fmtfloppy; 
extern OBJECT *vidoptions;
extern OBJECT *copyoptions;
extern OBJECT *ftydialog;
extern OBJECT *searching;
extern OBJECT *specapp;
extern OBJECT *openw;
extern OBJECT *compare;
extern OBJECT *quitopt;

extern char *oldname;
extern char *newname;
extern char *cfile1;
extern char *cfile2;
extern char *tgname;
extern char *drvid;
extern char *iconlabel;

extern VLNAME openline;
extern VLNAME flname;
extern VLNAME cmdline;
extern VLNAME ttpline;
extern VLNAME dirname;
extern VLNAME envline;

void rsc_init(void);
void rsc_title(OBJECT *tree, _WORD object, _WORD title);
void rsc_ltoftext(OBJECT *tree, _WORD object, long value);
void rsc_fixtmplt(TEDINFO *ted, char *valid, char *tmplt);
void rsc_tostmplt(TEDINFO *ted);
void rsc_hidemany(OBJECT *tree, const _WORD *items);
char *fmt_size(long value, _WORD *l);
