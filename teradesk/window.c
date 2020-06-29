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
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "events.h"
#include "resource.h"
#include "xfilesys.h"
#include "dir.h"
#include "edit.h"
#include "file.h"
#include "filetype.h"
#include "icon.h"
#include "prgtype.h"
#include "viewer.h"
#include "window.h"
#include "applik.h"

typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
} ITM_WINDOW;

extern boolean in_window(WINDOW *w, int x, int y);
extern boolean wd_sel_enable;

typedef struct
{
	WINDOW *w;
	int selected;
	int n;
} SEL_INFO;

static SEL_INFO selection;

static void desel_old(void);

/********************************************************************
 *																	*
 * Functions for changing the View menu.							*
 *																	*
 ********************************************************************/

static void wd_set_attrib(int attrib)
{
	menu_icheck(menu, MHIDDEN, ((attrib & FA_HIDDEN) ? 1 : 0));
	menu_icheck(menu, MSYSTEM, ((attrib & FA_SYSTEM) ? 1 : 0));
}

static void wd_set_sort(int type)
{
	int i;

	for (i = 0; i < 5; i++)
		menu_icheck(menu, i + MSNAME, (i == type) ? 1 : 0);
}

static void wd_set_mode(int mode)
{
	int i;

	for (i = 0; i < 2; i++)
		menu_icheck(menu, MSHOWTXT + i, (i == mode) ? 1 : 0);
}

/********************************************************************
 *																	*
 * Funkties voor het enablen en disablen van menupunten.			*
 *																	*
 ********************************************************************/

/* Funktie die menupunten enabled en disabled, die met objecten te
   maken hebben. */

static void itm_set_menu(WINDOW *w)
{
	int n, *list, i = 0;
	boolean showinfo = FALSE, ch_icon = FALSE;
	ITMTYPE type;

	if ((w == NULL) || (xw_exist(w) == FALSE) || (itm_list(w, &n, &list) == FALSE))
	{
		w = NULL;
		n = 0;
	}

	while ((i < n) && ((showinfo == FALSE) || (ch_icon == FALSE)))
	{
		type = itm_type(w, list[i++]);
		if ((type != ITM_PRINTER) && (type != ITM_TRASH) && (type != ITM_PREVDIR))
			showinfo = TRUE;
		ch_icon = TRUE;
	}

	if (n == 1)
		type = itm_type(w, list[0]);

	menu_ienable(menu, MOPEN, ((n == 1) && (type != ITM_TRASH) && (type != ITM_PRINTER)) ? 1 : 0);
	menu_ienable(menu, MSHOWINF, (showinfo == TRUE) ? 1 : 0);
	menu_ienable(menu, MAPPLIK, ((n == 1) && (type == ITM_PROGRAM)) ? 1 : 0);

	if ((ch_icon == TRUE) && (xw_type(w) == DESK_WIND))
	{
		menu_ienable(menu, MREMICON, 1);
		menu_ienable(menu, MCHNGICN, 1);
	}
	else
	{
		menu_ienable(menu, MREMICON, 0);
		menu_ienable(menu, MCHNGICN, 0);
	}

	selection.w = (n > 0) ? w : NULL;
	selection.selected = (n == 1) ? list[0] : -1;
	selection.n = n;

	if (n > 0)
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

	while (w != NULL)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->itm_select(w, -1, 0, FALSE);
		w = xw_next();
	}

	((ITM_WINDOW *) desk_window)->itm_func->itm_select(desk_window, -1, 0, TRUE);

	itm_set_menu(NULL);
}

/* Funktie die aangeroepen moet worden, als door een andere oorzaak
   dan het met de muis selekteren of deselekteren van objecten,
   objecten gedeselekteerd worden. */

void wd_reset(WINDOW *w)
{
	if (w)
	{
		if (selection.w == w)
			itm_set_menu(w);
	}
	else
	{
		if (xw_exist(selection.w) == 0)
			itm_set_menu(NULL);
	}
}

