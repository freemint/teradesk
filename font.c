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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */


#include <library.h>
#include <xdialog.h>

#include "resource.h"
#include "desk.h"
#include "xfilesys.h"
#include "lists.h"
#include "slider.h"
#include "error.h"
#include "font.h"
#include "screen.h"
#include "config.h"
#include "window.h"
#include "va.h"
#include "icon.h"
#include "events.h"
#include "open.h"


#define MFSIZES 100						/* Maximum number of available font sizes */

typedef char FNAME[34];					/* must not be shorter than 34 bytes */

typedef struct
{
	_WORD id;							/* font identification number */
	FNAME name;							/* font name and description (32 characters) */
#if 0									/* currently unused */
	_WORD flag;							/* flag that this is a vector font */
#endif
} FONTDATA;

typedef struct
{
	FONTDATA *fd;						/* Fonts information. */
	_WORD nf;							/* Number of fonts. */
	_WORD fdfont;						/* Current font index in fd */
	XDFONT font;						/* Font id., size and char dimensions */
	_WORD cursize;						/* Index of current size in array 'fsizes'. */
	_WORD fsizes[MFSIZES];				/* Array with sizes of the current font. */
	_WORD nfsizes;						/* Number of sizes in 'fsizes'. */
} FONTBLOCK;

static FONTBLOCK static_fbl;

static bool wdfopen = FALSE,
	nonmodal = FALSE;

static _WORD
#if _MINT_
 fcwhandle,
 fcwap_id,
#endif
 flblen = 0;


void fnt_setfont(_WORD font, _WORD height, XDFONT *data)
{
	data->id = vst_font(vdi_handle, font);
	data->size = xd_fnt_point(height, &data->cw, &data->ch);
}


/*
 * Draw sample text in Teradesk's font selector, using selected font.
 * This is the code for the userdef 'sample text' object
 */

static _WORD cdecl draw_text(PARMBLK *pb)
{
	FONTBLOCK *thefbl = (FONTBLOCK *) (((XUSERBLK *) (pb->pb_parm))->uv.ptr);
	char *thetext = ((XUSERBLK *) (pb->pb_parm))->ob_spec.free_string;
	_WORD extent[8];
	RECT r, tr;

	r.x = pb->pb_x;
	r.y = pb->pb_y;
	r.w = pb->pb_w;
	r.h = pb->pb_h;

	xd_clip_on(&r);						/* clipping does not seem to work with pb->c ? */

	if (nonmodal)
		clr_object(&r, G_WHITE, -1);	/* to make white background */
	else
		pclear(&r);						/* to show window colour and pattern */

	/* note: do not create a direct pointer to thefbl->font, it would be longer */

	thefbl->font.id = thefbl->fd[thefbl->fdfont].id;

	set_txt_default(&(thefbl->font));
	xd_fnt_point(thefbl->font.size, &(thefbl->font.cw), &(thefbl->font.ch));

	vqt_extent(vdi_handle, thetext, extent);

	tr.w = extent[2] - extent[0];
	tr.h = extent[7] - extent[1] + 4;

	tr.x = r.x + (r.w - tr.w) / 2;
	tr.y = r.y + (r.h - tr.h) / 2;

	w_transptext(tr.x, tr.y + 2, thetext);

	if (pb->pb_currstate & OS_SELECTED)
		invert(&tr);

	xd_clip_off();

	return 0;
}


#if _FONT_SEL

typedef struct
{
	SLIDER slider;
	struct fnt_dialog *fnt_dial;
} FNT_SLIDER;


typedef struct
{
	USERBLK userblock;
	struct fnt_dialog *fnt_dial;
	char *text;
} FNT_USERBLK;


typedef struct fnt_dialog
{
	XDINFO dialog;
	FNT_SLIDER sl_info;
	_WORD ap_id;						/* Application id of caller */
	_WORD win;							/* Window id of caller */
	_WORD colour;						/* Color of font */
	_WORD effect;						/* Text effects of font */
	FONTBLOCK fbl;
} FNT_DIALOG;

#endif


static _WORD fnt_find_selected(void)
{
	_WORD object;

	return ((object = xd_get_rbutton(wdfont, WDPARENT)) < 0) ? 0 : object - WDFONT1;
}


