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
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>
#include <internal.h>
#include <library.h>

#include "resource.h"
#include "sprintf.h"
#include "desk.h"
#include "error.h"
#include "events.h"
#include "open.h"
#include "file.h"		/* moved up */
#include "lists.h" 
#include "slider.h"
#include "xfilesys.h"
#include "copy.h"
#include "config.h"	
#include "font.h"
#include "window.h" 	/* before dir.h and viewer.h */
#include "showinfo.h"
#include "dir.h"
#include "icon.h"
#include "icontype.h"
#include "screen.h"
#include "viewer.h"
#include "video.h"
#include "filetype.h" /* must be before applik.h */
#include "applik.h"

typedef enum
{
	ICN_NO_UPDATE,
	ICN_REDRAW,
	ICN_DELETE
} icn_upd_type;

typedef struct
{
	int x;
	int y;
	ITMTYPE item_type;
	ITMTYPE tgt_type;
	boolean link;
	int icon_index;
	boolean selected;
	boolean newstate;
	INAME label; /* label of assigned icon */ 
	union
	{
		int drv;
		const char *name;
	} icon_dat;
	icn_upd_type update;
	INAME icon_name;	/* name of icon in resource */ 
} ICON;


typedef struct
{
	ICON ic;
	LNAME name;
} ICONINFO;


typedef struct
{
	ITM_INTVARS;
} DSK_WINDOW;

static ICON *desk_icons;
OBJECT *desktop;
static int icon_width, icon_height, m_icnx, m_icny, max_icons;
boolean noicons = FALSE;

extern int sl_noop, sl_abobutt;

static int dsk_hndlkey(WINDOW *w, int dummy_scancode, int dummy_keystate);
static void dsk_hndlbutton(WINDOW *w, int x, int y, int n, int button_state, int keystate);
static void dsk_top(WINDOW *w);
int arrow_form_do ( XDINFO *info, int *oldbutton ); 


static WD_FUNC dsk_functions =
{
	dsk_hndlkey,
	wd_hndlbutton,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L,
	0L 
};

static int icn_find(WINDOW *w, int x, int y);
static boolean icn_state(WINDOW *w, int icon);
static ITMTYPE icn_type(WINDOW *w, int icon);
static ITMTYPE icn_tgttype(WINDOW *w, int icon);
static int icn_icon(WINDOW *w, int icon);
static char *icn_name(WINDOW *w, int icon);
static char *icn_fullname(WINDOW *w, int icon);
static int icn_attrib(WINDOW *w, int icon, int mode, XATTR *attrib);
static boolean icn_islink(WINDOW *w, int icon);
static boolean icn_open(WINDOW *w, int item, int kstate);
static boolean icn_copy(WINDOW *dw, int dobject, WINDOW *sw, int n, int *list, ICND *icns, int x, int y, int kstate);
static boolean icn_userdef(WINDOW *dw, int dobject, WINDOW *sw, int n, int *list, int kstate);
static void icn_select(WINDOW *w, int selected, int mode, boolean draw);
static void icn_rselect(WINDOW *w, int x, int y);
static void icn_nselected(WINDOW *w, int *n, int *sel);
static boolean icn_xlist(WINDOW *w, int *nselected, int *nvisible, int **sel_list, ICND **icnlist, int mx, int my);
static boolean icn_list(WINDOW *w, int *nselected, int **sel_list);
static void icn_set_update(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);
static void icn_do_update(WINDOW *w);

static ITMFUNC itm_func =
{
	icn_find,					/* itm_find */
	icn_state,					/* itm_state */
	icn_type,					/* itm_type */
	icn_tgttype,				/* target object type */
	icn_icon,					/* itm_icon */
	icn_name,					/* itm_name */
	icn_fullname,				/* itm_fullname */
	icn_attrib,					/* itm_attrib */
	icn_islink,					/* itm_islink */
	icn_open,					/* itm_open */
	icn_copy,					/* itm_copy */
	item_showinfo,				/* itm_showinfo */
	icn_select,					/* itm_select */
	icn_rselect,				/* itm_rselect */
	icn_xlist,					/* itm_xlist */
	icn_list,					/* itm_list */
	0L,							/* wd_path */
	icn_set_update,				/* wd_set_update */
	icn_do_update,				/* wd_do_update */
	0L							/* wd_seticons */
};

void *icon_buffer;

OBJECT *icons;	/* use rsrc_load for icons. */
int *icon_data, n_icons;
WINDOW *desk_window;

extern int aes_version;

INAME iname;


/*
 * Mouse form (like a small icon) for placing a desktop icon
 */

MFORM icnm =
{8, 8, 1, 1, 0,
 0x1FF8, 0x1008, 0x1008, 0x1008, 0x1008, 0x1008, 0x1008, 0x1008,
 0x1008, 0x1008, 0x1008, 0x1008, 0xF00F, 0x8001, 0x8001, 0xFFFF,
 0x0000, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0,
 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x7FFE, 0x7FFE, 0x0000
};


/*
 * Funktie voor het opnieuw tekenen van de desktop.
 */

static void form_dialall(int what)
{
	form_dial(what, 0, 0, 0, 0, 
	screen_info.dsk.x, 
	screen_info.dsk.y, 
	screen_info.dsk.w, 
	screen_info.dsk.h);
}


void dsk_draw(void)
{
	form_dialall(FMD_START);
	form_dialall(FMD_FINISH);
}


/*
 * Determine whether an icon item is a file/folder/program/link or not
 */

boolean isfile(ITMTYPE type)
{
	if ((type == ITM_FOLDER) || (type == ITM_FILE) || (type == ITM_PROGRAM) || (type == ITM_LINK) )
		return TRUE;

	return FALSE;
}


/* 
 * Redraw icons which may be partially obscured
 * by other objects
 */

void draw_icrects( WINDOW *w, OBJECT *tree, RECT *r1)
{
	RECT r2, d;

	xw_getfirst(w, &r2);
	while (r2.w != 0 && r2.h != 0)
	{
		if (xd_rcintersect(r1, &r2, &d))
			objc_draw(tree, ROOT, MAX_DEPTH, d.x, d.y, d.w, d.h);
		xw_getnext(w, &r2);
	}
}


#define WF_OWNER	20

/*
 * Redraw part of the desktop.
 */

static void redraw_desk(RECT *r1)
{
	int owner;
	boolean redraw = TRUE;

	wind_update(BEG_UPDATE);

	/* In newer (multitasking) systems, redraw only what is owned by the desktop ? */

	/* Should this perhaps be 399 or 340 instead of 400 ? (i.e. Magic & Falcon TOS) */

	if ( aes_version >= 0x399)
	{
		xw_get(NULL, WF_OWNER, &owner);
		if (ap_id != owner)
			redraw = FALSE;
	}

	/* Actually redraw... */

	if (redraw)
		draw_icrects( NULL, desktop, r1);

	wind_update(END_UPDATE);
}


/* 
 * Wis een icoon van het beeldscherm. Deze routine wist
 * niet de informatie van het icoon. Deze funktie is nodig als
 * een icoon veranderd is. 
 * This routine in fact -hides- an icon object.
 */