/********************************************************************
 *																	*
 * Funktie voor het opvragen van het pad van een window.			*
 *																	*
 ********************************************************************/

const char *wd_toppath(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			return ((ITM_WINDOW *) w)->itm_func->wd_path(w);
		w = xw_next();
	}

	return NULL;
}

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

	while (w != NULL)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_set_update(w, type, fname1, fname2);
		w = xw_next();
	}

	((ITM_WINDOW *) desk_window)->itm_func->wd_set_update(desk_window, type, fname1, fname2);

	app_update(type, fname1, fname2);
}

void wd_do_update(void)
{
	WINDOW *w = xw_first();

	while (w != NULL)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_do_update(w);
		w = xw_next();
	}

	((ITM_WINDOW *) desk_window)->itm_func->wd_do_update(desk_window);
}

void wd_update_drv(int drive)
{
	WINDOW *w = xw_first();

	while (w != NULL)
	{
		if (xw_type(w) == DIR_WIND)
		{
			if (drive == -1)
				((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
			else
			{			
				const char *path = ((ITM_WINDOW *) w)->itm_func->wd_path(w);

#ifdef _MINT_
				if ((drive == ('U' - 'A')) &&
					((path[0] & 0xDF - 'A') == drive) &&
					(path[1] == ':'))
					((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
				else
				{
					XATTR attr;

					if ((x_attr(0, path, &attr) == 0) && (attr.dev == drive))
						((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
				}
#else
				if (((path[0] & 0xDF - 'A') == drive) && (path[1] == ':'))
					((ITM_WINDOW *) w)->itm_func->wd_set_update(w, WD_UPD_ALLWAYS, NULL, NULL);
#endif
			}
		}

		w = xw_next();
	}

	wd_do_update();
}

/********************************************************************
 *																	*
 * Funktie voor het sluiten van alle windows.						*
 *																	*
 ********************************************************************/

void wd_del_all(void)
{
	WINDOW *w = xw_last();

	while (w)
	{
		switch(xw_type(w))
		{
		case DIR_WIND :
			dir_close(w, 1);
			break;
		case TEXT_WIND :
			txt_closed(w);
			break;
		}

		w = xw_prev();
	}
}

/********************************************************************
 *																	*
 * Funktie voor het afhandelen van menu events.						*
 *																	*
 ********************************************************************/

static void wd_mode(int mode)
{
	WINDOW *w = xw_first();

	options.mode = mode;

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_disp_mode(w, mode);
		w = xw_next();
	}

	wd_set_mode(mode);
}

static void wd_sort(int sort)
{
	WINDOW *w = xw_first();

	options.sort = sort;

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_sort(w, sort);
		w = xw_next();
	}

	wd_set_sort(sort);
}

static void wd_attrib(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_attrib(w, options.attribs);
		w = xw_next();
	}
	wd_set_attrib(options.attribs);
}

void wd_seticons(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == DIR_WIND)
			((ITM_WINDOW *) w)->itm_func->wd_seticons(w);
		w = xw_next();
	}
}

void wd_hndlmenu(int item, int keystate)
{
	WINDOW *w;
	int object;

	switch (item)
	{
	case MOPEN:
		w = selection.w;
		object = selection.selected;
		if ((w != NULL) || (object >= 0))
		{
			if (itm_open(w, object, keystate) == TRUE)
				itm_select(w, object, 2, TRUE);
			itm_set_menu(selection.w);
		}
		break;
	case MSHOWINF:
		if ((w = selection.w) != NULL)
		{
			int n, *list;

			if ((itm_list(w, &n, &list) == TRUE) && (n > 0))
			{
				((ITM_WINDOW *) w)->itm_func->itm_showinfo(w, n, list);
				free(list);
			}
		}
		break;
	case MNEWDIR:
		if (((w = xw_top()) != NULL) && (xw_type(w) == DIR_WIND))
			((ITM_WINDOW *) w)->itm_func->wd_newfolder(w);
		break;
	case MCLOSE:
	case MCLOSEW:
		if ((w = xw_top()) != NULL)
		{
			switch(xw_type(w))
			{
			case DIR_WIND :
				dir_close(w, (item == MCLOSEW) ? 1 : 0);
				break;
			case TEXT_WIND :
				txt_closed(w);
				break;
			}
		}
		break;
	case MSELALL:
		if (((w = xw_top()) != NULL) && (xw_type(w) == DIR_WIND))
		{
			if (selection.w != w)
				desel_old();
			itm_select(w, 0, 4, TRUE);
			itm_set_menu(w);
		}
		break;
	case MCYCLE:
		xw_cycle();
		break;
	case MSETMASK:
		if (((w = xw_top()) != NULL) && (xw_type(w) == DIR_WIND))
			wd_set_filemask(w);
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
	case MHIDDEN:
		options.attribs ^= FA_HIDDEN;
		wd_attrib();
		break;
	case MSYSTEM:
		options.attribs ^= FA_SYSTEM;
		wd_attrib();
		break;
	}
}

/********************************************************************
 *																	*
 * Functions for initialisation of the windows modules and loading	*
 * and saving about windows.										*
 *																	*
 ********************************************************************/

void wd_init(void)
{
	menu[MOPEN].ob_state = DISABLED;
	menu[MSHOWINF].ob_state = DISABLED;
	menu[MAPPLIK].ob_state = DISABLED;
	menu[MREMICON].ob_state = DISABLED;
	menu[MCHNGICN].ob_state = DISABLED;
	menu[MCLOSE].ob_state = DISABLED;
	menu[MCLOSEW].ob_state = DISABLED;
	menu[MNEWDIR].ob_state = DISABLED;
	menu[MSELALL].ob_state = DISABLED;
	menu[MSETMASK].ob_state = DISABLED;
	menu[MCYCLE].ob_state = DISABLED;

	selection.w = NULL;
	selection.selected = -1;
	selection.n = 0;

	dir_init();
	txt_init();
	edit_init();
}

void wd_default(void)
{
	wd_del_all();

	wd_set_mode(options.mode);
	wd_set_sort(options.sort);
	wd_set_attrib(options.attribs);

	dir_default();
	txt_default();
	edit_default();
}

static int load_windows(XFILE *file)
{
	int type, error;
	long n;

	do
	{
		if ((n = x_fread(file, &type, sizeof(int))) != sizeof(int))
			 return (n < 0) ? (int) n : EEOF;

		switch (type)
		{
		case -1:
			error = 0;
			break;
		case DIR_WIND :
			error = dir_load_window(file);
			break;
		case TEXT_WIND :
			error = txt_load_window(file);
			break;
		}

		if (error < 0)
			return error;
	}
	while (type >= 0);

	return 0;
}

int wd_load(XFILE *file)
{
	int error;

	if ((error = dir_load(file)) < 0)
		return error;

	if ((error = txt_load(file)) < 0)
		return error;

	if ((error = edit_load(file)) < 0)
		return error;

	if ((error = load_windows(file)) < 0)
		return error;

	return 0;
}

static int save_windows(XFILE *file, boolean close)
{
	WINDOW *w;
	int dummy = -1, type, error;
	long n;

	w = xw_last();

	while (w != NULL)
	{
		switch(type = xw_type(w))
		{
		case DIR_WIND :
			if ((n = x_fwrite(file, &type, sizeof(int))) < 0)
				 return (int) n;
			if ((error = dir_save_window(file, w)) < 0)
				return error;
			if (close)
				dir_close(w, 1);
			break;
		case TEXT_WIND :
			if ((n = x_fwrite(file, &type, sizeof(int))) < 0)
				 return (int) n;
			if ((error = txt_save_window(file, w)) < 0)
				return error;
			if (close)
				txt_closed(w);
			break;
		default :
			if (close)
				return 1;
			break;
		}

		w = xw_prev();
	}

	return ((n = x_fwrite(file, &dummy, sizeof(int))) < 0) ? (int) n : 0;
}

int wd_save(XFILE *file)
{
	int error;

	if ((error = dir_save(file)) < 0)
		return error;

	if ((error = txt_save(file)) < 0)
		return error;

	if ((error = edit_save(file)) < 0)
		return error;

	if ((error = save_windows(file, FALSE)) < 0)
		return error;

	return 0;
}

/********************************************************************
 *																	*
 * Functies voor het tijdelijk sluiten en weer openen van windows.	*
 *																	*
 ********************************************************************/

static XFILE *config_file;

/*
 * Save current positions of the open windows into a file and
 * close the windows.
 *
 * Result: FALSE if there is no error, TRUE if there is an error.
 */

boolean wd_tmpcls(void)
{
	int error;

	wd_deselect_all();

	if ((config_file = x_fmemopen(O_DENYRW | O_RDWR, &error)) != NULL)
		error = save_windows(config_file, TRUE);

	if (error != 0)
	{
		if (config_file != NULL)
			x_fclose(config_file);
		if (error != 1)
			xform_error(error);
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * Load the positions of windows from a file and open them.
 */

void wd_reopen(void)
{
	int error;
	long offset;

	if ((offset = x_fseek(config_file, 0L, 0)) >= 0)
		error = load_windows(config_file);
	else
		error = (int) offset;

	x_fclose(config_file);

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

ITMTYPE itm_type(WINDOW *w, int item)
{
	ITMTYPE type = (((ITM_WINDOW *) w)->itm_func->itm_type) (w, item);

	if ((type == ITM_FILE) && (prg_isprogram(itm_name(w, item)) == TRUE))
		type = ITM_PROGRAM;

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
	return (((ITM_WINDOW *) w)->itm_func->itm_fullname) (w, item);
}

int itm_attrib(WINDOW *w, int item, int mode, XATTR *attrib)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_attrib) (w, item, mode, attrib);
}

long itm_info(WINDOW *w, int item, int which)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_info) (w, item, which);
}

