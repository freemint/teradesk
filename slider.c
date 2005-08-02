/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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
#include <stddef.h>
#include <vdi.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>
#include <xscncode.h>
#include <library.h>

#include "resource.h"
#include "desk.h"
#include "events.h"
#include "lists.h" 
#include "slider.h"
#include "file.h"


OBJECT *dialogo;


/* 
 * Set slider size and position 
 */

void sl_set_slider(OBJECT *tree, SLIDER *sl, XDINFO *info)
{
	int sh, s, sparent = sl->sparent, slines = sl->lines;

	sl->line = ((sl->n < slines) || (sl->line < 0)) ? 0 : min(sl->line, sl->n - slines);

	/* Determine slider size. Minimum size is equal to font height */

	if (sl->n > slines)
	{
		sh = (int)(((long)slines * (long)tree[sparent].r.h) / (long) sl->n);
		if (sh < screen_info.fnt_h)
			sh = screen_info.fnt_h;
	}
	else
		sh = tree[sparent].r.h;

	tree[sl->slider].r.h = sh - 2 * aes_ver3d;

	/* Determine slider position */

	s = sl->n - slines;
	tree[sl->slider].r.y = aes_ver3d + ( (s > 0) ? (int) (((long) (tree[sparent].r.h - sh) * (long) sl->line) / (long) s) : 0);

	if (info)
		xd_drawdeep(info, sparent);
}


static void do_arrows(int button, OBJECT *tree, SLIDER *sl, XDINFO *info)
{
	boolean redraw, first = TRUE;
	int mstate;

	xd_change(info, button, SELECTED, 1);

	do
	{
		redraw = FALSE;

		if (button == sl->up_arrow)
		{
			if (sl->line > 0)
			{
				sl->line--;
				redraw = TRUE;
				sl_set_slider(tree, sl, info);
			}
		}
		else
		{
			if (sl->line < (sl->n - sl->lines))
			{
				sl->line++;
				redraw = TRUE;
				sl_set_slider(tree, sl, info);
			}
		}

		if (redraw)
			sl->set_selector(sl, TRUE, info);

		mstate = xe_button_state() & 1;

		if (first && mstate)
		{
			wait(ARROW_DELAY);
			first = FALSE;
		}
	}
	while (mstate);

	xd_drawbuttnorm(info, button);
}


/*
 * Calculate to which item a slider points.
 * newpos= position (0:1000) 
 * lines=  number of items
 */

long calc_slpos(int newpos, long lines) 
{
	return ((lines * newpos) / 1000L);
}


/*
 * Calculate slider position (0:1000) for the first visible item
 * pos = index of first item
 * lines = number of items
 */

int calc_slmill(long pos, long lines)
{
	return (int)((1000L * pos) / lines);
}


static void do_slider(OBJECT *tree, SLIDER *sl, XDINFO *info)
{
	int newpos;
	long lines;

	wind_update(BEG_MCTRL);
	newpos = (long)graf_slidebox(tree, sl->sparent, sl->slider, 1);
	wind_update(END_MCTRL);

	/* 
	 * Fix what seems to be a bug in graf_slidebox of Atari AES4.1 ? 
	 * (wrong setting for small values of slider position)
	 */

	if ( newpos < 40 )
		newpos = 0;

	lines = (long)(sl->n - sl->lines);
	sl->line = (int)calc_slpos(lines, newpos);
	sl_set_slider(tree, sl, info);
}


static void do_bar(OBJECT *tree, SLIDER *sl, XDINFO *info)
{
	int my, oy, dummy, old, maxi, slines = sl->lines;

	graf_mkstate(&dummy, &my, &dummy, &dummy);
	objc_offset(tree, sl->slider, &dummy, &oy);

	do
	{
		old = sl->line;

		/* Note: do not use min() and max() here; code would be longer */

		if (my < oy)
		{
			sl->line -= slines;
			if (sl->line < 0)
				sl->line = 0;
		}
		else
		{
			sl->line += slines;
			maxi = sl->n - slines;
			if (sl->line > maxi)
				sl->line = maxi;
		}

		if (sl->line != old)
		{
			sl_set_slider(tree, sl, info);
			sl->set_selector(sl, TRUE, info);
		}
	}
	while (xe_button_state() & 0x1);
}


