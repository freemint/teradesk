/* 
 * Teradesk. Copyright (c)  1993 - 2002  W. Klaren,
 *                          2002 - 2003  H. Robbers,
 *                          2003 - 2013  Dj. Vukovic
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
#include "error.h"
#include <mint/cookie.h>
#include <mint/falcon.h>

#include "xfilesys.h"
#include "resource.h"
#include "desk.h"
#include "lists.h"
#include "slider.h"
#include "library.h"
#include "font.h"
#include "config.h"
#include "window.h"
#include "icon.h"
#include "events.h"
#include "screen.h"
#include "video.h"
#include "startprg.h"

#ifndef C_Lace
#define C_Lace 0x4C616365L     /* LaceScan */
#endif

#if _OVSCAN
static short oldstat = -1;				/* previous state of OVERscan */
short ovrstat = -1;						/* state of overscan          */
long over;								/* identification of overscan type */
#endif
static short vprefsold;					/* previous state of voptions */
static short vdohi;						/* upper word of vdo cookie value */
static short bltstat = 0;				/* presence of blitter        */
static short falmode = 0;				/* falcon video mode          */
static short fmtype;					/* falcon monitor type: 0=STmon 1=STcol 2=VGA 3=TV */
static short currez;					/* current res: 0=ST-low 1=ST-med 2=ST-Hi 4=TT-low 6=TT-hi 7=TT-med */
static long vdo;						/* id. of video hardware- shifter type- see below */
static bool st_ste = FALSE;				/* ST or STe */
static bool fal_mil = FALSE;			/* Falcon or Milan */

/* These are the shifter types returned from the _VDO cookie */

#define ST_VIDEO 	0x0000				/* ST, MegaST */
#define STE_VIDEO	0x0001				/* STe, megaSTe */
#define TT_VIDEO	0x0002				/* TT */
#define FAL_VIDEO	0x0003				/* Falcon */
#define MIL_VIDEO	0x0004				/* Milan */
#define ARA_VIDEO	0x0005				/* Aranym ? */
#define OTH_VIDEO	0x0006				/* Other; unknown */


/* 
 * These are the bitflags of the falcon video mode.
 * Note: there may already be other definitions in header files 
 * for these flags 
 */

#define VM_INQUIRE -1
#define VM_NPLANES 0x0007
#define VM_80COL   0x0008
#define VM_VGAMODE 0x0010
#define VM_PALMODE 0x0020
#define VM_OVSCAN  0x0040
#define VM_STMODE  0x0080
#define VM_DBLINE  0x0100

/* These are the values for falcon monitor tyoe */

#define MONO_MON	0
#define STCOL_MON	1
#define VGA_MON		2
#define TV_MON		3

/* These are the standard resolution codes as returned by Getrez() */

#define ST_LOWRES	0
#define ST_MEDRES	1
#define ST_HIGHRES	2
#define TT_LOWRES	7
#define TT_MEDRES	4
#define TT_HIGHRES	6

/* Which resolution code is selected by which radiobutton */

static const char rmap[10] = { VSTLOW, VSTMED, VSTHIGH, 0, VTTMED, 0, VTTHIGH, VTTLOW, 0, 0 };


/* 
 *  Routine get_set_video acquires data on current state of bliter (if any)
 *  and of current resolution, or sets same data
 *  parameter: 0=get, 1=set, 2=set & change rez
 */