boolean itm_open(WINDOW *w, int item, int kstate)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_open) (w, item, kstate);
}

void itm_select(WINDOW *w, int selected, int mode, boolean draw)
{
	if (xw_exist(w))
		(((ITM_WINDOW *) w)->itm_func->itm_select) (w, selected, mode, draw);
}

void itm_rselect(WINDOW *w, int x, int y)
{
	(((ITM_WINDOW *) w)->itm_func->itm_rselect) (w, x, y);
}

boolean itm_xlist(WINDOW *w, int *ns, int *nv, int **list, ICND **icns, int mx, int my)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_xlist) (w, ns, nv, list, icns, mx, my);
}

boolean itm_list(WINDOW *w, int *n, int **list)
{
	return (((ITM_WINDOW *) w)->itm_func->itm_list) (w, n, list);
}

/********************************************************************
 *																	*
 * Functies voor het afhandelen van muisklikken in een window.		*
 *																	*
 ********************************************************************/

static void itm_copy(WINDOW *sw, int n, int *list, WINDOW *dw,
					 int dobject, int kstate, ICND *icnlist, int x, int y)
{
	if (dw != NULL)
	{
		if ((dobject != -1) && (itm_type(dw, dobject) == ITM_FILE))
			dobject = -1;
		if (((ITM_WINDOW *)dw)->itm_func->itm_copy(dw, dobject, sw, n, list, icnlist, x, y, kstate) == TRUE)
			itm_select(sw, -1, 0, TRUE);
	}
	else
		alert_printf(1, MILLDEST);
}