static void erase_icon(int object)
{
	RECT r;
	int o1 = object + 1;

	xd_objrect(desktop, o1, &r);
	desktop[o1].ob_flags = HIDETREE;
	redraw_desk(&r);
	desktop[o1].ob_flags = NORMAL;
}


/*
 * Draw an icon (i.e. draw part of the desk within the rectangle)
 */

static void draw_icon(int object)
{
	RECT r;

	/* Find location of the required icon, then draw it */

	xd_objrect(desktop, object + 1, &r);
	redraw_desk(&r);
}


/*
 * Combine the two above.
 * Note: currently, this will redraw the rectangle twice!
 */

static void redraw_icon(int object)
{
	erase_icon(object);
	draw_icon(object);
}


/* 
 * routines voor selecteren/deselecteren van iconen
 * mode = 0 : deselect all, select selected
 *        1 : toggle selected
 *        2 : deselect selected
 *        3 : select selected
 */ 

static void dsk_setnws(void)
{
	int i;
	ICON *icn;

	for (i = 0; i < max_icons; i++)
	{
		icn = &desk_icons[i];
		if (icn->item_type != ITM_NOTUSED)
			icn->selected = icn->newstate;
	}
}


/*
 * Draw/redraw selected or deselected icons, i.e. those the state of which 
 * has changed. In order to improve speed, a summary surrounding rectangle
 * for all changed icons is computed and then all icons are redrawn
 * by one call to objc_draw, without redrawing the background. 
 * In case of colour icons, which may be animated and change their outline,
 * this is not satisfactory. In that case, draw each icon separately
 * including the background- which is noticeably slower.
 */

static void dsk_drawsel(void)
{
	int 
		i;						/* object counter */ 

	RECT 
		c, 						/* icon object rectangle */
		r = {-1, -1, -1, -1};	/* summary rectangle */

	ICON *icn;

	desktop[0].ob_type = G_IBOX;

	for (i = 0; i < max_icons; i++)
	{
		icn = &desk_icons[i];

		if (icn->item_type != ITM_NOTUSED)
		{
			if (icn->selected == icn->newstate)
				/* State has not changed; hide the icon temporarily */

				obj_hide(desktop[i + 1]);

			else
			{
				/* State has changed; draw the icon */

				desktop[i + 1].ob_state = (icn->newstate) ? SELECTED : NORMAL;
				xd_objrect(desktop, i + 1, &c);

				if ( colour_icons )
				{
					/* For colour icons draw each icon separately, with background */

					desktop[0].ob_type = G_BOX;
					redraw_desk(&c);
				}
				else
				{
					/* For monochrome, update surrounding rectangle for all icons */

					if ((c.x < r.x) || (r.x < 0)) /* left edge minimum  */
						r.x = c.x;
					if ((c.y < r.y) || (r.y < 0)) /* upper edge minimum */
						r.y = c.y;
					c.x += desktop[i + 1].r.w - 1;
					c.y += desktop[i + 1].r.h - 1;
					if (c.x > r.w)                /* right edge maximum */
						r.w = c.x;
					if (c.y > r.h)                /* lower edge maximum */
						r.h = c.y;
				}

			}
		}
	}

	/* This will happen only for mono icons */

	if ( r.x >= 0 )
	{
		r.w -= r.x + 1;
		r.h -= r.y + 1;

		redraw_desk(&r);
	}

	/* Set temporarily hidden icons to visible again */

	for (i = 0; i < max_icons; i++)
		if (desk_icons[i].selected == desk_icons[i].newstate)
			obj_unhide(desktop[i + 1]);

	desktop[0].ob_type = G_BOX;
}


/*
 * Compute rubberbox rectangle. 
 * Beware of unsymmetrical wind_update()
 */

void rubber_rect(int x1, int x2, int y1, int y2, RECT *r)
{
	int h;

	arrow_mouse();
	wind_update(END_MCTRL);

	if ((h = x2 - x1) < 0)
	{
		r->x = x2;
		r->w = -h + 1;
	}
	else
	{
		r->x = x1;
		r->w = h + 1;
	}

	if ((h = y2 - y1) < 0)
	{
		r->y = y2;
		r->h = -h + 1;
	}
	else
	{
		r->y = y1;
		r->h = h + 1;
	}
}


/*
 * Rubberbox function 
 */

static void rubber_box(int x, int y, RECT *r)
{
	int 
		x1 = x, 
		y1 = y, 
		x2 = x, 
		y2 = y, 
		ox = x, 
		oy = y, 
		kstate;

	boolean 
		released;

	set_rect_default();

	wind_update(BEG_MCTRL);
	graf_mouse(POINT_HAND, NULL);

	draw_rect(x1, y1, x2, y2);

	/* Loop until mouse button is released */

	do
	{
		released = xe_mouse_event(0, &x2, &y2, &kstate);

		/* Menu bar is inaccessible */

		if (y2 < screen_info.dsk.y)
			y2 = screen_info.dsk.y;

		if (released || (x2 != ox) || (y2 != oy))
		{
			draw_rect(x1, y1, ox, oy);
			if (!released)
			{
				draw_rect(x1, y1, x2, y2);
				ox = x2;
				oy = y2;
			}
		}
	}
	while (!released);

	rubber_rect(x1, x2, y1, y2, r);
}


/*
 * Externe funkties voor iconen.
 */

static void icn_select(WINDOW *w, int selected, int mode, boolean draw)
{
	int i;
	boolean *newstate, *iselected;

	/* Newstates after the switch below is equal to selection state of all icons */

	for (i = 0; i < max_icons; i++)
	{
		newstate = &desk_icons[i].newstate;
		iselected = &desk_icons[i].selected;

		if (desk_icons[i].item_type != ITM_NOTUSED)
		{
			switch(mode)
			{
				case 0:
					*newstate = (i == selected) ? TRUE : FALSE;
					break;
				case 1:
					*newstate = *iselected;
					break;
				case 2:
					*newstate = (i == selected) ? FALSE : *iselected;
					break;
				case 3:
					*newstate = (i == selected) ? TRUE : *iselected;
					break;
				case 4:
					*newstate = TRUE;
					break;
			}
		}
	}

	if (mode == 1)
		desk_icons[selected].newstate = (desk_icons[selected].selected) ? FALSE : TRUE;

	if (draw)
		dsk_drawsel();

	dsk_setnws();
}


static void icn_rselect(WINDOW *w, int x, int y)
{
	int i;
	RECT r1, r2;
	ICONBLK *monoblk;
	boolean *newstate, sel;
	OBJECT *deskto;

	rubber_box(x, y, &r1);

	for (i = 0; i < max_icons; i++)
	{
		newstate = &(desk_icons[i].newstate);
		sel = desk_icons[i].selected;
		deskto = &desktop[i + 1];

		if (desk_icons[i].item_type != ITM_NOTUSED)
		{
			monoblk = &(deskto->ob_spec.ciconblk->monoblk);

			r2.x = desktop[0].r.x + deskto->r.x + monoblk->ic.x;
			r2.y = desktop[0].r.y + deskto->r.y + monoblk->ic.y;
			r2.w = monoblk->ic.w;
			r2.h = monoblk->ic.h;

			if (rc_intersect2(&r1, &r2))
				*newstate = (sel) ? FALSE : TRUE;
			else
			{
				r2.x = desktop[0].r.x + deskto->r.x + monoblk->tx.x;
				r2.y = desktop[0].r.y + deskto->r.y + monoblk->tx.y;
				r2.w = monoblk->tx.w;
				r2.h = monoblk->tx.h;
				if (rc_intersect2(&r1, &r2))
					*newstate = (sel) ? FALSE : TRUE;
				else
					*newstate = sel;
			}
		}
	}
	dsk_drawsel();
	dsk_setnws();
}


