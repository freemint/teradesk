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
#include <time.h>
#include <tos.h>
#include <vdi.h>
#include <xdialog.h>
#include <xscncode.h>
#include <mint.h>

#include "desk.h"
#include "resource.h"
#include "printer.h"
#include "error.h"
#include "xfilesys.h"
#include "window.h"
#include "file.h"
#include "events.h"

#define PBUFSIZ	1024L

static boolean prtchar(char ch)
{
	long time;
	boolean ready = FALSE, result;
	int button;

	do
	{
		time = clock() + 1000;
		while ((clock() < time) && (Cprnos() == 0));
		if (Cprnos() != 0)
		{
			Cprnout(ch);
			result = FALSE;
			ready = TRUE;
		}
		else
		{
			button = alert_printf(2, MPRNRESP);
			result = TRUE;
			ready = (button == 2) ? TRUE : FALSE;
		}
	}
	while (ready == FALSE);

	return result;
}

static int print_file(WINDOW *w, int item)
{
	int handle, error = 0, i, key, result = 0, r;
	char *buffer;
	const char *name;
	long l;
	boolean stop = FALSE;

	if ((name = itm_fullname(w, item)) == NULL)
		return XFATAL;

	buffer = x_alloc(PBUFSIZ);

	if (buffer != NULL)
	{
		graf_mouse(HOURGLASS, NULL);

		if ((handle = x_open(name, O_DENYW | O_RDONLY)) >= 0)
		{
			do
			{
				if ((l = x_read(handle, PBUFSIZ, buffer)) >= 0)
				{
					for (i = 0; i < (int) l; i++)
					{
						if ((stop = prtchar(buffer[i])) == TRUE)
							break;
					}

					if ((r = key_state(&key, TRUE)) > 0)
					{
						if (key == ESCAPE)
							stop = TRUE;
					}
					else if (r < 0)
						stop = TRUE;
				}
				else
					error = (int) l;
			}
			while ((l == PBUFSIZ) && (stop == FALSE));

			x_close(handle);
		}
		else
			error = handle;

		if (stop == TRUE)
			result = XABORT;

		if (error != 0)
			result = xhndl_error(MEPRINT, error, itm_name(w, item));

		graf_mouse(ARROW, NULL);
		x_free(buffer);
	}
	else
	{
		xform_error(ENSMEM);
		result = XFATAL;
	}

	free(name);

	return result;
}

static boolean check_print(WINDOW *w, int n, int *list)
{
	int i;
	boolean noerror;
	char *mes = "";

	for (i = 0; i < n; i++)
	{
		noerror = FALSE;

		switch (itm_type(w, list[i]))
		{
		case ITM_PRINTER:
			rsrc_gaddr(R_STRING, MPRINTER, &mes);
			break;
		case ITM_TRASH:
			rsrc_gaddr(R_STRING, MTRASHCN, &mes);
			break;
		case ITM_DRIVE:
			rsrc_gaddr(R_STRING, MDRIVE, &mes);
			break;
		case ITM_FOLDER:
			rsrc_gaddr(R_STRING, MFOLDER, &mes);
			break;
		case ITM_PROGRAM:
			rsrc_gaddr(R_STRING, MPROGRAM, &mes);
			break;
		case ITM_FILE:
			noerror = TRUE;
			break;
		}
		if (noerror == FALSE)
			break;
	}
	if (noerror == FALSE)
		alert_printf(1, MNOPRINT, mes);

	return noerror;
}

boolean prt_file(WINDOW *w, int item)
{
	return (print_file(w, item) == 0) ? TRUE : FALSE;
}

boolean item_print(WINDOW *wd, int n, int *list)
{
	int i = 0, button, result = 0;
	boolean noerror = TRUE;
	XDINFO info;

	if (check_print(wd, n, list) == FALSE)
		return FALSE;

	rsc_ltoftext(print, NITEMS, n);
	xd_open(print, &info);

	button = xd_form_do(&info, 0);

	if (button == PRINTOK)
	{
		while ((i < n) && (result != XFATAL) && (result != XABORT))
		{
			result = print_file(wd, list[i]);

			if (result == XFATAL)
				noerror = FALSE;

			rsc_ltoftext(print, NITEMS, n - i - 1);
			xd_draw(&info, NITEMS, 1);
			i++;
		}
	}
	else
		noerror = FALSE;

	xd_change(&info, button, NORMAL, 0);
	xd_close(&info);

	return noerror;
}