/* Routines voor het tekenen van de omhullende van een icoon. */

static void get_minmax(ICND *icns, int n, int *clip)
{
	int i, j;

	for (i = 0; i < n; i++)
		for (j = 0; j < icns[i].np; j++)
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

static void clip_coords(int *clip, int x, int y, int *nx, int *ny)
{
	int h;

	*nx = x;
	*ny = y;

	h = screen_info.dsk_x - clip[0];
	if (x < h)
		*nx = h;

	h = screen_info.dsk_y - clip[1];
	if (y < h)
		*ny = h;

	h = screen_info.dsk_x + screen_info.dsk_w - 1 - clip[2];
	if (x > h)
		*nx = h;

	h = screen_info.dsk_y + screen_info.dsk_h - 1 - clip[3];
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

	if (itm_type(w, object) != ITM_FILE)
		itm_select(w, object, mode, TRUE);
}

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

static void itm_move(WINDOW *src_wd, int src_object, int old_x, int old_y)
{
	int x = old_x, y = old_y;
	WINDOW *cur_wd = src_wd, *new_wd;
	int cur_object = src_object, new_object;
	int clip[4];
	int ox, oy, kstate, *list, n, nv, i;
	boolean cur_state = TRUE, new_state, mreleased;
	ICND *icnlist;

	if (itm_type(src_wd, src_object) == ITM_PREVDIR)
	{
		wait_button();
		return;
	}

	if ((itm_list(src_wd, &n, &list) == FALSE) || (n == 0))
		return;

	for (i = 0; i < n; i++)
	{
		if (itm_type(src_wd, list[i]) == ITM_PREVDIR)
			itm_select(src_wd, list[i], 2, TRUE);
	}

	free(list);

	if (itm_xlist(src_wd, &n, &nv, &list, &icnlist, old_x, old_y) == FALSE)
		return;

	get_minmax(icnlist, nv, clip);

	wind_update(BEG_MCTRL);
	graf_mouse(FLAT_HAND, NULL);
	draw_icns(icnlist, nv, x, y, clip);

	do
	{
		ox = x;
		oy = y;
		mreleased = xe_mouse_event(0, &x, &y, &kstate);

		if ((x != ox) || (y != oy))
		{
			draw_icns(icnlist, nv, ox, oy, clip);
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
				/* Test if destination window is the desktop and if the
				   destination object is -1 (no object). If this is true,
				   clip the mouse coordinates. */

				if ((xw_type(cur_wd) == DESK_WIND) && (cur_object == -1) && (xw_type(src_wd) == DESK_WIND))
					clip_coords(clip, x, y, &x, &y);

				itm_copy(src_wd, n, list, cur_wd, cur_object, kstate, icnlist, x, y);
			}
			else
				alert_printf(1, MILLCOPY);
		}
		else
			alert_printf(1, MILLCOPY);
	}

	free(list);
	free(icnlist);
}