static void icn_nselected(WINDOW *w, int *n, int *sel)
{
	int i, s, c = 0;

	for (i = 0; i < max_icons; i++)
	{
		if ((desk_icons[i].item_type != ITM_NOTUSED) && desk_icons[i].selected )
		{
			c++;
			s = i;
		}
	}

	*n = c;
	*sel = (c == 1) ? s : -1;
}


/*
 * Note: WINDOW *w parameter is not used in the following functions 
 * but must exist because these are ITMFUNCs
 */

static boolean icn_state(WINDOW *w, int icon)
{
	return desk_icons[icon].selected;
}


static ITMTYPE icn_type(WINDOW *w, int icon)
{
	return desk_icons[icon].item_type;
}


static ITMTYPE icn_tgttype(WINDOW *w, int icon)
{
	return desk_icons[icon].tgt_type;
}


static int icn_icon(WINDOW *w, int icon)
{
	return desk_icons[icon].icon_index;
}


/*
 * Return pointer to the name of a (desktop) object
 */

static char *icn_name(WINDOW *w, int icon)
{
	ICON *icn = &desk_icons[icon];

	static char name[16]; /* must exist after return from this function */

	if (isfile(icn->item_type))
		/* this is a file, folder, program or link- treat as a file(name) */
		return fn_get_name(icn->icon_dat.name);
	else
	{
		if (icn->item_type == ITM_DRIVE)
		{
			/* this is a disk volume. Return text "drive X" */

			sprintf(name, get_freestring(MDRVNAME), (char)icn->icon_dat.drv);
			return name;
		}
		else
			return NULL;
	}
}


/*
 * Get fullname of an object to which the icon has been assigned.
 * When an item can not have a name, return NULL
 * (i.e. only files, folders, programs and disk volumes can have fullnames) 
 */

static char *icn_fullname(WINDOW *w, int icon)
{
	ICON *icn = &desk_icons[icon];
	char *s;

	if (!isfile(icn->item_type) && (icn->item_type != ITM_DRIVE))
		return NULL;

	if (icn->item_type == ITM_DRIVE)
	{
		if ((s = strdup(adrive)) != NULL)
			s[0] = icn->icon_dat.drv;
	}
	else
		s = strdup(icn->icon_dat.name);

	return s;
}


/*
 * Get attributes of an object to which the icon has been assigned
 */

static int icn_attrib(WINDOW *w, int icon, int mode, XATTR *attr)
{
	int error;
	ICON *icn = &desk_icons[icon];

	if (isfile(icn->item_type))
	{
		error = (int) x_attr(mode, icn->icon_dat.name, attr);
		return error;
	}
	else
		return 0;
}


/*
 * Does a desktop icon represent a link?
 */

static boolean icn_islink(WINDOW *w, int icon)
{
	return desk_icons[icon].link;
}


static void get_icnd(int object, ICND *icnd, int mx, int my)
{
	int tx, ty, tw, th, ix, iy, iw, obj = object + 1;

	ICONBLK *h;		

	icnd->item = object;
	icnd->np = 9;
	icnd->m_x = desktop[obj].r.x + desktop[obj].r.w / 2 - mx + screen_info.dsk.x;
	icnd->m_y = desktop[obj].r.y + desktop[obj].r.h / 2 - my + screen_info.dsk.y;
	h = &(desktop[obj].ob_spec.ciconblk->monoblk);
	tx = desktop[obj].r.x + h->tx.x - mx + screen_info.dsk.x;
	ty = desktop[obj].r.y + h->tx.y - my + screen_info.dsk.y;
	tw = h->tx.w - 1;
	th = h->tx.h - 1;
	ix = desktop[obj].r.x + h->ic.x - mx + screen_info.dsk.x;
	iy = desktop[obj].r.y + h->ic.y - my + screen_info.dsk.y;
	iw = h->ic.w - 1;

	icnd->coords[0] = tx;
	icnd->coords[1] = ty;
	icnd->coords[2] = ix;
	icnd->coords[3] = ty;
	icnd->coords[4] = ix;
	icnd->coords[5] = iy;
	icnd->coords[6] = ix + iw;
	icnd->coords[7] = iy;
	icnd->coords[8] = ix + iw;
	icnd->coords[9] = ty;
	icnd->coords[10] = tx + tw;
	icnd->coords[11] = ty;
	icnd->coords[12] = tx + tw;
	icnd->coords[13] = ty + th;
	icnd->coords[14] = tx;
	icnd->coords[15] = ty + th;
	icnd->coords[16] = tx;
	icnd->coords[17] = ty + 1;
}


/*
 * Create a list of selected icon items.
 * This routine will return FALSE and a NULL list pointer
 * if no list has been created
 */

static boolean icn_list(WINDOW *w, int *nselected, int **sel_list)
{
	int i, j = 0, n = 0, *list;

	for (i = 0; i < max_icons; i++)
		if ((desk_icons[i].item_type != ITM_NOTUSED) && desk_icons[i].selected )
			n++;

	if ((*nselected = n) == 0)
	{
		*sel_list = NULL;
		return FALSE;
	}

	list = malloc_chk((long) n * sizeof(int));

	*sel_list = list;

	if (list)
	{
		for (i = 0; i < max_icons; i++)
			if ((desk_icons[i].item_type != ITM_NOTUSED) && desk_icons[i].selected )
				list[j++] = i;

		return TRUE;
	}

	return FALSE;	
}


/* 
 * Routine voor het maken van een lijst met geseleketeerde iconen. 
 */

static boolean icn_xlist(WINDOW *w, int *nsel, int *nvis, int **sel_list, ICND **icns, int mx, int my)
{
	int i, *list, n;
	ICND *icnlist;

	if (!icn_list(desk_window, nsel, sel_list))
		return FALSE;

	n = *nsel;
	*nvis = n;

	if (n == 0)
	{
		*icns = NULL;
		return TRUE;
	}

	icnlist = malloc_chk((long)n * sizeof(ICND));
	*icns = icnlist;

	if (icnlist)
	{
		list = *sel_list;

		for (i = 0; i < n; i++)
			get_icnd(list[i], &icnlist[i], mx, my);

		return TRUE;
	}

	free(*sel_list);
	return FALSE;

}