int keyfunc(XDINFO *info, SLIDER *sl, int scancode)
{
	boolean redraw = FALSE;
	int selected;

	switch (scancode)
	{
	case CTL_CURUP:
		if ((sl->type != 0) && ((selected = sl->findsel()) != 0))
		{
			selected += sl->first;
			obj_deselect(dialogo[selected]);
			obj_select(dialogo[selected - 1]);
			redraw = TRUE;
		}
		else if (sl->line > 0)
		{
			sl->line--;
			sl_set_slider(dialogo, sl, info);
			redraw = TRUE;
		}
		break;
	case CTL_CURDOWN:
		if ((sl->type != 0) && ((selected = sl->findsel()) != (sl->lines - 1)))
		{
			selected += sl->first;
			obj_deselect(dialogo[selected]);
			obj_select(dialogo[selected + 1]);
			redraw = TRUE;
		}
		else if (sl->line < (sl->n - sl->lines))
		{
			sl->line++;
			sl_set_slider(dialogo, sl, info);
			redraw = TRUE;
		}
		break;
	default:
		return 0;
	}

	if (redraw)
		sl->set_selector(sl, TRUE, info);

	return 1;
}


int sl_handle_button(int button, OBJECT *tree, SLIDER *sl, XDINFO *dialog)
{
	int button2 = button & 0x7FFF;

	if ((button2 == sl->up_arrow) || (button2 == sl->down_arrow))
		do_arrows(button2, tree, sl, dialog);
	else if (button2 == sl->slider)
	{
		do_slider(tree, sl, dialog);
		(*sl->set_selector) (sl, TRUE, dialog);
	}
	else if (button2 == sl->sparent)
		do_bar(tree, sl, dialog);
	else
		return FALSE;

	return TRUE;
}


int sl_form_do(OBJECT *tree, int start, SLIDER *sl, XDINFO *info)
{
	int button;

	dialogo = tree;

	do
	{
		button = xd_kform_do(info, start, (userkeys)keyfunc, sl);
	}
	while (sl_handle_button(button, tree, sl, info));

	return button;
}


/* 
 * Initialize sizes and positions of the slider and its listbox.
 * Note: this routine does -NOT- draw the elements because XDINFO
 * is missing!
 */

void sl_init(OBJECT *tree, SLIDER *slider)
{
	slider->set_selector(slider, FALSE, NULL);
	sl_set_slider(tree, slider, NULL);
}


/*
 * Fill-in and redraw contents of scrolled fields FTYPE1... FTYPEn
 * in the list-manipulation dialog. See lists.c 
 */

void set_selector(SLIDER *slider, boolean draw, XDINFO *info) 
{
	int i;
	LSTYPE *f;
	OBJECT *o;

	for (i = 0; i < NLINES; i++)
	{
		o = &setmask[FTYPE1 + i];

		if ((f = get_item( slider->list, i + slider->line)) == NULL)
			*o->ob_spec.tedinfo->te_ptext = 0;
		else
			cv_fntoform(o, 0, f->filetype );
	}

	/* Note: this will redraw the slider and also FTYPE1...FTYPEn */

	if (draw && info)
	{
		xd_drawdeep(info, FTPARENT);
		xd_drawdeep(info, FTSPAR);
	}
}




/*
 * Initiate a (FTYPE, PRGTYPE, ICONTYPE...) list-scroller slider
 */

void ls_sl_init ( int n, void *set_sel, SLIDER *sl, LSTYPE **list )
{
	sl->type = 1;
	sl->up_arrow = FTUP;		/* up-arrow object   */
	sl->down_arrow = FTDOWN;	/* down-arrow object */
	sl->slider = FTSLIDER;		/* slider object     */
	sl->sparent = FTSPAR;		/* slider parent (background) object    */
	sl->lines = NLINES;			/* number of visible lines in the box   */
	sl->n = n;					/* number of items in the list          */
	sl->line = 0;				/* index of first item shown in the box */
	sl->list = list;			/* pointer to list of items to be shown */
	sl->set_selector = set_sel;	/* selector routine */
	sl->first = FTYPE1;			/* first object in the scrolled box     */
	sl->findsel = find_selected;

	xd_set_rbutton(setmask, FTPARENT, FTYPE1 );
	sl_init ( setmask, sl );
}


/* 
 * See also icon.c for icn_sl_init() 
 */