void get_set_video(_WORD set)
{
#if _OVSCAN
	void *s;							/* superv.stack pointer */
	void *logb;							/* logical screen base  */
	void *phyb;							/* physical screen base */
	char *acia;

	static _WORD ov_max_h;
	static _WORD ov_max_w;
	static _WORD idi;
	static _WORD menu_h;

	static const _WORD std_x[4] = { 320, 640, 1280, 0 };
	static const _WORD std_y[4] = { 200, 400, 800, 0 };
#endif
	WINDOW *w;

	/* Where is the screen ? */

#if _OVSCAN
	logb = Logbase();
	phyb = Physbase();
#endif

	if (set == 0)						/* get data */
	{
		/* What is the size of the screen ? (call vq_extnd() etc.) */

		xd_screensize();

		/* Which is the current standard resolution ? */

		currez = Getrez();

#if 0									/* not used, at least for the time being */
		options.vrez = currez;
#endif

		/* Find about video hardware (shifter; will be 0xffffffff without cookie */

		find_cookie(C__VDO, &vdo);

		vdohi = (_WORD) (vdo >> 16);	/* use only the upper word */

		/* 
		 * Note: Currently Falcon and Milan hardware are identically 
		 * treated (should they be?). Same for Aranym.
		 * If this is a Falcon or Milan, in which mode it is ? 
		 * currez obtained above will be meaningless, it will have
		 * to be constructed from the mode setting and monitor type.
		 * It will be used only to set/read radio buttons but ignored
		 * in actual resolution change
		 */

		if (vdohi == FAL_VIDEO || vdohi == MIL_VIDEO || vdohi == ARA_VIDEO)
		{
			fal_mil = TRUE;
			falmode = VsetMode(-1);
			fmtype = VgetMonitor();
			bltstat = 0;				/* No ST blitter */

			if ((falmode & VM_STMODE) == 0)
			{
				/* ST-compatibility mode OFF */

				if ((falmode & VM_80COL) == 0)
					currez = TT_LOWRES;
				else
					currez = TT_MEDRES;
			} else
			{
				/* ST-compatibility mode ON */

				if ((falmode & VM_80COL) == 0)
				{
					currez = ST_LOWRES;
				} else
				{
					if (xd_nplanes == 1)
						currez = ST_HIGHRES;
					else
						currez = ST_MEDRES;
				}
			}
		} else if (vdohi == ST_VIDEO || vdohi == STE_VIDEO)
		{
			st_ste = TRUE;

			/* 
			 * Try to find out about a couple of overscan types 
			 * This can become active only on a ST-type machine
			 * (should this also be on a TT ?)
			 */

#if _OVSCAN
			if (find_cookie(C_OVER, &over) || find_cookie(C_Lace, &over))
			{
				/* There is ST-overscan/lacescan. Set initial values */

				if (ovrstat < 0)
				{
					ov_max_w = xd_screen.g_w;
					ov_max_h = xd_screen.g_h;
					menu_h = xd_screen.g_h - xd_desk.g_h;
				}

				/* Detect nonstandard overscanned resolution */

				ovrstat = 0;
				for (idi = 0; idi < 3; idi++)
				{
					if (xd_screen.g_h > std_y[idi] && xd_screen.g_h < std_y[idi + 1])
					{
						ov_max_w = xd_screen.g_w;
						ov_max_h = xd_screen.g_h;
						menu_h = xd_screen.g_h - xd_desk.g_h;
						ovrstat = 1;
						break;
					}
				}

				if (ovrstat > 0)
					options.vprefs |= VO_OVSCAN;
				else
					options.vprefs &= ~VO_OVSCAN;
			}
#endif
			/* Is there a blitter in this machine ? */

			if (tos_version >= 0x104)
				bltstat = Blitmode(-1);	/* Function known only to tos >= 1.4 ? */
			else
				bltstat = 0;

			/* Get current blitter state; insert into options  */

			if (bltstat & 0x0001)
				options.vprefs |= VO_BLITTER;
			else
				options.vprefs &= ~VO_BLITTER;
		}
	} else								/* set data (set=1 or set=2) */
	{
		if (st_ste)
		{
			/* Set blitter, if present */

			if (bltstat & 0x0002)		/* Get current blitter state; insert into options  */
			{
				if (options.vprefs & VO_BLITTER)
					bltstat |= 0x0001;
				else
					bltstat &= ~0x0001;

				bltstat = Blitmode(bltstat);
			}
#if _OVSCAN
			/* 
			 * Set overscan (Lacescan, in fact)
			 * that which is below is ok but not enough !!!!
			 */

			if ((over != -1L) && ((vprefsold ^ options.vprefs) & VO_OVSCAN))
			{
				oldstat = ovrstat;

				/* 
				 * The following segment is executed in supervisor mode.
				 * Note: perhaps use Ssystem() here when appropriate? 
				 */

				s = (void *) Super(0L);
				acia = (char *) 0xFFFC00L;	/* address of the acia chip reg  HR 240203 (long) */

				if (options.vprefs & VO_OVSCAN)
				{
					*acia = 0xD6;		/* value for the acia reg- switch overscan ON */
					ovrstat = 1;
					xd_screen.g_h = ov_max_h;
					xd_screen.g_w = ov_max_w;
				} else
				{
					*acia = 0x96;		/* value for the acia reg- switch overscan OFF */
					ovrstat = 0;
					xd_screen.g_w = std_x[idi];
					xd_screen.g_h = std_y[idi];
				}

				/* 
				 * An attempt to change resolution (to the same one) will 
				 * provoke Lacescan to adapt 
				 */

				xd_desk.g_w = xd_screen.g_w;
				xd_desk.g_h = xd_screen.g_h - menu_h;
				xw_deskwin->xw_size.g_w = xd_screen.g_w;
				xw_deskwin->xw_size.g_h = xd_screen.g_h;

				/* Note: return from supervisor before Setscreen()... */

				Super((void *) s);
				Setscreen(logb, phyb, currez);
			}
#endif
		}


		/* fal_mil ? */
		/* 
		 * Exit with "OK" from the Video options dialog will always cause
		 * the desktop to be completely regenerated. 
		 * Can be convenient to recover from screen corruption
		 */
		if (set == 1)
		{
			/* Calculate window sizes to fit the screen */

			wd_sizes();
			w = xw_first();

			while (w)
			{
				if (wd_adapt(w))
				{
					wd_forcesize(w, &(w->xw_size), TRUE);
					set_sliders((TYP_WINDOW *) w);
					wd_type_draw((TYP_WINDOW *) w, FALSE);	/* TeraDesk draws */
				}

				w = xw_next(w);
			}

			menu_bar(menu, 0);
			regen_desktop(desktop);
			clean_up();
			arrow_mouse();
		} else							/* Set > 1 */
		{
			/* Change resolution */
			_WORD ignor;
			_WORD wisgr, wiscr;

			if (fal_mil)
			{
				wisgr = falmode;
				wiscr = 1;
			} else
			{
				wisgr = currez + 2;
				wiscr = 0;
			}

			/* 
			 * This will actually (almost) reset the computer immediately
			 * and change the video mode, but this call is recognized only 
			 * in some versions of AES 4. Otherwise it will be ignored. 
			 * Unfortunately, e.g. TOS 2.06 will return OK in this case,
			 * but e.g. TOS 4.04 will not, as is proper.
			 */

			if (aes_version >= 0x340)
				shel_write(SHW_RESCHNG, wisgr, wiscr, &ignor, NULL);

			/* If still alive, wait a bit for termination message */

			if (wait_to_quit())
				return;

			/* 
			 * If shel_write() did not succeed, attempt to
			 * change resolution directly. Unfortunately, not all AESes
			 * return error if shel_write(5,...) above fails...
			 */

#if 0									/* this does not work for the time being */
			if (iret == 0)
			{
				/* Besides, HOW to do this ? */
			}
#endif

		}
	}
}


