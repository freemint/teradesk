/*
 * Xdialog Library. Copyright (c) 1993 - 2002  W. Klaren,
 *                                2002 - 2003  H. Robbers,
 *                                2003 - 2009  Dj. Vukovic
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
 #include <tos.h>
 #include <vdi.h>
#else
 #include <aesbind.h>
 #include <osbind.h>
 #include <vdibind.h>
#endif

#include <ctype.h>
#include <stddef.h>
#include <library.h>
#include <vaproto.h>

#include "xdialog.h"


int 
	xe_mbshift,
	xd_kstate; 	/* kbd state while clicking a button; */

/* 
 * Funktie voor het converteren van een VDI scancode  naar een
 * eigen scancode. 
 */

int xe_keycode(int scancode, int kstate)
{
	static const char 
		num[] = {0,27,49,50,51,52,53,54,55,56,57,48,45,61,8},
		numk[] = {55,56,57,52,53,54,49,50,51,48};

	int 
		keycode, 
		nkstate, 
		scan,
		ctrl;

	/* Zet key state om in eigen formaat */

	nkstate = (kstate & (K_RSHIFT | K_LSHIFT)) ? XD_SHIFT : 0;
	nkstate = nkstate | ((kstate & 0xC) << 7);
	ctrl = nkstate & (XD_CTRL | XD_ALT);

	/* Bepaal scancode */

	scan = ((unsigned int)scancode & 0xFF00) >> 8;

	/* Controleer of de scancode hoort bij een ASCII teken */

	if(ctrl && scan < 15)
		keycode = num[scan];
	else if(ctrl && scan > 102 && scan < 113)
		keycode = numk[scan - 103];
	else if (ctrl && scan == 74)
		keycode = 45;
	else if (ctrl && scan == 78)
		keycode = 43;
	else if ((scan < 59) || (scan == 74) || (scan == 78) || (scan == 83) ||
		(scan == 96) || ((scan >= 99) && (scan <= 114)) || (scan >= 117))
	{
		if (scan >= 120)
			scan -= 118;

		if ((keycode = scancode & 0xFF) == 0)
		{
#ifdef __PUREC__
			keycode = touppc(((unsigned char)(Keytbl((void *) -1, (void *) -1, (void *) -1)->unshift[scan])));
#else
			keycode = touppc((int)((unsigned char)(((char *) ((_KEYTAB *) Keytbl((void *) -1, (void *) -1, (void *) -1))->unshift)[scan])));
#endif
		}

	}
	else
	{
		nkstate |= XD_SCANCODE;
		keycode = scan;
	}

	keycode |= nkstate;

	return keycode;
}


/*
 * Should dialog activity be routed to a dialog or to a nonmodal dialog?
 * This is better than just checking whether a dialog is opened.
 */
 
int xd_isdopen(void)
{
	if
	(
		xd_dialogs && 
		!(
			xd_dialogs->dialmode == XD_WINDOW && 
			xd_nmdialogs && 
			xw_top() == xd_nmdialogs->window
		)
	)
		return 1;	/* normal dialog open */

	return 0;		/* not open or nonmodal on top */
}


/* 
 * Vervanging van evnt_multi, die eigen keycode terug levert. 
 */