static int icn_find(WINDOW *w, int x, int y)
{
	int ox, oy, ox2, oy2, i, object = -1;
	ICONBLK *p;	
	OBJECT *de;

	objc_offset(desktop, 0, &ox, &oy);

	if ((i = desktop[0].ob_head) == -1)
		return -1;

	while (i != 0)
	{
		de = &desktop[i];
		if (de->ob_type == G_ICON || de->ob_type == G_CICON)
		{
			RECT h;

			ox2 = ox + de->r.x;
			oy2 = oy + de->r.y;
			p = &(de->ob_spec.ciconblk->monoblk);

			h.x = ox2 + p->ic.x;
			h.y = oy2 + p->ic.y;
			h.w = p->ic.w;
			h.h = p->ic.h;

			if (inrect(x, y, &h))
				object = i - 1;
			else
			{
				h.x = ox2 + p->tx.x;
				h.y = oy2 + p->tx.y;
				h.w = p->tx.w;
				h.h = p->tx.h;

				if (inrect(x, y, &h))
					object = i - 1;
			}
		}
		i = de->ob_next;
	}
	return object;
}

/********************************************************************
 *																	*
 * Funkties voor het verversen van de desktop.						*
 *																	*
 ********************************************************************/

/*
 * Remove a desktop icon
 */

void remove_icon(int object, boolean draw)
{
	ITMTYPE type = icn_type(desk_window, object);

	if (draw)
		erase_icon(object);

	objc_delete(desktop, object + 1);

	free(desktop[object + 1].ob_spec.ciconblk);	
	if (isfile(type))
		free(desk_icons[object].icon_dat.name);

	desk_icons[object].item_type = ITM_NOTUSED;
}


/*
 * Set icon name (label)
 */

static void icn_set_name(ICON *icn, char *newiname)
{
	if (strncmp(icn->label, fn_get_name(icn->icon_dat.name), 12) == 0)
		strsncpy(icn->label, fn_get_name(newiname), sizeof(icn->label));
	free(icn->icon_dat.name);
	icn->icon_dat.name = newiname;
}


static void icn_set_update(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2)
{
	int i;
	ICON *icon;
	char *new;

	for (i = 0; i < max_icons; i++)
	{
		icon = &desk_icons[i];

		if (isfile(icon->item_type))
		{
			if (strcmp(icon->icon_dat.name, fname1) == 0)
			{
				if (type == WD_UPD_DELETED)
					icon->update = ICN_DELETE;

				if (type == WD_UPD_MOVED)
				{
					if (strcmp(fname2, icon->icon_dat.name) != 0)
					{
						if ((new = strdup(fname2)) == NULL)
							return;
						else
						{
							icon->update = ICN_REDRAW;
							icn_set_name(icon, new);
						}
					}
				}
			}
		}
	}
}


static void icn_do_update(WINDOW *w)
{
	int i;

	ICON *icn;

	for (i = 0; i < max_icons; i++)
	{
		icn = &desk_icons[i];

		if (icn->item_type != ITM_NOTUSED)
		{
			if (icn->update == ICN_DELETE)
				remove_icon(i, TRUE);

			if (icn->update == ICN_REDRAW)
			{
				redraw_icon(i);
				icn->update = ICN_NO_UPDATE;
			}
		}
	}
}


/********************************************************************
 *																	*
 * Funktie voor het openen van iconen.								*
 *																	*
 ********************************************************************/

static boolean icn_open(WINDOW *w, int item, int kstate)
{
	ICON *icn = &desk_icons[item];
	ITMTYPE type = icn->item_type;

	if (isfile(type))
	{
		char *new;
		int button;

		if (x_exist(icn->icon_dat.name, EX_FILE | EX_DIR) == FALSE)
		{
			if ((button = alert_printf(1, AICNNFND, icn->label)) == 3)
				return FALSE;

			if (button == 2)
			{
				remove_icon(item, TRUE);
				return FALSE;
			}
			else if ((new = locate(icn->icon_dat.name, (type == ITM_FOLDER) ? L_FOLDER : L_FILE)) == NULL)
				return FALSE;
			else
			{
				icn_set_name(icn, new);
				redraw_icon(item);
			}
		}
	}

	return item_open(w, item, kstate, NULL, NULL);
}


/********************************************************************
 *																	*
 * Funkties voor het verwijderen en toevoegen van iconen.			*
 * HR 151102: locate icons in resource by name, not by index		*
 *																	*
 ********************************************************************/


/*
 * Provide a default icon index.
 * First try a default icon according to item type;
 * if that does not succeed, return index of the first icon.
 */

int default_icon(ITMTYPE type)
{
	int it, ic = 0;
	INAME name;

	switch(type)
	{
		case ITM_FOLDER:
		case ITM_PREVDIR:
			it = FOINAME;
			break;
		case ITM_PROGRAM:
			it = APINAME;
			break;
		case ITM_LINK:
		case ITM_FILE:
			it = FIINAME;
			break;
		case ITM_TRASH:
			it = TRINAME;
			break;
		case ITM_PRINTER:
			it = PRINAME;
		default:
			it = 0;
	}

	if (!noicons)
		alert_iprint(MICNFND);
	noicons = TRUE;

	if (type != 0)
		ic = rsrc_icon_rscid(it, (char *)&name); 

	return ic;
}


/*
 * Return index of an icon given by name
 */

int rsrc_icon(const char *name)
{
	int i = 0;
	CICONBLK *h;

	do
	{
		OBJECT *ic = icons + i;
		if (ic->ob_type == G_ICON || ic->ob_type == G_CICON)
		{
			h = ic->ob_spec.ciconblk;
			if (strnicmp(h->monoblk.ib_ptext, name, 12) == 0)
				return i;
		}
	}
	while ((icons[i++].ob_flags&LASTOB) == 0);

	return -1;
}


/* 
 * This routine gets icon id and name from language-dependent name 
 * as defined in desktop.rsc. 
 * The routine is used for some basic icon types which should
 * always exist in the icon resource file 
 * (floppy, disk, file, folder, etc)
 *  
 * If an icon is not found, an alert is issued and the first icon
 * from the resource file is used.
 *
 * 
 * id           = index of icon name string in DESKTOP.RSC (e.g. HDINAME, FINAME...)
 * name         = icon name from DESKTOP.RSC
 * return value = index of icon in ICONS.RSC 
 */

int rsrc_icon_rscid ( int id, char *name )
{
	char *nnn;
	int ic = -1;

	if ((nnn = get_freestring(id)) != NULL)
	{
		strcpy ( name, nnn );
		ic = rsrc_icon( name );
	}

	if ( ic < 0 )
		ic = default_icon(0);

	return ic;
}


/*
 * Add an icon onto the desktop
 */

