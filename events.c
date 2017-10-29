/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2013  Dj. Vukovic
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, MA  02111-1307  USA
 */


#include <library.h>
#include <xdialog.h>
#include <xscncode.h>

#include "desktop.h"
#include "desk.h"
#include "events.h"
#include "error.h" 


static _WORD event(_WORD evflags, _WORD mstate, _WORD *key)
{
	XDEVENT
		events;

	_WORD
		result;


	xd_clrevents(&events);

	events.ev_mflags = MU_TIMER | evflags;
	events.ev_mbclicks = 2;
	events.ev_mbmask = 1;
	events.ev_mbstate = mstate;

	do
	{
		if ((result = xe_xmulti(&events)) & MU_MESAG)
		{
			/* 
			 * Message received; handle AV-protocol and FONT protocol messages, 
			 * AP_TERM and SH_WDRAW. If AP_TERM is received, -1 will be returned 
			 * Some day, drag & drop receive messages may be handled here
			 */
			if (hndlmessage(events.ev_mmgpbuf) != 0)
				return -1;
		}
	} 
	while (!(result == MU_TIMER) && !(result & ~(MU_MESAG | MU_TIMER)));

	*key = events.xd_keycode;

	return result;
}


/*
 * Check if there is a key pressed.
 *
 * Parameters:
 *
 * key		- pressed key.
 * hndl_msg	- handle message events
 *
 * Result	: 0 if there is no key pressed
 *			  1 if there is a key pressed
 *			 -1 if a message is received which terminates the program
 */

_WORD key_state(_WORD *key, bool hndl_msg)
{
	_WORD result;

	result = event( (hndl_msg ? (MU_KEYBD | MU_MESAG) : MU_KEYBD), 0, key);

	if (result == -1)
	{
		/* only if hndl_msg == TRUE can -1 be returned from event() */
		return -1;
	}
	else if (result & MU_KEYBD)
		return 1;
	else
		return 0;
}


/*
 * Wait dt milliseconds
 */

void wait(_WORD dt)
{
	evnt_timer(dt);
}


/*
 * This is a routine for confirmation of an abort caused by pressing [ESC] 
 * during multiple copy, delete, print, etc.
 * An alert is posted with a text "Abort current operation?"
 * If hndl_msg = TRUE, this routine will process AV_PROTOCOL 
 * messages and also AP_TERM and SH_WDRAW.
 * If a message is recived which terminates the desktop (i.e. AP_TERM)
 * then TRUE will be returned, as if OK was selected. 
 * Beware that using this routine in some loop may slow down time-critical
 * operations noticeably
 */

bool escape_abort( bool hndl_msg )
{
	_WORD
		key,
		r;


	if ((r = key_state(&key, hndl_msg)) > 0)
	{
		if (key == ESCAPE)
			if ( alert_printf(2, AABOOP) == 1 )
				return TRUE;
	}
	else if (r < 0)
		return TRUE;
	
	return FALSE;
} 

