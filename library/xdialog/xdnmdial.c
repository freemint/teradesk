/*
 * Xdialog Library. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                                      2002, 2003  H. Robbers,
 *                                            2003  Dj. Vukovic
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


#ifdef __PUREC__
 #include <np_aes.h>
 #include <vdi.h>
#else
 #include <aesbind.h>
 #include <vdibind.h>
#endif

#include <stddef.h>

#include "xdialog.h"

#include "internal.h"


#define XD_NMWDFLAGS	(NAME | CLOSER | MOVER)


/*
 * Funktie voor het afhandelen van een keyboard event in een
 * niet modale dialoogbox.
 */

int __xd_hndlkey(WINDOW *w, int key, int kstate)
{
	int next_obj, nkeys, kr;
	int cont, key_handled = TRUE;
	XDINFO *info = ((XD_NMWINDOW *)w)->xd_info;
	KINFO *kinfo;
	OBJECT *tree = info->tree;

	xd_wdupdate(BEG_UPDATE);

	nkeys = ((XD_NMWINDOW *)w)->nkeys;
	kinfo = ((XD_NMWINDOW *)w)->kinfo;

	if ((next_obj = xd_find_key(tree, kinfo, nkeys, key)) >= 0)
		cont = xd_form_button(info, next_obj, 1, &next_obj);
	else
	{
		cont = xd_form_keybd(info, 0, key, &next_obj, &kr);
		if (kr)
			key_handled = xd_edit_char(info, kr);
	}

	if (cont)
	{
		if (next_obj != 0)
			xd_edit_init(info, next_obj, -1);
		xd_wdupdate(END_UPDATE);
	}
	else
	{
		xd_wdupdate(END_UPDATE);
		info->func->dialbutton(info, next_obj);
	}

	return key_handled;
}


/*
 * Funktie voor het afhandelen van een button event in een
 * niet modale dialoogbox.
 */

void __xd_hndlbutton(WINDOW *w, int x, int y, int n, int bstate, int kstate)
{
	XDINFO *info = ((XD_NMWINDOW *)w)->xd_info;
	int next_obj, cmode;
	int cont;


	if ((next_obj = objc_find(info->tree, ROOT, MAX_DEPTH, x, y)) != -1)
	{
		xd_wdupdate(BEG_UPDATE);

		if ((cont = xd_form_button(info, next_obj, n, &next_obj)) != FALSE)
			cmode = x;

		if (cont)
		{
			if (next_obj != 0)
				xd_edit_init(info, next_obj, cmode);
			xd_wdupdate(END_UPDATE);
		}
		else
		{
			xd_wdupdate(END_UPDATE);
			info->func->dialbutton(info, next_obj);
		}
	}
}


/*
 * Funktie voor het afhandelen van een window topped event in een
 * niet modale dialoogbox.
 */

void __xd_topped(WINDOW *w)
{
	xw_set(w, WF_TOP);
}


/*
 * Funktie voor het afhandelen van een window closed event in een
 * niet modale dialoogbox.
 */

void __xd_closed(WINDOW *w)
{
	XDINFO *info = ((XD_NMWINDOW *)w)->xd_info;

	info->func->dialclose(info);
}


/*
 * Funktie die wordt aangeroepen als een menu van de niet-modale
 * dialoogbox geselekteerd is.
 */

void __xd_hndlmenu(WINDOW *w, int title, int item)
{
	XDINFO *info = ((XD_NMWINDOW *)w)->xd_info;

	info->func->dialmenu(info, title, item);
}


/*
 * Funktie die wordt aangeroepen als een dialoogbox het bovenste
 * window van een applicatie is geworden.
 */

void __xd_top(WINDOW *w)
{
	XDINFO *info = ((XD_NMWINDOW *)w)->xd_info;

	if(info->func->dialtop != 0L)
		info->func->dialtop(info);
}