static int add_icon(ITMTYPE type, ITMTYPE tgttype, boolean link, int icon, const char *text, 
                    int ch, int x, int y, boolean draw, const char *fname)
{
	int 
		i = 0,
		ix, iy, 
		icon_no;

	CICONBLK 
		*h;			/* ciconblk (the largest) */
	
	ICON *icn;
	OBJECT *deskto;

	if (icon < 0)
		return -1;

	icon_no = min(n_icons - 1, icon);

	while ((desk_icons[i].item_type != ITM_NOTUSED) && (i < max_icons - 1))
		i++;

	icn = &desk_icons[i];
	deskto = &desktop[i + 1];

	if (icn->item_type != ITM_NOTUSED)
	{
		alert_iprint(MTMICONS); 
		if (isfile(type))
			free(fname);

		return -1;
	}

	ix = min(x, m_icnx);
	iy = min(y, m_icny);

	icn->x = ix;
	icn->y = iy;
	icn->tgt_type = (tgttype == ITM_PROGRAM) ? ITM_FILE : type;
	icn->link = link;
	icn->item_type = (link) ? ITM_FILE : type;
	icn->icon_index = icon_no;
	icn->selected = FALSE;
	icn->update = ICN_NO_UPDATE;

	deskto->ob_head = -1;
	deskto->ob_tail = -1;
	deskto->ob_type = icons[icon_no].ob_type;
	deskto->ob_flags = NONE;
	deskto->ob_state = NORMAL;
	deskto->r.x = ix * icon_width;
	deskto->r.y = iy * icon_height;
	deskto->r.w = ICON_W;
	deskto->r.h = ICON_H;

	if ((h = malloc_chk(sizeof(CICONBLK))) == NULL)		/* ciconblk (the largest) */
	{
		icn->item_type = ITM_NOTUSED;
		if (isfile(type))
			free(fname);
		return -1;
	}

	deskto->ob_spec.ciconblk = h;
	*h = *icons[icon_no].ob_spec.ciconblk;

	strsncpy(icn->label, text, sizeof(INAME));	 
	strsncpy(icn->icon_name,
	        icons[icon_no].ob_spec.ciconblk->monoblk.ib_ptext,
	        sizeof(INAME));	

	h->monoblk.ib_char &= 0xFF00;

	switch (type) 
	{
	case ITM_DRIVE:
		icn->icon_dat.drv = ch;
		h->monoblk.ib_ptext = icn->label;
		h->monoblk.ib_char |= ch;
		break;
	case ITM_TRASH:
	case ITM_PRINTER:
		h->monoblk.ib_ptext = icn->label;
		h->monoblk.ib_char |= 0x20;
		break;
	case ITM_FOLDER:
	case ITM_PROGRAM:
	case ITM_FILE:
		icn->icon_dat.name = fname;
		h->monoblk.ib_ptext = icn->label;
		h->monoblk.ib_char |= 0x20;
		break;
	}

	objc_add(desktop, 0, i + 1);

	if (draw)
		draw_icon(i);

	return i;
}


/*
 * Compute (round-up) position of an icon on the desktop
 */

static void comp_icnxy(int mx, int my, int *x, int *y)
{
	/* Note: do not use min() here */

	*x = (mx - screen_info.dsk.x) / icon_width;
	if (*x > m_icnx)
		*x = m_icnx;
	*y = (my - screen_info.dsk.y) / icon_height;
	if (*y > m_icny)
		*y = m_icny;
}


static void get_iconpos(int *x, int *y)
{
	int dummy, mx, my;

	wind_update(BEG_MCTRL);
	graf_mouse(USER_DEF, &icnm);
	evnt_button(1, 1, 1, &mx, &my, &dummy, &dummy);
	arrow_mouse();
	wind_update(END_MCTRL);
	comp_icnxy(mx, my, x, y);
}


/* 
 * Get type of desktop icon (trashcan/printer/drive) from the dialog
 */

static ITMTYPE get_icntype(void)
{
	int object = xd_get_rbutton(addicon, ICBTNS);

	switch (object)
	{
		case ATRASH:
			return ITM_TRASH;
		case APRINTER:
			return ITM_PRINTER;
		default:
			return ITM_DRIVE;
	}
}


/*
 * Enter desk icon type (trash/printer/drive) into dialog (set radio button)
 */

static void set_icntype(ITMTYPE type)
{
	int object;

	switch (type)
	{
		case ITM_TRASH:
			object = ATRASH;
			break;
		case ITM_PRINTER:
			object = APRINTER;
			break;
		default:
			object = ADISK;
			break;
	}

	xd_set_rbutton(addicon, ICBTNS, object);
}


/*
 * Initiate the icon-selector slider
 */

void icn_sl_init(int line, SLIDER *sl)
{
	sl->type = 0;
	sl->up_arrow = ICNUP;
	sl->down_arrow = ICNDWN;
	sl->slider = ICSLIDER;	/* slider object                        */
	sl->sparent = ICPARENT;	/* slider parent (background) object    */
	sl->lines = 1;			/* number of visible lines in the box   */
	sl->n = n_icons;		/* number of items in the list          */
	sl->line = line;		/* index of first item shown in the box */
	sl->list = NULL;		/* pointer to list of items to be shown */
	sl->set_selector = set_iselector;
	sl->first = 0;			/* first object in the scrolled box     */
	sl->findsel = 0L;

	/* note: sl_init() could come here ... */
}


void set_iselector(SLIDER *slider, boolean draw, XDINFO *info)
{
	OBJECT *h1, *ic;

	ic = icons + slider->line;
	h1 = addicon + ICONDATA;

	h1->ob_type = ic->ob_type;
	h1->ob_spec = ic->ob_spec;

	/* Center the icon image in the background box */

	h1->r.x = (addicon[ICONBACK].r.w - ic->r.w) / 2;
	h1->r.y = (addicon[ICONBACK].r.h - ic->r.h) / 2;

	/* 
	 * In ST-med resolution, move the icon object a little bit upwards 
	 * (otherwise it goes out of the box- but why only in ST-med?)
	 */
 
	if ( currez == 1 )
		h1->r.y = h1->r.y - 8;

	if (draw)
		xd_draw(info, ICONBACK, 1);
}


static int icn_dialog(int *icon_no, int startobj)
{
	int button;
	SLIDER sl_info;

	icn_sl_init(*icon_no, &sl_info);

	button = sl_dialog(addicon, startobj, &sl_info);

	*icon_no = sl_info.line;

	return button;
}


/* 
 * Install a desktop icon ONLY for drives, printers and trashcan
 */

void dsk_insticon(void)
{
	int x, y, button, icon_no = 0;
	ITMTYPE type;

	get_iconpos(&x, &y);

	rsc_title(addicon, AITITLE, DTADDICN);

	obj_hide(addicon[CHNBUTT]);
	obj_hide(addicon[ICSHFIL]);
	obj_hide(addicon[ICSHFLD]);
	obj_hide(addicon[ICNTYPE]);
	obj_hide(addicon[ICNTYPT]);
	obj_unhide(addicon[ADDBUTT]);
	obj_unhide(addicon[ICBTNS ]);
	obj_unhide(addicon[DRIVEID]);
	obj_unhide(addicon[ICNLABEL]);

	set_icntype(ITM_DRIVE);
	addicon[DRIVEID].ob_spec.tedinfo->te_ptext[0] = 0;

	button = icn_dialog(&icon_no, DRIVEID);

	if (button == ADDICNOK)
	{
		type = get_icntype();
		if (type != ITM_DRIVE)
			*drvid = 0;
		add_icon(type, type, FALSE, icon_no, iconlabel, *drvid & 0xDF, x, y, TRUE, NULL);
	}
}


