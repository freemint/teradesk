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

#include <np_aes.h>
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "resource.h"
#include "xfilesys.h"
#include "screen.h"
#include "lists.h" /* must be before slider.h */
#include "slider.h"
#include "error.h"
#include "font.h"
#include "va.h"	


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
	int fsize;					/* Current size of the font. */
	int chw;					/* Width of a character. */
	int chh;					/* Height of a character. */

	int cursize;				/* Index of current size in array 'fsize'. */
	int fsizes[MFSIZES];		/* Array with sizes of the current font. */
	int nfsizes;				/* Number of sizes in 'fsizes'. */
} FONTBLOCK;

static FONTBLOCK fbl;


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

static void draw_sample(RECT *r, RECT *c, char *text, FONTBLOCK *fbl)
{
	int x, y, extent[8];
	RECT rr;

	xd_clip_on(c);
	clear(r);
	xd_rcintersect(r, c, &rr); /* Why this complication ? */
	xd_clip_on(&rr);

	set_txt_default(fbl->fd[fbl->font].id, fbl->fsize);
	fnt_point(fbl->fsize, &fbl->chw, &fbl->chh);

	vqt_extent(vdi_handle, text, extent);

	x = r->x + (r->w - extent[2] + extent[0] - 1) / 2;
	y = r->y + (r->h - extent[7] + extent[1] - 1) / 2;

	vswr_mode(vdi_handle, MD_TRANS); 
	v_gtext(vdi_handle, x, y, text);

	xd_clip_off();
}


/*
 * Draw sample text in Teradesk's font selector
 */

static int cdecl draw_text(PARMBLK *pb)
{
	draw_sample(&pb->r, &pb->c, (char *)pb->pb_parm, &fbl);
	return 0;
}


#define _FONT_SEL 1 /* Compile nonmodal font selector routines */

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
	FNT_USERBLK userblock;

	int ap_id;					/* Application id of caller. */
	int win;					/* Window id of caller. */
	int color;					/* Color of font. */
	int effect;					/* Text effects of font. */

	FONTBLOCK fbl;
} FNT_DIALOG;


/*
 * Draw sample text in the nonmodal fontselector dialog
 */

static int cdecl mdraw_text(PARMBLK *pb)
{
	FNT_DIALOG *fnt_dial = ((FNT_USERBLK *) pb->pb_parm)->fnt_dial;
	draw_sample(&pb->r, &pb->c, (char *)(((FNT_USERBLK *) pb->pb_parm)->text), &fnt_dial->fbl);
	return 0;
}

#endif


static int fnt_find_selected(void) 
{
	int object;

	return ((object = xd_get_rbutton(wdfont, WDPARENT)) < 0) ? 0 : object - WDFONT1;
}


/*
 * The really working part of the routines the set the listbox in fontselector(s)
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
			o->ob_spec.tedinfo->te_ptext = "                ";
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

	if (draw == TRUE)
		xd_draw(info, WDPARENT, MAX_DEPTH);
}


/*
 * Set font selector listbox for Teradesk
 */

static void set_fselector(SLIDER *slider, boolean draw, XDINFO *info)
{
	set_theselector(slider, draw, info, fbl.font, fbl.fd, fbl.nf);
}


#if _FONT_SEL


/*
 * Set font selector listbox for the nonmodal dialog
 */

