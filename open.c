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

#include <np_aes.h>			/* HR 151102: modern */
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "open.h"
#include "printer.h"
#include "resource.h"
#include "xfilesys.h"
#include "dir.h"
#include "edit.h"
#include "file.h"
#include "viewer.h"
#include "window.h"
#include "applik.h"

boolean item_open(WINDOW *w, int item, int kstate)
{
	const char *path;
	int button;
	ITMTYPE type;
	APPLINFO *appl;
	boolean alternate, deselect = FALSE;

	alternate = (kstate & 8) ? TRUE : FALSE;

	type = itm_type(w, item);

	switch (type)
	{
	case ITM_TRASH:
	case ITM_PRINTER:
		alert_printf(1, MICNOPEN);
		break;
	case ITM_DRIVE:
		if ((path = itm_fullname(w, item)) != NULL)
		{
			if (check_drive(path[0] - 'A') == FALSE)
			{
				free(path);
				return FALSE;
			}
			else
				deselect = dir_add_window(path);
		}
		else
			return FALSE;
		break;
	case ITM_PREVDIR:
		if ((path = fn_get_path(wd_path(w))) != NULL)
			deselect = dir_add_window(path);
		else
			return FALSE;
		break;
	case ITM_FOLDER:
		if ((path = itm_fullname(w, item)) != NULL)
			deselect = dir_add_window(path);
		else
			return FALSE;
		break;
	case ITM_PROGRAM:
		if ((path = itm_fullname(w, item)) != NULL)
		{
			deselect = app_exec(path, NULL, NULL, NULL, 0, kstate, FALSE);
			free(path);
		}
		break;
	case ITM_FILE:
		if ((alternate == FALSE) && (appl = app_find(itm_name(w, item))) != NULL)
			deselect = app_exec(NULL, appl, w, &item, 1, kstate, FALSE);
		else
		{
			if (edit_installed() == FALSE)
				openfile[EDITFILE].ob_state |= DISABLED;
			else
				openfile[EDITFILE].ob_state &= ~DISABLED;

			button = xd_dialog(openfile, 0);

			switch (button)
			{
			case SHOWFILE:
				deselect = txt_add_window(w, item, kstate);
				break;
			case EDITFILE:
				deselect = call_editor(w, item, kstate);
				break;
			case PRTFILE:
				deselect = prt_file(w, item);
				break;
			}
		}
		break;
	}

	return deselect;
}