/*
 * The really working part of the routines that set 
 * the listbox in fontselector(s)
 */

static void set_theselector(SLIDER *slider, bool draw, XDINFO *info, _WORD thefont, FONTDATA *fd, _WORD nnf)
{
	_WORD i, isl;
	OBJECT *o = &wdfont[WDFONT1];

	for (i = 0; i < NLINES; i++)
	{
		o->ob_state &= ~OS_SELECTED;
		isl = i + slider->line;

		if (isl >= nnf)
			*o->ob_spec.tedinfo->te_ptext = 0;
		else
		{
			o->ob_spec.tedinfo->te_ptext = fd[isl].name;

			if (isl == thefont)
				o->ob_state |= OS_SELECTED;
		}

		o++;
	}

	if (draw)
		xd_drawdeep(info, WDPARENT);
}


/*
 * Set font selector listbox for TeraDesk
 */

static void set_fselector(SLIDER *slider, bool draw, XDINFO *info)
{
	set_theselector(slider, draw, info, static_fbl.fdfont, static_fbl.fd, static_fbl.nf);
}


#if _FONT_SEL

/*
 * Set font selector listbox for the systemwide nonmodal font-selector dialog
 */

static void mset_fselector(SLIDER *slider, bool draw, XDINFO *info)
{
	FNT_DIALOG *fnt_dial = ((FNT_SLIDER *) slider)->fnt_dial;

	set_theselector(slider, draw, info, fnt_dial->fbl.fdfont, fnt_dial->fbl.fd, fnt_dial->fbl.nf);
}
#endif


/*
 * Note: fd is locally modified here
 */

static _WORD set_font(SLIDER *sl_info, _WORD oldfont, _WORD *font, FONTDATA *fd, _WORD nf)
{
	_WORD i, j = 0, line;

	/* Find the font; if not found, take the first one */

	for (i = 0; i < nf; i++)
	{
		if (fd->id == oldfont)
		{
			j = i;
			break;
		}

		fd++;
	}

	*font = line = j;

	/* Note: sl->info->lines is number of visible lines in the selector */

	if (line > (nf - sl_info->lines))
	{
		if ((line = nf - sl_info->lines) < 0)
			line = 0;
	}

	return line;
}


/*
 * Get available font size(s). Return index of the nearest suitable size,
 * and put that font size into dialog field. 
 */

static _WORD get_size(FONTBLOCK *fbl)
{
	_WORD n = 1, height = 99, dummy, i, error, tmp, minsize = 0;

	vst_font(vdi_handle, fbl->fd[fbl->fdfont].id);

	height = fbl->fsizes[0] = xd_vst_point(height, &dummy);

	while (n < MFSIZES - 1 && fbl->fsizes[n - 1] != (height = xd_vst_point(height - 1, &dummy)))
		fbl->fsizes[n++] = height;

	error = abs(fbl->fsizes[0] - fbl->font.size);

	for (i = 1; i < n; i++)
	{
		if ((tmp = abs(fbl->fsizes[i] - fbl->font.size)) < error)
		{
			error = tmp;
			minsize = i;
		}
	}

	itoa(fbl->fsizes[minsize], wdfont[WDFSIZE].ob_spec.free_string, 10);
	fbl->nfsizes = n;

	return minsize;
}


/*
 * Count available fonts.
 * Font name can be at most 32 characters long
 * (16 characters name + 16 characters description)
 */

static void font_count(FONTBLOCK *fbl,	/* pointer to structure with fonts data */
					   bool prop		/* true if proportional fonts are counted */
	)
{
	FNAME name;							/* intermediate buffer for font name and description */
	_WORD i, iw, mw, dummy, lnf = 0;
	XDFONT fnt;
	FONTDATA *h;

	/* Determine available form length only the first time */

	if (flblen == 0)
		flblen = (_WORD) strlen(wdfont[WDFONT1].ob_spec.tedinfo->te_ptext) + 1;

	for (i = 0; i <= nfonts; i++)
	{
		h = &(fbl->fd[lnf]);
		h->id = vqt_name(vdi_handle, i + 1, name);
		fnt_setfont(h->id, 10, &fnt);

		/* Identify nonproportional fonts by comparing widths of "i" and "m" */

		vqt_width(vdi_handle, 'i', &iw, &dummy, &dummy);
		vqt_width(vdi_handle, 'm', &mw, &dummy, &dummy);

		/* If font is non-proportional, or proportional are also counted... */

		if (prop || (iw == mw))
		{
#if 0									/* currently unused */
			h->flag = name[32];
#endif
			name[32] = 0;
			strsncpy(h->name, name, flblen);
			lnf++;
		}
	}

	fbl->nf = lnf;
}