static void mset_fselector(SLIDER *slider, boolean draw, XDINFO *info)
{
	FNT_DIALOG *fnt_dial = ((FNT_SLIDER *) slider)->fnt_dial;
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
 * Count (nonproportional ?) fonts
 */

void font_count(FONTDATA *fda, int *nfn, boolean prop)
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

		vqt_width(vdi_handle, 'i', &iw, &dummy, &dummy);
		vqt_width(vdi_handle, 'm', &mw, &dummy, &dummy);

		if ( prop || (iw == mw))
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
					xd_change(info, curobj, NORMAL, 1);
				fbl->font = newfont;
				fbl->cursize = get_size(fbl);
				state = SELECTED;
			}
			break;
		case WDFSUP:
			if (fbl->cursize < fbl->nfsizes - 1)
				fbl->cursize = fbl->cursize + 1;
			break;
		case WDFSDOWN:
			if (fbl->cursize > 0)
				fbl->cursize = fbl->cursize - 1;
			break;
	}

	fbl->fsize = fbl->fsizes[fbl->cursize];
	itoa(fbl->fsize, wdfont[WDFSIZE].ob_spec.free_string, 10);
	xd_draw(info, WDFTEXT, 0);
	xd_draw(info, WDFSIZE, 0);
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

	static 
		USERBLK userblock;


	/* Allocate space for font data */

	if ((fbl.fd = malloc((nfonts + 1) * sizeof(FONTDATA))) == NULL)
	{
		xform_error(ENSMEM);
		return FALSE;
	}

	/* Change type of the object where the sample text is to be displayed */

	if (o->ob_type != G_USERDEF)
		xd_userdef(o, &userblock, draw_text);

	/* Set dialog title */

	rsc_title(wdfont, WDFTITLE, title);

	/* Count fonts and get available sizes */

	fbl.fsize = wd_font->size;
	font_count(fbl.fd, &fbl.nf, prop);
	fbl.cursize = get_size(&fbl);
	fbl.fsize = fbl.fsizes[fbl.cursize];

	/* Initialize slider */

	fnt_sl_init( fbl.nf, set_fselector, &sl_info, wd_font->id, &fbl.font, fbl.fd);

	/* Open the dialog */

	xd_open(wdfont, &info);

	/* Loop until told to exit */

	while (stop == FALSE)
	{
		button = sl_form_do(wdfont, 0, &sl_info, &info) & 0x7FFF;

		switch(button)
		{
			case WDFOK:
				if ((fbl.fd[fbl.font].id != wd_font->id) || (fbl.fsize != wd_font->size))
				{
					wd_font->id = fbl.fd[fbl.font].id;
					wd_font->size = fbl.fsize;
					wd_font->cw = fbl.chw;
					wd_font->ch = fbl.chh;
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

	xd_change(&info, button, NORMAL, 0);
	xd_close(&info);
	free(fbl.fd);
	return ok;
}


#if _FONT_SEL

/*
 * Event handler for the close button of the nonmodal dialog box.
 */

void fnt_close(XDINFO *dialog)
{
	FNT_DIALOG *fnt_dial = (FNT_DIALOG *) dialog;

#if _DOZOOM
	/* DjV 060 290603 conditional */
	xd_nmclose(dialog, NULL, FALSE);
#else
	xd_nmclose(dialog);
#endif

	free(fnt_dial->fbl.fd);
	free(fnt_dial);
}


/*
 * Handler for the buttons of the nonmodal font dialog
 */

void fnt_hndlbutton(XDINFO *dialog, int button)
{
	FNT_DIALOG *fnt_dial = (FNT_DIALOG *) dialog;

	int msg[8];

	button &= 0x7FFF;

	if (sl_handle_button(button, wdfont, &fnt_dial->sl_info.slider, dialog))
		return;

	switch (button)
	{
		case WDFOK:
			msg[0] = FONT_CHANGED;
			msg[1] = fnt_dial->ap_id;
			msg[2] = 0;
			msg[3] = fnt_dial->win;
			msg[4] = fnt_dial->fbl.fd[fnt_dial->fbl.font].id;
			msg[5] = fnt_dial->fbl.fsize;
			msg[6] = fnt_dial->color;
			msg[7] = fnt_dial->effect;
			appl_write(fnt_dial->ap_id, 16, msg);
		case WDFCANC:
			xd_change(dialog, button, NORMAL, 0);
			fnt_close(dialog);
			break;
		default:
			do_fd_button(dialog, &fnt_dial->sl_info.slider, &fnt_dial->fbl, button);
			break;
	}
}


XD_NMFUNC fnt_funcs =
{
	fnt_hndlbutton,
	fnt_close,
	0L,
	0L
};


/*
 * Nonmodal font selector dialog- for a systemwide font selector
 */

void fnt_mdialog(int ap_id, int win, int id, int size, int color,
				 int effect, int prop)
{
	FNT_DIALOG 
		*fnt_dial;

	char
		*wtitle;

	OBJECT 
		*o = &wdfont[WDFTEXT];

	if ((fnt_dial = malloc(sizeof(FNT_DIALOG))) == NULL)
	{
		xform_error(ENSMEM);
		return;
	}

	/* Who calls this ? */

	fnt_dial->ap_id = ap_id;	/* Application id of caller. */
	fnt_dial->win = win;		/* Window id of caller. */
	fnt_dial->color = color;	/* Color of font. */
	fnt_dial->effect = effect;	/* Text effects of font. */

	/* Allocate memory for fonts data */

	if ((fnt_dial->fbl.fd = malloc((nfonts + 1) * sizeof(FONTDATA))) == NULL)
	{
		free(fnt_dial);
		xform_error(ENSMEM);
		return;
	}

	/* Set object type for display of sample text */

	fnt_dial->userblock.fnt_dial = fnt_dial;
	fnt_dial->userblock.text = (char *) o->ob_spec.index;

	fnt_dial->userblock.userblock.ub_code = mdraw_text; 
	fnt_dial->userblock.userblock.ub_parm = (long) &fnt_dial->userblock;

	o->ob_type = (o->ob_type & 0xFF00) | G_USERDEF;
	o->ob_spec.userblk = &fnt_dial->userblock.userblock;

	/* Set dialog title */

	rsc_title(wdfont, WDFTITLE, DTVFONT);

	/* Count fonts and get available sizes */

	font_count(&fnt_dial->fbl.fd[fnt_dial->fbl.nf], &fnt_dial->fbl.nf, prop);
	fnt_dial->fbl.fsize = size;
	fnt_dial->fbl.cursize = get_size(&fnt_dial->fbl);

	fnt_dial->fbl.fsize = fnt_dial->fbl.fsizes[fnt_dial->fbl.cursize];

	/* Initialize slider */

	fnt_sl_init( fnt_dial->fbl.nf, mset_fselector, &fnt_dial->sl_info.slider, id, &fnt_dial->fbl.font, fnt_dial->fbl.fd);
	fnt_dial->sl_info.fnt_dial = fnt_dial;

	/* This dialog is -always- windowed. Set window title now */

	wtitle = get_freestring(DTFONSEL);

#if _DOZOOM
	xd_nmopen(wdfont, &fnt_dial->dialog, &fnt_funcs, 0, -1, -1,
			  NULL, NULL, FALSE, wtitle);
#else
	xd_nmopen(wdfont, &fnt_dial->dialog, &fnt_funcs, 0, -1, -1, NULL, wtitle);
#endif
}


#endif

