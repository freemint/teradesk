/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2017  Dj. Vukovic
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
#include "stringf.h"
#include "desk.h"
#include "error.h"
#include "events.h"
#include "open.h"
#include "file.h"
#include "lists.h"
#include "slider.h"
#include "xfilesys.h"
#include "copy.h"
#include "config.h"
#include "font.h"
#include "window.h"						/* before dir.h and viewer.h */
#include "showinfo.h"
#include "dir.h"
#include "icon.h"
#include "icontype.h"
#include "prgtype.h"
#include "screen.h"
#include "viewer.h"
#include "video.h"
#include "filetype.h"					/* must be before applik.h */
#include "applik.h"


typedef enum
{
	ICN_NO_UPDATE,
	ICN_REDRAW,
	ICN_DELETE
} ICNUPDTYPE;

typedef struct
{
	_WORD x;							/* position on the desktop */
	_WORD y;							/* position on the desktop */
	_WORD icon_index;					/* index in the list of icons */
	union
	{
		_WORD drv;						/* disk volume (partition) name */
		char *name;						/* file/folder name to which this icon applies */
	} icon_dat;
	INAME label;						/* text in icon label */
	INAME icon_name;					/* name of this icon in the resource file */
	ITMTYPE item_type;					/* item type of the object to which this icon applies */
	ITMTYPE tgt_type;					/* type of the target object if this obect is a link */
	ICNUPDTYPE update;
	bool link;							/* true if the icon represents a symbolic link */
	bool selected;						/* true if icon is selected */
	bool newstate;						/* new selected/deselected state */
} ICON;									/* Data for one desktop icon */


typedef struct
{
	VLNAME name;
	ICON ic;
} ICONINFO;


typedef struct
{
	ITM_INTVARS;
} DSK_WINDOW;

OBJECT *icons;							/* use rsrc_load for icons. */
OBJECT *desktop;						/* desktop background object */

WINDOW *desk_window;					/* pointer to desktop background window */

XDINFO icd_info;

_WORD *icon_data;
_WORD iconw = ICON_W;					/* icon sell size */
_WORD iconh = ICON_H;					/* icon cell size */
_WORD sl_noop;							/* if 1, do not open icon dialog- it is already open */
_WORD n_icons;							/* number of icons in the icons file */

INAME iname;

static OBJECT **svicntree;
static void *svicnrshdr;
static ICON *desk_icons;				/* An array of desktop ICONs */

static XUSERBLK dxub;

static _WORD m_icnx;
static _WORD m_icny;
static _WORD icn_xoff = 0;
static _WORD icn_yoff = 0;
static _WORD max_icons;

static ITMTYPE icd_itm_type;			/* type of an item entered by name in the dialog */
static ITMTYPE icd_tgt_type;

static bool icd_islink;

bool noicons = FALSE;


static void dsk_diskicons(_WORD *x, _WORD *y, _WORD ic, char *name);

static void dsk_do_update(void);

static void set_dsk_obtype(_WORD type);

static _WORD dsk_defaultpatt(void);

static _WORD chng_icon(_WORD object);

static void redraw_icon(_WORD object, _WORD mode);


/*
 * Functions that apply to desktop window
 */

static WD_FUNC dsk_functions = {
	0L,									/* handle keypress */
	wd_hndlbutton,						/* handle button */
	0L,									/* redraw */
	xw_nop1,							/* topped */
	xw_nop1,							/* bottomed */
	xw_nop1,							/* newtop */
	0L,									/* closed */
	0L,									/* fulled */
	xw_nop2,							/* arrowed */
	0L,									/* hslid */
	0L,									/* vslid */
	0L,									/* sized */
	0L,									/* moved */
	0L,									/* hndlmenu */
	0L,									/* top */
	0L,									/* iconify */
	0L									/* uniconify */
};


/*
 * Functions that apply to desktop items
 */

static _WORD icn_find(WINDOW *w, _WORD x, _WORD y);

static bool icn_state(WINDOW *w, _WORD icon);

static ITMTYPE icn_type(WINDOW *w, _WORD icon);

static ITMTYPE icn_tgttype(WINDOW *w, _WORD icon);

static const char *icn_name(WINDOW *w, _WORD icon);

static char *icn_fullname(WINDOW *w, _WORD icon);

static _WORD icn_attrib(WINDOW *w, _WORD icon, _WORD mode, XATTR *attrib);

static bool icn_islink(WINDOW *w, _WORD icon);

static bool icn_open(WINDOW *w, _WORD item, _WORD kstate);

static bool icn_copy(WINDOW *dw, _WORD dobject, WINDOW *sw, _WORD n, _WORD *list, ICND *icns, _WORD x, _WORD y,
					 _WORD kstate);
static void icn_rselect(WINDOW *w, _WORD x, _WORD y);

static ICND *icn_xlist(WINDOW *w, _WORD *nselected, _WORD *nvisible, _WORD **sel_list, _WORD mx, _WORD my);

static _WORD *icn_list(WINDOW *w, _WORD *nselected);

static void icn_set_update(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);

static void icn_do_update(WINDOW *w);

static void icn_select(WINDOW *w, _WORD selected, _WORD mode, bool draw);

static ITMFUNC itm_func = {
	icn_find,							/* itm_find */
	icn_state,							/* itm_state */
	icn_type,							/* itm_type */
	icn_tgttype,						/* target object type */
	icn_name,							/* itm_name */
	icn_fullname,						/* itm_fullname */
	icn_attrib,							/* itm_attrib */
	icn_islink,							/* itm_islink */
	icn_open,							/* itm_open */
	icn_copy,							/* itm_copy */
	icn_select,							/* itm_select */
	icn_rselect,						/* itm_rselect */
	icn_xlist,							/* itm_xlist */
	icn_list,							/* itm_list */
	0L,									/* wd_path */
	icn_set_update,						/* wd_set_update */
	icn_do_update,						/* wd_do_update */
	0L									/* wd_seticons */
};


/*
 * Mouse form (like a small icon) for placing a desktop icon
 */

static const MFORM icnm = {
	8, 8, 1, 1, 0,
	{ 0x1FF8, 0x1008, 0x1008, 0x1008, 0x1008, 0x1008, 0x1008, 0x1008,
	  0x1008, 0x1008, 0x1008, 0x1008, 0xF00F, 0x8001, 0x8001, 0xFFFF },
	{ 0x0000, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0,
	  0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x0FF0, 0x7FFE, 0x7FFE, 0x0000 }
};


/*
 * Funktie voor het opnieuw tekenen van de desktop.
 */

static void form_dialall(_WORD what)
{
	form_dial(what, xd_desk.g_x, xd_desk.g_y, xd_desk.g_w, xd_desk.g_h, xd_desk.g_x, xd_desk.g_y, xd_desk.g_w, xd_desk.g_h);
}


void dsk_draw(void)
{
	form_dialall(FMD_START);
	form_dialall(FMD_FINISH);
}


/* 
 * Some toutines to manipulate setting desktop icon grid
 */

static _WORD icn_atoi(_WORD obj)
{
	return atoi(wdoptions[obj].ob_spec.tedinfo->te_ptext);
}


static void icn_itoa(_WORD val, _WORD obj)
{
	itoa(val, wdoptions[obj].ob_spec.tedinfo->te_ptext, 10);
}


static void set_maxicons(void)
{
	m_icnx = (xd_desk.g_w - icn_xoff) / iconw - 1;
	m_icny = (xd_desk.g_h - icn_yoff) / iconh - 1;
}


/*
 * A routine to set desktop icons in a new grid
 */

static void put_newgrid(void)
{
	_WORD i;
	ICON *icn = desk_icons;
	OBJECT *deskto = &desktop[1];

	for (i = 0; i < max_icons; i++)
	{
		if (icn->item_type != ITM_NOTUSED)
		{

			redraw_icon(i, 1);			/* erase at old position */

			if (icn->x > m_icnx)
				icn->x = m_icnx;

			if (icn->y > m_icny)
				icn->y = m_icny;

			deskto->ob_x = icn->x * iconw + icn_xoff;
			deskto->ob_y = icn->y * iconh + icn_yoff;

			redraw_icon(i, 2);			/* draw at new position */
		}

		deskto++;
		icn++;
	}
}


/*
 * Determine whether an icon item is a file/folder/program/link or not.
 * This is determined by analyzing the stored item type, not by
 * actually inquiring the item represented by the icon.
 */

bool isfile(ITMTYPE type)
{
	return type == ITM_FILE || type == ITM_PROGRAM || type == ITM_FOLDER || type == ITM_LINK;
}


/*
 * Similar to above, but includes network objects
 */

static bool isfilenet(ITMTYPE type)
{
	return (isfile(type) || type == ITM_NETOB);
}


/*
 * Increment position for placing an icon on the screen.
 * Start a new row if right edge of screen reached.
 */

static void incr_pos(_WORD *x, _WORD *y)
{
	if ((*x += 1) > m_icnx)
	{
		*x = 0;
		*y += 1;
	}
}


/*
 * Hide an object (e.g. "Skip" button) is there is only one
 * object selected in a list
 */

void hideskip(_WORD n, OBJECT *obj)
{
	if (n < 2)
	{
		obj->ob_flags |= OF_HIDETREE;
	} else
	{
		obj->ob_flags &= ~OF_HIDETREE;
	}
}



/*
 * Set desktop icon type correctly for executable files (programs)
 * and also for network objects.
 * This is needed after changes in the list of executable filetypes,
 * or after a link is made to reference a network object.
 * Beware that, if desktop icons reference items on currently nonexistent
 * disk volumes, any program-type items on those locations will be
 * degraded to ordinary files.
 */