/*
 * Handle dialog for editing a desk icon 
 */

static int chng_icon(int object)
{
	int button = -1, icon_no;
	ITMTYPE type;
	ICON *icn = &desk_icons[object];
	int startobj;

	type = icn->item_type;

	rsc_title(addicon, AITITLE, DTCHNICN);

	obj_hide(addicon[ADDBUTT]);
	obj_hide(addicon[ICNTYPE]);
	obj_hide(addicon[ICNTYPT]);
	obj_unhide(addicon[CHNBUTT]);
	obj_unhide(addicon[ICNLABEL]);

	if ((type == ITM_DRIVE) || (type == ITM_TRASH) || (type == ITM_PRINTER))
	{
		/* It is a disk, printer or trashcan icon */

		startobj = DRIVEID;
		obj_hide(addicon[ICSHFIL]);
		obj_hide(addicon[ICSHFLD]);
		obj_unhide(addicon[ICBTNS]);
		obj_unhide(addicon[DRIVEID]);

		set_icntype(icn->item_type); /* set type into dialog  */

		*drvid = (char)icn->icon_dat.drv;
	}
	else
	{
		/* It is a folder/file icon */

		startobj = ICNLABEL;
		if ( type == ITM_FOLDER || type == ITM_PREVDIR )
		{
			obj_hide(addicon[ICSHFIL]);
			obj_unhide(addicon[ICSHFLD]);
		}
		else
		{
			obj_hide(addicon[ICSHFLD]);
			obj_unhide(addicon[ICSHFIL]);
		}
		obj_hide(addicon[ICBTNS]);
		obj_hide(addicon[DRIVEID]);

		*drvid = 0;
	}

	strcpy(iconlabel, icn->label);
	icon_no = icn->icon_index;

	/* Dialog for selecting icon and icon type */

	button = icn_dialog(&icon_no, startobj);

	if (button == CHNICNOK)
	{
		CICONBLK *h = desktop[object + 1].ob_spec.ciconblk;		/* ciconblk (the largest) */

		desktop[object + 1].ob_type = icons[icon_no].ob_type;	/* may change colours ;-) */
		icn->icon_index = icon_no;

		strsncpy(icn->label, iconlabel, sizeof(INAME));		/* secure cpy */
		strsncpy(icn->icon_name,
		        icons[icon_no].ob_spec.ciconblk->monoblk.ib_ptext,
		        sizeof(INAME));	/* maintain rsrc icon name; secure cpy */

		*h = *icons[icon_no].ob_spec.ciconblk;
		h->monoblk.ib_ptext = icn->label;
		h->monoblk.ib_char &= 0xFF00;

		if(startobj == DRIVEID)
		{
			icn->item_type = get_icntype(); /* get icontype from the dialog */

			if (icn->item_type != ITM_DRIVE)
				*drvid = 0;
			icn->icon_dat.drv = *drvid & 0xDF;
			h->monoblk.ib_char |= (int)(*drvid) & 0xDF;
		}
		else
			h->monoblk.ib_char |= 0x20;
	}

	return button;
}


/*
 * Move n icons to a new position on the desktop
 */

static void mv_icons(ICND *icns, int n, int mx, int my)
{
	int i, x, y, obj;

	for (i = 0; i < n; i++)
	{
		obj = icns[i].item;
		erase_icon(obj);
		x = (mx + icns[i].m_x - screen_info.dsk.x) / icon_width;
		y = (my + icns[i].m_y - screen_info.dsk.y) / icon_height;
		/* Note: do not use min() here */
		if ( x > m_icnx )
			x = m_icnx;
		if ( y > m_icny )
			y = m_icny;
		desk_icons[obj].x = x;
		desk_icons[obj].y = y;
		desktop[obj + 1].r.x = x * icon_width;
		desktop[obj + 1].r.y = y * icon_height;
		draw_icon(obj);
	}
}


static boolean icn_copy(WINDOW *dw, int dobject, WINDOW *sw, int n,
						int *list, ICND *icns, int x, int y, int kstate)
{
	int item, ix, iy, icon;
	const char *fname, *nameonly;
	INAME tolabel;

	ITMTYPE type;

	/* 
	 * A specific situation can arise if object are moved on the desktop:
	 * if selected icons are moved so that the cursor is, at the time 
	 * of button release, on one of the selected icons, a copy is attempted
	 * instead of icon movement. Code below cures this problem.
	 */

	if ( dobject > -1 && sw == desk_window && dw == desk_window )
	{
		int i;

		for ( i = 0; i < n; i++ )
		{
			if ( list[i] == dobject )
			{
				dobject = -1;
				break;
			}
		}
	}

	/* Now, act accordingly */

	if (dobject != -1)
		return item_copy(dw, dobject, sw, n, list, kstate);
	else
	{
		if (sw == desk_window)
		{
			mv_icons(icns, n, x, y);
			return FALSE;
		}
		else
		{
			if (n > 1)
			{
				alert_iprint(MMULITEM);
				return FALSE;
			}
			else
			{
				item = list[0];

				if ((fname = itm_fullname(sw, item)) != NULL)
				{
					boolean link  = itm_islink(sw, item, TRUE);

					type = itm_tgttype(sw, item);
					nameonly = fn_get_name(fname);
					cramped_name( nameonly, tolabel, (int)sizeof(INAME) );

					icon = icnt_geticon( nameonly, type, link );
					comp_icnxy(x, y, &ix, &iy);
					add_icon(type, type, link, icon, tolabel, 0, ix, iy, TRUE, fname);
				}

				return TRUE;
			}
		}
	}
}


/* 
 * funkties voor het laden en opslaan van de posities van de iconen. 
 * Remove all desktop icons 
 */

static void rem_all_icons(void)
{
	int i;

	for (i = 0; i < max_icons; i++)
		if (desk_icons[i].item_type != ITM_NOTUSED)
			remove_icon(i, FALSE);
}


/*
 * Set desktop background pattern and colour
 */

void set_dsk_background(int pattern, int color)
{
	options.dsk_pattern = (unsigned char)pattern;
	options.dsk_color = (unsigned char)color;
	desktop[0].ob_spec.obspec.fillpattern = pattern;
	desktop[0].ob_spec.obspec.interiorcol = color;
}


/*
 * Dummy routine: desktop keyboard handler
 */

static int dsk_hndlkey(WINDOW *w, int dummy_scancode, int dummy_keystate)
{
	return 0;
}



/*
 * (Re)generate desktop
 */

void regen_desktop(OBJECT *desk_tree)
{
	wind_set(0, WF_NEWDESK, desk_tree, 0);
	dsk_draw();
}


static ICONINFO this;


/*
 * Configuration table for one desktop icon
 */

