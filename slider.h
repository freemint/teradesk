/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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


/* 
 * Slider type: 0 - item(s) can not be selected in the listbox
 *              1 - item(s) can be selected in the listbox
 */

typedef struct sliderinfo
{
	LSTYPE **list;				/* pointer to the list scrolled by this slider */
	OBJECT *tree;				/* root object of the dialog */
	void (*set_selector) (struct sliderinfo *slider, bool draw, XDINFO *info);
	_WORD (*findsel) (void);	/* funktie voor het vinden van het geselekteerde object */
	_WORD type;					/* slider type (0 or 1) */
	_WORD up_arrow;				/* 'up arrow' object index */
	_WORD down_arrow;			/* 'down arrow' object index */
	_WORD slider;				/* index of the slider object */
	_WORD sparent;				/* index of the slider parent (background) object */
	_WORD lines;				/* number of visible lines (list items) */
	_WORD n;					/* total number of lines (list items) */
	_WORD line;					/* index of the first visible item (0 - n-lines) */
	_WORD first;				/* index van eerste regel in objectboom */
} SLIDER;


void sl_init(SLIDER *slider);
void sl_set_slider(SLIDER *slider, XDINFO *info);
_WORD sl_handle_button(_WORD button, SLIDER *sl, XDINFO *dialog);
_WORD sl_form_do(_WORD start, SLIDER *slider, XDINFO *info);
_WORD keyfunc(XDINFO *info, SLIDER *sl, _WORD scancode); 
_WORD calc_slpos(_WORD newpos, long lines);
_WORD calc_slmill(long pos, long lines);

