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
#include <stdarg.h>
#include <stdio.h>

#include "desktop.h"			/* HR 151102: only 1 rsc */
#include "error.h"

int vaprintf(int def, const char *string, va_list argpoint)
{
	char s[256];

	vsprintf(s, string, argpoint);
	return form_alert(def, s);
}

int alert_msg(int def, const char *string, ...)
{
	va_list argpoint;
	int button;

	va_start(argpoint, string);
	button = vaprintf(def, string, argpoint);
	va_end(argpoint);

	return button;
}

int alert_printf(int def, int message,...)
{
	va_list argpoint;
	int button;
	char *string;

	rsrc_gaddr(R_STRING, message, &string);

	va_start(argpoint, message);
	button = vaprintf(def, string, argpoint);
	va_end(argpoint);

	return button;
}

void xform_error(int error)
{
	int message;

	switch (error)
	{
	case EFILNF:
		message = MEFILENF;
		break;
	case EPTHNF:
		message = MEPATHNF;
		break;
	case ENSMEM:
		message = MENSMEM;
		break;
	case ELOCKED:
		message = MELOCKED;
		break;
	case EPLFMT:
		message = MEPLFMT;
		break;
	case ECOMTL:
		message = MCMDTLNG;
		break;
	case EREAD:
		message = MEREAD;
		break;
	case EFNTL:
		message = MEFNTLNG;
		break;
	case EPTHTL:
		message = MEPTHTLN;
		break;
	case _XDVDI:
		message = MVDIERR;
		break;
	default:
		message = -1;
		break;
	}

	if (message == -1)
	{
		if ((error <= EINVFN) && ((error > XABORT) || (error < XFATAL)))
			alert_printf(1, MERROR, error);
	}
	else
		alert_printf(1, message);
}

static char *get_message(int error)
{
	static char buffer[32], *s;
	int msg;

	switch (error)
	{
	case EFILNF:
		msg = TFILNF;
		break;
	case EPTHNF:
		msg = TPATHNF;
		break;
	case EACCDN:
		msg = TACCDN;
		break;
	case ENSMEM:
		msg = TENSMEM;
		break;
	case EDSKFULL:
		msg = TDSKFULL;
		break;
	case ELOCKED:
		msg = TLOCKED;
		break;
	case EPLFMT:
		msg = TPLFMT;
		break;
	case EFNTL:
		msg = TFNTLNG;
		break;
	case EPTHTL:
		msg = TPTHTLNG;
		break;
	case EREAD:
		msg = TREAD;
		break;
	case EEOF:
		msg = TEOF;
		break;
	default:
		rsrc_gaddr(R_STRING, TERROR, &s);
		sprintf(buffer, s, error);
		return buffer;
	}

	rsrc_gaddr(R_STRING, msg, &s);

	return s;
}

void hndl_error(int message, int error)
{
	if (error > EINVFN)
		return;

	alert_printf(1, message, get_message(error));
}

int xhndl_error(int msg, int error, const char *file)
{
	int button;
	char *message;

	if ((error >= XFATAL) && (error <= XERROR))
		return error;
	else
	{
		if ((error < 0) && (error >= -17))
			return XFATAL;
		else
		{
			message = get_message(error);

			if ((error == EFILNF) || (error == EACCDN) || (error == ELOCKED))
			{
				char *buttons;

				rsrc_gaddr(R_STRING, TSKIPABT, &buttons);
				button = alert_printf(1, msg, file, message, buttons);
				return ((button == 1) ? XERROR : XABORT);
			}
			else
			{
				char *buttons;

				rsrc_gaddr(R_STRING, TABORT, &buttons);
				alert_printf(1, msg, file, message, buttons);
				return XABORT;
			}
		}
	}
}