/*
 * Initialize slider for the font selector(s)
 */

static void fnt_sl_init(_WORD nf, void *set_sel, SLIDER *sl, _WORD oldfont, _WORD *newfont, FONTDATA *fd)
{
	sl->type = 0;
	sl->tree = wdfont;
	sl->up_arrow = WDFUP;
	sl->down_arrow = WDFDOWN;
	sl->slider = FSLIDER;
	sl->sparent = FSPARENT;
	sl->lines = NLINES;
	sl->n = nf;
	sl->line = set_font(sl, oldfont, newfont, fd, nf);
	sl->set_selector = set_sel;
	sl->first = WDFONT1;
	sl->findsel = fnt_find_selected;

	sl_init(sl);
}


/* 
 * Aux. routine to set font effects buttons. There are eight effects.
 * These buttons must be in a proper sequence, starting with WDFBOLD.
 */

static void set_effects(FONTBLOCK *fbl)
{
	_WORD i;

	for (i = 0; i < 7; i++)
	{
		set_opt(wdfont, fbl->font.effects, FE_BOLD << i, WDFBOLD + i);
	}
}


/* 
 * Handle buttons in the font dialog- all but OK and Cancel
 */

static void do_fd_button(XDINFO *info,	/* dialog info */
						 SLIDER *sl_info,	/* slider info */
						 FONTBLOCK *fbl, _WORD button)
{
	_WORD curobj;						/* index of object in the listbox */
	_WORD newfont;						/* aux local font index */
	bool drawbutt = TRUE;

	switch (button)
	{
	case WDFONT1:
	case WDFONT2:
	case WDFONT3:
	case WDFONT4:
		curobj = fbl->fdfont - sl_info->line + WDFONT1;

		if (((newfont = sl_info->line + button - WDFONT1) < fbl->nf) && (curobj != button))
		{
			if ((curobj >= WDFONT1) && (curobj < WDFONT1 + NLINES))
				xd_drawbuttnorm(info, curobj);

			fbl->fdfont = newfont;
			fbl->cursize = get_size(fbl);
		}

		drawbutt = FALSE;
		xd_drawdeep(info, WDPARENT);

		break;
	case WDFSUP:
		if (fbl->cursize < fbl->nfsizes - 1)
			fbl->cursize += 1;
		break;
	case WDFSDOWN:
		if (fbl->cursize > 0)
			fbl->cursize -= 1;
		break;
	case WDFCUP:
		fbl->font.colour += 1;		/* limited further below */
		break;
	case WDFCDOWN:
		fbl->font.colour -= 1;		/* limited further below */
		break;
	case WDFBOLD:
	case WDFLIG:
	case WDFITAL:
	case WDFUNDER:
	case WDFHOLL:
	case WDFSHAD:
	case WDFINV:
		get_opt(wdfont, &(fbl->font.effects), FE_BOLD << (button - WDFBOLD), button);
		drawbutt = FALSE;
		break;
	default:
		drawbutt = FALSE;
		break;
	}

	fbl->font.size = fbl->fsizes[fbl->cursize];

	itoa(fbl->font.size, wdfont[WDFSIZE].ob_spec.free_string, 10);

	fbl->font.colour = limcolour(fbl->font.colour);

	set_selcolpat(info, WDFCOL, fbl->font.colour, 8);

	xd_drawthis(info, WDFTEXT);
	xd_drawthis(info, WDFSIZE);

	if (drawbutt)
		xd_change(info, button, OS_SELECTED, 1);
}


/*
 * Save about 20 bytes by using this routine...
 */

static void *alloc_fd(void)
{
	return malloc_chk((nfonts + 1) * sizeof(FONTDATA));
}


/*
 * Dialog for selecting window (directory or text) font
 */