void icn_fix_ictype(void)
{
	ICON *ic;
	XATTR att;
	_WORD i;

	ic = desk_icons;

	for (i = 0; i < max_icons; i++)
	{
		/* Is this either ITM_FILE or ITM_PROGRAM (not a folder) ? */

		if (isfileprog(ic->item_type))
		{
			char *tgtname = x_fllink(ic->icon_dat.name);

			/* Is either this or the target item an executable file type ? */

#if _MINT_
			if (ic->link && x_netob(tgtname))
			{
				ic->item_type = ITM_FILE;
				ic->tgt_type = ITM_NETOB;
			} else
#endif
			{
				if ((x_attr(1, x_inq_xfs(tgtname), tgtname, &att) >= 0) && dir_isexec(fn_get_name(tgtname), &att))
				{
					ic->tgt_type = ITM_PROGRAM;
#if _MINT_
					if (!ic->link)
#endif
						ic->item_type = ITM_PROGRAM;
				} else
				{
					/* Maybe this was a program once, but now is just a file */

					ic->item_type = ITM_FILE;
					ic->tgt_type = ITM_FILE;
				}
			}

			free(tgtname);
		}

		ic++;
	}
}


/*
 * Redraw icons that may be partially obscured by other objects
 */

void draw_icrects(WINDOW *w, OBJECT *tree, GRECT *r1)
{
	GRECT r2, d;

	xw_getfirst(w, &r2);

	while (r2.g_w != 0 && r2.g_h != 0)
	{
		if (xd_rcintersect(r1, &r2, &d))
			draw_tree(tree, &d);

		xw_getnext(w, &r2);
	}
}


#define WF_OWNER	20

/*
 * Redraw part of the desktop.
 */

void redraw_desk(GRECT *r1)
{
	_WORD owner = 0;
	bool redraw = TRUE;

	xd_begupdate();

	/* In newer (multitasking) systems, redraw only what is owned by the desktop ? */

	/* Should this perhaps be 399 or 340 instead of 400 ? (i.e. Magic & Falcon TOS) */

	if (aes_version >= 0x399)
	{
		wind_get_int(0, WF_OWNER, &owner);

		if (ap_id != owner)
			redraw = FALSE;
	}

	/* Actually redraw... */

	if (redraw)
		draw_icrects(NULL, desktop, r1);

	xd_endupdate();
}


/*
 * Erase and/or draw an icon
 * mode=1: erase,  mode=2: draw,  mode=3: redraw (erase, then draw)
 */

