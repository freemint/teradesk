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

#include <np_aes.h>			/* modern */
#include <stdarg.h>
#include <sprintf.h>
#include <stddef.h>
#include <vdi.h>
#include <xdialog.h> 

#include "desktop.h"
#include "error.h"

/* 
 * DjV 035 120203
 * get_freestring routine gets pointer to a free-string in the resource,
 * identified by its index; handy for composing various texts.
 */

char *get_freestring( int stringid )
{
	OBSPEC s;
	xd_gaddr(R_STRING, stringid, &s);
	return s.free_string;
}


/* 
 * Retrieve a text string related to some errors, identified by error code.
 * Anything undefined is "TOS error #%d"
 * 
 */

char *get_message(int error)
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
	case EFRVAL:
		msg = TFRVAL;
		break;
	case ECOMTL:
		msg = TCMDTLNG;
		break;
	default:
		rsrc_gaddr(R_STRING, TERROR, &s);
		sprintf(buffer, s, error);
		return buffer;
	}

	rsrc_gaddr(R_STRING, msg, &s);

	return s;
}



#if DEBUG

/* 
 * Display an alert box while debugging 
 */

int alert_msg(const char *string, ...)
{
	char alert[256];
	va_list argpoint;
	int button;
	sprintf(alert, "[1][ %s ][ Ok ]", string);
	va_start(argpoint, string);
	button = vaprintf(1, alert, argpoint);
	va_end(argpoint);

	return button;
}
 
#endif


/* 
 * Display an alert box identified by "message" (alert-box id.) and  
 * contining some texts; return button index 
 */

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



/*
 * Display a single-button alert box (" ! " icon) with a text 
 * identified only by string id "message";
 */

int alert_iprint( int message )
{
	return alert_printf( 1, AGENALRT, get_freestring( message ) );
}


/* 
 * Display an alert box (" ! " icon, except in one case) 
 * text being identified only by error code.
 * Anything undefined is "TOS error %d"
 */

void xform_error(int error)
{
	/* Hopefully this optimization will work OK, hard to test it all now */

	if ( error == _XDVDI )
		alert_printf( 1, AVDIERR );	/* because of another icon here */
	else
		if ( error < 0 )
			alert_printf( 1, AGENALRT, get_message(error) );

}


/* 
 * Display an alert box identified by "message" and additional
 * error-message text idenified by "error" code 
 */

void hndl_error(int message, int error)
{
	if (error > EINVFN)
		return;

	alert_printf(1, message, get_message(error));
}


/* 
 * display alert box (" ! " icon) for some file-related errors;
 * DjV 068 230703: earlier, "msg" identified the alert-box form,
 * now it idenifies the first message text in AGFALERT alert box.
 * The alert box first displayes text "msg", then "file", then
 * the text associated to error code "error".
 */

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
			char *buttons;

			/* Get some text related to error code */

			message = get_message(error);

			/* Display appropriate alert-box */

			if ((error == EFILNF) || (error == EACCDN) || (error == ELOCKED))
			{
				/* For: File not found, Access denied, File locked: */

				rsrc_gaddr(R_STRING, TSKIPABT, &buttons);
				button = alert_printf(1, AGFALERT, get_freestring(msg), file, message, buttons);
				return ((button == 1) ? XERROR : XABORT);
			}
			else if ( error != ENOMSG )
			{
				/* For all the rest... */

				rsrc_gaddr(R_STRING, TABORT, &buttons);
				alert_printf(1, AGFALERT, get_freestring(msg), file, message, buttons);
				return XABORT;
			}
			else
				return XABORT;
		}
	}
}