boolean in_window(WINDOW *w, int x, int y)
{
	GRECT work;

	xw_get(w, WF_WORKXYWH, &work);

	if ((x >= work.g_x) && (x < (work.g_x + work.g_w)) &&
		(y >= work.g_y) && (y < (work.g_y + work.g_h)))
		return TRUE;
	else
		return FALSE;
}

#pragma warn -par

void wd_hndlbutton(WINDOW *w, int x, int y, int n, int bstate,
				   int kstate)
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
			itm_set_menu(w);

			wait_button();

			if (itm_open(w, item, kstate) == TRUE)
				itm_select(w, item, 2, TRUE);
		}
		else
		{
			if ((m_state == 0) || (itm_state(w, item) == FALSE))
			{
				itm_select(w, item, (kstate & 3) ? 1 : 0, TRUE);
				itm_set_menu(w);
			}

			if ((m_state != 0) && (itm_state(w, item) == TRUE))
				itm_move(w, item, x, y);
		}

		itm_set_menu(selection.w);
	}
	else if (in_window(w, x, y) == TRUE)
	{
		if (((m_state == 0) || ((kstate & 3) == 0)) && (selection.w == w))
			itm_select(w, -1, 0, TRUE);
		if (m_state)
			itm_rselect(w, x, y);
		itm_set_menu(w);
	}
}

#pragma warn .par
