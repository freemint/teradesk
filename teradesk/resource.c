/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren.
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

#include <aes.h>
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
#include "window.h"

OBJECT *menu,
	   *setprefs,
	   *setprgprefs,
	   *addprgtype,
	   *openfile,
	   *newfolder,
	   *driveinfo,
	   *folderinfo,
	   *fileinfo,
	   *infobox,
	   *addicon,
	   *getcml,
	   *nameconflict,
	   *copyinfo,
	   *print,
	   *setmask,
	   *applikation,
	   *seticntype,
	   *addicntype,
	   *loadmods,
	   *viewmenu,
	   *stabsize,
	   *wdoptions,
	   *wdfont;

char *dirname,
	 *drvid,
	 *iconlabel,
	 *cmdline1,
	 *cmdline2,
	 *oldname,
	 *newname,
	 *cpfile,
	 *cpfolder,
	 *filetype,
	 *tabsize,
	 *copybuffer,
	 *applname,
	 *appltype,
	 *applcmdline,
	 *applfkey,
	 *prgname,
	 *icnname,
	 *vtabsize;

static void set_menubox(int box)
{
	int x, dummy, offset;

	objc_offset(menu, box, &x, &dummy);

	if ((offset = x + menu[box].ob_width + 2 * screen_info.fnt_w - max_w) > 0)
		menu[box].ob_x -= offset;
}


static void rsc_xalign(OBJECT *tree, int left, int right, int object)
{
	tree[object].ob_x = tree[left].ob_x + tree[left].ob_width + 1;
	tree[object].ob_width = tree[right].ob_x - tree[object].ob_x - 1;
}

static void rsc_yalign(OBJECT *tree, int up, int down, int object)
{
	tree[object].ob_y = tree[up].ob_y + tree[up].ob_height + 1;
	tree[object].ob_height = tree[down].ob_y - tree[object].ob_y - 1;
}

/* Funktie voor het verwijderen van een menupunt uit een menu */

static void mn_del(int item)
{
	int i, y, ch_h = screen_info.fnt_h;
	OBJECT *tree = menu;

	tree[MNOPTBOX].ob_height -= ch_h;
	y = tree[item].ob_y;
	i = tree[MNOPTBOX].ob_head;

	while (i != MNOPTBOX)
	{
		if (tree[i].ob_y > y)
			tree[i].ob_y -= ch_h;
		i = tree[i].ob_next;
	}

	objc_delete(menu, item);
}

static void rsc_fixmenus(void)
{
	GRECT desk;

	if (tos2_0() == FALSE)
	{
		int dummy;
		long mnsize;
		GRECT boxrect;
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

		if (tos1_4() == FALSE)
			buffer.size = 8000L;
		else
			wind_get(0, WF_SCREEN, &dummy, &dummy, &buffer.words.high, &buffer.words.low);

		if (mnsize > buffer.size)
		{
			mn_del(MAPPLIK);
			mn_del(MOPTIONS);
		}
	}

	set_menubox(MNOPTBOX);
	set_menubox(MNVIEWBX);

	xw_get(NULL, WF_WORKXYWH, &desk);
	menu[menu->ob_head].ob_width = desk.g_w;
}

