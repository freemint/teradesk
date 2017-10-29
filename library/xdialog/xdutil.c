/* 
 * Xdialog Library. Copyright (c) 1993 - 2002  W. Klaren,
 *                                2002 - 2003  H. Robbers,
 *                                2003 - 2007  Dj. Vukovic
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


#include <library.h>
#include "xdialog.h"



/* 
 * Funktie voor het omzetten van een RECT structuur naar een pxy array. 
 * (set diagonal points of a rectangle, not the whole perimeter)
 */

void xd_rect2pxy(RECT *r, _WORD *pxy)
{
	pxy[0] = r->x;
	pxy[1] = r->y;
	pxy[2] = pxy[0] + r->w - 1;
	pxy[3] = pxy[1] + r->h - 1;
}


/* 
 * Funktie voor het berekenen van de doorsnede van twee rechthoeken. 
 */

_WORD xd_rcintersect(RECT *r1, RECT *r2, RECT *dest)
{
	_WORD xmin, xmax, ymin, ymax;


	xmin = max(r1->x, r2->x);
	ymin = max(r1->y, r2->y);
	xmax = min(r1->x + r1->w, r2->x + r2->w);
	ymax = min(r1->y + r1->h, r2->y + r2->h);
	dest->x = xmin;
	dest->y = ymin;
	dest->w = xmax - xmin;
	dest->h = ymax - ymin;

	if ((dest->w <= 0) || (dest->h <= 0))
		return FALSE;

	return TRUE;
}


/*
 * Funktie die bepaalt of een bepaald punt binnen een gegeven
 * rechthoek ligt.
 *
 * Parameters:
 *
 * x	- x coordinaat punt,
 * y	- y coordinaat punt,
 * r	- rechthoek.
 *
 * Resultaat : TRUE als het punt binnen de rechthoek ligt, FALSE
 *			   als dit niet het geval is.
 */

_WORD xd_inrect(_WORD x, _WORD y, RECT *r)
{
	if
	(
		(x >= r->x) &&
		(x < (r->x + r->w)) && 
		(y >= r->y) && 
		(y < (r->y + r->h))
	)
		return TRUE;
	else
		return FALSE;
}


/* 
 * Funktie voor het berekenen van de grootte van de schermbuffer. 
 * Compute size of the screen buffer, depending on resolution
 * and number of colours planes
 */

long xd_initmfdb(RECT *r, MFDB *mfdb)
{
	long size;

	mfdb->fd_w = (r->w + 16) & 0xFFF0;
	mfdb->fd_h = r->h;
	mfdb->fd_wdwidth = mfdb->fd_w / 16;
	mfdb->fd_stand = 0;
	mfdb->fd_nplanes = xd_nplanes;

	size = ((long)(mfdb->fd_w) * r->h * xd_nplanes) / 8;

	return size;
}


/* 
 * Funktie voor het installeren van user-defined objects. 
 */

void xd_userdef(OBJECT *object, USERBLK *userblk, _WORD cdecl(*code) (PARMBLK *parmblock))
{
	object->ob_type = (object->ob_type & 0xFF00) | G_USERDEF;
	userblk->ub_code = code;
	userblk->ub_parm = object->ob_spec.index;
	object->ob_spec.userblk = userblk;
}


/* 
 * Funktie voor het installeren van "extended" user-defined objects. 
 * This routine keeps the 'extended' object type identification, and replaces
 * the 'basic' type with G_USERDEF. It saves original object flags and
 * object type into userblk fields.
 */

void xd_xuserdef(OBJECT *object, XUSERBLK *userblk, _WORD cdecl(*code) (PARMBLK *parmblock))
{
	userblk->ub_code = code;
	userblk->ub_parm = userblk;
	userblk->ob_type = object->ob_type;
	userblk->ob_flags = object->ob_flags;
	userblk->uv.ptr = 0L;	/* clear all of .uv */
	userblk->ob_spec = object->ob_spec;

	/* note: it is shorter to set new type in -two- lines of code, as below */

	object->ob_type &= 0xFF00;
	object->ob_type |= G_USERDEF;

#if 0 /* no need here */
	object->ob_flags &= ~(AES3D_1 | AES3D_2); /* see XDDRAW.C - xd_set_userobjects */
#endif
	object->ob_spec.userblk = (USERBLK *)userblk;
}


/* 
 * Funktie die rechthoek om object bepaalt 
 */

void xd_objrect(OBJECT *tree, _WORD object, RECT *r)
{
	OBJECT *obj = &tree[object];

	objc_offset(tree, object, &r->x, &r->y);
	r->w = obj->ob_width;
	r->h = obj->ob_height;
}


/*
 * Get extended object type
 */

_WORD xd_xobtype(OBJECT *tree)
{
	return ((tree->ob_type >> 8) & 0xFF);
}

/* 
 * Funktie voor het bepalen van de parent van een object. 
 * find parent of object "object"
 */

_WORD xd_obj_parent(OBJECT *tree, _WORD object)
{
	_WORD i = tree[object].ob_next, j;

	while (i >= 0)
	{
		if ((j = tree[i].ob_head) >= 0)
		{
			do
			{
				if (j == object)
					return i;

				j = tree[j].ob_next;
			}
			while (j != i);
		}

		i = tree[i].ob_next;
	}

	return -1;
}


