/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2017  Dj. Vukovic
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */


#include <library.h>
#include <xdialog.h>

#include "desktop.h"
#include "error.h"
#include "stringf.h"


/* 
 * Obtain a pointer to a free-string in the resource,
 * identified by its index; handy for composing various texts.
 */

char *get_freestring(_WORD stringid)
{
	OBSPEC s;

	rsrc_gaddr(R_STRING, stringid, &s);
	return s.free_string;
}


/* 
 * Retrieve a text string related to some errors, identified by error code.
 * Anything undefined is "TOS error #%d" or (X)BIOS error #%d.
 * Beware that in such a case the resulting string should not be longer 
 * than 39 characters. There is no checking of buffer overflow!
 * Possibly an error-vs-message table could be used here instead of the 
 * case structure. However, gain appears to be small (about 40 bytes) and
 * there is a loss in clarity.
 */

char *get_message(_WORD error)
{
	static char buffer[40];
	_WORD msg;

	switch (error)
	{
	case EMEDIUMTYPE:					/* -7 */
	case ENODEV:
		msg = TUNKM;
		break;
	case ESPIPE:						/* -6 */
	case ESECTOR:						/* -8 */
		msg = TSEEKE;
		break;
	case EROFS:							/* -13 */
		msg = TWPROT;
		break;
	case EBADSEC:						/* -16 */
		msg = TBADS;
		break;
	case ENOENT:						/* -33 */
		msg = TFILNF;
		break;
	case EBUSY:							/* -2 */
	case ENXIO:							/* -46 */
	case ENOTDIR:						/* -34 */
		msg = TPATHNF;
		break;
	case EACCES:						/* -36 */
		msg = TACCDN;
		break;
	case ENOMEM:						/* -39 */
		msg = TENSMEM;
		break;
	case ELOCKED:						/* -58 */
		msg = TLOCKED;
		break;
	case ENOEXEC:						/* -66 */
		msg = TPLFMT;
		break;
	case EIO:							/* -90 */
		msg = TREAD;
		break;
	case ENOSPC:						/* -91 */
		msg = TDSKFULL;
		break;
	case EEOF:							/* -2051 */
		msg = TEOF;
		break;
	case EFRVAL:						/* -2053 */
		msg = TFRVAL;
		break;
	case E2BIG:							/* -125 */
		msg = TCMDTLNG;
		break;
	case ENAMETOOLONG:					/* -86 */
		msg = TPTHTLNG;
		break;
	case EFNTL:							/* -2066 */
		msg = TFNTLNG;
		break;
	case XDVDI:							/* -4096 */
		msg = MVDIERR;
		break;
	case XDNMWINDOWS:					/* -4097 */
		msg = MTMWIND;
		break;
	default:
		sprintf(buffer, get_freestring(error < ENOMEDIUM ? TERROR : TXBERROR), error);
		return buffer;
	}

	return get_freestring(msg);
}



/* 
 * Display an alert box while debugging 
 */

_WORD alert_msg(const char *string, ...)
{
	char alert[256];
	char s[256];
	va_list argpoint;
	_WORD button;

	sprintf(alert, "[1][ %s ][ Ok ]", string);
	va_start(argpoint, string);
	vsprintf(s, string, argpoint);
	button = form_alert(1, s);
	va_end(argpoint);

	return button;
}


/* 
 * Display an alert box identified by "message" (alert-box id.) and  
 * possibly containing some other texts; return button index 
 */

_WORD alert_printf(_WORD def,			/* default button index */
				   _WORD message,		/* message text id. */
				   ...					/* other text to be printed */
	)
{
	va_list argpoint;
	_WORD button;
	char *string;
	char s[256];

	string = get_freestring(message);
	va_start(argpoint, message);
	vsprintf(s, string, argpoint);
	button = form_alert(def, s);
	va_end(argpoint);

	return button;
}



/*
 * Display a single-button alert box (" ! " icon) with a text 
 * identified only by string-id "message";
 */

void alert_iprint(_WORD message)
{
	alert_printf(1, AGENALRT, get_freestring(message));
}


/*
 * Display an alert generally corresponding to an inability
 * to perform an operation. Form: <object type> can not be <operation>.
 */

void alert_cantdo(_WORD msg1, _WORD msg2)
{
	alert_printf(1, ACANTDO, get_freestring(msg1), get_freestring(msg2));
}


/*
 * Display a single-button alert box (" Stop " icon) with a text 
 * identified only by string-id "message", This is called
 * for fatal errors which will stop TeraDesk.
 */

void alert_abort(_WORD message)
{
	alert_printf(1, AFABORT, get_freestring(message));
}


/*
 * Display a two-buttons alert box (" ? " icon) with a text 
 * identified only by string id "message", 
 */

_WORD alert_query(_WORD message)
{
	return alert_printf(1, AQUERY, get_freestring(message));
}


/* 
 * Display an alert box (" ! " icon, except in one case), 
 * text being identified only by error code.
 * Error code ENOMSG will not create an alert.
 * Anything undefined is "TOS error %d".
 * This routine ignores error codes >= 0.
 */

void xform_error(_WORD error)
{
	/* Hopefully this optimization will work OK, hard to test it all now */

	if (error == XDVDI)
		alert_abort(MVDIERR);			/* because of another icon here */
	else if (error < 0 && error != ENOMSG)
		alert_printf(1, AGENALRT, get_message(error));
}


/* 
 * Display an alert box identified by "message" (alert-box id.)
 * and additional error-message text idenified by "error" code 
 */

void hndl_error(_WORD message, _WORD error)
{
	if (error > ENOSYS)
		return;

	alert_printf(1, message, get_message(error));
}


/* 
 * display an alert box (" ! " icon) for some file-related errors;
 * earlier, "msg" identified the alert-box form,
 * now it idenifies the first message text in AGFALERT alert box.
 * The alert box first displays text "msg", then "file", then
 * the text associated to error code "error".
 */

_WORD xhndl_error(_WORD msg, _WORD error, const char *file)
{
	_WORD button = 0, txtid = 0;
	char shnam[30];

	cramped_name(file, shnam, sizeof(shnam));

	if (error >= XFATAL && error <= XERROR)
		return error;
	if ((error < 0) && (error >= -17))
		return XFATAL;
	/* Display the appropriate alert-box */

	if (error == ENOENT || error == EACCES || error == ELOCKED || error == EFNTL || error == ENAMETOOLONG)
		/* For: File not found, Access denied, File locked, Name too long: */
		txtid = TSKIPABT;
	else if (error != ENOMSG)
		/* For all the rest... */
		txtid = TABORT;

	if (txtid)
		button = alert_printf
			(1, AGFALERT, get_freestring(msg), shnam, get_message(error), get_freestring(txtid));

	return txtid == TSKIPABT && button == 1 ? XERROR : XABORT;
}
