/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                         2003, 2004, 2005  Dj. Vukovic
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


#include <np_aes.h>
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <mint.h>
#include <xdialog.h>
#include <internal.h>

#include "resource.h"
#include "desk.h"
#include "xfilesys.h"
#include "lists.h" /* must be before slider.h */
#include "slider.h"
#include "error.h"
#include "font.h"
#include "screen.h"
#include "config.h"
#include "va.h"	
#include "window.h"
#include "icon.h"
#include "events.h"


#define MFSIZES 100 /* Maximum number of available font sizes */

typedef struct
{
	int id;
	char name[17];
	int flag;
} FONTDATA;

typedef struct
{
	FONTDATA *fd;				/* Fonts information. */
	int nf;						/* Number of fonts. */
	int font;					/* Current font index in fd */

	/* note: preserve structure of 6 items below to be identical to FONT */

	int id;						/* font id */
	int fsize;					/* Current size of the font. */
	int colour;					/* Currect colour of the font */
	int effect;					/* Current effects of the font */
	int chw;					/* Width of a character. */
	int chh;					/* Height of a character. */

	int cursize;				/* Index of current size in array 'fsize'. */
	int fsizes[MFSIZES];		/* Array with sizes of the current font. */
	int nfsizes;				/* Number of sizes in 'fsizes'. */
} FONTBLOCK;

static FONTBLOCK 
	fbl;

static boolean
	wdfopen = FALSE,
	nonmodal = FALSE;

#if _MINT_
static int 
	fcwhandle, 
	fcwap_id;
#endif

int fnt_point(int height, int *cw, int *ch)
{
	int dummy, r;

	r = vst_point(vdi_handle, height, &dummy, &dummy, &dummy, ch);
	vqt_width(vdi_handle, ' ', cw, &dummy, &dummy);

	return r;
} 


void fnt_setfont(int font, int height, FONT *data)
{
	data->id = vst_font(vdi_handle, font);
	data->size = fnt_point(height, &data->cw, &data->ch);
}


/*
 * Draw sample text in desired font size 
 */

static void draw_sample(RECT *r, RECT *c, char *text, FONTBLOCK *fbl, int sel)
{
	int extent[8];
	RECT tr, rr;

	xd_rcintersect(r, c, &rr);
	xd_clip_on(&rr);

	if (nonmodal)
		clr_object( r, WHITE, -1); 	/* to make white background */
	else
		pclear(r); /* to show window colour and pattern */

	fbl->id = fbl->fd[fbl->font].id;
	set_txt_default((FONT *)(&fbl->id));

	fnt_point(fbl->fsize, &fbl->chw, &fbl->chh);

	vqt_extent(vdi_handle, text, extent);

	tr.w = extent[2] - extent[0];
	tr.h = extent[7] - extent[1] + 4;

	tr.x = r->x + (r->w - tr.w) / 2;
	tr.y = r->y + (r->h - tr.h) / 2;

	w_transptext(tr.x, tr.y + 2, text);

	if(sel)
		invert(&tr);

	xd_clip_off();
}


/*
 * Draw sample text in Teradesk's font selector, using selected font
 */

static int cdecl draw_text(PARMBLK *pb)
{
	FONTBLOCK *thefbl = (FONTBLOCK *)(((XUSERBLK *)(pb->pb_parm))->other);
	char *thetext = ((XUSERBLK *)(pb->pb_parm))->ob_spec.free_string;
	draw_sample(&pb->r, &pb->c, thetext, thefbl, (pb->pb_currstate & SELECTED));
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
	int ap_id;					/* Application id of caller. */
	int win;					/* Window id of caller. */
	int color;					/* Color of font. */
	int effect;					/* Text effects of font. */
	FONTBLOCK fbl;
} FNT_DIALOG;

#endif


static int fnt_find_selected(void) 
{
	int object;

	return ((object = xd_get_rbutton(wdfont, WDPARENT)) < 0) ? 0 : object - WDFONT1;
}


/*
 * The really working part of the routines that set 
 * the listbox in fontselector(s)
 */

static void set_theselector
(
	SLIDER *slider, 
	boolean draw, 
	XDINFO *info,
	int thefont,
	FONTDATA *fd,
	int nnf
)
{
	int i;
	OBJECT *o;

	for (i = 0; i < NLINES; i++)
	{
		o = &wdfont[WDFONT1 + i];

		if (i + slider->line >= nnf)
		{
/*
			o->ob_spec.tedinfo->te_ptext = "                ";
*/
			*o->ob_spec.tedinfo->te_ptext = 0;
			o->ob_state &= ~SELECTED;
		}
		else
		{
			if (i + slider->line == thefont)
				o->ob_state |= SELECTED;
			else
				o->ob_state &= ~SELECTED;		
			o->ob_spec.tedinfo->te_ptext = fd[i + slider->line].name;
		}
	}

	if (draw)
		xd_drawdeep(info, WDPARENT);
}