/*
 * Aux. size-optimization routine
 */

static void vd_setstring(_WORD to, _WORD from)
{
	xd_get_obspecp(&vidoptions[to])->free_string = get_freestring(from);
}


/*
 * Aux. size-optimization routine
 * parameter toob: index of the last button to be enabled.
 * Note: buttons must be in sequence: VSTLOW, VSTMED, VSTHIGH, VTTLOW, VTTMED, VTTHIGH
 */

static void vd_enable_rez(_WORD toob)
{
	_WORD i;

	for (i = VSTLOW; i <= toob; i++)
		obj_enable(vidoptions[i]);
}


/* 
 * Routine voptions() handles video options dialog.
 * Dialog appearance may be different, depending on detected (video) hardware
 * i.e. there are some Falcon-specific options 
 */

short voptions(void)
{
	short rcode = 0;					/* return code of this routine */
	_WORD button;						/* selected button */
	_WORD newrez;						/* desired resolution   */
	_WORD newmode;						/* desired video mode (Falcon) */
	_WORD rimap[16];					/* inverse to rmap */

	/* dimensioning will be problematic if object */
	/* indices are large - check in desktop.rsc   */

	static const char npc[] = { 0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4 };	/* = f(np) */
	const char *ap;						/* to display # of colours */
	char *s = vidoptions[VNCOL].ob_spec.free_string;	/* same */
	long ncc;

	_WORD npmax;						/* max settable number of colour planes */
	_WORD npmin;						/* min settable number of colour planes */
	_WORD npp;							/* previous number of colour planes */
	_WORD np = xd_nplanes;				/* number of colour planes: 1 to 16 */
	bool editcol = FALSE;				/* permit editting number of colours */
	bool qquit = FALSE;
	XDINFO info;						/* video options dialog */

	newmode = falmode;

	/* which resolution code is selected by which button */

	rimap[0] = -1;
	rimap[VSTLOW] = ST_LOWRES;
	rimap[VSTMED] = ST_MEDRES;
	rimap[VSTHIGH] = ST_HIGHRES;
	rimap[VTTLOW] = TT_LOWRES;
	rimap[VTTMED] = TT_MEDRES;
	rimap[VTTHIGH] = TT_HIGHRES;

	/* Find current video configuration */

	get_set_video(0);

	/* Set radiobutton for current resolution selected */

	xd_set_rbutton(vidoptions, VREZOL, (_WORD) rmap[currez]);

	/* Set some hardware-specific dialog elements */

	switch (vdohi)
	{
	case ST_VIDEO:
#if _OVSCAN
		/* Overscan */

		if (over != -1L)
		{
			obj_enable(vidoptions[VOVERSCN]);
			set_opt(vidoptions, options.vprefs, VO_OVSCAN, VOVERSCN);
		}
#endif
		/* fall through */
	case STE_VIDEO:
		if (currez == ST_HIGHRES)	/* st-high? disable low and med res */
			obj_enable(vidoptions[VSTHIGH]);
		else						/* st-low/med? disable hi res */
			vd_enable_rez(VSTMED);	/* ST-low to ST-med */

		/* Set button for blitter. Practically ST and STE only */

		if (bltstat & 0x0002)		/* blitter is present */
		{
			obj_enable(vidoptions[VBLITTER]);
			set_opt(vidoptions, options.vprefs, VO_BLITTER, VBLITTER);
		}
		break;
	case TT_VIDEO:
		if (currez == TT_HIGHRES)	/* tt-high? disable low and med res */
			obj_enable(vidoptions[VTTHIGH]);
		else						/* tt-low/med? disable hi res */
			vd_enable_rez(VTTMED);	/* ST-Low to TT-Med */
		break;
	case FAL_VIDEO:
	case MIL_VIDEO:
	case ARA_VIDEO:
		vd_setstring(VTTLOW, TFALLOW);
		vd_setstring(VTTMED, TFALMED);
		vd_setstring(VBLITTER, TINTERL);

		if (fmtype != MONO_MON)
		{
			editcol = TRUE;
			vd_enable_rez(VTTMED);	/* ST-Low to TT-Med */
			obj_disable(vidoptions[VSTHIGH]);	/* except ST-High */

			obj_enable(vidoptions[VBLITTER]);	/* double line/interlace */
			if (fmtype != VGA_MON)
				obj_enable(vidoptions[VOVERSCN]);
		}

		if (fmtype == MONO_MON || fmtype == VGA_MON)
			obj_enable(vidoptions[VSTHIGH]);

		/* Display state of overscan and double line/interlace */

		set_opt(vidoptions, newmode, VM_OVSCAN, VOVERSCN);
		set_opt(vidoptions, newmode, VM_DBLINE, VBLITTER);

		break;
	default:
		/* For the time being, just enable all */

		vd_enable_rez(VTTHIGH);
		break;
	}

	/* Set button for saving the palette */

	set_opt(vidoptions, options.vprefs, SAVE_COLOURS, SVCOLORS);

	/* Open the dialog... */

	vprefsold = options.vprefs;

	if (chk_xd_open(vidoptions, &info) >= 0)
	{
		/* Loop until OK or Cancel */

		do
		{
			/* Redraw display of the current number of colours */

			npmin = 1;
			npmax = 16;
			ap = empty;
			ncc = 0x00000001L << np;
			npp = np;

			if (ncc > 1024)
			{
				ncc /= 1024;
				ap = "K";
			}

			ltoa(ncc, s, 10);
			strcat(s, ap);

			xd_drawthis(&info, VNCOL);

			button = xd_form_do(&info, ROOT);

			/* Which standard mode is currently selected */

			newrez = rimap[xd_get_rbutton(vidoptions, VREZOL)];

			/* There are some mode dependencies on a Falcon... */

			if (fal_mil)
			{
				newmode &= ~(VM_STMODE | VM_80COL | VM_NPLANES);
				newmode |= npc[np];

				if (newrez <= ST_HIGHRES)
					newmode |= VM_STMODE;

				if (newrez != ST_LOWRES && newrez != TT_LOWRES)
					newmode |= VM_80COL;

				switch (newrez)
				{
				case ST_LOWRES:
					npmin = 4;
					npmax = 4;
					break;
				case ST_MEDRES:
					npmin = 2;
					npmax = 2;
					break;
				case ST_HIGHRES:
					npmin = 1;
					npmax = 1;
					break;
				default:
					if (fmtype == VGA_MON)
					{
						if ((newmode & VM_80COL) != 0)
							npmax = 8;
						else
							npmin = 2;
					}
					break;
				}

				np = minmax(npmin, np, npmax);
				if (np != npp)
				{
					bell();
					goto next;			/* redraw button then loop again */
				}

				get_opt(vidoptions, &newmode, VM_DBLINE, VBLITTER);
				get_opt(vidoptions, &newmode, VM_OVSCAN, VOVERSCN);
			} else if (st_ste)
			{
				/* Set blitter, (couldn't have been selected if not present) */

				get_opt(vidoptions, &options.vprefs, VO_BLITTER, VBLITTER);
#if _OVSCAN
				/* Set overscan option (could not have been selected if not present) */

				get_opt(vidoptions, &options.vprefs, VO_OVSCAN, VOVERSCN);
#endif
			}

			switch (button)
			{
			case VNCOLUP:
				if (np < npmax)
					np <<= 1;
				break;
			case VNCOLDN:
				if (np > npmin)
					np >>= 1;
				break;
			case VIDOK:
				qquit = TRUE;

				/* Set save palette flag */

				get_opt(vidoptions, &options.vprefs, SAVE_COLOURS, SVCOLORS);

				/* Will resolution be changed? Display an alert */

				if ((newrez != currez && newrez != -1) || (newmode != falmode))
				{
					if (alert_printf(1, ARESCH) == 1)
					{
						currez = newrez;	/* new becomes old */
						falmode = newmode;	/* same */
						rcode = 1;	/* to initiate resolution change */
					}
				}

				break;
			case VIDCANC:
				options.vprefs = vprefsold;
				qquit = TRUE;
				break;
			default:
				break;
			}

		  next:;

			xd_drawbuttnorm(&info, button);

			if (!editcol)
				np = npp;
		} while (!qquit);

		xd_close(&info);

		if (button == VIDOK)
		{
			get_set_video(1);
		}
	}

	return rcode;
}