void rsc_init(void)
{
	char tmp[5], *tosversion;
	int v = get_tosversion(), i, o;

	xd_gaddr(R_TREE, MENU, &menu);
	xd_gaddr(R_TREE, OPTIONS, &setprefs);
	xd_gaddr(R_TREE, PRGOPTNS, &setprgprefs);
	xd_gaddr(R_TREE, ADDPTYPE, &addprgtype);
	xd_gaddr(R_TREE, OPENFILE, &openfile);
	xd_gaddr(R_TREE, NEWDIR, &newfolder);
	xd_gaddr(R_TREE, GETCML, &getcml);
	xd_gaddr(R_TREE, DRVINFO, &driveinfo);
	xd_gaddr(R_TREE, FLDRINFO, &folderinfo);
	xd_gaddr(R_TREE, FILEINFO, &fileinfo);
	xd_gaddr(R_TREE, INFOBOX, &infobox);
	xd_gaddr(R_TREE, ADDICON, &addicon);
	xd_gaddr(R_TREE, NAMECONF, &nameconflict);
	xd_gaddr(R_TREE, COPYINFO, &copyinfo);
	xd_gaddr(R_TREE, PRINT, &print);
	xd_gaddr(R_TREE, SETMASK, &setmask);
	xd_gaddr(R_TREE, APPLIKAT, &applikation);
	xd_gaddr(R_TREE, SETICONS, &seticntype);
	xd_gaddr(R_TREE, ADDITYPE, &addicntype);
	xd_gaddr(R_TREE, VIEWMENU, &viewmenu);
	xd_gaddr(R_TREE, STABSIZE, &stabsize);
	xd_gaddr(R_TREE, WOPTIONS, &wdoptions);
	xd_gaddr(R_TREE, WDFONT, &wdfont);

#ifndef _MINT_
	if (getcml->ob_width > max_w)
		xd_gaddr(R_TREE, LGETCML, &getcml);
#endif

	dirname = newfolder[DIRNAME].ob_spec.tedinfo->te_ptext;
	drvid = addicon[DRIVEID].ob_spec.tedinfo->te_ptext;
	iconlabel = addicon[ICNLABEL].ob_spec.tedinfo->te_ptext;
	cmdline1 = getcml[CMDLINE1].ob_spec.tedinfo->te_ptext;
	cmdline2 = getcml[CMDLINE2].ob_spec.tedinfo->te_ptext;
	oldname = nameconflict[OLDNAME].ob_spec.tedinfo->te_ptext;
	newname = nameconflict[NEWNAME].ob_spec.tedinfo->te_ptext;
	cpfile = copyinfo[CPFILE].ob_spec.tedinfo->te_ptext;
	cpfolder = copyinfo[CPFOLDER].ob_spec.tedinfo->te_ptext;
	filetype = setmask[FILETYPE].ob_spec.tedinfo->te_ptext;
	tabsize = setprefs[TABSIZE].ob_spec.tedinfo->te_ptext;
	copybuffer = setprefs[COPYBUF].ob_spec.tedinfo->te_ptext;
	applname = applikation[APNAME].ob_spec.tedinfo->te_ptext;
	appltype = applikation[APTYPE].ob_spec.tedinfo->te_ptext;
	applcmdline = applikation[APCMLINE].ob_spec.tedinfo->te_ptext;
	applfkey = applikation[APFKEY].ob_spec.tedinfo->te_ptext;
	prgname = addprgtype[PRGNAME].ob_spec.tedinfo->te_ptext;
	icnname = addicntype[AITTYPE].ob_spec.tedinfo->te_ptext;
	vtabsize = stabsize[VTABSIZE].ob_spec.tedinfo->te_ptext;

	rsc_xalign(applikation, APPREV, APNEXT, APTYPE);
	rsc_xalign(wdoptions, DSKPDOWN, DSKPUP, DSKPAT);
	rsc_xalign(wdoptions, DSKCDOWN, DSKCUP, DSKCOLOR);
	rsc_xalign(wdfont, WDFSUP, WDFSDOWN, WDFSIZE);

	rsc_yalign(addicon, ICNUP, ICNDWN, ICPARENT);
	rsc_yalign(setprgprefs, PUP, PDOWN, PSPARENT);
	rsc_yalign(addicntype, ITUP, ITDOWN, ITPARENT);
	rsc_yalign(seticntype, IUP, IDOWN, ISPARENT);
	rsc_yalign(setmask, FTUP, FTDOWN, FTSPAR);
	rsc_yalign(wdfont, WDFUP, WDFDOWN, FSPARENT);

	*drvid = 0;
	*iconlabel = 0;
	*cmdline1 = 0;
	*cmdline2 = 0;

	strcpy(infobox[INFOVERS].ob_spec.tedinfo->te_ptext, INFO_VERSION);
	infobox[COPYRGHT].ob_spec.tedinfo->te_ptext = INFO_COPYRIGHT;

	tosversion = infobox[INFOTV].ob_spec.tedinfo->te_ptext;

	o = (int) strlen(itoa(v, tmp, 16)) - 4;

	for (i = 0; i < 4; i++)
		tosversion[i] = ((i + o) >= 0) ? tmp[i + o] : ' ';

	rsc_fixmenus();
}

void rsc_title(OBJECT *tree, int object, int title)
{
	char *s;

	xd_gaddr(R_STRING, title, &s);
	tree[object].ob_spec.userblk->ub_parm = (long) s;
}

/*
 * Write a long into a formatted text field. Text is right justified
 * and padded with spaces.
 *
 * Parameters:
 *
 * tree		- object tree.
 * object	- index of formatted text field.
 * value	- value to convert.
 */

void rsc_ltoftext(OBJECT *tree, int object, long value)
{
	int l1, l2, i;
	char s[16], *p;
	TEDINFO *ti;

	ti = tree[object].ob_spec.tedinfo;
	p = ti->te_ptext;
	l1 = (int) strlen(ti->te_pvalid);	/* Length of text field. */
	ltoa(value, s, 10);					/* Convert value to ASCII. */
	l2 = (int) strlen(s);				/* Length of number. */

	i = 0;

	while (i < (l1 - l2))
	{
		*p++ = ' ';						/* Fill with spaces. */
		i++;
	}

	strcpy(p, s);						/* Copy number. */
}