/*
 * Set font selector listbox for TeraDesk
 */

static void set_fselector(SLIDER *slider, boolean draw, XDINFO *info)
{
	set_theselector(slider, draw, info, fbl.font, fbl.fd, fbl.nf);
}


#if _FONT_SEL

/*
 * Set font selector listbox for the nonmodal font-selector dialog
 */

static void mset_fselector(SLIDER *slider, boolean draw, XDINFO *info)
{
	FNT_DIALOG *fnt_dial = ((FNT_SLIDER *)slider)->fnt_dial;

	set_theselector(slider, draw, info, fnt_dial->fbl.font, fnt_dial->fbl.fd, fnt_dial->fbl.nf);
}
#endif


static int set_font(SLIDER *sl_info, int oldfont, int *font, FONTDATA *fd, int nf)
{
	int i = 0, line;

	while ((fd[i].id != oldfont) && (i < (nf - 1)))
		i++;

	*font = line = (fd[i].id == oldfont) ? i : 0;

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

static int get_size(FONTBLOCK *fbl)
{
	int 
		n = 1, 
		height = 99, 
		dummy, i, 
		error, 
		tmp, 
		min = 0;


	vst_font(vdi_handle, fbl->fd[fbl->font].id);

	height = fbl->fsizes[0] = vst_point(vdi_handle, height, &dummy, &dummy, &dummy, &dummy);

	while (n < MFSIZES - 1 && fbl->fsizes[n - 1] != (height = vst_point(vdi_handle, height - 1, &dummy, &dummy, &dummy, &dummy)))
		fbl->fsizes[n++] = height;

	error = abs(fbl->fsizes[0] - fbl->fsize);

	for (i = 1; i < n; i++)
	{
		if ((tmp = abs(fbl->fsizes[i] - fbl->fsize)) < error)
		{
			error = tmp;
			min = i;
		}
	}

	itoa(fbl->fsizes[min], wdfont[WDFSIZE].ob_spec.free_string, 10);
	fbl->nfsizes = n;
	return min;
}


/*
 * Count available fonts
 */

void font_count
(
	FONTDATA *fda,	/* resulting font data */ 
	int *nfn,		/* number of fonts counted */ 
	boolean prop	/* true if proportional fonts are counted */
)
{
	char name[34];

	int i, j, iw, mw, dummy, lnf = 0;
	char *s;
	FONT fnt;
	FONTDATA *h;

	for (i = 0; i <= nfonts; i++)
	{
		h = &fda[lnf];
		s = h->name;
		h->id = vqt_name(vdi_handle, i + 1, name);
		fnt_setfont(h->id, 10, &fnt);

		/* 
		 * Identify nonproportional fonts by comparing widths
		 * of "i" and "m"
		 */

		vqt_width(vdi_handle, 'i', &iw, &dummy, &dummy);
		vqt_width(vdi_handle, 'm', &mw, &dummy, &dummy);

		/* If font is non-proportional, or proportional are also counted... */

		if ( prop || (iw == mw) )
		{
			strsncpy(s, name, sizeof(h->name));	
			j = (int) strlen(h->name);
			while (j < 16)
				s[j++] = ' ';
			h->flag = (int)name[32];
			lnf++;
		}
	}
	*nfn = lnf;
}


/*
 * Initialize slider for the font selector(s)
 */

static void fnt_sl_init( int nf, void *set_sel, SLIDER *sl, int oldfont, int *newfont, FONTDATA *fd)
{
	sl->type = 0;
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
	sl_init(wdfont, sl);
}


/* 
 * Handle buttons in the font dialog- all but OK and Cancel
 */

static void do_fd_button
(
	XDINFO *info,		/* dialog info */
	SLIDER *sl_info,	/* slider info */
	FONTBLOCK *fbl,
	int button
)
{
	int 
		curobj,				/* index of object in the listbox */ 
		newfont,			/* aux local font index */ 
		state = NORMAL;		/* state to set current button to */

#if _MINT_
	boolean
		drawbutt = TRUE;
#endif

	switch (button)
	{
		case WDFONT1:
		case WDFONT2:
		case WDFONT3:
		case WDFONT4:
			curobj = fbl->font - sl_info->line + WDFONT1;
			if (((newfont = sl_info->line + button - WDFONT1) < fbl->nf) && (curobj != button))
			{
				if ((curobj >= WDFONT1) && (curobj < WDFONT1 + NLINES)) 
					xd_drawbuttnorm(info, curobj);
				fbl->font = newfont;
				fbl->cursize = get_size(fbl);
				state = SELECTED;
			}

			/* This is needed only in XaAES */
#if _MINT_
			drawbutt = FALSE;
			xd_drawdeep(info, WDPARENT);
#endif
			break;
		case WDFSUP:
			if (fbl->cursize < fbl->nfsizes - 1)
				fbl->cursize = fbl->cursize + 1;
			break;
		case WDFSDOWN:
			if (fbl->cursize > 0)
				fbl->cursize = fbl->cursize - 1;
			break;
		case WDFCUP:
			fbl->colour += 1;
			break;
		case WDFCDOWN:
			fbl->colour -= 1;
			break;
	}


	fbl->fsize = fbl->fsizes[fbl->cursize];
	itoa(fbl->fsize, wdfont[WDFSIZE].ob_spec.free_string, 10);
	wdfont[WDFCOL].ob_spec.obspec.interiorcol = limcolor(fbl->colour);
	fbl->colour = wdfont[WDFCOL].ob_spec.obspec.interiorcol;
	xd_drawthis(info, WDFCOL);
	xd_drawthis(info, WDFTEXT);
	xd_drawthis(info, WDFSIZE);

#if _MINT_
	if (drawbutt)				/* "if (drawbutt)" needed only in XaAES */
#endif
		xd_change(info, button, state, 1);
}


/*
 * Dialog for selecting window (directory or text) font
 */

boolean fnt_dialog(int title, FONT *wd_font, boolean prop)
{
	int 
		button;

	XDINFO 
		info;

	SLIDER 
		sl_info;

	boolean 
		stop = FALSE, 
		ok = FALSE;

	OBJECT 
		*o = &wdfont[WDFTEXT];

	XUSERBLK 
		*xub = (XUSERBLK *)(o->ob_spec.userblk);

	/* Check if it is already open- curently can not be so twice */

	if ( wdfopen )
	{
		alert_iprint(TDOPEN);
		return FALSE;
	}

	/* Allocate space for font data */

	if ((fbl.fd = malloc_chk((nfonts + 1) * sizeof(FONTDATA))) == NULL)
		return FALSE;

	wdfopen = TRUE;

	/* Set code for the userdefined object which draws the sample text */

	xub->ub_code = draw_text;
	(FONTBLOCK *)(xub->other) = &fbl;

	/* Set dialog title */

	rsc_title(wdfont, WDFTITLE, title);

	/* Count fonts and get available and current size and colour */

	fbl.fsize = wd_font->size;
	fbl.colour = wd_font->colour;
	fbl.effect = wd_font->effects;
	font_count(fbl.fd, &fbl.nf, prop);
	fbl.cursize = get_size(&fbl);
	fbl.fsize = fbl.fsizes[fbl.cursize];
	wdfont[WDFCOL].ob_spec.obspec.interiorcol = fbl.colour;

	/* Initialize slider */

	fnt_sl_init( fbl.nf, set_fselector, &sl_info, wd_font->id, &fbl.font, fbl.fd);

	/* Open the dialog */

	xd_open(wdfont, &info);

	/* Loop until told to exit */

	while (!stop)
	{
		button = sl_form_do(wdfont, 0, &sl_info, &info) & 0x7FFF;

		switch(button)
		{
			case WDFOK:
				if ((fbl.fd[fbl.font].id != wd_font->id) || (fbl.fsize != wd_font->size) || (fbl.colour != wd_font->colour) )
				{
					fbl.id = fbl.fd[fbl.font].id;
					*wd_font = *(FONT *)(&fbl.id);
					ok = TRUE;
				}

			case WDFCANC:
				stop = TRUE;
				break;
			default:
				do_fd_button(&info, &sl_info, &fbl, button);
				break;
		}
	}

	xd_buttnorm(&info, button);
	xd_close(&info);
	free(fbl.fd);
	wdfopen = FALSE;

	return ok;
}


#if _FONT_SEL

/*
 * Event handler for the close button of the nonmodal dialog box.
 */

void fnt_close(XDINFO *dialog)
{
	FNT_DIALOG *fnt_dial = (FNT_DIALOG *)dialog;

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

void fnt_hndlbutton(XDINFO *dialog, int button)
{
	FNT_DIALOG *fnt_dial = (FNT_DIALOG *)dialog;

	int msg[8], *msgp = msg;

	button &= 0x7FFF;

	if (sl_handle_button(button, wdfont, &fnt_dial->sl_info.slider, dialog))
		return;

	switch (button)
	{
		case WDFOK:
			/* Unfortunately, message structure is not the same as for VA */
			*msgp++ = FONT_CHANGED;
			*msgp++ = ap_id;
			*msgp++ = 0;
			*msgp++ = fnt_dial->win;
			*msgp++ = fnt_dial->fbl.fd[fnt_dial->fbl.font].id;
			*msgp++ = fnt_dial->fbl.fsize;
			*msgp++ = fnt_dial->fbl.colour;
			*msgp   = fnt_dial->fbl.effect;
			appl_write(fnt_dial->ap_id, 16, msg);
		case WDFCANC:
			xd_buttnorm(dialog, button);
			fnt_close(dialog);
			break;
		default:
			do_fd_button(dialog, &fnt_dial->sl_info.slider, &fnt_dial->fbl, button);
			break;
	}
}


XD_NMFUNC fnt_funcs =
{
	fnt_hndlbutton,	/* handle button */
	fnt_close,		/* handle close  */
	0L,				/* handle menu   */
	0L				/* handle top    */
};


/*
 * Nonmodal font selector dialog- for a systemwide font selector.
 * Note: this uses the FONT protocol which is understood by apps
 * such as ST-Guide, Multistrip, Taskbar, aMail, etc. 
 */

void fnt_mdialog(int cl_ap_id, int win, int id, int size, int color,
				 int effect, int prop)
{
	FNT_DIALOG 
		*fnt_dial;

	char
		*wtitle;

	OBJECT 
		*o = &wdfont[WDFTEXT];

	XUSERBLK 
		*xub = (XUSERBLK *)(o->ob_spec.userblk);


	/* 
	 * Check if it is already open- can't be twice for the time being
	 * (because the same objects from the resource are used)
	 */

	if ( wdfopen )
	{
		int msg[8], *msgp = msg;

		/* 
		 * If font selector is already open, display an alert and
		 * just return the old values as if new ones were selected 
		 */ 

		alert_iprint(TDOPEN);

		*msgp++ = FONT_CHANGED;
		*msgp++ = ap_id;
		*msgp++ = 0;
		*msgp++ = win;
		*msgp++ = id;
		*msgp++ = size;
		*msgp++ = color;
		*msgp   = effect;

		appl_write(cl_ap_id, 16, msg);

		return;
	}
	wdfopen = TRUE;

	if ((fnt_dial = malloc_chk(sizeof(FNT_DIALOG))) == NULL)
		return;

	/* Who calls this ? */

	fnt_dial->ap_id = cl_ap_id;	/* Application id of the caller */
	fnt_dial->win = win;		/* Window id of the caller */
	fnt_dial->color = color;	/* Color of the font */
	fnt_dial->effect = effect;	/* Text effects of the font */

	/* Allocate memory for fonts data */

	if ((fnt_dial->fbl.fd = malloc_chk((nfonts + 1) * sizeof(FONTDATA))) == NULL)
	{
		free(fnt_dial);
		return;
	}

	/* Set userdef object type for display of sample text */

	xub->ub_code = draw_text;
	(FONTBLOCK *)(xub->other) = &fnt_dial->fbl;

	/* Count existing fonts and get available sizes */

	font_count(fnt_dial->fbl.fd, &fnt_dial->fbl.nf, prop);

	fnt_dial->fbl.fsize = size;
	fnt_dial->fbl.colour = color;
	fnt_dial->fbl.effect = effect;
	fnt_dial->fbl.cursize = get_size(&fnt_dial->fbl);

	fnt_dial->fbl.fsize = fnt_dial->fbl.fsizes[fnt_dial->fbl.cursize];

	wdfont[WDFCOL].ob_spec.obspec.interiorcol = color;

	/* Initialize slider */

	fnt_dial->sl_info.fnt_dial = fnt_dial;
	fnt_sl_init( fnt_dial->fbl.nf, mset_fselector, &(fnt_dial->sl_info.slider), id, &(fnt_dial->fbl.font), fnt_dial->fbl.fd);

	/* This dialog is -always- windowed. Set window and dialog titles now */

	wtitle = get_freestring(DTFONSEL);
	rsc_title(wdfont, WDFTITLE, DTAFONT);
	nonmodal = TRUE;

#if _MINT_
	wd_restoretop(0, &fcwhandle, &fcwap_id); /* Remember the topped window */
#endif

#if _DOZOOM
	xd_nmopen(wdfont, (XDINFO *)fnt_dial, &fnt_funcs, 0, /* -1, -1, not used */ NULL, NULL, FALSE, wtitle);
#else
 	xd_nmopen(wdfont, (XDINFO *)fnt_dial, &fnt_funcs, 0, /* -1, -1, not used */ NULL, wtitle);
#endif

}

#endif