int xe_xmulti(XDEVENT *events)
{
	static int
		level = 0;

	int
		r,
		old_mtlocount,
		old_mflags;


	level++;

	old_mtlocount = events->ev_mtlocount;
	old_mflags = events->ev_mflags;

	/* 
	 * Check if the time out time is shorter than the minimum time.
	 * If true set to the minimum time. 
	 */

	if 
	(
		((events->ev_mflags & MU_TIMER) != 0) && 
		(events->ev_mthicount == 0) && 
		(events->ev_mtlocount < xd_min_timer)
	)
		events->ev_mtlocount = xd_min_timer;

	/* 
	 * No message events to be received when a dialog is opened 
	 * and this dialog is not in a window. 
	 */

	if (xd_dialogs && (xd_dialogs->dialmode != XD_WINDOW) && !xd_nmdialogs)
		events->ev_mflags &= ~MU_MESAG;

	/* Wait for an event */

	events->xd_keycode = 0; /* why ??? */

#ifdef __PUREC__

	EvntMulti((EVENT *)events);

#else
	events->ev_mwhich = evnt_multi
	(
		events->ev_mflags,
		events->ev_mbclicks,
		events->ev_mbmask,
		events->ev_mbstate,
		events->ev_mm1flags,
		events->ev_mm1.g_x,
		events->ev_mm1.g_y,
		events->ev_mm1.g_w,
		events->ev_mm1.g_h,
		events->ev_mm2flags,
		events->ev_mm2.g_x,
		events->ev_mm2.g_y,
		events->ev_mm2.g_w,
		events->ev_mm2.g_h,
		events->ev_mmgpbuf,
		(((unsigned long)events->ev_mthicount) << 16) | (unsigned long)events->ev_mtlocount,
		&events->ev_mmox,
		&events->ev_mmoy,
		&events->ev_mmobutton,
		&events->ev_mmokstate,
		&events->ev_mkreturn,
		&events->ev_mbreturn
	);
#endif
	xe_mbshift = events->ev_mmokstate;

	/* AV_SENDKEY message is transformed into a keyboard event */

	if (((r = events->ev_mwhich) & MU_MESAG) && (events->ev_mmgpbuf[0] == AV_SENDKEY))
	{
		events->ev_mkreturn = events->ev_mmgpbuf[4];
		events->ev_mmokstate = events->ev_mmgpbuf[3];
		r &= ~MU_MESAG;
		r |= MU_KEYBD;
	}

	/* If this is a keyboard event... */

	if (r & MU_KEYBD)
	{
		events->xd_keycode = xe_keycode(events->ev_mkreturn, events->ev_mmokstate);

		if (events->ev_mmgpbuf[0] == AV_SENDKEY)
			events->xd_keycode |= XD_SENDKEY;

		if(!xd_isdopen() && (level == 1))
		{
			if (xw_hndlkey(events->xd_keycode, events->ev_mmokstate))
				r &= ~MU_KEYBD;
		}
	}

	/* If this is a message event... */

	if (r & MU_MESAG)
	{
		if ((events->ev_mmgpbuf[0] == MN_SELECTED) && xd_dialogs)
		{ 
/* no need
			if (xd_menu)
*/
				menu_tnormal(xd_menu, events->ev_mmgpbuf[3], 1);
			r &= ~MU_MESAG;
		}
		else if (xw_hndlmessage(events->ev_mmgpbuf)) /* window-handling messages processed */
		{
			r &= ~MU_MESAG;

		}
	}

	/* If this is a button event... */

	if ((r & MU_BUTTON) && !xd_isdopen() && (level == 1))
	{
		int mmobutton = events->ev_mmobutton;

		if (xd_rbdclick && mmobutton == 2)
			events->ev_mbreturn = 2;	/* right button is double click */

		/* complete handling of the button press(es) happens within this call */

		if (xw_hndlbutton(events->ev_mmox, events->ev_mmoy,
						  events->ev_mbreturn, mmobutton,
						  events->ev_mmokstate))
			r &= ~MU_BUTTON;

		/* 
		 * This should fix the problem that occured when right mouse
		 * button was pressed for a long time - multiple VA_START
		 * messages were sent to the right-button-extension application.
		 * Teradesk now waits until the button is released 
		 * before proceeding further
		 */

		if(mmobutton == 2)
			while(xe_button_state());
	}

	events->ev_mflags = old_mflags;
	events->ev_mtlocount = old_mtlocount;
	events->ev_mwhich = r;

	level--;

	return r;
}


/*
 * Bepaal de huidige toestand van de muis buttons.
 */

int xe_button_state(void)
{
	int
		dummy,
		mstate;


	graf_mkstate(&dummy, &dummy, &mstate, &dummy);

	return mstate;
}


/*
 * Funktie voor het wachten op een bepaald muis event.
 *
 * Parameters:
 *
 * mstate	- toestand muisknoppen waarop het muis event moet plaatsvinden,
 * x		- huidige x coordinaat muis,
 * y		- huidige y coordinaat muis,
 * kstate	- toestand SHIFT, CONTROL en ALTERNATE toetsen.
 *
 * Resultaat : TRUE als het event heeft plaatsgevonden, FALSE als het
 *			   event niet is opgetreden.
 */

int xe_mouse_event(int mstate, int *x, int *y, int *kstate)
{
	XDEVENT
		events;

	int
		flags;


	xd_clrevents(&events);
	events.ev_mflags = MU_TIMER | MU_BUTTON;
	events.ev_mbclicks = 2;
	events.ev_mbmask = 1;
	events.ev_mbstate = mstate;

	flags = xe_xmulti(&events);

	*x = events.ev_mmox;
	*y = events.ev_mmoy;
	*kstate = events.ev_mmokstate;

	return (flags & MU_BUTTON) ? TRUE : FALSE;
}