bool fnt_dialog(_WORD title, XDFONT *wd_font, bool prop)
{
	_WORD button;
	XDINFO info;
	SLIDER sl_info;
	bool stop = FALSE, ok = FALSE;
	OBJECT *o = &wdfont[WDFTEXT];
	XUSERBLK *xub = (XUSERBLK *) (o->ob_spec.userblk);

	/* Check if it is already open- currently can not be so twice */

	if (wdfopen)
	{
		alert_iprint(TDOPEN);
		return FALSE;
	}

	/* Allocate space for font data */

	if ((static_fbl.fd = alloc_fd()) == NULL)
		return FALSE;

	wdfopen = TRUE;

	/* Set code for the userdefined object which draws the sample text */

	xub->ub_code = draw_text;
	xub->uv.ptr = (void *) &static_fbl;

	/* Set dialog title */

	rsc_title(wdfont, WDFTITLE, title);

	/* Count fonts and get available and current size and colour */

	*(&static_fbl.font) = *wd_font;

	font_count(&static_fbl, prop);
	static_fbl.cursize = get_size(&static_fbl);
	static_fbl.font.size = static_fbl.fsizes[static_fbl.cursize];
	set_effects(&static_fbl);

	/* Initialize slider */

	fnt_sl_init(static_fbl.nf, set_fselector, &sl_info, wd_font->id, &static_fbl.fdfont, static_fbl.fd);

	/* Open the dialog */

	if (chk_xd_open(wdfont, &info) >= 0)
	{
		set_selcolpat(&info, WDFCOL, static_fbl.font.colour, 8);

		/* Loop until told to exit */

		while (!stop)
		{
			button = sl_form_do(ROOT, &sl_info, &info) & 0x7FFF;

			switch (button)
			{
			case WDFOK:
				if ((static_fbl.fd[static_fbl.fdfont].id != wd_font->id) ||
					(static_fbl.font.size != wd_font->size) ||
					(static_fbl.font.colour != wd_font->colour) || (static_fbl.font.effects != wd_font->effects))
				{
					/* font is changed only if really changed */

					static_fbl.font.id = static_fbl.fd[static_fbl.fdfont].id;
					*wd_font = *(&static_fbl.font);
					ok = TRUE;
				}
				stop = TRUE;
				break;
			case WDFCANC:
				stop = TRUE;
				break;
			default:
				do_fd_button(&info, &sl_info, &static_fbl, button);
				break;
			}
		}

		xd_buttnorm(&info, button);
		xd_close(&info);
	}

	free(static_fbl.fd);
	obj_unhide(wdfont[WDFEFF]);

	wdfopen = FALSE;

	return ok;
}


#if _FONT_SEL

/*
 * Event handler for the close button of the nonmodal dialog box.
 */

static void fnt_close(XDINFO *dialog)
{
	FNT_DIALOG *fnt_dial = (FNT_DIALOG *) dialog;

#if _MINT_
	wd_restoretop(1, &fcwhandle, &fcwap_id);	/* restore previously topped window */
#endif

#if _DOZOOM
	xd_nmclose(dialog, NULL, FALSE);
#else
	xd_nmclose(dialog);
#endif

	free(fnt_dial->fbl.fd);
	free(fnt_dial);
	wdfopen = FALSE;
	nonmodal = FALSE;
}


/*
 * Handler for the buttons of the nonmodal font dialog
 */

static void fnt_hndlbutton(XDINFO *dialog, _WORD button)
{
	FNT_DIALOG *fnt_dial = (FNT_DIALOG *) dialog;
	FONTBLOCK *thefbl = &(fnt_dial->fbl);
	_WORD msg[8], *msgp = msg;

	button &= 0x7FFF;

	if (sl_handle_button(button, &fnt_dial->sl_info.slider, dialog))
		return;

	switch (button)
	{
	case WDFOK:
		/* Unfortunately, message structure is not the same as for VA */
		*msgp++ = FONT_CHANGED;
		*msgp++ = ap_id;
		*msgp++ = 0;
		*msgp++ = fnt_dial->win;
		*msgp++ = thefbl->fd[thefbl->fdfont].id;
		*msgp++ = thefbl->font.size;
		*msgp++ = thefbl->font.colour;
		*msgp = thefbl->font.effects;
		appl_write(fnt_dial->ap_id, 16, msg);
		/* fall through */
	case WDFCANC:
		xd_buttnorm(dialog, button);
		fnt_close(dialog);
		break;
	default:
		do_fd_button(dialog, &fnt_dial->sl_info.slider, thefbl, button);
		break;
	}
}


