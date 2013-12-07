/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2008  Dj. Vukovic
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


#include <np_aes.h>
#include <vdi.h>
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


/* 
 * Set slider size and position 
 */

void sl_set_slider(SLIDER *sl, XDINFO *info)
{
	int 
		sh,
		s,
		sn = sl->n,
		slines = sl->lines,
		slh = sl->tree[sl->sparent].r.h;


	sl->line = ((sn < slines) || (sl->line < 0)) ? 0 : min(sl->line, sn - slines);

	/* Determine slider size. Minimum size is equal to character height */

	if (sn > slines)
	{
		sh = (int)(((long)slines * (long)slh) / (long)sn);

		if (sh < xd_fnt_h)
			sh = xd_fnt_h;
	}
	else
		sh = slh;
	
	/* Compensation for 3D effects */

	sl->tree[sl->slider].r.h = sh - 2 * aes_ver3d;

	/* Determine slider position */

	s = sn - slines;
	sl->tree[sl->slider].r.y = aes_ver3d + ( (s > 0) ? (int) (((long)(slh - sh) * (long)(sl->line)) / (long)s) : 0);

	if (info)
		xd_drawdeep(info, sl->sparent);
}


/*
 * Calculate to which item a slider points.
 */

long calc_slpos
(
	int newpos,	/* position (0:1000) */
	long lines	/* number of items */
) 
{
	return ((lines * newpos) / 1000L);
}


/*
 * Calculate slider position (0:1000) for the first visible item
 */

int calc_slmill
(
	long pos,	/* index of first item */
	long lines	/* number of items */
)
{
	return (lines) ? (int)((1000L * pos) / lines) : 0;
}


static void do_slider
(
	SLIDER *sl,
	XDINFO *info
)
{
	long
		lines;

	int
		newpos;


	xd_begmctrl();
	newpos = graf_slidebox(sl->tree, sl->sparent, sl->slider, 1);
	xd_endmctrl();

	/* 
	 * Fix what seems to be a bug in graf_slidebox of Atari AES4.1 ? 
	 * (wrong setting for small values of slider position)
	 */

	if ( newpos < 40 )
		newpos = 0;

	lines = (long)(sl->n - sl->lines);
	sl->line = (int)calc_slpos(lines, newpos);
	sl_set_slider(sl, info);
}


/*
 * This routine handles clicking on the slider parent object
 * i.e. paging up or down through a list with a slider
 */

static void do_bar(SLIDER *sl, XDINFO *info)
{
	int
		my,
		oy,
		dummy,
		old,
		maxi,
		slines = sl->lines;

	graf_mkstate(&dummy, &my, &dummy, &dummy);
	objc_offset(sl->tree, sl->slider, &dummy, &oy);

	do
	{
		old = sl->line;

		if (my < oy)
		{
			maxi = sl->line;
			sl->line -= slines;
		}
		else
		{
			maxi = sl->n - slines;
			sl->line += slines;
		}

		sl->line = minmax(0, sl->line, maxi);

		if (sl->line != old)
		{
			sl_set_slider(sl, info);
			sl->set_selector(sl, TRUE, info);
		}
	}
	while (xe_button_state() & 0x1);
}


/*
 * This routine handles movement of the selected item in the selector
 * with more than one items displayed. If some work gets done in this
 * routine, it returns 1; if the work is to be done elsewhere, it returns 0.
 * part of the code acctivated by j <> 0 is only for the use of keyfunc()
 * in list_edit(); otherwise it is completely redundant
 */

int keyfunc(XDINFO *info, SLIDER *sl, int scancode)
{
	int
		k = 0,
		j = 0,
		selected;

	switch (scancode)
	{
		case CTL_CURUP:
		{
			if ((sl->type != 0) && ((selected = sl->findsel()) != 0))
				k = -1;
			else if(sl->line > 0)
				j = -1;
			break;
		}
		case CTL_CURDOWN:
		{
			if ((sl->type != 0) && ((selected = sl->findsel()) != (sl->lines - 1)) )
				k = 1;
			else if (sl->line < (sl->n - sl->lines))
 				j = 1;
		}
		default:
		{
			break;
		}
	}

	if(k != 0)
	{
		selected += sl->first;
		obj_deselect(sl->tree[selected]);
		obj_select(sl->tree[selected + k]);
		sl->set_selector(sl, TRUE, info);
		return 1;
	}

	if(j != 0)
	{
		sl->line += j;
		sl_set_slider(sl, info);
		sl->set_selector(sl, TRUE, info);
		return 1;
	}

	return 0;
}


/*
 * This routine handles the pressed arrow buttons in a slider
 */

int sl_handle_button(int button, SLIDER *sl, XDINFO *info)
{
	int 
		j = 0,
		button2 = button & 0x7FFF;


	if (button2 == sl->up_arrow)
	{
		if(sl->line > 0)
			j = -1;
	}
	else if (button2 == sl->down_arrow)
	{
		if(sl->line < (sl->n - sl->lines))
			j = 1;
	}
	else if (button2 == sl->slider)
	{
		do_slider(sl, info);
		sl->set_selector(sl, TRUE, info);
	}
	else if (button2 == sl->sparent)
		do_bar(sl, info);
	else
		return FALSE;

	if(j != 0)
	{
		sl->line += j;
		sl_set_slider(sl, info);
		sl->set_selector(sl, TRUE, info);
	}

	return TRUE;
}


/*
 * Note: in case of a double click, bit 0x8000 is set in returned value
 */

int sl_form_do(int start, SLIDER *sl, XDINFO *info)
{
	int button;

	do
	{
		button = xd_kform_do(info, start, (userkeys)keyfunc, sl);
	}
	while (sl_handle_button(button, sl, info));

	return button;
}


/* 
 * Initialize sizes and positions of the slider and its listbox.
 * Note: this routine does -NOT- draw the elements because XDINFO
 * is missing!
 */

void sl_init(SLIDER *slider)
{
	slider->set_selector(slider, FALSE, NULL);
	sl_set_slider(slider, NULL);
}