static void redraw_icon(_WORD object, _WORD mode)
{
	GRECT r;
	_WORD o1 = object + 1;

	xd_objrect(desktop, o1, &r);

	if ((mode & 1) != 0)
	{
		desktop[o1].ob_flags = OF_HIDETREE;
		redraw_desk(&r);
		desktop[o1].ob_flags = OF_NONE;
	}

	if ((mode & 2) != 0)
		redraw_desk(&r);
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
	ICON *icn;
	_WORD i;

	icn = desk_icons;

	for (i = 0; i < max_icons; i++)
	{
		if (icn->item_type != ITM_NOTUSED)
			icn->selected = icn->newstate;

		icn++;
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
 * Monochrome icons are treated like the colour ones if TeraDesk is set
 * for display of background pictures.
 */

static void dsk_drawsel(void)
{
	ICON *icn;							/* pointer to current icon */
	OBJECT *deskti;
	_WORD i;							/* object counter */
	GRECT c;								/* icon object rectangle */
	GRECT r = { SHRT_MAX, SHRT_MAX, -1, -1};		/* summary rectangle */

	icn = desk_icons;

	set_dsk_obtype(G_IBOX);

	for (i = 1; i <= max_icons; i++)
	{
		deskti = &desktop[i];

		if (icn->item_type != ITM_NOTUSED)
		{
			if (icn->selected == icn->newstate)
			{
				/* State has not changed; hide the icon temporarily */

				obj_hide(desktop[i]);
			} else
			{
				/* State has changed; draw the icon */

				if (icn->newstate)
					deskti->ob_state |= OS_SELECTED;
				else
					deskti->ob_state &= ~OS_SELECTED;

				xd_objrect(desktop, i, &c);

				if (colour_icons || (options.dsk_colour == 0))
				{
					/* Draw each colour icon separately, -with background- */

					set_dsk_obtype(G_BOX);
					redraw_desk(&c);
				} else
				{
					/* For monochrome, update surrounding rectangle for all icons */

					if (c.g_x < r.g_x)		/* left edge minimum  */
						r.g_x = c.g_x;

					if (c.g_y < r.g_y)		/* upper edge minimum */
						r.g_y = c.g_y;

					c.g_x += deskti->ob_width - 1;
					c.g_y += deskti->ob_height - 1;

					if (c.g_x > r.g_w)		/* right edge maximum */
						r.g_w = c.g_x;

					if (c.g_y > r.g_h)		/* lower edge maximum */
						r.g_h = c.g_y;
				}
			}
		}

		icn++;
	}

	/* This will happen only for mono icons */

	if (r.g_w >= 0)
	{
		r.g_w -= r.g_x + 1;
		r.g_h -= r.g_y + 1;

		redraw_desk(&r);
	}

	/* Set temporarily hidden icons to visible again */

	icn = desk_icons;

	for (i = 0; i < max_icons; i++)
	{
		if (icn->item_type != ITM_NOTUSED)
			obj_unhide(desktop[i + 1]);

		icn++;
	}

	set_dsk_obtype(G_BOX);
}


/*
 * Compute rubberbox rectangle. Beware of asymmetrical wind_update()
 */

void rubber_rect(_WORD x1, _WORD x2, _WORD y1, _WORD y2, GRECT *r)
{
	_WORD h;

	arrow_mouse();
	xd_endmctrl();

	if ((h = x2 - x1) < 0)
	{
		r->g_x = x2;
		r->g_w = 1 - h;
	} else
	{
		r->g_x = x1;
		r->g_w = h + 1;
	}

	if ((h = y2 - y1) < 0)
	{
		r->g_y = y2;
		r->g_h = 1 - h;
	} else
	{
		r->g_y = y1;
		r->g_h = h + 1;
	}
}


/*
 * A size-saving aux. function for setting-up rubberbox drawing
 * Beware of asymmetrical wind_update()
 */

void start_rubberbox(void)
{
	set_rect_default();
	xd_begmctrl();
	graf_mouse(POINT_HAND, NULL);
}


/*
 * Rubberbox function
 */

static void rubber_box(_WORD x, _WORD y, GRECT *r)
{
	_WORD x1, x2, ox, y1, y2, oy, kstate;
	bool released;

	x1 = x2 = ox = x;
	y1 = y2 = oy = y;

	start_rubberbox();
	draw_rect(x1, y1, x2, y2);

	/* Loop until mouse button is released */

	do
	{
		released = xe_mouse_event(0, &x2, &y2, &kstate);

		/* Menu bar is inaccessible */

		if (y2 < xd_desk.g_y)
			y2 = xd_desk.g_y;

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
	} while (!released);

	rubber_rect(x1, x2, y1, y2, r);
}


/*
 * Change the selection state of some items
 * mode = 0: select selected and deselect the rest.
 * mode = 1: invert the status of selected.
 * mode = 2: deselect selected.
 * mode = 3: select selected.
 * mode = 4: select all (maybe)
 * External consideration is needed for mode 4
 */

void changestate(_WORD mode, bool *newstate, _WORD i, _WORD selected, bool iselected, bool iselected4)
{
	switch (mode)
	{
	case 0:
		*newstate = (i == selected) ? TRUE : FALSE;
		break;
	case 1:
		*newstate = iselected;
		break;
	case 2:
		*newstate = (i == selected) ? FALSE : iselected;
		break;
	case 3:
		*newstate = (i == selected) ? TRUE : iselected;
		break;
	case 4:
		*newstate = iselected4;
		break;
	}
}


/*
 * Externe funkties voor iconen.
 */
static void icn_select(WINDOW *w, _WORD selected, _WORD mode, bool draw)
{
	_WORD i;
	ICON *icn = desk_icons;

	(void) w;
	/* Newstates after the switch below is equal to selection state of all icons */

	for (i = 0; i < max_icons; i++)
	{
		if (icn->item_type != ITM_NOTUSED)
			changestate(mode, &(icn->newstate), i, selected, icn->selected, TRUE);

		icn++;
	}

	if (mode == 1)
		desk_icons[selected].newstate = (desk_icons[selected].selected) ? FALSE : TRUE;

	if (draw)
		dsk_drawsel();

	dsk_setnws();
}


static void icn_rselect(WINDOW *w, _WORD x, _WORD y)
{
	ICONBLK *monoblk;
	OBJECT *deskto;
	ICON *icn;
	GRECT r1, r2;
	_WORD ox, oy, i;

	(void) w;
	icn = desk_icons;

	rubber_box(x, y, &r1);

	for (i = 1; i <= max_icons; i++)
	{
		deskto = &desktop[i];

		if (icn->item_type != ITM_NOTUSED)
		{
			monoblk = &(deskto->ob_spec.ciconblk->monoblk);

			r2.g_x = monoblk->ib_xicon;
			r2.g_y = monoblk->ib_yicon;
			r2.g_w = monoblk->ib_wicon;
			r2.g_h = monoblk->ib_hicon;

			ox = desktop[0].ob_x + deskto->ob_x;
			oy = desktop[0].ob_y + deskto->ob_y;
			r2.g_x += ox;
			r2.g_y += oy;

			if (rc_intersect2(&r1, &r2))
			{
				icn->newstate = !icn->selected;
			} else
			{
				r2.g_x = monoblk->ib_xtext;
				r2.g_y = monoblk->ib_ytext;
				r2.g_w = monoblk->ib_wtext;
				r2.g_h = monoblk->ib_htext;
				r2.g_x += ox;
				r2.g_y += oy;

				if (rc_intersect2(&r1, &r2))
					icn->newstate = !icn->selected;
				else
					icn->newstate = icn->selected;
			}
		}

		icn++;
	}

	dsk_drawsel();
	dsk_setnws();
}


/*
 * Check if a desktop icon is used and selected
 */

static bool dsk_isselected(_WORD i)
{
	return ((desk_icons[i].item_type != ITM_NOTUSED) && desk_icons[i].selected);
}


/*
 * Count the selected desktop icons
 */

#if 0
static void icn_nselected(WINDOW *w, _WORD *n, _WORD *sel)
{
	_WORD i, s, c = 0;

	(void) w;
	for (i = 0; i < max_icons; i++)
	{
		if (dsk_isselected(i))
		{
			c++;
			s = i;
		}
	}

	*n = c;
	*sel = (c == 1) ? s : -1;
}
#endif


/*
 * Note: WINDOW *w parameter is not used in the following functions
 * but must exist because these are ITMFUNCs
 */

static bool icn_state(WINDOW *w, _WORD icon)
{
	(void) w;
	return desk_icons[icon].selected;
}


/*
 * Return type of a desktop icon item
 */

static ITMTYPE icn_type(WINDOW *w, _WORD icon)
{
	(void) w;
	return desk_icons[icon].item_type;
}


/*
 * Return type of the target object of a desktop icon
 */

static ITMTYPE icn_tgttype(WINDOW *w, _WORD icon)
{
	(void) w;
	return desk_icons[icon].tgt_type;
}


/*
 * Return pointer to the name of a (desktop) object
 */

static const char *icn_name(WINDOW *w, _WORD icon)
{
	ICON *icn = &desk_icons[icon];

	/*
	 * "name" must exist after return from this function and be
	 * sufficiently long to accomodate string MDRVNAME + drive letter
	 */

	static char name[20];

	(void) w;
	if (isfile(icn->item_type))
	{
		/* this is a file, folder, program or link- treat as a file(name) */
		return fn_get_name(icn->icon_dat.name);
	} else
	{
		if (icn->item_type == ITM_DRIVE)
		{
			/* this is a disk volume. Return text "drive X" */

			sprintf(name, get_freestring(MDRVNAME), (char) icn->icon_dat.drv);
			return name;
		} else if (icn->item_type == ITM_NETOB)
		{
			return icn->icon_dat.name;
		}
	}

	return NULL;
}


/*
 * Return the identification of the icon for an object given by name.
 * This routine is used when assigning window icons from desktop ones.
 */

_WORD icn_iconid(const char *name)
{
	_WORD i;
	ICON *icn = desk_icons;

	if (name)
	{
		for (i = 0; i < max_icons; i++)
		{
			if (isfile(icn->item_type))
			{
				if (strcmp(name, icn->icon_dat.name) == 0)
					return icn->icon_index;
			}

			icn++;
		}
	}

	return -1;
}


/*
 * Get fullname of an object to which the icon has been assigned.
 * When an item can not have a name, or there is an error, return NULL
 * (i.e. only files, folders, programs and disk volumes can have fullnames).
 * This routine allocates space for the name.
 */

static char *icn_fullname(WINDOW *w, _WORD icon)
{
	ICON *icn = &desk_icons[icon];
	char *s = NULL;

	(void) w;
	if (icn->item_type == ITM_DRIVE)
	{
		if ((s = strdup(adrive)) != NULL)
			*s = icn->icon_dat.drv;
	} else if (isfilenet(icn->item_type))
	{
		s = strdup(icn->icon_dat.name);
	}

	return s;
}


/*
 * Get attributes of an object to which a desktop icon has been assigned
 */

static _WORD icn_attrib(WINDOW *w, _WORD icon, _WORD mode, XATTR *attr)
{
	ICON *icn = &desk_icons[icon];

	(void) w;
	if (isfile(icn->item_type))
		return (_WORD) x_attr(mode, FS_INQ, icn->icon_dat.name, attr);

	return 0;
}


/*
 * Does a desktop icon represent a link?
 */

static bool icn_islink(WINDOW *w, _WORD icon)
{
	(void) w;
#if _MINT_
	return desk_icons[icon].link;
#else
	(void) icon;
	return FALSE;
#endif
}


void icn_coords(ICND *icnd, GRECT *tr, GRECT *ir)
{
	_WORD *icndcoords = &(icnd->coords[0]), c;

	icnd->np = 9;

	*icndcoords++ = tr->g_x;				/* [0] */
	*icndcoords++ = tr->g_y;				/* [1] */
	*icndcoords++ = ir->g_x;				/* [2] */
	*icndcoords++ = tr->g_y;				/* [3] */
	*icndcoords++ = ir->g_x;				/* [4] */
	*icndcoords++ = ir->g_y;				/* [5] */
	c = ir->g_x + ir->g_w;
	*icndcoords++ = c;						/* [6] */
	*icndcoords++ = ir->g_y;				/* [7] */
	*icndcoords++ = c;						/* ir.g_x + ir.g_w    [8] */
	*icndcoords++ = tr->g_y;				/* [9] */
	c = tr->g_x + tr->g_w;
	*icndcoords++ = c;						/*[10] */
	*icndcoords++ = tr->g_y;				/*[11] */
	*icndcoords++ = c;						/* tr.g_x + tr.g_w     [12] */
	c = tr->g_y + tr->g_h;
	*icndcoords++ = tr->g_y + tr->g_h;		/*[13] */
	*icndcoords++ = tr->g_x;				/*[14] */
	*icndcoords++ = c;						/* tr.g_y + tr.g_h [15] */
	*icndcoords++ = tr->g_x;				/*[16] */
	*icndcoords = tr->g_y + 1;				/*[17] */
}



static void get_icnd(_WORD object, ICND *icnd, _WORD mx, _WORD my)
{
	GRECT tr, ir;
	_WORD dx, dy;
	ICONBLK *h;
	OBJECT *desktopobj = &desktop[object + 1];

	h = &(desktopobj->ob_spec.ciconblk->monoblk);

	dx = desktopobj->ob_x - mx + xd_desk.g_x;
	dy = desktopobj->ob_y - my + xd_desk.g_y;

	tr.g_x = dx + h->ib_xtext;
	tr.g_y = dy + h->ib_ytext;
	tr.g_w = h->ib_wtext - 1;
	tr.g_h = h->ib_htext - 1;
	ir.g_x = dx + h->ib_xicon;
	ir.g_y = dy + h->ib_yicon;
	ir.g_w = h->ib_wicon - 1;
	ir.g_h = h->ib_hicon - 1;

	icnd->item = object;
	icnd->m_x = dx + desktopobj->ob_width / 2;
	icnd->m_y = dy + desktopobj->ob_height / 2;

	icn_coords(icnd, &tr, &ir);
}


/*
 * Create a list of selected icon items.
 * This routine will return a NULL list pointer
 * if no list has been created, either because there are no
 * selected items, or because list allocation failed.
 * If returned pointer is NULL, *nselected will be 0.
 */

static _WORD *icn_list(WINDOW *w, _WORD *nselected)
{
	_WORD *list;						/* pointer to a list item */
	_WORD *sel_list;					/* pointer to the list */
	_WORD i;								/* item counter */
	_WORD n = 0;							/* item count */

	(void) w;
	/* First count the selected items */

	for (i = 0; i < max_icons; i++)
	{
		if (dsk_isselected(i))
			n++;
	}

	/* If nothing is selected, return NULL */

	if (n)
	{
		/* Then create a list... */

		list = malloc_chk((long) n * sizeof(*list));

		if (list)
		{
			*nselected = n;
			sel_list = list;

			for (i = 0; i < max_icons; i++)
			{
				if (dsk_isselected(i))
					*list++ = i;
			}

			return sel_list;
		}
	}

	*nselected = 0;

	return NULL;
}


/*
 * Routine voor het maken van een lijst met geseleketeerde iconen.
 */

static ICND *icn_xlist(WINDOW *w, _WORD *nsel, _WORD *nvis, _WORD **sel_list, _WORD mx, _WORD my)
{
	ICND *icns, *icnlist;
	_WORD *list, i, n;

	(void) w;
	if ((*sel_list = icn_list(desk_window, nsel)) != NULL)
	{
		n = *nsel;
		*nvis = n;

#if 0									/* this can never happen ? */

		if (n == 0)
		{
			*icns = NULL;
			return TRUE;

		}
#endif

		icnlist = malloc_chk((long) n * sizeof(ICND));

		if (icnlist)
		{
			icns = icnlist;

			list = *sel_list;

			for (i = 0; i < n; i++)
			{
				get_icnd(*list++, icnlist++, mx, my);
			}

			return icns;
		}

		free(*sel_list);
	}

	return NULL;
}


/*
 * Find the icon at position x,y in window w
 */

static _WORD icn_find(WINDOW *w, _WORD x, _WORD y)
{
	ICONBLK *p;
	OBJECT *de;
	_WORD ox,  oy;
	_WORD ox2, oy2;
	_WORD i;
	_WORD object = -1;

	(void) w;
	objc_offset(desktop, 0, &ox, &oy);

	if ((i = desktop[0].ob_head) == -1)
		return -1;

	while (i != 0)
	{
		de = &desktop[i];

		if (de->ob_type == G_ICON || de->ob_type == G_CICON)
		{
			GRECT h;

			/* first look at the icon rectangle */

			ox2 = ox + de->ob_x;
			oy2 = oy + de->ob_y;
			p = &(de->ob_spec.ciconblk->monoblk);

			h.g_x = p->ib_xicon;
			h.g_y = p->ib_yicon;
			h.g_w = p->ib_wicon;
			h.g_h = p->ib_hicon;
			h.g_x += ox2;
			h.g_y += oy2;

			if (xd_inrect(x, y, &h))
			{
				object = i - 1;
			} else
			{
				/* then look at icon label rectangle */

				h.g_x = p->ib_xtext;
				h.g_y = p->ib_ytext;
				h.g_w = p->ib_wtext;
				h.g_h = p->ib_htext;
				h.g_x += ox2;
				h.g_y += oy2;

				if (xd_inrect(x, y, &h))
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

void remove_icon(_WORD object, bool draw)
{
	ITMTYPE type = icn_type(desk_window, object);

	if (draw)
		redraw_icon(object, 1);			/* erase icon */

	free(desktop[object + 1].ob_spec.ciconblk);
	objc_delete(desktop, object + 1);

	if (isfilenet(type))
		free(desk_icons[object].icon_dat.name);

	desk_icons[object].item_type = ITM_NOTUSED;
}


/*
 * Set icon name from the full name of the item.
 * Icon label is changed only if it reflects real item name,
 * otherwise, icon label is kept.
 */

static void icn_set_name(ICON *icn, char *newiname)
{
	char *tname = icn->icon_dat.name;

	/* If label is same as (old) item name, copy new name */

	if (!x_netob(tname) && strncmp(icn->label, fn_get_name(tname), 12) == 0)
		strsncpy(icn->label, fn_get_name(newiname), sizeof(icn->label));

	/* If name has changed, deallocate old name string, use new one */

	if (strcmp(tname, newiname) != 0)
	{
		free(tname);
		icn->icon_dat.name = newiname;
	} else
	{
		free(newiname);
	}
}


static void icn_set_update(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2)
{
	ICON *icn = desk_icons;
	char *new;
	_WORD i;

	(void) w;
	for (i = 0; i < max_icons; i++)
	{
		if (isfilenet(icn->item_type))
		{
			if (strcmp(icn->icon_dat.name, fname1) == 0)
			{
				if (type == WD_UPD_DELETED)
					icn->update = ICN_DELETE;

				if (type == WD_UPD_MOVED)
				{
					if (strcmp(fname2, icn->icon_dat.name) != 0)
					{
						if ((new = strdup(fname2)) == NULL)
						{
							return;
						} else
						{
							icn->update = ICN_REDRAW;
							icn_set_name(icn, new);
						}
					}
				}
			}
		}

		icn++;
	}
}


static void dsk_do_update(void)
{
	_WORD i;
	ICON *icn = desk_icons;

	for (i = 0; i < max_icons; i++)
	{
		if (icn->item_type != ITM_NOTUSED)
		{
			if (icn->update == ICN_DELETE)
				remove_icon(i, TRUE);

			if (icn->update == ICN_REDRAW)
			{
				redraw_icon(i, 3);		/* redraw icon, i.e. erase, then draw */
				icn->update = ICN_NO_UPDATE;
			}
		}

		icn++;
	}
}


/*
 * Item function, must have WINDOW * as argument
 */

static void icn_do_update(WINDOW *w)
{
	(void) w;
	dsk_do_update();
}


/********************************************************************
 *																	*
 * Funktie voor het openen van iconen.								*
 *																	*
 ********************************************************************/

/*
 * Open an object represented by a desktop icon
 */

static bool icn_open(WINDOW *w, _WORD item, _WORD kstate)
{
	ICON *icn = &desk_icons[item];
	ITMTYPE type = icn->item_type;

	if (isfilenet(type))				/* file, folder, program or link ? */
	{
		/* If the selected object can be opened... */

		_WORD button;

		if (icn->tgt_type != ITM_NETOB && !x_exist(icn->icon_dat.name, EX_FILE | EX_DIR))
		{
			/* Object does not exist */

			if ((button = alert_printf(1, AAPPNFND, icn->label)) == 3)
				return FALSE;

			if (button == 2)
			{
				remove_icon(item, TRUE);
				return FALSE;
			}

			sl_noop = 0;
			obj_hide(addicon[CHICNSK]);
			button = chng_icon(item);
			xd_close(&icd_info);
			dsk_do_update();

			if (button != CHNICNOK)
				return FALSE;
		}
	}

	return item_open(w, item, kstate, NULL, NULL);
}


/*
 * Provide index of the default icon for the specified item type.
 * First try a default icon according to item type;
 * if that does not succeed, return index of the first icon.
 * Issue an alert that an icon is missing (but only if such an
 * alert has not been issued earlier).
 * String 'name' is retrieved internally but not used further.
 */

_WORD default_icon(ITMTYPE type)
{
	INAME name;
	_WORD it, ic = 0;

	if (!noicons)
	{
		alert_iprint(MICNFND);
		noicons = TRUE;
	}

	/* Choose appropriate name string, depending on item type */

	switch (type)
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
	case ITM_NETOB:
		it = FIINAME;
		break;
	case ITM_TRASH:
		it = TRINAME;
		break;
	case ITM_PRINTER:
		it = PRINAME;
		break;
	default:
		it = 0;
		break;
	}


	/*
	 * Retrieve icon index from icon name. If icon with the specified
	 * name is not found, return -1; if id is 0, return index
	 * of the first icon in the file
	 */

	ic = rsrc_icon_rscid(it, (char *) &name);

	/* Beware; this can return -1 */

	return ic;
}


/*
 * Return index of an icon given by its name in the resource file.
 * If icon with the specified name is not found, return -1.
 * If name is NULL, return index of the first icon in the file.
 */

_WORD rsrc_icon(const char *name)
{
	CICONBLK *h;
	OBJECT *ic = icons;
	_WORD i = 0;

	for (;;)
	{
		if (ic->ob_type == G_ICON || ic->ob_type == G_CICON)
		{
			if (!name)
				return i;				/* index of first -icon- object, maybe not first object */

			h = ic->ob_spec.ciconblk;

			if (strnicmp(h->monoblk.ib_ptext, name, sizeof(INAME) - 1) == 0)
				return i;
		}

		if ((ic->ob_flags & OF_LASTOB) != 0)
			return -1;

		i++;
		ic++;
	}
}


/*
 * This routine gets icon id and name from language-dependent name as
 * defined in desktop.rsc. The routine is used for some basic icon types
 * which should always exist in the icon resource file (floppy, disk, file,
 * folder, etc)
 *
 * id           = index of icon name string in DESKTOP.RSC (e.g. HDINAME, FINAME...)
 * name         = returned icon name from DESKTOP.RSC
 * return value = index of icon in ICONS.RSC
 *
 * If id is 0, or an icon with the name can not be found,
 * index of the first icon is returned. It is assumed that there is
 * at least one icon in the file.
 *
 */

_WORD rsrc_icon_rscid(_WORD id, char *name)
{
	char *nnn = NULL;
	_WORD ic;

	if (id)
		nnn = get_freestring(id);

	ic = rsrc_icon(nnn);

	if (ic < 0)
		ic = rsrc_icon(NULL);

	strsncpy(name, icons[ic].ob_spec.ciconblk->monoblk.ib_ptext, sizeof(INAME));

	return ic;
}


/*
 * Find disk volume label in order to insert it as icon label
 * if that is not specified. Don't do that for floppies, as most likely
 * they will not be in the drive. Beware that this may cause problems with
 * other ejectable disks too.
 * Note A:\ = drive 0, B:\ = drive 1, etc.
 */

static void icn_disklabel(_WORD drive, char *ilabel)
{
	INAME vlabel;

	if (drive > 1 && *ilabel == 0 && x_getlabel(drive, vlabel) == 0)
		strcpy(ilabel, vlabel);
}


/*
 * An aux. size-reducing function which initializes some object data
 * mostly for icon objects.
 */

void init_obj(OBJECT *obj, _WORD otype)
{
	obj->ob_next = -1;
	obj->ob_head = -1;
	obj->ob_tail = -1;
	obj->ob_flags = OF_NONE;
	obj->ob_state = OS_NORMAL;
	obj->ob_type = otype;
	obj->ob_width = iconw;
	obj->ob_height = iconh;
}


/*
 * Add an icon onto the desktop; return index of the icon in desk_icons[].
 * Note1: in case of an error, fname will be deallocated.
 * Note2: see similar object-setting code in set_obji() in windows.c
 */

static _WORD add_icon(ITMTYPE type, ITMTYPE tgttype, bool link, _WORD icon, const char *text,
					  _WORD ch, _WORD x, _WORD y, bool draw, char *fname)
{
	ICON *icn;
	OBJECT *deskto;
	CICONBLK *h;						/* ciconblk (the largest) */
	XATTR attr;
	_WORD i, ix, iy, icon_no;

	h = NULL;
	i = 0;

	/* Find the first unused slot in desktop icons */

	while ((desk_icons[i].item_type != ITM_NOTUSED) && (i < max_icons - 1))
		i++;

	icn = &desk_icons[i];				/* pointer to a slot in icons list */
	deskto = &desktop[i + 1];			/* pointer to a desktop object */

	/* Check for some errors */

	if (icon >= 0)
	{
		if (icn->item_type != ITM_NOTUSED)
		{
			alert_iprint(MTMICONS);
		} else
		{
			if ((h = malloc_chk(sizeof(CICONBLK))) == NULL)	/* ciconblk (the largest) */
			{
				icn->item_type = ITM_NOTUSED;
			} else
			{
				icon_no = min(n_icons - 1, icon);

				/* Put the icon there */

				ix = min(x, m_icnx);
				iy = min(y, m_icny);
				icn->x = ix;
				icn->y = iy;
				icn->tgt_type = tgttype;
				icn->link = link;
				icn->item_type = type;
				icn->icon_index = icon_no;
				icn->selected = FALSE;
				icn->update = ICN_NO_UPDATE;
				icn->icon_dat.name = NULL;	/* this also sets icon_dat.drv to 0 */

				init_obj(deskto, icons[icon_no].ob_type);

				if (link)
				{
					deskto->ob_state |= OS_CHECKED;
					icn->item_type = ITM_FILE;
				}

				if (isfile(type) && x_attr(1, FS_INQ, fname, &attr) >= 0 && ((attr.st_attr & FA_HIDDEN) != 0))
					deskto->ob_state |= OS_DISABLED;

				deskto->ob_x = ix * iconw + icn_xoff;
				deskto->ob_y = iy * iconh + icn_yoff;
				deskto->ob_spec.ciconblk = h;
				*h = *icons[icon_no].ob_spec.ciconblk;

				strsncpy(icn->label, text, sizeof(INAME));
				strsncpy(icn->icon_name, icons[icon_no].ob_spec.ciconblk->monoblk.ib_ptext, sizeof(INAME));

				h->monoblk.ib_char &= 0xFF00;
				h->monoblk.ib_ptext = icn->label;

				switch (type)
				{
				case ITM_DRIVE:
					icn->icon_dat.drv = ch;
					h->monoblk.ib_char |= ch;
					break;
				case ITM_FOLDER:
				case ITM_PREVDIR:
				case ITM_PROGRAM:
				case ITM_FILE:
				case ITM_NETOB:
					icn->icon_dat.name = fname;
					/* fall through */
				case ITM_TRASH:
				case ITM_PRINTER:
					h->monoblk.ib_char |= 0x20;
					break;
				case ITM_NOTUSED:
				case ITM_LINK:
					break;
				}

				objc_add(desktop, 0, i + 1);

				if (draw)
					redraw_icon(i, 2);	/* draw icon */

				return i;
			}
		}
	}

	/* In case of an error, deallocate structures and exit */

	if (isfilenet(type))
		free(fname);

	free(h);

	return -1;
}


/*
 * A shorter form of add_icon() mostly to be used for devices
 * which can not be links and do not have a filename attached.
 * Return index of this icon in desk_icons[]
 */

static _WORD add_devicon(ITMTYPE type, _WORD icon, const char *text, _WORD ch, _WORD x, _WORD y)
{
	return add_icon(type, type, FALSE, icon, text, ch, x, y, TRUE, NULL);
}


/*
 * Compute (round-up) position of an icon on the desktop
 * Do not use min() here; this is shorter.
 */

static void comp_icnxy(_WORD mx, _WORD my, _WORD *x, _WORD *y)
{
	/* Note: do not use min() here, this is shorter */

	*x = (mx - xd_desk.g_x - icn_xoff) / iconw;

	if (*x > m_icnx)
		*x = m_icnx;

	*y = (my - xd_desk.g_y - icn_yoff) / iconh;

	if (*y > m_icny)
		*y = m_icny;
}


static void get_iconpos(_WORD *x, _WORD *y)
{
	_WORD dummy;
	_WORD mx, my;

	xd_begmctrl();
	graf_mouse(USER_DEF, &icnm);
	evnt_button(1, 1, 1, &mx, &my, &dummy, &dummy);
	arrow_mouse();
	xd_endmctrl();
	comp_icnxy(mx, my, x, y);
}


/*
 * Get type of desktop icon (trashcan/printer/drive) from the dialog
 */

static ITMTYPE get_icntype(void)
{
	_WORD object = xd_get_rbutton(addicon, ICBTNS);

	switch (object)
	{
	case ADISK:
		return ITM_DRIVE;
	case APRINTER:
		return ITM_PRINTER;
	case ATRASH:
		return ITM_TRASH;
	default:
		return ITM_FILE;
	}
}


/*
 * Enter desk icon type (trash/printer/drive) into dialog (set radio button).
 * Return start object for dialog redraw.
 */

static _WORD set_icntype(ITMTYPE type)
{
	_WORD startobj, object;

	switch (type)
	{
	case ITM_DRIVE:
		object = ADISK;
		startobj = DRIVEID;
		break;
	case ITM_PRINTER:
		object = APRINTER;
		startobj = ICNLABEL;
		break;
	case ITM_TRASH:
		object = ATRASH;
		startobj = ICNLABEL;
		break;
	default:
		object = AFILE;
		startobj = IFNAME;
		break;
	}

	xd_set_rbutton(addicon, ICBTNS, object);
	return startobj;
}


/*
 * Initiate the icon-selector slider
 */

void icn_sl_init(_WORD line, SLIDER *sl)
{
	sl->type = 0;
	sl->tree = addicon;					/* root dialog object */
	sl->up_arrow = ICNUP;
	sl->down_arrow = ICNDWN;
	sl->slider = ICSLIDER;				/* slider object                        */
	sl->sparent = ICPARENT;				/* slider parent (background) object    */
	sl->lines = 1;						/* number of visible lines in the box   */
	sl->n = n_icons;					/* number of items in the list          */
	sl->line = line;					/* index of first item shown in the box */
	sl->list = NULL;					/* pointer to list of items to be shown */
	sl->set_selector = set_iselector;
	sl->first = 0;						/* first object in the scrolled box     */
	sl->findsel = 0L;

	addicon[ICONDATA].ob_state &= ~OS_SELECTED;

	sl_init(sl);
}


void set_iselector(SLIDER *slider, bool draw, XDINFO *info)
{
	OBJECT *h1;
	OBJECT *ic = icons + slider->line;

	h1 = addicon + ICONDATA;
	h1->ob_type = ic->ob_type;
	h1->ob_spec = ic->ob_spec;

	/* Center the icon image in the background box */

	h1->ob_x = (addicon[ICONBACK].ob_width - ic->ob_width) / 2;
	h1->ob_y = (addicon[ICONBACK].ob_height - ic->ob_height) / 2;

	h1->ob_width = ic->ob_width;
	h1->ob_height = ic->ob_height;

	/*
	 * In low resolutions, move the icon object up for a little bit,
	 * otherwise it goes out of the background box (why?)
	 */

	if (xd_desk.g_h < 300)
		h1->ob_y -= 8;

	if (draw)
		xd_draw(info, ICONBACK, 1);
}


/*
 * Currently, this routine is used only for setting icons.
 * If sl_noop == 1, the dialog will not be opened (assuming already open).
 * This routine may open, but never closes the icon selector dialog.
 */

_WORD icn_dialog(SLIDER *sl_info, _WORD *icon_no, _WORD startobj, _WORD bckpatt, _WORD bckcol)
{
	XUSERBLK *xub = (XUSERBLK *) (addicon[ICONBACK].ob_spec.userblk);
	_WORD rdro = ROOT, button;
	bool again = FALSE;
	static const char so[] = { DRIVEID, ICNLABEL, ICNLABEL, IFNAME };

	/*
	 * Background colour and pattern
	 * (can't use set_selcolpat() here because the dialog is not open yet)
	 */

	xub->uv.fill.colour = bckcol;
	xub->uv.fill.pattern = bckpatt;

	/* Initialize slider */

	icn_sl_init(*icon_no, sl_info);

	/* Loop until told to stop... */

	do
	{
		switch (startobj)
		{
		case DRIVEID:
			obj_unhide(addicon[DRIVEID]);
			goto setmore;
		case ICNLABEL:
		case ICNTYPE:
			obj_hide(addicon[DRIVEID]);
		  setmore:;
			obj_hide(addicon[INAMBOX]);
			break;
		case IFNAME:
			obj_unhide(addicon[INAMBOX]);
			obj_hide(addicon[DRIVEID]);
			xd_init_shift(&addicon[IFNAME], dirname);
			break;
		}

		if (!again && sl_noop == 0)
			xd_open(addicon, &icd_info);
		else
			xd_drawdeep(&icd_info, rdro);

		sl_noop = 1;

		button = sl_form_do(startobj, sl_info, &icd_info) & 0x7FFF;

		again = FALSE;

		/* The following if-thens yield smaller code than a case structure */

		if (button >= ADISK && button <= AFILE)
		{
			xd_set_rbutton(addicon, ICBTNS, button);

			startobj = (_WORD) so[button - ADISK];
			rdro = ROOT;
			*iconlabel = 0;
			again = TRUE;
		} else if (button == ICONDATA)
		{
			rdro = ICONBACK;
			again = TRUE;
		} else if (button == CHNICNOK && *dirname)
		{
			icd_islink = FALSE;
			icd_itm_type = diritem_type(dirname);
			icd_tgt_type = icd_itm_type;
#if _MINT_
			if (mint && (icd_itm_type != ITM_NOTUSED))
			{
				VLNAME tgtname;

				if (x_rdlink(sizeof(VLNAME), (char *) (&tgtname), (const char *) dirname) == 0)
				{
					icd_islink = TRUE;
					icd_tgt_type = diritem_type(tgtname);
				}
			}
#endif
			if (icd_tgt_type == ITM_NOTUSED)
			{
				xd_buttnorm(&icd_info, button);
				again = TRUE;
				rdro = ROOT;
			}
		}
	} while (again);

	xd_buttnorm(&icd_info, button);

	*icon_no = sl_info->line;
	return button;
}


/*
 * Aux. size-optimization function; hide or unhide some dialog objects
 */

static void icn_hidesome(void)
{
	static const _WORD items[] = { ICSHFIL, ICSHFLD, ICSHPRG, ICNTYPE, ICNTYPT, DRIVEID, ADDBUTT, 0 };

	rsc_hidemany(addicon, items);
	obj_unhide(addicon[CHNBUTT]);
	obj_unhide(addicon[ICNLABEL]);
	obj_unhide(addicon[ICBTNS]);
	xd_set_child(addicon, ICBTNS, 0);
}


/*
 * Manually install one or more desktop icons.
 * A dialog is opened to handle this.
 */

void dsk_insticon(WINDOW *w, _WORD n, _WORD *list)
{
	_WORD x, y, startobj, icnind, icon_no, nn = n, i = 0, button = 0;
	ITMTYPE itype;
	SLIDER sl_info;
	char *name;
	const char *nameonly;
	char *ifname = NULL;

	get_iconpos(&x, &y);
	rsc_title(addicon, AITITLE, DTADDICN);
	icn_hidesome();
	obj_hide(addicon[CHICNRM]);

	if (n == 0)
		nn = 1;

	sl_noop = 0;

	options.cprefs &= ~CF_FOLL;

	while ((i < nn) && (button != CHNICNAB))
	{
		*drvid = 0;
		*iconlabel = 0;
		icd_islink = FALSE;
		name = NULL;

		hideskip(n, &addicon[CHICNSK]);

		if (n > 0)
		{
			ITMTYPE ttype;
			bool link;

			if (*list < 0)
				break;
#if _MINT_
		  reread:;

			if (mint)
			{
				set_opt(addicon, options.cprefs, CF_FOLL, IFOLLNK);
				obj_unhide(addicon[IFOLLNK]);
			}
#endif

			ttype = itm_tgttype(w, *list);

			itm_follow(w, *list, &link, &name, &itype);

			if (name == NULL)
			{
				xform_error(ENOMEM);
				break;					/* go to update */
			} else
			{
				nameonly = fn_get_name(name);
#if _MINT_
				cramped_name(nameonly, iconlabel, sizeof(INAME));
#else
				strcpy(iconlabel, nameonly);	/* shorter, and safe in single-TOS */
#endif
				icon_no = icnt_geticon(nameonly, itype, ttype);
				strsncpy(dirname, name, sizeof(VLNAME));
				button = AFILE;
				startobj = IFNAME;
				free(name);				/* name is not needed anymore */
			}
		} else							/* (n <= 0) */
		{
			*dirname = 0;
			xd_set_child(addicon, ICBTNS, 1);

			icon_no = rsrc_icon_rscid(HDINAME, iconlabel);
			button = ADISK;
			startobj = DRIVEID;
		}

		xd_set_rbutton(addicon, ICBTNS, button);

		button = icn_dialog(&sl_info, &icon_no, startobj, options.dsk_pattern, options.dsk_colour);

#if _MINT_
		if (button == IFOLLNK)
		{
			/*
			 * If state of the  'follow link' has changed, return to the
			 * beginning and redisplay the dialog.
			 */

			options.cprefs ^= CF_FOLL;
			goto reread;
		}
#endif

		if (button == CHNICNOK)
		{
			itype = get_icntype();

			if (itype == ITM_DRIVE)
			{
				if (*drvid < 'A')
				{
					dsk_diskicons(&x, &y, icon_no, iconlabel);
					break;
				}

				icn_disklabel(*drvid - 'A', iconlabel);
			} else
			{
				*drvid = 0;
			}

			if (itype == ITM_FILE)		/* file or folder or program or link */
			{
				itype = icd_itm_type;
				ifname = strdup(dirname);
			} else
			{
				icd_tgt_type = itype;
			}

			icnind = add_icon(itype, icd_tgt_type, icd_islink, icon_no, iconlabel, *drvid & 0x5F, x, y, FALSE, ifname	/* deallocated inside if add_icon fails */
				);

			if (icnind < 0)
				break;

			desk_icons[icnind].update = ICN_REDRAW;

			incr_pos(&x, &y);
		}

		i++;
		list++;

	}									/* loop */

	xd_close(&icd_info);

	dsk_do_update();

	obj_unhide(addicon[CHICNRM]);
	obj_hide(addicon[IFOLLNK]);
}


/*
 * Size optimization function for confirming that an itemtype is
 * that of a trashcan or a printer. Return string index
 * corresponding to item type, or 0 if type is not a printer
 * or a trashcan.
 * Now this routine is extended to handle unknown objects and network objects.
 */

_WORD trash_or_print(ITMTYPE type)
{
	switch (type)
	{
	case ITM_TRASH:
		return MTRASHCN;
	case ITM_PRINTER:
		return MPRINTER;
	case ITM_NOTUSED:
		return MUNKNOWN;
	case ITM_NETOB:
		return MNETOB;
	default:
		return 0;
	}
}


/*
 * Handle the dialog for editing a desk icon.
 * Parameter 'object' is the index of the icon in desk_icons[]
 */

static _WORD chng_icon(_WORD object)
{
	_WORD button = -1, icon_no, startobj;
	ICON *icn = &desk_icons[object];
	SLIDER sl_info;

	rsc_title(addicon, AITITLE, DTCHNICN);
	icn_hidesome();

	startobj = set_icntype(icn->item_type);	/* set type into dialog  */

	if (startobj == IFNAME)
	{
		*drvid = 0;
		strsncpy(dirname, icn->icon_dat.name, sizeof(VLNAME));
	} else
	{
		*drvid = (char) icn->icon_dat.drv;
	}

	strcpy(iconlabel, icn->label);
	icon_no = icn->icon_index;

	/* Dialog for selecting icon and icon type */

	button = icn_dialog(&sl_info, &icon_no, startobj, options.dsk_pattern, options.dsk_colour);

	if (button == CHICNRM)
		icn->update = ICN_DELETE;

	if (button == CHNICNOK)
	{
		CICONBLK *h = desktop[object + 1].ob_spec.ciconblk;	/* ciconblk (the largest) */

		icn->update = ICN_REDRAW;
		desktop[object + 1].ob_type = icons[icon_no].ob_type;	/* may change colours ;-) */
		icn->icon_index = icon_no;

		strsncpy(icn->label, iconlabel, sizeof(INAME));
		strsncpy(icn->icon_name, icons[icon_no].ob_spec.ciconblk->monoblk.ib_ptext, sizeof(INAME));

		*h = *icons[icon_no].ob_spec.ciconblk;
		h->monoblk.ib_ptext = icn->label;
		h->monoblk.ib_char &= 0xFF00;

		icn->item_type = get_icntype();	/* get icontype from the dialog */

		if (startobj == DRIVEID)
		{
			if (icn->item_type != ITM_DRIVE)
				*drvid = 0;

			icn->icon_dat.drv = toupper(*drvid);
			h->monoblk.ib_char |= icn->icon_dat.drv;
		} else
		{
			h->monoblk.ib_char |= 0x20;

			if (startobj == IFNAME)
			{
				char *new;

				/* Note: must not clear the filename */

				if (*dirname && (new = strdup(dirname)) != NULL)
				{
					icn_set_name(icn, new);
					icn->item_type = icd_itm_type;
					icn->tgt_type = icd_tgt_type;
					icn->link = icd_islink;
				} else
				{
					button = CHNICNAB;
				}
			}
		}
	}

	return button;
}


/*
 * Move a number of icons to a new position on the desktop
 */

static void mv_icons(ICND *icns, _WORD n, _WORD mx, _WORD my)
{
	_WORD i, x, y, obj;
	ICND *icnsi = icns;

	for (i = 0; i < n; i++)
	{
		obj = icnsi->item;
		redraw_icon(obj, 1);			/* erase icon */

		x = (mx + icnsi->m_x - xd_desk.g_x - icn_xoff) / iconw;
		y = (my + icnsi->m_y - xd_desk.g_y - icn_yoff) / iconh;

		/* Note: do not use min() here, this is shorter */

		if (x > m_icnx)
			x = m_icnx;

		if (y > m_icny)
			y = m_icny;

		desk_icons[obj].x = x;
		desk_icons[obj].y = y;
		desktop[obj + 1].ob_x = x * iconw + icn_xoff;
		desktop[obj + 1].ob_y = y * iconh + icn_yoff;

		redraw_icon(obj, 2);			/* draw icon */

		icnsi++;
	}
}


/*
 * Install a desktop icon by moving a directory item to the desktop,
 * or manipulate desktop icons in other ways.
 */

static bool icn_copy(WINDOW *dw,		/* pointer to destination window */
					 _WORD dobject,		/* destination object index */
					 WINDOW *sw,		/* pointer to source window */
					 _WORD n,			/* number of items to copy */
					 _WORD *list,		/* list of item indixes to copy */
					 ICND *icns,		/* pointer to icons */
					 _WORD x,			/* position */
					 _WORD y,			/* position */
					 _WORD kstate		/* keyboard state */
	)
{
	_WORD i;
	_WORD item, ix, iy, icon;
	ITMTYPE type, ttype;
	INAME tolabel;
	bool link;
	char *fname;
	const char *nameonly;

	/*
	 * A specific situation can arise when object are moved on the desktop:
	 * if selected icons are moved so that the cursor is, at the time
	 * of button release, on one of the selected icons, a copy is attempted
	 * instead of icon movement. Code below cures this problem.
	 * An opportunity is taken to check if only icons should be
	 * removed or files and folders really deleted.
	 */

	if (dobject > -1 && sw == desk_window && dw == desk_window)
	{
		if (itm_type(dw, dobject) == ITM_TRASH)
		{
			_WORD b = alert_printf(1, AREMICNS);

			if (b == 1)
			{
				dsk_chngicon(n, list, FALSE);
				return TRUE;
			} else if (b == 3)
			{
				return FALSE;
			}
		}

		for (i = 0; i < n; i++)
		{
			if (list[i] == dobject)
			{
				dobject = -1;
				break;
			}
		}
	}

	/* Now, act accordingly */

	if (dobject != -1)
		/* Copy to an object */
		return item_copy(dw, dobject, sw, n, list, kstate);

	if (sw == desk_window)
	{
		/* Move the icons about on the desktop */

		mv_icons(icns, n, x, y);
		return FALSE;
	}

	/* Install desktop icons */
	comp_icnxy(x, y, &ix, &iy);

	/*
	 * Note: currently it is not possible to follow links.
	 * A dialog could be opened here to enable it.
	 * If the user wishes to follow the links when installing
	 * desktop icons, he should 'Set desk icons...'
	 */

	options.cprefs &= ~CF_FOLL;

	for (i = 0; i < n; i++)
	{
		if ((item = list[i]) < 0)	/* was -1 earlier */
			return FALSE;

		ttype = itm_tgttype(sw, item);

		itm_follow(sw, item, &link, &fname, &type);

		/*
		 * Note: name will be deallocated in add_icon() if it fails;
		 * Otherwise it must be kept; add_icon just uses the pointer
		 */

		if (fname == NULL)
			return FALSE;
		nameonly = fn_get_name(fname);
#if _MINT_
		cramped_name(nameonly, tolabel, sizeof(INAME));
#else
		strcpy(tolabel, nameonly);	/* shorter, and safe in single-TOS */
#endif
		icon = icnt_geticon(nameonly, type, ttype);
		add_icon(type, ttype, link, icon, tolabel, 0, ix, iy, TRUE, fname);
		incr_pos(&ix, &iy);
	}

	return TRUE;
}


/*
 * funkties voor het laden en opslaan van de posities van de iconen.
 * Remove all desktop icons
 */

static void rem_all_icons(void)
{
	_WORD i;

	for (i = 0; i < max_icons; i++)
	{
		if (desk_icons[i].item_type != ITM_NOTUSED)
			remove_icon(i, FALSE);
	}
}


/*
 * Temporarily change the type of desktop object.
 * However, if desktop colour is 0, do nothing.
 */

static void set_dsk_obtype(_WORD type)
{
	if ((desktop[0].ob_type & 0xFF00) != 0)
	{
		dxub.ob_type &= 0xFF00;
		dxub.ob_type |= type;
	} else
	{
		if (options.dsk_colour > 0)
			desktop[0].ob_type = type;
	}
}


/*
 * Set desktop background pattern and colour. This routine does -not-
 * actually change display, just sets colour and pattern indices
 * in TeraDesk. It can also set background (desktop) object type
 * to userdef, depending on colour (in order to display more colours)
 */

void set_dsk_background(_WORD pattern, _WORD colour)
{
	options.dsk_pattern = pattern;
	options.dsk_colour = colour;

	if (desktop[0].ob_type == G_BOX)
	{
		if (colour > 0)
		{
			desktop[0].ob_type |= (XD_BCKBOX << 8);
			xd_xuserdef(&desktop[0], &dxub, ub_bckbox);
		} else
		{
			desktop[0].ob_spec.obspec.interiorcol = colour;
			desktop[0].ob_spec.obspec.fillpattern = pattern;
		}
	}

	dxub.uv.fill.colour = colour;
	dxub.uv.fill.pattern = pattern;
	dxub.ob_flags = 0;
}


#if 0									/* there is in fact no need for his routine */

/*
 * Dummy routine: desktop keyboard handler
 */

static _WORD dsk_hndlkey(WINDOW *w, _WORD dummy_scancode, _WORD dummy_keystate)
{
	return 0;
}

#endif


/*
 * (Re)generate desktop and draw menu bar
 */
void regen_desktop(OBJECT *desk_tree)
{
	wind_set_ptr_int(0, WF_NEWDESK, desk_tree, 0);
	dsk_draw();

	if (desk_tree)
	{
		menu_bar(menu, 1);
	}
}


static ICONINFO this;


/*
 * Configuration table for one desktop icon
 */

static CfgEntry const Icon_table[] = {
	CFG_HDR("icon"),
	CFG_BEG(),
	CFG_S("name", this.ic.icon_name),
	CFG_S("labl", this.ic.label),
	CFG_E("type", this.ic.item_type),
	CFG_E("tgtt", this.ic.tgt_type),
	CFG_BD("link", this.ic.link),
	CFG_D("driv", this.ic.icon_dat.drv),
	CFG_S("path", this.name),
	CFG_D("xpos", this.ic.x),
	CFG_D("ypos", this.ic.y),
	CFG_END(),
	CFG_LAST()
};


/*
 * Load or save configuration of one desktop icon.
 * Beware: this.ic.icon_dat.drv is 0 to 26; NOT 'A' to 'Z'
 */

static void icon_cfg(XFILE *file, int lvl, int io, int *error)
{
	_WORD i;

	*error = 0;

	if (io == CFG_SAVE)
	{
		ICON *ic = desk_icons;

		for (i = 0; i < max_icons; i++)
		{
			if (ic->item_type != ITM_NOTUSED)
			{
				this.ic = *ic;
				this.name[0] = 0;

				if (ic->item_type == ITM_DRIVE)
					this.ic.icon_dat.drv -= 'A';
				else
					this.ic.icon_dat.drv = 0;

				if (ic->tgt_type == ic->item_type)
					this.ic.tgt_type = 0;	/* for smaller config files */

				if (isfilenet(ic->item_type))
					strcpy(this.name, ic->icon_dat.name);

				*error = CfgSave(file, Icon_table, lvl, CFGEMP);
			}

			ic++;

			if (*error != 0)
				break;
		}
	} else
	{
		memclr(&this, sizeof(this));

		*error = CfgLoad(file, Icon_table, (_WORD) sizeof(VLNAME), lvl);

		if (*error == 0)
		{
			ITMTYPE it = this.ic.item_type;

			if (this.ic.icon_name[0] == 0
				|| ((it < ITM_DRIVE || it > ITM_FILE) && it != ITM_NETOB) || this.ic.icon_dat.drv > ('Z' - 'A'))
			{
				*error = EFRVAL;
			} else
			{
				_WORD nl, icon;
				char *name = NULL;

				/* SOME icon index should always be provided */

				icon = rsrc_icon(this.ic.icon_name);	/* find by name */

				if (icon < 0)
					icon = default_icon(it);	/* display an alert */

				nl = (_WORD) strlen(this.name);

				if (nl)
					name = strdup(this.name);
				else if (isfilenet(it))
					name = strdup(empty);

				if (nl == 0 || name != NULL)
				{
					if (this.ic.tgt_type == 0)	/* for smaller config files */
						this.ic.tgt_type = it;

					*error = add_icon
						(it,
						 (ITMTYPE) this.ic.tgt_type,
						 this.ic.link,
						 icon, this.ic.label, this.ic.icon_dat.drv + 'A', this.ic.x, this.ic.y, FALSE, name);
				} else
				{
					*error = ENOMEM;
				}
			}
		}
	}
}


/*
 * Configuration table for desktop icons
 */

static CfgEntry const DskIcons_table[] = {
	CFG_HDR("deskicons"),
	CFG_BEG(),
	CFG_D("xoff", icn_xoff),
	CFG_D("yoff", icn_yoff),
	CFG_D("iconw", iconw),
	CFG_D("iconh", iconh),
	CFG_NEST("icon", icon_cfg), /* Repeating group */
	CFG_ENDG(),
	CFG_LAST()
};


/*
 * Load or save configuration of desktop icons
 */

void dsk_config(XFILE *file, int lvl, int io, int *error)
{
	*error = handle_cfg(file, DskIcons_table, lvl, CFGEMP, io, rem_all_icons, dsk_default);

	if (io == CFG_LOAD && *error >= 0)
		regen_desktop(desktop);
}


static _WORD aes_supports_coloricons(void)
{
	OBJECT *tree = NULL;
	OBJECT *obj;

	if (!rsrc_gaddr(R_TREE, 0, &tree) || tree == NULL || tree[0].ob_type != G_BOX)
		return FALSE;
	obj = &tree[1];
	/*
	 * The ob_spec of a coloricon has an index, not a file offset,
	 * therefore the ob_spec of the first coloricon has
	 * a value of zero in the resource file.
	 * If after loading it is still zero, or points to the resource header,
	 * or the object type was changed, assume there is no coloricon support
	 */
	if (obj->ob_type != G_CICON)
	{
		return FALSE;
	}
	if (obj->ob_spec.index == 0)
	{
		return FALSE;
	}
	if ((void *) obj->ob_spec.index == _AESrscmem)
	{
		return FALSE;
	}
	return TRUE;
}


/*
 * Load the icon file. Result: TRUE if no error
 * Use standard functions of the AES
 */

bool load_icons(void)
{
	OBJECT **svtree = _AESrscfile;
	void *svrshdr = _AESrscmem;

	/*
	 * Geneva 4 returns information that it supports colour icons, but
	 * that doesn't seem to work; thence a fix below:
	 * if reading of colour icons file fails, then fall back to black/white.
	 * Therefore, in Geneva 4 (and other similar cases, if any), remove
	 * cicons.rsc from TeraDesk folder. Geneva 6 seems to work ok.
	 * Colour icons can be loaded only if this capability has
	 * previously been detected.
	 * 
	 * Do not simplify the first statement below.
	 */

	/* try to load colour icons */
	if (!rsrc_load("cicons.rsc"))
	{
		colour_icons = FALSE;
	} else if (!aes_supports_coloricons())
	{
		colour_icons = FALSE;
		rsrc_free();
	}

	if (!colour_icons && !rsrc_load("icons.rsc"))	/* try to load mono icons */
	{
		alert_abort(MICNFRD);			/* no icons loaded */
		return FALSE;
	} else
	{
		_WORD i = 0;

		rsrc_gaddr(R_TREE, 0, &icons);	/* That's all you need. */
		svicntree = _AESrscfile;
		svicnrshdr = _AESrscmem;
		n_icons = 0;
		icons++;

		do
		{
			n_icons++;
		} while ((icons[i++].ob_flags & OF_LASTOB) == 0);
	}

	_AESrscfile = svtree;
	_AESrscmem = svrshdr;

	return TRUE;
}


void free_icons(void)
{
	OBJECT **svtree = _AESrscfile;
	void *svrshdr = _AESrscmem;

	_AESrscfile = svicntree;
	_AESrscmem = svicnrshdr;

	rsrc_free();

	_AESrscfile = svtree;
	_AESrscmem = svrshdr;
}


/*
 * Routine voor het initialiseren van de desktop.
 * Return FALSE in case of error.
 */

bool dsk_init(void)
{
	_WORD i, error;
	bfobspec *obsp0;					/* save in size by pointing to obspec of desktop root */

	/* Open the desktop window */

	if ((desk_window = xw_open_desk(DESK_WIND, &dsk_functions, sizeof(DSK_WINDOW), &error)) != NULL)
	{
		((DSK_WINDOW *) desk_window)->itm_func = &itm_func;

		/*
		 * Determine the number of icons that can fit on the screen.
		 * Allow 50% overlapping, and also always allow for at least 64 icons.
		 * This should ensure that configuration files for at least ST-High
		 * can be loaded even in ST-Low resolution
		 */

		set_maxicons();
		max_icons = max((m_icnx + 1) * (m_icny + 1) * 3 / 2, 64);

		if (((desktop = malloc_chk((long) (max_icons + 1) * sizeof(OBJECT))) != NULL)
			&& ((desk_icons = malloc_chk((size_t) max_icons * sizeof(ICON))) != NULL))
		{
			init_obj(&desktop[0], G_BOX);

			obsp0 = &desktop[0].ob_spec.obspec;
			obsp0->framesize = 0;
			obsp0->fillpattern = dsk_defaultpatt();
			obsp0->interiorcol = G_GREEN;

			/* override size set in init_obj() */
			desktop[0].ob_x = xd_desk.g_x;
			desktop[0].ob_y = xd_desk.g_y;
			desktop[0].ob_width = xd_desk.g_w;
			desktop[0].ob_height = xd_desk.g_h;

			/* Mark all icon slots as unused */

			for (i = 0; i < max_icons; i++)
				desk_icons[i].item_type = ITM_NOTUSED;

#ifdef MEMDEBUG
			atexit(rem_all_icons);
#endif
			return TRUE;
		} else
		{
			free(desktop);
		}
	} else
	{
		if (error == ENOMEM)
			xform_error(ENOMEM);
	}

	return FALSE;
}


/*
 * Add missing disk icons A to Z. Icon and label are the same for all
 * disks (i.e. it is possible here that a floppy gets the hard-disk icon).
 * If icon name (label) is not specified, disk volume label is read
 * (except for A and B disks).
 */

static void dsk_diskicons(_WORD *x, _WORD *y, _WORD ic, char *iconname)
{
	ICON *icn;
	long drives;
	_WORD d, i, j;
	bool have;

	drives = drvmap();

	/* Check all drive letters A to Z (drives '0' to '9' not supported) */

	for (i = 0; i <= ('Z' - 'A'); i++)
	{
		d = 'A' + i;
		have = FALSE;

		/* Is there an icon for it ? */

		icn = desk_icons;

		for (j = 0; j < max_icons; j++)
		{
			if ((icn->item_type == ITM_DRIVE) && (icn->icon_dat.drv == d))
			{
				have = TRUE;
				break;
			}

			icn++;
		}

		if (!have && btst(drives, i))
		{
			/* If icon label is not specified, use disk volume labels */

			_WORD icnind;

			INAME thelabel;

			strcpy(thelabel, iconname);	/* needed for more than one drive */
			icn_disklabel(i, thelabel);

			icnind = add_devicon(ITM_DRIVE, ic, thelabel, d, *x, *y);

			if (icnind < 0)
				break;

			desk_icons[icnind].update = ICN_REDRAW;

			incr_pos(x, y);
		}
	}
}


/*
 * Return default desktop pattern, depending on current video mode.
 * This mimicks the behaviour of the built-in desktop.
 */

_WORD dsk_defaultpatt(void)
{
	return (xd_ncolours > 2) ? 7 : 4;
}


/*
 * Remove all icons, set desktop background,
 * then add default icons (disk drives, printer and trash can)
 */

void dsk_default(void)
{
	_WORD x = 2;
	_WORD y = 0;
	_WORD ic;

	rem_all_icons();
	set_dsk_background(dsk_defaultpatt(), G_GREEN);

	/* Identify icons by name, not index;  use names from the rsc file. */

	/* Two floppies in the first row */

	ic = rsrc_icon_rscid(FLINAME, iname);
	add_devicon(ITM_DRIVE, ic, iname, 'A', 0, 0);
	add_devicon(ITM_DRIVE, ic, iname, 'B', 1, 0);

	/* Hard disks; continue after the floppies */

	ic = rsrc_icon_rscid(HDINAME, iname);
	dsk_diskicons(&x, &y, ic, iname);

	/* Printer and trashcan in the next two rows */

	y++;
	ic = rsrc_icon_rscid(PRINAME, iname);
	add_devicon(ITM_PRINTER, ic, iname, 0, 0, y);

	y++;
	ic = rsrc_icon_rscid(TRINAME, iname);
	add_devicon(ITM_TRASH, ic, iname, 0, 0, y);
}


/*
 * Change a desktop icon (or more than one of them).
 * This routine should be activated only if n > 0.
 * The routine also handles removal of a whole group of icons
 * without opening the dialog, if dialog == FALSE.
 */

void dsk_chngicon(_WORD n, _WORD *list, bool dialog)
{
	_WORD *ilist = list, button = -1, i = 0;

	sl_noop = 0;

	hideskip(n, &addicon[CHICNSK]);

	while ((i < n) && (button != CHNICNAB))
	{
		if (dialog)
			button = chng_icon(*ilist);	/* The dialog */
		else
			desk_icons[*ilist].update = ICN_DELETE;

		i++;
		ilist++;
	}

	if (dialog)
		xd_close(&icd_info);

	/*
	 * When all is finished and the dialog is closed,
	 * redraw affected icons.
	 */

	dsk_do_update();
}


/*
 * Limit a colour index to values existing in the current video mode.
 * (there will always be at last two colours)
 */

_WORD limcolour(_WORD col)
{
	return minmax(0, col, xd_ncolours - 1);
}


/*
 * Limit a pattern index to the available number of patterns and
 * hatches. In the earlier versions of TeraDesk, patterns were
 * limited to the first 8 because patterns specified in OBSPEC are
 * limited to 3-bit indices. With the change to userdef root object,
 * the limitations of AES became irelevant.
 */

_WORD limpattern(_WORD pat)
{
	return minmax(1, pat, 37);
}


/*
 * Set pattern in a colour / pattern selector box.
 * The dialog must be opened in order to do this.
 * Pattern and colour ranges are limited.
 * If parameter pat is negative, the object is not drawn,
 * and the fill is set to solid.
 */

void set_selcolpat(XDINFO *info, _WORD obj, _WORD col, _WORD pat)
{
	XUSERBLK *xub = (XUSERBLK *) (info->tree[obj].ob_spec.userblk);

	xub->uv.fill.colour = limcolour(col);

	if (pat >= 0)
	{
		xub->uv.fill.pattern = limpattern(pat);
		xd_drawthis(info, obj);
	} else
	{
		xub->uv.fill.pattern = 0;
	}
}


/*
 * Handle window options dialog
 */

void dsk_options(void)
{
	XUSERBLK *xubd = (XUSERBLK *) (wdoptions[DSKPAT].ob_spec.userblk);
	XUSERBLK *xubw = (XUSERBLK *) (wdoptions[WINPAT].ob_spec.userblk);
	XDINFO info;
	_WORD colour, pattern, wcolour, wpattern, button = 0;
	bool ok = FALSE;
	bool icn_rearr = FALSE;
	bool stop = FALSE;
	bool draw = FALSE;

	/* Initial values... */

	icn_itoa(options.tabsize, TABSIZE);
	icn_itoa(iconw, IXGRID);
	icn_itoa(iconh, IYGRID);
	icn_itoa(icn_xoff, IXOF);
	icn_itoa(icn_yoff, IYOF);

	set_opt(wdoptions, options.sexit, SAVE_WIN, SOPEN);

	xubd->uv.fill.pattern = options.dsk_pattern;
	xubd->uv.fill.colour = options.dsk_colour;
	xubw->uv.fill.pattern = options.win_pattern;
	xubw->uv.fill.colour = options.win_colour;

	/* Open the dialog */

	if (chk_xd_open(wdoptions, &info) >= 0)
	{
		while (!stop)
		{
			colour = xubd->uv.fill.colour;
			pattern = xubd->uv.fill.pattern;
			wcolour = xubw->uv.fill.colour;
			wpattern = xubw->uv.fill.pattern;

			button = xd_form_do(&info, ROOT);

			switch (button)
			{
			case WODIR:
			case WOVIEWER:
				{
					_WORD oldcol = options.win_colour, oldpat = options.win_pattern;

					options.win_colour = wcolour;
					options.win_pattern = wpattern;

					if (wd_type_setfont(button))
						ok = TRUE;

					xd_drawbuttnorm(&info, button);

					options.win_colour = oldcol;
					options.win_pattern = oldpat;
				}
				break;
			case DSKCUP:
				colour++;
				set_selcolpat(&info, DSKPAT, colour, pattern);
				break;
			case DSKCDOWN:
				colour--;
				set_selcolpat(&info, DSKPAT, colour, pattern);
				break;
			case DSKPUP:
				pattern++;
				set_selcolpat(&info, DSKPAT, colour, pattern);
				break;
			case DSKPDOWN:
				pattern--;
				set_selcolpat(&info, DSKPAT, colour, pattern);
				break;
			case WINCUP:
				wcolour++;
				set_selcolpat(&info, WINPAT, wcolour, wpattern);
				break;
			case WINCDOWN:
				wcolour--;
				set_selcolpat(&info, WINPAT, wcolour, wpattern);
				break;
			case WINPUP:
				wpattern++;
				set_selcolpat(&info, WINPAT, wcolour, wpattern);
				break;
			case WINPDOWN:
				wpattern--;
				set_selcolpat(&info, WINPAT, wcolour, wpattern);
				break;
			case WOPTOK:
				/* Desktop pattern & colour */

				if ((options.dsk_pattern != pattern) || (options.dsk_colour != colour))
				{
					set_dsk_background(pattern, colour);
					draw = TRUE;
				}

				/* window pattern & colour */

				if ((options.win_pattern != wpattern) || (options.win_colour != wcolour))
				{
					options.win_pattern = wpattern;
					options.win_colour = wcolour;
					ok = TRUE;
				}

				/* Tab size */

				options.tabsize = icn_atoi(TABSIZE);

				if (options.tabsize < 1)
					options.tabsize = 1;

				/* Icon grid size */

				{
					_WORD iw, ih, xo, yo;

					iw = icn_atoi(IXGRID);
					ih = icn_atoi(IYGRID);

					if (iw < ICON_W / 2)
						iw = ICON_W / 2;

					if (ih < ICON_H / 2)
						ih = ICON_H / 2;

					xo = icn_atoi(IXOF);
					yo = icn_atoi(IYOF);

					if (iconw != iw || iconh != ih || icn_xoff != xo || icn_yoff != yo)
					{
						iconw = iw;
						iconh = ih;
						icn_xoff = xo;
						icn_yoff = yo;
						icn_rearr = TRUE;

						set_maxicons();	/* how many icons can be in a row or column */
					}
				}

				/* Save open windows? */

				get_opt(wdoptions, &options.sexit, SAVE_WIN, SOPEN);
				stop = TRUE;
				break;
			default:
				stop = TRUE;
				break;
			}
		}

		xd_buttnorm(&info, button);
		xd_close(&info);

		/* 
		 * Updates must not be done before the dialogs are closed.
		 * Update only what is needed 
		 */

		if (icn_rearr)
			put_newgrid();

		if (draw)
			redraw_desk(&xd_desk);

		if (ok)
			wd_drawall();
	}
}