static XD_NMFUNC fnt_funcs = {
	fnt_hndlbutton,						/* handle button */
	fnt_close,							/* handle close  */
	0L,									/* handle menu   */
	0L									/* handle top    */
};


/*
 * Nonmodal font selector dialog- for a systemwide font selector.
 * Note: this uses the FONT protocol which is understood by apps
 * such as ST-Guide, Multistrip, Taskbar, aMail, etc. 
 */

void fnt_mdialog(_WORD cl_ap_id, _WORD win, _WORD id, _WORD size, _WORD colour, _WORD effect, _WORD prop)
{
	FNT_DIALOG *fnt_dial;
	FONTBLOCK *thefbl;
	char *wtitle;
	OBJECT *o = &wdfont[WDFTEXT];
	XUSERBLK *xub = (XUSERBLK *) (o->ob_spec.userblk);

	/* 
	 * Check if it is already open- can't be twice for the time being
	 * (because the same objects from the resource are used)
	 */

	if (wdfopen)
	{
		_WORD msg[8];
		_WORD *msgp = msg;

		/* 
		 * If font selector is already open, display an alert and
		 * just return old values as if new ones were selected 
		 */

		alert_iprint(TDOPEN);

		*msgp++ = FONT_CHANGED;
		*msgp++ = ap_id;
		*msgp++ = 0;
		*msgp++ = win;
		*msgp++ = id;
		*msgp++ = size;
		*msgp++ = colour;
		*msgp = effect;

		appl_write(cl_ap_id, 16, msg);

		return;
	}

	wdfopen = TRUE;

	if ((fnt_dial = malloc_chk(sizeof(*fnt_dial))) == NULL)
		return;

	thefbl = &(fnt_dial->fbl);

	/* Who calls this ? */

	fnt_dial->ap_id = cl_ap_id;			/* Application id of the caller */
	fnt_dial->win = win;				/* Window id of the caller */
	fnt_dial->colour = colour;			/* Colour of the font */
	fnt_dial->effect = effect;			/* Text effects of the font */

	/* Allocate memory for fonts data */

	if ((thefbl->fd = alloc_fd()) == NULL)
	{
		free(fnt_dial);
		return;
	}

	/* Set userdef object type for display of sample text */

	xub->ub_code = draw_text;
	xub->uv.ptr = (void *) thefbl;

	/* Count existing fonts and get available sizes */

	font_count(thefbl, prop);
	thefbl->font.size = size;
	thefbl->font.colour = colour;
	thefbl->font.effects = effect;
	set_effects(thefbl);

	thefbl->cursize = get_size(thefbl);
	thefbl->font.size = thefbl->fsizes[thefbl->cursize];

	/* Initialize slider */

	fnt_dial->sl_info.fnt_dial = fnt_dial;
	fnt_sl_init(thefbl->nf, mset_fselector, &(fnt_dial->sl_info.slider), id, &(fnt_dial->fbl.fdfont), fnt_dial->fbl.fd);

	/* This dialog is -always- windowed. Set window and dialog titles now */

	wtitle = get_freestring(DTFONSEL);

	rsc_title(wdfont, WDFTITLE, DTAFONT);

	hyppage = NULL;

	nonmodal = TRUE;

#if _MINT_
	wd_restoretop(0, &fcwhandle, &fcwap_id);	/* Remember the topped window */
#endif

#if _DOZOOM
	xd_nmopen(wdfont, (XDINFO *) fnt_dial, &fnt_funcs, 0, /* -1, -1, not used */ NULL, NULL, FALSE, wtitle);
#else
	xd_nmopen(wdfont, (XDINFO *) fnt_dial, &fnt_funcs, 0, /* -1, -1, not used */ NULL, wtitle);
#endif

	set_selcolpat(&(fnt_dial->dialog), WDFCOL, colour, -1);
}

#endif
