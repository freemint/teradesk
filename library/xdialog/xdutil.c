/*
 * Xdialog Library. Copyright (c) 1993, 1994, 2002 W. Klaren.
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
 #include <aes.h>
 #include <vdi.h>
#else
 #include <aesbind.h>
 #include <vdibind.h>
#endif

#include <stddef.h>

#include "xdialog.h"
#include "internal.h"

/* Funktie voor het omzetten van een GRECT structuur naar een pxy
   array. */

void xd_rect2pxy(GRECT *r, int *pxy)
{
	pxy[0] = r->g_x;
	pxy[1] = r->g_y;
	pxy[2] = r->g_x + r->g_w - 1;
	pxy[3] = r->g_y + r->g_h - 1;
}

/* Funktie voor het berekenen van de doorsnede van twee rechthoeken. */

int xd_rcintersect(GRECT *r1, GRECT *r2, GRECT *dest)
{
	int xmin, xmax, ymin, ymax, h1, h2;

	xmin = max(r1->g_x, r2->g_x);
	ymin = max(r1->g_y, r2->g_y);

	h1 = r1->g_x + r1->g_w - 1;
	h2 = r2->g_x + r2->g_w - 1;
	xmax = min(h1, h2);

	h1 = r1->g_y + r1->g_h - 1;
	h2 = r2->g_y + r2->g_h - 1;
	ymax = min(h1, h2);

	dest->g_x = xmin;
	dest->g_y = ymin;
	dest->g_w = xmax - xmin + 1;
	dest->g_h = ymax - ymin + 1;

	if ((dest->g_w <= 0) || (dest->g_h <= 0))
		return FALSE;

	return TRUE;
}

/*
 * Funktie die bepaalt of een bepaald punt binnen een gegeven
 * rechthoek ligt.
 *
 * Parmeters:
 *
 * x	- x coordinaat punt,
 * y	- y coordinaat punt,
 * r	- rechthoek.
 *
 * Resultaat : TRUE als het punt binnen de rechthoek ligt, FALSE
 *			   als dit niet het geval is.
 */

int xd_inrect(int x, int y, GRECT *r)
{
	if ((x >= r->g_x) && (x < (r->g_x + r->g_w)) && (y >= r->g_y) && (y < (r->g_y + r->g_h)))
		return TRUE;
	else
		return FALSE;
}

/* Funktie voor het berekenen van de grootte van de schermbuffer. */

long xd_initmfdb(GRECT *r, MFDB *mfdb)
{
	long size;

	mfdb->fd_w = (r->g_w + 16) & 0xFFF0;
	mfdb->fd_h = r->g_h;
	mfdb->fd_wdwidth = mfdb->fd_w / 16;
	mfdb->fd_stand = 0;
	mfdb->fd_nplanes = xd_nplanes;

	size = ((long) (mfdb->fd_w) * (long) r->g_h * (long) xd_nplanes) / 8L;

	return size;
}

/* Funktie voor het installeren van user-defined objects. */

void xd_userdef(OBJECT *object, USERBLK *userblk, int cdecl(*code) (PARMBLK *parmblock))
{
	object->ob_type = (object->ob_type & 0xFF00) | G_USERDEF;
	userblk->ub_code = code;
	userblk->ub_parm = object->ob_spec.index;
	object->ob_spec.userblk = userblk;
}

/* Funktie voor het installeren van user-defined objects. */

void xd_xuserdef(OBJECT *object, XUSERBLK *userblk, int cdecl(*code) (PARMBLK *parmblock))
{
	userblk->ub_code = code;
	userblk->ub_parm = userblk;
	userblk->ob_type = object->ob_type;
	userblk->ob_flags = object->ob_flags;
	userblk->ob_spec = object->ob_spec.index;

	object->ob_type = (object->ob_type & 0xFF00) | G_USERDEF;
	object->ob_flags &= ~(AES3D_1 | AES3D_2);
	object->ob_spec.userblk = (USERBLK *)userblk;
}

/* Funktie die rechthoek om object bepaalt */

void xd_objrect(OBJECT *tree, int object, GRECT *r)
{
	OBJECT *obj = &tree[object];

	objc_offset(tree, object, &r->g_x, &r->g_y);
	r->g_w = obj->ob_width;
	r->g_h = obj->ob_height;
}

/* Funktie voor het bepalen van de parent van een object. */

int xd_obj_parent(OBJECT *tree, int object)
{
	int i = tree[object].ob_next, j;

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

/* Funktie voor het bepalen van de geselekteerde radiobutton */

int xd_get_rbutton(OBJECT *tree, int rb_parent)
{
	int i = tree[rb_parent].ob_head;
	OBJECT *obj;

	while ((i > 0) && (i != rb_parent))
	{
		obj = &tree[i];

		if ((obj->ob_state & SELECTED) && (obj->ob_flags & RBUTTON))
			return i;

		i = obj->ob_next;
	}

	return -1;
}

/* Funktie voor het bepalen van de geselekteerde radiobutton */

void xd_set_rbutton(OBJECT *tree, int rb_parent, int object)
{
	int i = tree[rb_parent].ob_head;
	OBJECT *obj;

	while ((i > 0) && (i != rb_parent))
	{
		obj = &tree[i];

		if (obj->ob_flags & RBUTTON)
		{
			if (i == object)
				obj->ob_state |= SELECTED;
			else
				obj->ob_state &= ~SELECTED;
		}

		i = obj->ob_next;
	}
}

/*
 * Funktie voor het verkrijgen van de 'ob_spec': werkt voor normale
 * zowel als USERDEF als XUSERDEF objecten!
 */

long xd_get_obspec(OBJECT *object)
{
	if ((object->ob_type & 0xFF) == G_USERDEF)
	{
		USERBLK *userblk = object->ob_spec.userblk;

		if (IS_XUSER(userblk))
			return ((XUSERBLK *)userblk)->ob_spec;
		else
			return userblk->ub_parm;
	}
	else
		return object->ob_spec.index;
}

/*
 * Funktie voor het zetten van de 'ob_spec': werkt voor normale zowel
 * als USERDEF als XUSERDEF objecten!
 */

void xd_set_obspec(OBJECT *object, long obspec)
{
	if ((object->ob_type & 0xFF) == G_USERDEF)
	{
		USERBLK *userblk = object->ob_spec.userblk;

		if (IS_XUSER(userblk))
			((XUSERBLK *)userblk)->ob_spec = obspec;
		else
			userblk->ub_parm = obspec;
	}
	else
		object->ob_spec.index = obspec;
}

/* tristate-button functions... */

int xd_get_tristate(int ob_state)
{
	return ob_state & TRISTATE_MASK;
}

int xd_set_tristate(int ob_state, int state)
{
	return (ob_state & ~TRISTATE_MASK) | state;
}

int xd_is_tristate(OBJECT *object)
{
	return ((object->ob_type >> 8) & 0xFF) == XD_RECTBUTTRI;
}

/*
 * Funktie voor het aanzetten van clipping.
 *
 * Parameter:
 *
 * r	- clipping rechthoek.
 */

void xd_clip_on(GRECT *r)
{
	int pxy[4];

	xd_rect2pxy(r, pxy);
	vs_clip(xd_vhandle, 1, pxy);
}

/*
 * Funktie voor het uitschakelen van clipping.
 */

void xd_clip_off(void)
{
	int pxy[4];

	vs_clip(xd_vhandle, 0, pxy);
}