/* 
 * This function sets one of the radiobutton objects within
 * a parent object. The argument is the index of the button object
 * to be set/selected. 
 * If object == parent, then GET the index of the selected button.
 */

_WORD xd_set_rbutton(OBJECT *tree, _WORD rb_parent, _WORD object)
{
	OBJECT
		*obj;

	_WORD
		i = tree[rb_parent].ob_head;	/* first child of parent */


	while ((i > 0) && (i != rb_parent))	/* until last child */
	{
		obj = &tree[i];

		if (obj->ob_flags & OF_RBUTTON)	/* watch radiobuttons only */
		{
			if(object == rb_parent)
			{
				if ((obj->ob_state & OS_SELECTED) && (obj->ob_flags & OF_RBUTTON))
					return i;
			}
			else
			{
				if (i == object)
					obj->ob_state |= OS_SELECTED;	/* select this one */
				else
					obj->ob_state &= ~OS_SELECTED;	/* deselect others */
			}
		}

		i = obj->ob_next;
	}

	return -1;
}


/* 
 * Funktie voor het bepalen van de geselekteerde radiobutton 
 * This function returns the index of the selected button object
 */

_WORD xd_get_rbutton(OBJECT *tree, _WORD rb_parent)
{
	return xd_set_rbutton(tree, rb_parent, rb_parent);
}


/*
 * Enable or disable all children of the parent
 */

void xd_set_child(OBJECT *tree, _WORD rb_parent, _WORD enab)
{
	OBJECT
		*obj;

	_WORD
		i = tree[rb_parent].ob_head;	/* first child of parent */


	while ((i > 0) && (i != rb_parent))	/* until last child */
	{
		obj = &tree[i];

		if (enab)
			obj->ob_state &= ~OS_DISABLED;	/* enable it */
		else
			obj->ob_state |= OS_DISABLED;	/* disable it */

		i = obj->ob_next;
	}
}


/*
 * Funktie voor het verkrijgen van de 'ob_spec': werkt voor normale
 * zowel als USERDEF als XUSERDEF objecten!
 * Similar to earlier xd_get_obspec(), but returns a pointer only 
 */

OBSPEC *xd_get_obspecp(OBJECT *object)
{
	if ((object->ob_type & 0xFF) == G_USERDEF)
	{
		USERBLK *userblk = object->ob_spec.userblk;

		if (IS_XUSER(userblk))
			return &(((XUSERBLK *)userblk)->ob_spec);
		else
			return (OBSPEC *)&userblk->ub_parm;
	}
	else
		return &(object->ob_spec);
}


/*
 * Return the pointer to the validtion string of an editable field
 */

char *xd_pvalid(OBJECT *object)
{
	return xd_get_obspecp(object)->tedinfo->te_pvalid;
}


/*
 * Funktie voor het zetten van de 'ob_spec': werkt voor normale zowel
 * als USERDEF als XUSERDEF objecten!
 */

void xd_set_obspec(OBJECT *object, OBSPEC *obspec)
{
	if ((object->ob_type & 0xFF) == G_USERDEF)
	{
		USERBLK *userblk = object->ob_spec.userblk;

		if (IS_XUSER(userblk))
			((XUSERBLK *)userblk)->ob_spec = *obspec;
		else
			userblk->ub_parm = *(long *)obspec;
	}
	else
		object->ob_spec = *obspec;
}


#if 0 /* There are currently no tristate objects in TeraDesk */

/* 
 * Tristate-button functions...
 */

_WORD xd_get_tristate(_WORD ob_state)
{
	return ob_state & TRISTATE_MASK;
}

_WORD xd_set_tristate(_WORD ob_state, _WORD state)
{
	return (ob_state & ~TRISTATE_MASK) | (state&0xff);
}

_WORD xd_is_tristate(OBJECT *object)
{
	return(xd_xobtype(object) == XD_RECBUTTRI);
}

#endif


/*
 * Funktie voor het aanzetten van clipping.
 *
 * Parameter:
 *
 * r	- clipping rechthoek.
 */

void xd_clip_on(RECT *r)
{
	_WORD
		pxy[4];

	xd_rect2pxy(r, pxy);
	vs_clip(xd_vhandle, 1, pxy);
}


/*
 * Funktie voor het uitschakelen van clipping.
 */

void xd_clip_off(void)
{
	_WORD
		pxy[4];

	vs_clip(xd_vhandle, 0, pxy);
}


/*
 * Obtain font size with fewer arguments
 */

_WORD xd_vst_point(_WORD height, _WORD *ch)
{
	_WORD
		dummy;

	return vst_point(xd_vhandle, height, &dummy, &dummy, &dummy, ch);
}


_WORD xd_fnt_point(_WORD height, _WORD *cw, _WORD *ch)
{
	_WORD
		dummy,
		r;

	r = xd_vst_point(height, ch);
	vqt_width(xd_vhandle, ' ', cw, &dummy, &dummy);
	return r;
}