CfgEntry Icon_table[] =
{
	{CFG_HDR,0,"icon" },
	{CFG_BEG},
	{CFG_S, 0, "name", this.ic.icon_name	},
	{CFG_S, 0, "labl", this.ic.label		},
	{CFG_D, 0, "type", &this.ic.item_type	},
	{CFG_D, 0, "tgtt", &this.ic.tgt_type	},
	{CFG_D, 0, "link", &this.ic.link		},
	{CFG_D, 0, "driv", &this.ic.icon_dat.drv},
	{CFG_S, 0, "path", this.name			},
	{CFG_D, 0, "xpos", &this.ic.x			},
	{CFG_D, 0, "ypos", &this.ic.y			},
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configuration of one desktop icon
 */

static CfgNest icon_cfg
{
	int i;

	*error = 0;

	if (io == CFG_SAVE)
	{
		for (i = 0; i < max_icons; i++)
		{
			ICON *ic = desk_icons + i;
	
			if (ic->item_type != ITM_NOTUSED)
			{
				this.ic = *ic;
	
				this.name[0] = 0;
	
				if (ic->item_type == ITM_DRIVE)
					this.ic.icon_dat.drv -= 'A';
				else
					this.ic.icon_dat.drv = 0;
	
				if (isfile(ic->item_type))
					strcpy(this.name, ic->icon_dat.name);
		
				*error = CfgSave(file, Icon_table, lvl, CFGEMP); 
			}

			if ( *error != 0 )
				break;
		}
	}
	else
	{
		memset(&this, 0, sizeof(this));

		*error = CfgLoad(file, Icon_table, (int)sizeof(LNAME), lvl); 

		if ( *error == 0 )
		{
			if (   this.ic.icon_name[0] == 0 
				|| this.ic.item_type < ITM_DRIVE
				|| this.ic.item_type > ITM_FILE 
				|| this.ic.icon_dat.drv > 'Z' - 'A'
				)
					*error = EFRVAL;
			else
			{
				int icon;
				char *name;

				/* SOME icon index should always be provided */

				icon = rsrc_icon(this.ic.icon_name); /* find by name */
				if (icon < 0)
					icon = default_icon(this.ic.item_type); /* display an alert */

				name = malloc(strlen(this.name) + 1);
				if (name)
				{
					if (this.ic.tgt_type == 0) /* compatibility */
						this.ic.tgt_type = this.ic.item_type;

					strcpy(name, this.name);
					*error = add_icon((ITMTYPE)this.ic.item_type,
									(ITMTYPE)this.ic.tgt_type, 
									this.ic.link,
									icon,
									this.ic.label,
									this.ic.icon_dat.drv + 'A',
							 		this.ic.x,
							 		this.ic.y,
							 		FALSE,
							 		name
						 			);
				}
				else
					*error = ENSMEM;
			}
		}
	}
}


/*
 * Configuration table for desktop icons
 */

static CfgEntry DskIcons_table[] =
{
	{CFG_HDR,0,"deskicons" },
	{CFG_BEG},
	{CFG_NEST,0,"icon", icon_cfg },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Load or save configuration of desktop icons
 */

CfgNest dsk_config
{
	*error = handle_cfg(file, DskIcons_table, lvl, CFGEMP, io, rem_all_icons, dsk_default);

	if ( io == CFG_LOAD && *error >= 0 )
		regen_desktop(desktop);
}


/*
 * Load the icon file. Result: TRUE if no error
 * Use standard functions of the AES 
 */

static void **svicntree, *svicnrshdr;

boolean load_icons(void)
{
	void **svtree = _GemParBlk.glob.ptree;
	void *svrshdr = _GemParBlk.glob.rshdr;

	char 
		*colour_irsc = "cicons.rsc", 	/* name of the colour icons file */
		*mono_irsc = "icons.rsc";		/* name of the mono icons file */

	int error;

	/* 
	 * Geneva 4 returns information that it supports
	 * colour icons, but that doesn't seem to work; thence a fix below:
	 * if there is no colour icons file, or if number of colours is
	 * less than 16, fall back to black/white (assuming that a change
	 * of the resolution or palette will always mean a restart of TeraDesk)
	 * Therefore, in Geneva 4 (and other similar cases, if any), remove 
	 * cicons.rsc from TeraDesk folder. Geneva 6 seems to work ok.
	 */

	if ( xshel_find(colour_irsc, &error) == NULL || !xd_colaes)
		colour_icons = FALSE;

	if (rsrc_load( colour_icons ? colour_irsc : mono_irsc) == 0)
	{
		alert_abort(MICNFRD); 
		return FALSE;
	}
	else
	{
		int i = 0;

		rsrc_gaddr(R_TREE, 0, &icons);		/* That's all you need. */
		svicntree = _GemParBlk.glob.ptree;
		svicnrshdr = _GemParBlk.glob.rshdr;

		n_icons = 0;
		icons++;

		do n_icons++; while ( (icons[i++].ob_flags&LASTOB) == 0 );
	}

	_GemParBlk.glob.ptree = svtree;
	_GemParBlk.glob.rshdr = svrshdr;

	return TRUE;
}


void free_icons(void)
{
	void **svtree = _GemParBlk.glob.ptree;
	void *svrshdr = _GemParBlk.glob.rshdr;

	_GemParBlk.glob.ptree = svicntree;
	_GemParBlk.glob.rshdr = svicnrshdr;

	rsrc_free();

	_GemParBlk.glob.ptree = svtree;
	_GemParBlk.glob.rshdr = svrshdr;
}


/* 
 * Routine voor het initialiseren van de desktop. 
 * Return FALSE in case of error
 */

boolean dsk_init(void)
{
	int i, error;

	/* Open the desktop window */

	if ((desk_window = xw_open_desk(DESK_WIND, &dsk_functions,
									sizeof(DSK_WINDOW), &error)) == NULL)
	{
		if (error == XDNSMEM)
			xform_error(ENSMEM);

		return FALSE;
	}

	((DSK_WINDOW *) desk_window)->itm_func = &itm_func;

	/* Number of icons that can fit on the screen */

	m_icnx = screen_info.dsk.w / ICON_W - 1;
	m_icny = screen_info.dsk.h / ICON_H - 1;

	/* Always allow for at least 64 icons */

	if ((max_icons = (m_icnx + 1) * (m_icny + 1)) < 64)
		max_icons = 64;

	if (   ((desktop    = malloc_chk((long)(max_icons + 1) * sizeof(OBJECT))) == NULL)
	    || ((desk_icons = malloc_chk((long) max_icons      * sizeof(ICON  ))) == NULL)
	   )
	{
		return FALSE;
	}

	/*  
	 *  When using nonstandard resolutions (i.e. like with overscan)
	 *  unpleasant effects occur- icon positions shift;
	 *  as a brute-force fix: always replace calculated icon width and height
	 *  with initial values- in standard resolutions the result should probably
	 *  be the same anyway. Perhaps overscan software should be looked into,
	 *  as it does not properly inform GEM of the change (can it?)
	 */
 
	icon_width  = ICON_W; 
	icon_height = ICON_H; 

	desktop[0].ob_next = -1;
	desktop[0].ob_head = -1;
	desktop[0].ob_tail = -1;
	desktop[0].ob_type = G_BOX;
	desktop[0].ob_flags = NORMAL;
	desktop[0].ob_state = NORMAL;
	desktop[0].ob_spec.obspec.framesize = 0;
	desktop[0].ob_spec.obspec.textmode = 1;
	desktop[0].r = screen_info.dsk;

	for (i = 0; i < max_icons; i++)
		desk_icons[i].item_type = ITM_NOTUSED;

#ifdef MEMDEBUG
	atexit(rem_all_icons);
#endif
	return TRUE;
}


static void incr_pos(int *x, int *y)
{
	if ((*x += 1) > m_icnx)
	{
		*x = 0;
		*y += 1;
	}
}


/*
 * Remove all icons, then add default ones (drives, printer and trash can)
 */

void dsk_default(void)
{
	int i, x = 2, y = 0, ic;
	long drives = drvmap();

	rem_all_icons();

	/* Identify icons by name, not index;  use names from the rsc file. */

	/* Two floppies */

	ic = rsrc_icon_rscid( FLINAME, iname );
	add_icon(ITM_DRIVE, ITM_DRIVE, FALSE, ic, iname, 'A', 0, 0, FALSE, NULL);
	add_icon(ITM_DRIVE, ITM_DRIVE, FALSE, ic, iname, 'B', 1, 0, FALSE, NULL);
	
	/* Hard disks */

	ic = rsrc_icon_rscid ( HDINAME, iname );

	for (i = 2; i < 32; i++)
	{
		if (btst(drives, i))
		{
			add_icon(ITM_DRIVE, ITM_DRIVE, FALSE, ic, iname, (i < 26) ? 'A' + i : '0' + i - 26, x, y, FALSE, NULL); 
			incr_pos(&x, &y);
		}
	}

	/* Trash and printer */

	ic = rsrc_icon_rscid ( TRINAME, iname );
	add_icon(ITM_TRASH, ITM_TRASH, FALSE, ic, iname, 0, x, y, FALSE, NULL);
	incr_pos(&x, &y);
	ic = rsrc_icon_rscid ( PRINAME, iname );
	add_icon(ITM_PRINTER, ITM_PRINTER, FALSE, ic, iname, 0, x, y, FALSE, NULL);	

	/* Regenerate desktop */

	regen_desktop(desktop);
}


/*
 * Remove some desktop icons
 */

void dsk_remicon(void)
{
	int button, i, n, *list;

	button = alert_query(MREMICNS);

	if (button == 1)
	{
		if (icn_list(desk_window, &n, &list))
		{
			for (i = 0; i < n; i++)
				remove_icon(list[i], TRUE);
			wd_reset(desk_window);
			free(list);
		}
	}
}


/* 
 * Change a desktop icon 
 */

void dsk_chngicon(void)
{
	int 
		button = -1,
		i = 0, 
		n,  
		*list;


	if ( icn_list(desk_window, &n, &list) )
	{
		/* See sl_dialog() for the meaning of below */

		sl_noop = 2; /* Open dialog, do not close */
		sl_abobutt = CHNICNAB;	/* abort button */

		while ((i < n) && (button != CHNICNAB))
		{
			if ( i == (n - 1) )
				sl_noop &= ~(0x02);

			button = chng_icon(list[i++]); /* The dialog */

			sl_noop = 3; /* Do not open or close after the first one */
		}

		/* When all is finished, redraw icons */

		for (i = 0; i < n; i++)
			redraw_icon(list[i]);

		free(list);
	}
	else
		dsk_insticon();

	/* This is related to opening/closing the icon selector dialog */

	sl_noop = 0;
	sl_abobutt = -1;

}


/*
 * Limit colour index to existing values
 */

int limcolor(int col)
{
	return minmax(0, col, xd_ncolors - 1);
}


/*
 * Limit pattern index to first 8 
 */

int limpattern(int pat)
{
	return minmax(1, pat, min(7, xd_npatterns - 1));
}


/* 
 * Set colour in a colour / pattern selector box 
 */

static void set_selcol(XDINFO *info, int obj, int col)
{
	wdoptions[obj].ob_spec.obspec.interiorcol = limcolor(col);
	xd_draw(info, obj, 0);
}


/*
 * Set pattern in a colour / pattern selector box
 */

static void set_selpat(XDINFO *info, int obj, int pat)
{
	wdoptions[obj].ob_spec.obspec.fillpattern = limpattern( pat );
	xd_draw(info, obj, 0);
}


/*
 * Handle window options dialog
 */

void dsk_options(void)
{
	int 
		color, 
		pattern,
		wcolor, 
		wpattern,
		button = -1, 
		oldbutton = -1; 	/* aux for arrow_form_do */

	XDINFO 
		info;

	boolean
		ok = FALSE,
		stop = FALSE, 
		draw = FALSE;

	/* Initial states */
	
	wdoptions[DSKPAT].ob_spec.obspec.fillpattern = limpattern((int)options.dsk_pattern);
	wdoptions[DSKPAT].ob_spec.obspec.interiorcol = limcolor((int)options.dsk_color) ;
	wdoptions[WINPAT].ob_spec.obspec.fillpattern = limpattern((int)options.win_pattern);
	wdoptions[WINPAT].ob_spec.obspec.interiorcol = limcolor( (int)options.win_color);

	/* Open the dialog */

	xd_open(wdoptions, &info);

	while (!stop)
	{
		color = wdoptions[DSKPAT].ob_spec.obspec.interiorcol;
		pattern = wdoptions[DSKPAT].ob_spec.obspec.fillpattern;
		wcolor = wdoptions[WINPAT].ob_spec.obspec.interiorcol; 
		wpattern = wdoptions[WINPAT].ob_spec.obspec.fillpattern;

		button = arrow_form_do ( &info, &oldbutton );

		switch (button)
		{
		case WOVIEWER:
		case WODIR:
			if ( wd_type_setfont( (button == WOVIEWER) ? DTVFONT : DTDFONT) )
				ok = TRUE;	
			break;
		case DSKCUP:
			color += 2;
		case DSKCDOWN:
			color--;
			set_selcol(&info, DSKPAT, color);
			break;
		case DSKPUP:
			pattern += 2;
		case DSKPDOWN:
			pattern--;
			set_selpat(&info, DSKPAT, pattern);
			break;
		case WINCUP:
			wcolor += 2;
		case WINCDOWN:
			wcolor--;
			set_selcol(&info, WINPAT, wcolor);
			break;
		case WINPUP:
			wpattern += 2;
		case WINPDOWN:
			wpattern--;
			set_selpat(&info, WINPAT, wpattern);
			break;
		case WOPTOK:
			if ((options.dsk_pattern != (unsigned char)pattern) ||
				(options.dsk_color != (unsigned char)color))
			{
				set_dsk_background(pattern, color);
				draw = TRUE;
			}

			/* window pattern & colour */

			if ((options.win_pattern != (unsigned char)wpattern) ||
				(options.win_color != (unsigned char)wcolor))
			{
				options.win_pattern= (unsigned char)wpattern;
				options.win_color= (unsigned char)wcolor; 
				ok = TRUE;
			}						
		default:
			stop = TRUE;
			break;
		}
	}

	xd_buttnorm(&info, button);
	xd_close(&info);

	/* Updates must not be done before the dialogs are closed */

	if (draw)
		redraw_desk(&screen_info.dsk);
	if (ok)
		wd_drawall();
}



