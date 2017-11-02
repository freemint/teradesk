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

#include "xscncode.h"
#include "xerror.h"


/* 
 * It seems that information on the small font is not in fact needed,
 * and so that code is disabled by seting _SMALL_FONT to 0
 */

#define _SMALL_FONT 0

_WORD aes_flags = 0;					/* proper appl_info protocol (works with ALL Tos) */
_WORD colour_icons = 0;					/* result of appl_getinfo(2,,,)  */
_WORD aes_wfunc = 0;					/* result of appl_getinfo(11,,,) */
_WORD aes_ctrl = 0;						/* result of appl_getinfo(65,,,) */
_WORD xd_colaes = 0;					/* more than 4 colours available */
_WORD xd_has3d = 0;						/* result of appl-getinfo(13...) */
_WORD aes_hor3d = 0;					/* 3d enlargement value */
_WORD aes_ver3d = 0;					/* 3d enlargement value */
_WORD xd_fdo_flag = FALSE;				/* Flag voor form_do  */
_WORD xd_bg_col = G_WHITE;				/* colour of background object */
_WORD xd_ind_col = G_LWHITE;			/* colour of indicator object  */
_WORD xd_act_col = G_LWHITE;			/* colour of activator object  */
_WORD xd_sel_col = G_BLACK;				/* colour of selected object   */

_WORD brd_l, brd_r, brd_u, brd_d;		/* object border sizes */

/* Window elements for xdialog wndows */

#define XD_WDFLAGS ( NAME | MOVER | CLOSER )

_WORD xd_dialmode = XD_BUFFERED;
_WORD xd_posmode = XD_CENTERED;			/* Position mode */
_WORD xd_vhandle;						/* Vdi handle for library functions */
_WORD xd_nplanes;						/* Number of planes the current resolution */
_WORD xd_ncolours;						/* Number of colours in the current resolution */
_WORD xd_fnt_w;							/* screen font width */
_WORD xd_fnt_h;							/* screen font height */
_WORD xd_pix_height;					/* pixel size */
_WORD xd_rbdclick = 1;					/* if 1, right button is always doubleclick */
_WORD xd_min_timer;						/* Minimum time passed to xe_multi(). */

void *(*xd_malloc) (size_t size);
void (*xd_free) (void *block);

const char *xd_prgname;					/* Name of program, used in title bar of windows */

char *xd_cancelstring;					/* Possible texts on cancel/abort/undo buttons */

OBJECT *xd_menu = NULL;					/* Pointer to menu tree of the program */

/*
 function which is called during a 
 form_do, if the message is not for
 the dialog window */
static _WORD(*xd_usermessage) (_WORD *message);
static _WORD xd_nmnitems;				/* Number of menu titles that have to be disabled */

/* Array with indices to menu titles in xd_menu,
   which have to be disabled. The first index is
   not a title, but the index of the info item. */
static const _WORD *xd_mnitems;
static _WORD xd_upd_ucnt = 0;			/* Counter for wind_update */
static _WORD xd_upd_mcnt = 0;			/* Counter for wind_update */
static _WORD xd_msoff_cnt = 0;			/* Counter for xd_mouse_on/xd_mouse_off */


static _WORD xd_oldbutt = -1;

XDOBJDATA *xd_objdata = NULL;			/* Arrays with USERBLKs */

XDINFO *xd_dialogs = NULL;				/* Chained list of modal dialog boxes. */
XDINFO *xd_nmdialogs = NULL;			/* List with non modal dialog boxes. */

GRECT xd_screen;						/* screen dimensions */
GRECT xd_desk;							/* Dimensions of desktop background. */

#if _SMALL_FONT
XDFONT xd_small_font;					/* small font definition */
#endif
XDFONT xd_regular_font;					/* system font definition */


static void __xd_redraw(WINDOW *w, GRECT *area);
static void __xd_moved(WINDOW *w, GRECT *newpos);



static WD_FUNC xd_wdfuncs = {
	0L,									/* hndlkey */
	0L,									/* hndlbutton */
	__xd_redraw,						/* redraw */
	__xd_topped,						/* topped */
	xw_nop1,							/* bottomed */
	__xd_topped,						/* newtop */
	0L,									/* closed */
	0L,									/* fulled */
	xw_nop2,							/* arrowed */
	0L,									/* hslider */
	0L,									/* vslider */
	0L,									/* sized */
	__xd_moved,							/* moved */
	0L,									/* hndlmenu */
	0L,									/* top */
	0L,									/* iconify */
	0L									/* uniconify */
};


static WD_FUNC xd_nmwdfuncs = {
	__xd_hndlkey,						/* hndlkey */
	__xd_hndlbutton,					/* hndlbutton */
	__xd_redraw,						/* redraw */
	__xd_topped,						/* topped */
	xw_nop1,							/* bottomed */
	__xd_topped,						/* newtop */
	__xd_closed,						/* closed */
	0L,									/* fulled */
	xw_nop2,							/* arrowed */
	0L,									/* hslid */
	0L,									/* vslid */
	0L,									/* sized */
	__xd_moved,							/* moved */
	0L,									/* __xd_hndlmenu, currently there are no menus in nonmodal dialogs */
	__xd_top,							/* top */
	0L,									/* uniconify */
	0L									/* iconify */
};

/********************************************************************
 *																	*
 * Hulpfunkties.													*
 *																	*
 ********************************************************************/

/*
 * Clear the events structure with zeros
 */

void xd_clrevents(XDEVENT *ev)
{
	memset(ev, 0, sizeof(XDEVENT));
}


/* 
 * Funktie voor het opslaan van het scherm onder de dialoogbox. 
 * save screen under the dialog?
 */

static void xd_save(XDINFO *info)
{
	MFDB source;
	_WORD pxy[8];

	source.fd_addr = NULL;

	xd_rect2pxy(&info->drect, pxy);

	pxy[4] = 0;
	pxy[5] = 0;
	pxy[6] = info->drect.g_w - 1;
	pxy[7] = info->drect.g_h - 1;

	xd_mouse_off();
	vro_cpyfm(xd_vhandle, S_ONLY, pxy, &source, &info->mfdb);
	xd_mouse_on();
}


/* 
 * Funktie voor het opnieuw tekenen van het scherm. 
 * Restore saved part of the screen?
 */

static void xd_restore(XDINFO *info)
{
	MFDB dest;
	_WORD pxy[8];

	dest.fd_addr = NULL;

	pxy[0] = 0;
	pxy[1] = 0;
	pxy[2] = info->drect.g_w - 1;
	pxy[3] = info->drect.g_h - 1;

	xd_rect2pxy(&info->drect, &pxy[4]);
	xd_mouse_off();
	vro_cpyfm(xd_vhandle, S_ONLY, pxy, &info->mfdb, &dest);
	xd_mouse_on();
}


/* 
 * Enable all items in a menu (OS_NORMAL or OS_DISABLED).
 * First menu item will always be set to OS_NORMAL in order to enable
 * access to accessorries.
 */

void xd_enable_menu(_WORD state)
{
	if (xd_menu)
	{
		_WORD i;

		for (i = 0; i < xd_nmnitems; i++)
			xd_menu[xd_mnitems[i]].ob_state = (i == 1) ? OS_NORMAL : state;

		menu_bar(xd_menu, 1);
	}
}


/* 
 * Funktie die ervoor zorgt dat een dialoogbox binnen het scherm valt.
 * Find the intersection of two areas for clipping ? 
 */

static void xd_clip(XDINFO *info, GRECT *clip)
{
	if (info->drect.g_x < clip->g_x)
		info->drect.g_x = clip->g_x;

	if (info->drect.g_y < clip->g_y)
		info->drect.g_y = clip->g_y;

	if ((info->drect.g_x + info->drect.g_w) > (clip->g_x + clip->g_w))
		info->drect.g_x = clip->g_x + clip->g_w - info->drect.g_w;

	if ((info->drect.g_y + info->drect.g_h) > (clip->g_y + clip->g_h))
		info->drect.g_y = clip->g_y + clip->g_h - info->drect.g_h;
}


/* 
 * Funktie voor het verplaatsen van een dialoog naar een nieuwe positie 
 */

static void xd_set_position(XDINFO *info, _WORD x, _WORD y)
{
	_WORD dx, dy;
	OBJECT *tree = info->tree;

	dx = x - info->drect.g_x;
	dy = y - info->drect.g_y;
	tree->ob_x += dx;
	tree->ob_y += dy;
	info->drect.g_x = x;
	info->drect.g_y = y;
}


/* 
 * Get the values of size enlargement of a bordered object.
 * This function sets (in brd_l, brd_r, etc.) separate values for 
 * enlargements on all four sides of an object.
 */

static void xd_border(OBJECT *tree)
{
	_WORD old_x, old_y, x, y, w, h;

	/* Remember old position */

	old_x = tree->ob_x;
	old_y = tree->ob_y;

	/* form_center() will return actual object size incl. border */

	form_center(tree, &x, &y, &w, &h);

	/* Compute border enlargements on all four sides of the object */

	brd_l = tree->ob_x - x;
	brd_r = w - tree->ob_width - brd_l;
	brd_u = tree->ob_y - y;
	brd_d = h - tree->ob_height - brd_u;

	/* Return to previous object position */

	tree->ob_x = old_x;
	tree->ob_y = old_y;
}


/* 
 * Calculate position of a dialog on the screen
 */

void xd_calcpos(XDINFO *info, XDINFO *prev, _WORD pmode)
{
	_WORD dummy;
	OBJECT *tree = info->tree;

	/* 
	 * Find border sizes. Attention: proper use of this data relies on
	 * an assumption that ALL dialogs opened one from another
	 * will have identical borders!!!
	 */

	xd_border(tree);

	/* Now position the dialog appropriately */

	if (pmode == XD_CENTERED && prev == NULL)
	{
		form_center(tree, &info->drect.g_x, &info->drect.g_y, &info->drect.g_w, &info->drect.g_h);
	} else
	{
		info->drect.g_w = tree->ob_width + brd_l + brd_r;
		info->drect.g_h = tree->ob_height + brd_u + brd_d;

		switch (pmode)
		{
		case XD_CENTERED:
			info->drect.g_x = prev->drect.g_x + (prev->drect.g_w - info->drect.g_w) / 2;
			info->drect.g_y = prev->drect.g_y + (prev->drect.g_h - info->drect.g_h) / 2;
			if (xd_desk.g_w > 400 && xd_desk.g_h > 300)
			{
				/* 
				 * If there is room on the screen,
				 * stack dialogs a little to the right & down, for nicer looks 
				 */
				info->drect.g_x += 16;
				info->drect.g_y += 16;
			}
			break;
		case XD_MOUSE:
			graf_mkstate(&info->drect.g_x, &info->drect.g_y, &dummy, &dummy);
			info->drect.g_x -= info->drect.g_w / 2;
			info->drect.g_y -= info->drect.g_h / 2;
			break;
		case XD_CURRPOS:
			info->drect.g_x = tree->ob_x - brd_l;
			info->drect.g_y = tree->ob_y - brd_u;
			break;
		}

		xd_clip(info, &xd_desk);

		tree->ob_x = info->drect.g_x + brd_l;
		tree->ob_y = info->drect.g_y + brd_u;
	}
}


/*
 * Determine screen size by calling vq_extnd
 */

void xd_screensize(void)
{
	_WORD work_out[57];

	vq_extnd(xd_vhandle, 0, work_out);

	xd_screen.g_x = 0;
	xd_screen.g_y = 0;
	xd_screen.g_w = work_out[0] + 1;		/* Screen width (pixels)  */
	xd_screen.g_h = work_out[1] + 1;		/* Screen height (pixels) */
}


/*
 * Toggle on/off and redraw a radiobutton within a parent object
 */

static void xd_rbutton(XDINFO *info, _WORD parent, _WORD object)
{
	_WORD i, prvstate, newstate;
	OBJECT *tree = info->tree, *treei;

	i = tree[parent].ob_head;

	if (info->dialmode == XD_WINDOW)
		xd_cursor_off(info);

	do
	{
		treei = &tree[i];

		if (treei->ob_flags & OF_RBUTTON)
		{
			prvstate = treei->ob_state;
			newstate = (i == object) ? treei->ob_state | OS_SELECTED : treei->ob_state & ~OS_SELECTED;

			if (newstate != prvstate)
				xd_change(info, i, newstate, TRUE);
		}

		i = treei->ob_next;
	} while (i != parent);

	if (info->dialmode == XD_WINDOW)
		xd_cursor_on(info);
}


/********************************************************************
 *																	*
 * Funktie ter vervanging van wind_update.							*
 *																	*
 ********************************************************************/


#if 0									/* This is never used in TeraDesk */

/*
 * This routine takes care of multiple-level wind-updates.
 * Only on the first level is actual update done.
 */

_WORD xd_wdupdate(_WORD mode)
{
	switch (mode)
	{
	case BEG_UPDATE:
		if (++xd_upd_ucnt != 1)
			return 1;
		break;
	case END_UPDATE:
		if (--xd_upd_ucnt != 0)
			return 1;
		break;
	case BEG_MCTRL:
		if (++xd_upd_mcnt != 1)
			return 1;
		break;
	case END_MCTRL:
		if (--xd_upd_mcnt != 0)
			return 1;
		break;
	}

	return wind_update(mode);
}
#endif


/*
 * Same as above, but with fewer arguments
 */

void xd_begupdate(void)
{
	if (++xd_upd_ucnt != 1)
		return;

	wind_update(BEG_UPDATE);
}

void xd_endupdate(void)
{
	if (--xd_upd_ucnt != 0)
		return;

	wind_update(END_UPDATE);
}


void xd_begmctrl(void)
{
	if (++xd_upd_mcnt != 1)
		return;

	wind_update(BEG_MCTRL);
}


void xd_endmctrl(void)
{
	if (--xd_upd_mcnt != 0)
		return;

	wind_update(END_MCTRL);
}


/* 
 * Hide the mouse pointer.
 */

void xd_mouse_off(void)
{
	if (xd_msoff_cnt == 0)
		graf_mouse(M_OFF, NULL);

	xd_msoff_cnt++;
}


/*
 * Show the mouse pointer.
 */

void xd_mouse_on(void)
{
	if (xd_msoff_cnt == 1)
		graf_mouse(M_ON, NULL);

	xd_msoff_cnt--;
}


/********************************************************************
 *																	*
 * Funkties voor het afhandelen van toetsen in dialoogboxen.		*
 *																	*
 ********************************************************************/

/* Funktie voor het zetten van de informatie over de toetsen */

_WORD xd_set_keys(OBJECT *tree, KINFO *kinfo)
{
	_WORD i = 0, cur = 0;

	for (;;)
	{
		char *h = NULL;
		OBJECT *c_obj = &tree[cur];
		_WORD etype = xd_xobtype(c_obj), state = c_obj->ob_state;

		/* Use AES 4 extended object state if it is there. */

		if (xd_is_xtndbutton(etype) || (c_obj->ob_type & 0xff) == G_BUTTON)
		{
			if (state & OS_WHITEBAK)
			{
				char *p = xd_get_obspecp(c_obj)->free_string;
				_WORD und = (state << 1) >> 9;

				if (und >= 0)
				{
					und &= 0x7f;
					if (und < (_WORD)strlen(p))
						h = p + und;
				}
			}
#if 0									/* Currently not used anywhere in teradesk */
			else if (xd_is_xtndbutton(etype))
			{
				/* I_A: changed to let '#' through if doubled! */

				/* find single '#' */
				for (h = xd_get_obspecp(c_obj)->free_string; (h = strchr(h, '#')) != 0 && (h[1] == '#'); h += 2)
					;

				if (h)
					h++;				/* pinpoint exactly */
			}
#endif
		}

		if (h)							/* one of the above options. */
		{
			_WORD ch = touppc(*h);		/* toupper() does not work above code 127 */

			if (isupper(ch) || isdigit(ch) || (ch > 127))
				kinfo[i].key = XD_ALT | ch;
			else
				kinfo[i].key = 0;

			kinfo[i].object = cur;
			i++;
		}

		if (xd_is_xtndspecialkey(etype))
		{
			kinfo[i].key = ckeytab[etype - XD_UP];
			kinfo[i].object = cur;
			i++;
		}

		if ((c_obj->ob_flags & OF_LASTOB) || (i == MAXKEYS))
			return i;

		cur++;
	}
}


/* 
 * Return TRUE if an object is selectable; otherwise return FALSE
 * if this object or any of its ancestors is hidden or disabled. 
 */

static _WORD xd_selectable(OBJECT *tree, _WORD object)
{
	_WORD parent;
	OBJECT *o = &tree[object];

	if ((o->ob_flags & OF_HIDETREE) || (o->ob_state & OS_DISABLED))
		return FALSE;

	parent = xd_obj_parent(tree, object);

	while (parent > 0)
	{
		o = tree + parent;

		if ((o->ob_flags & OF_HIDETREE) || (o->ob_state & OS_DISABLED))
			return FALSE;

		parent = xd_obj_parent(tree, parent);
	}

	return TRUE;
}


/* 
 * Find if a key is a hotkey. Also, handle the HELP key. 
 */

_WORD xd_find_key(OBJECT *tree, KINFO *kinfo, _WORD nk, _WORD key)
{
	_WORD i;
	_WORD k = (key & 0xFF);

	/* Display a page from the hypertext manual */
	if (((unsigned short) key == HELP) && (xd_nmdialogs || (xd_dialmode == XD_WINDOW)))
	{
		opn_hyphelp();
		return 0;
	}

	/* Create "uppercase" of characters above 127 for other codepages */

	if (k > 127)
		key = ((key & 0xFF00) | touppc(k));

	for (i = 0; i < nk; i++)
	{
		if ((kinfo[i].key == key) && xd_selectable(tree, kinfo[i].object))
			return kinfo[i].object;
	}

	return -1;
}


/********************************************************************
 *																	*
 * Funkties voor het afhandelen van messages.						*
 *																	*
 ********************************************************************/

/*
 * Find a dialog which is related to a window
 */

XDINFO *xd_find_dialog(WINDOW *w)
{
	XDINFO *info = xd_dialogs;			/* look among normal dialogs first */

	while ((info != NULL) && (info->window != w))
		info = info->prev;

	if (info == NULL)					/* if not found there... */
	{
		info = xd_nmdialogs;			/* ...look among nonmodal dialogs */

		while ((info != NULL) && (info->window != w))
			info = info->prev;
	}

	return info;
}


/* 
 * Funktie voor het afhandelen van een redraw event. 
 */

static void __xd_redraw(WINDOW *w, GRECT *area)
{
	XDINFO *info = xd_find_dialog(w);

	xd_begupdate();
	xd_redraw(info, ROOT, MAX_DEPTH, area, XD_RDIALOG | XD_RCURSOR);
	xd_endupdate();
}


/* 
 * Funktie voor het afhandelen van een window moved event. 
 */

static void __xd_moved(WINDOW *w, GRECT *newpos)
{
	XDINFO *info = xd_find_dialog(w);

#if 0									/* no need if there are no menus in dialog windows */
	GRECT work;

	xw_setsize(w, newpos);
	xw_getwork(w, &work);				/* work area may be modified by menu height */
	xd_set_position(info, work.g_x, work.g_y);
#endif
	xw_setsize(w, newpos);
	xd_set_position(info, w->xw_work.g_x, w->xw_work.g_y);
}


/*
 * Set focus to an object in a dialog
 * (has to be done to enable keyboard navigation in XaAES)
 * Focus is set by simulating a mouse click upon an object,
 * but the number of clicks is 0, which seems to work.
 */

static void xd_focus(OBJECT *tree, _WORD obj)
{
	_WORD d;

	if (obj > 0 && xd_xobtype(tree + obj) != XD_DRAGBOX && xd_oldbutt < 0)
		form_button(tree, obj, 0, &d);
}


/********************************************************************
 *																	*
 * Funkties ter vervanging van objc_edit.							*
 *																	*
 ********************************************************************/

/* 
 * Funktie die de positie van de cursor in de template string bepaalt
 * uit de positie van de cursor in de text string. 
 */

_WORD xd_abs_curx(OBJECT *tree, _WORD object, _WORD curx)
{
	XUSERBLK *blk = xd_get_scrled(tree, object);
	char *tmplt;
	char *s;
	char *h;

	if (blk)
		curx -= blk->uv.ob_shift;

	tmplt = xd_get_obspecp(tree + object)->tedinfo->te_ptmplt;

	if ((s = strchr(tmplt, '_')) != NULL)
	{
		while ((curx > 0) && *s)
		{
			if (*s++ == '_')
				curx--;
		}

		if ((h = strchr(s, '_')) != NULL)
			s = h;

		return (_WORD) (s - tmplt);
	}

	return 0;
}


/* 
 * Funktie die uit de positie van de cursor in de template string
 * de positie in de text string bepaald. 
 */

static _WORD xd_rel_curx(OBJECT *tree, _WORD edit_obj, _WORD curx)
{
	char *tmplt = xd_get_obspecp(tree + edit_obj)->tedinfo->te_ptmplt;
	_WORD i;
	_WORD x = 0;

	for (i = 0; i < curx; i++)
	{
		if (tmplt[i] == '_')
			x++;
	}

	return x;
}


/* 
 * Funktie voor het wissen van een teken uit een string. 
 * When a character is deleted, remaining characters are 
 * shifted left one space. 
 */

static void str_delete(char *s, _WORD pos)
{
	char *h = &s[pos];
	char ch;

	if (*h)
	{
		do
		{
			ch = h[1];
			*h++ = ch;
		} while (ch);
	}
}


/* 
 * Funktie voor het tussenvoegen van een karakter in een string. 
 */

static void str_insert(char *s, _WORD pos, _WORD ch, _WORD curlen, _WORD maxlen)
{
	_WORD i, m;

	if (curlen >= maxlen)
	{
		bell();
	} else if (pos < maxlen)
	{
		m = curlen + 1;

		for (i = m; i > pos; i--)
			s[i] = s[i - 1];

		s[pos] = (char) ch;
	}
}


/*
 * Return 1 if a key represents a printable character; otherwise return 0.
 * This should cure some problems with German and similar keyboards.
 * @=64, [=91, \`92, ]=93, {=123, }=125
 */

static _WORD xd_ischar(_WORD key)
{
	_WORD k = key & 0xFF;

	if ((key & (XD_SCANCODE | XD_CTRL)) ||
		((key & XD_ALT) && !(k == 64 || k == 91 || k == 92 || k == 93 || k == 123 || k == 125)) ||
		key == SHFT_TAB)	/* for moving backwards in dialogs */
		return 0;

	return 1;
}


/* 
 * Funktie die controleert of een toets is toegestaan op positie in string. 
 */

static _WORD xd_chk_key(char *valid, _WORD pos, _WORD key)
{
	_WORD ch = key & 0xFF;
	_WORD cch = key & 0xDF;	/* uppercase, but valid only  above '?' */
	char cvalid = valid[pos];

	if (xd_ischar(key))
	{
		switch (cvalid)
		{
		case 'x':						/* anything uppercase */
			if (isupper(cch) || (key & 0x80))	/* or national */
				return cch;
			return ch;
		case 'X':						/* anything */
			return ch;
		case 'N':						/* uppercase letters, numers and spaces */
		case 'n':						/* all letters, numbers and spaces */
		case '9':						/* numbers only */
			if (isdigit(ch))
				return ch;
			if (cvalid == '9')		/* if 'N' or 'n' treat as 'A' or 'a' */
				break;
			/* fall through */
		case 'A':						/* uppercase letters and spaces */
		case 'a':						/* all letters and spaces */
			if (ch == ' ' || (key & 0x80))
				return ch;
			if (isupper(cch))
				return (cvalid & 0x20) ? ch : cch;
			break;
		case 'F':						/* filenames characters incl. wildcards */
		case 'P':						/* pathnames characters incl. wildcards */
		case 'f':						/* filenames characters without wildcards */
		case 'p':						/* pathnames characters without wildcards */
			if (isupper(cch))		/* A-Z ONLY; always good */
				return cch;			/* return unchanged uppercase */
			if (
				   /* 
				    * note: "{}@" permitted for the sake of German TOS;
				    * ":" permitted because of network objects
				    */
				   (strchr("_:-0123456789{}@", ch) != NULL) ||	/* additional permitted characters */
				   ((cvalid == 'P' || cvalid == 'p') && strchr("\\.", ch) != NULL) ||	/* characters for paths */
				   ((cvalid == 'F' || cvalid == 'P') && strchr("*?~![]", ch) != NULL) ||	/* wildcards */
				   (key & 0x80))
				return ch;
			break;
		}
	}

	return 0;
}


static _WORD xd_chk_skip(OBJECT *tree, _WORD edit_obj, _WORD key)
{
	char *s = xd_get_obspecp(tree + edit_obj)->tedinfo->te_ptmplt;
	char *h;

	if (!xd_ischar(key))
		return 0;

	if (((h = strchr(s, key & 0xFF)) != NULL) && ((h = strchr(h, '_')) != NULL))
		return xd_rel_curx(tree, edit_obj, (_WORD) (h - s));

	return 0;
}


/*
 * Check if the object is a scrolled editable text field, and
 * return a pointer to XUSERBLK if it is
 */

void *xd_get_scrled(OBJECT *tree, _WORD edit_obj)
{
	if (xd_xobtype(&tree[edit_obj]) == XD_SCRLEDIT)
		return (XUSERBLK *) tree[edit_obj].ob_spec.userblk->ub_parm;

	return NULL;
}


/* 
 * Initialize shift position for scrolled text field so as to
 * agree with initial cursor position at the end of the string
 * (this amounts to initially displaying the -trailing- end of the text).
 * Note: use carefully, there is no checking of object type
 */

void xd_init_shift(OBJECT *obj, const char *text)
{
	_WORD tl = (_WORD) strlen(text);	/* real string length */
	_WORD vl = (_WORD) strlen(xd_pvalid(obj));	/* form length */

	/* offset of first visible char */

	((XUSERBLK *) (obj->ob_spec.userblk->ub_parm))->uv.ob_shift = max(tl - vl, 0);
}


/* 
 * HR 021202: scrolling in scrolling editable texts. 
 * Note: "pos" is position in the edited text, not in the visible field.
 * flen is the length of the editable field.
 * The object need not be of the scrolled type.
 */

static bool xd_shift(XUSERBLK *blk, _WORD pos, _WORD flen, _WORD clen)
{
	if (blk)
	{
		_WORD shift = blk->uv.ob_shift;

		if (pos == clen)				/* trailing end of text is visible */
			shift = clen - flen;
		if (shift < 0)					/* can't go off the first character */
			shift = 0;
		if (pos < shift)				/* scroll left */
			shift = pos;
		if (pos > shift + flen)			/* scroll right */
			shift = pos - flen;
		if (shift != blk->uv.ob_shift)	/* there was a change in shift amount */
			return blk->uv.ob_shift = shift, true;
	}

	return false;
}


/*
 * Edit an editable text. This need not be of the scrolled type. 
 */

_WORD xd_edit_char(XDINFO *info, _WORD key)
{
	TEDINFO *tedinfo;
	GRECT clip;
	XUSERBLK *blk;
	char *val;
	char *str;
	OBJECT *tree;
	_WORD edit_obj;
	_WORD newpos;				/* new position in edited string (in tedinfo->te_ptext) */
	_WORD oldpos;							/* previous position in same */
	_WORD curlen;							/* length of edited string   */
	_WORD maxlen;							/* maximum possible length of editable string */
	_WORD flen;							/* length of editable field (of tedinfo->te_pvalid) */
	_WORD pos;
	_WORD ch;
	_WORD result = TRUE;
	_WORD m_on = FALSE;

	tree = info->tree;

	if ((edit_obj = info->edit_object) <= 0)
		return FALSE;

	tedinfo = xd_get_obspecp(tree + edit_obj)->tedinfo;
	str = tedinfo->te_ptext;
	val = tedinfo->te_pvalid;
	blk = xd_get_scrled(tree, edit_obj);
	oldpos = newpos = info->cursor_x;
	curlen = (_WORD) strlen(str);
	flen = (_WORD) strlen(val);

	objc_offset(tree, edit_obj, &clip.g_x, &clip.g_y);
	clip.g_h = xd_regular_font.ch;
	clip.g_w = xd_regular_font.cw * tedinfo->te_tmplen;

	if (blk)
	{
		/* only scrolled-text fields are handled here */

		clip.g_x -= xd_regular_font.cw + 2;	/* HR: This temporary until ub_scrledit() handles templates properly. */
		clip.g_w += 2 * xd_regular_font.cw + 4;
		clip.g_h += 4;
		maxlen = (_WORD) XD_MAX_SCRLED;
	} else
	{
		maxlen = flen;
	}
	
	if (xd_ischar(key))
		key &= 0xFF;

	switch ((unsigned short) key)
	{
	case SHFT_CURLEFT:
		newpos = 0;
		goto setcursor;
	case SHFT_CURRIGHT:
		newpos = curlen;
		goto setcursor;
	case CURLEFT:
		if (oldpos > 0)
			newpos = oldpos - 1;
		goto setcursor;
	case CURRIGHT:
		if (oldpos < curlen)
			newpos = oldpos + 1;

	  setcursor:;

		if (oldpos != newpos)
		{
			xd_mouse_off();
			xd_cursor_off(info);

			info->cursor_x = newpos;
			m_on = TRUE;
			goto shift_redraw;
		}
		break;
	case DELETE:
		if (oldpos >= curlen)
			break;
		goto doshift;
	case BACKSPC:
		if (oldpos <= 0)
			break;
		/* else fall through */
	case ESCAPE:
	  doshift:;

		xd_mouse_off();
		xd_cursor_off(info);

		if (key == ESCAPE)
		{
			*str = 0;
			info->cursor_x = 0;
			curlen = 0;
		} else
		{
			if (key == BACKSPC)
				info->cursor_x--;
			str_delete(str, info->cursor_x);
			curlen--;
		}
		m_on = TRUE;
		goto shift_redraw;
	case INSERT:						/* call the fileselector */
		if (blk)
		{
			xd_cursor_off(info);
			get_fsel(info, str, tree[edit_obj].ob_flags);
			curlen = (_WORD) strlen(str);
			goto shift_redraw;
		}
		break;
	case HOME:
	case HELP:
		/* These codes have no effect */
		break;
	default:
		pos = oldpos;
		if (oldpos == maxlen)
			pos--;

		/* For a scrolled editable text use only the first validation character */

		if ((ch = xd_chk_key(val, (blk) ? 0 : pos, key)) == 0 && !blk)
			pos = xd_chk_skip(tree, edit_obj, key);

		if (ch != 0 || pos > 0)
		{
			xd_mouse_off();
			xd_cursor_off(info);

			if (ch != 0)
			{
				info->cursor_x = pos + 1;
				str_insert(str, pos, ch, curlen, maxlen);
			} else
			{
				_WORD i;

				info->cursor_x = pos;

				str += oldpos;

				for (i = oldpos; i < pos; i++)
					*str++ = ' ';

				*str = 0;
			}

			m_on = TRUE;
			curlen++;

		  shift_redraw:;

			xd_shift(blk, info->cursor_x, flen, curlen);	/* scrolling editable texts. */
			xd_redraw(info, edit_obj, 0, &clip, XD_RDIALOG);
			xd_cursor_on(info);

			if (m_on)
				xd_mouse_on();
		} else
		{
			result = FALSE;
		}
		break;
	}

	return result;
}


void xd_edit_end(XDINFO *info)
{
	if (info->edit_object > 0)
	{
		xd_cursor_off(info);
		info->edit_object = -1;
		info->cursor_x = 0;
	}
}


void xd_edit_init(XDINFO *info, _WORD object, _WORD curx)
{
	OBJECT *tree = info->tree;

	if ((object > 0) && xd_selectable(tree, object))
	{
		XUSERBLK *blk = xd_get_scrled(tree, object);
		TEDINFO *ted = xd_get_obspecp(tree + object)->tedinfo;
		_WORD x, dummy, maxlen;

		xd_focus(tree, object);

		maxlen = (_WORD) strlen(ted->te_ptext);

		if (curx >= 0)
		{
			objc_offset(tree, object, &x, &dummy);
			x = (curx - x + xd_regular_font.cw / 2) / xd_regular_font.cw;

			if (blk)
				x += blk->uv.ob_shift;
			else
				x = xd_rel_curx(tree, object, x);

			if (x > maxlen)
				x = maxlen;
		}

		if (info->edit_object != object)
		{
			xd_edit_end(info);
			info->edit_object = object;
			info->cursor_x = (curx == -1) ? maxlen : x;
			xd_cursor_on(info);
		} else if ((curx >= 0) && (x != info->cursor_x))
		{
			xd_cursor_off(info);
			info->cursor_x = x;
			xd_cursor_on(info);
		}

		xd_shift(blk, info->cursor_x, (_WORD) strlen(ted->te_pvalid), maxlen);
	}
}


/********************************************************************
 *																	*
 * Funkties ter vervanging van form_keybd.							*
 *																	*
 ********************************************************************/

/* 
 * Funktie voor het vinden van het volgende editable+selectable object.
 * Note: if no editable objects are found, routine returns "start" ! 
 */

_WORD xd_find_obj(OBJECT *tree, _WORD start, _WORD which)
{
	_WORD theflag;
	_WORD obj = 0;
	_WORD flag = OF_EDITABLE;
	_WORD inc = ((tree[start].ob_flags & OF_LASTOB) != 0) ? 0 : 1;

	switch (which)
	{
	case FMD_BACKWARD:
		inc = -1;
		obj = start + inc;
		break;
	case FMD_FORWARD:
		obj = start + inc;
		break;
	case FMD_DEFLT:
		flag = OF_DEFAULT;
		break;
	}

	while (obj >= 0)
	{
		theflag = tree[obj].ob_flags;

		if ((theflag & flag) && xd_selectable(tree, obj))
			return obj;

		if (theflag & OF_LASTOB)
			obj = -1;
		else
			obj += inc;
	}

	return start;
}


/*
 * Find a Cancel/Abort/Undo button in a dialog 
 * Originally from XaAES, but completely reworked for multilingual use.
 * Now all 'Cancel' words have to be defined in a string which can
 * e.g. be read from the resource file
 * Argument 'ob' is the pointer to the dialog object tree.
 */

static _WORD xd_find_cancel(OBJECT *ob)
{
	_WORD f = 0;
	char *s = NULL;
	char t[16] = { '|' };					/* for "cancel" words up to 13 characters long */

	do
	{
		t[1] = 0;

		/* Consider only normal buttons and buttons with underlined text */

		if (((ob[f].ob_type) == G_BUTTON || (xd_xobtype(&ob[f]) == XD_BUTTON))
			&& (ob[f].ob_flags & (OF_SELECTABLE | OF_TOUCHEXIT | OF_EXIT)) != 0)
		{
			/* 
			 * Attention: no checking of case! 
			 * Define 'Cancel' words in *xd_cancelstring in exact case.
			 * 'Cancel' words must not be longer than 13 characters
			 * ( '|' + word + '|' + '\0'  should fit in t[] )
			 */

			s = xd_get_obspecp(ob + f)->free_string;

			/* Copy and strip no more than 13 characters + terminator */

			if (s != NULL && *s >= ' ')
			{
				strsncpy(&t[1], s, 14);
				strip_name(t, t);
			}

			strcat(t, "|");

			if (strstr(xd_cancelstring, t) != NULL)
				return f;
		}

	} while (!(ob[f++].ob_flags & OF_LASTOB));

	return -1;
}


_WORD xd_form_keybd(XDINFO *info, _WORD kobnext, _WORD kchar, _WORD *knxtobject, _WORD *knxtchar)
{
	OBJECT *tree = info->tree;
	_WORD i = xd_find_cancel(tree), mode = FMD_FORWARD;

	*knxtobject = kobnext;
	*knxtchar = 0;

	if (xd_ischar(kchar))
		kchar &= 0xFF;

	switch ((unsigned short) kchar)
	{
	case CURUP:
	case SHFT_TAB:
		mode = FMD_BACKWARD;
		/* fall through */
	case CURDOWN:
	case TAB:
		if ((i = xd_find_obj(tree, info->edit_object, mode)) > 0)
			*knxtobject = i;
		break;
	case RETURN:
		i = xd_find_obj(tree, 0, FMD_DEFLT);
		/* fall through */
	case UNDO:
		if (i > 0)
		{
			xd_change(info, i, OS_SELECTED, TRUE);
			*knxtobject = i;
			return FALSE;
		}
		break;
	default:
		*knxtchar = kchar;
		break;
	}

	return TRUE;
}


/********************************************************************
 *																	*
 * Funkties ter vervanging van form_button.							*
 *																	*
 ********************************************************************/


/*
 * Handle waits and redraws related to previously pressed arrow buttons
 */

_WORD xd_oldarrow(XDINFO *info)
{
	if (xd_oldbutt > 0)
	{
		_WORD event;
		XDEVENT tev;

		xd_clrevents(&tev);
		tev.ev_mflags = MU_BUTTON | MU_TIMER;
		tev.ev_mbmask = 1;
		tev.ev_mtlocount = 200;			/* 200ms delay while arrow pressed */

		do
		{
			event = xe_xmulti(&tev);
		} while (!(event & (MU_TIMER | MU_BUTTON)));

		if (event & MU_BUTTON)
		{
			xd_drawbuttnorm(info, xd_oldbutt);
			xd_oldbutt = -1;
		} else
		{
			return 1;					/* still pressed */
		}
	}

	return 0;							/* not pressed anymore */
}


_WORD xd_form_button(XDINFO *info, _WORD object, _WORD clicks, _WORD *result)
{
	OBJECT *tree = info->tree;
	_WORD parent, oldstate, dummy;
	_WORD flags = tree[object].ob_flags;
	_WORD etype = xd_xobtype(&tree[object]);

	/* Handle arrow (boxchar) buttons */

	if (etype >= XD_CUP && etype <= XD_CRIGHT)
	{
		xd_oldbutt = object;
		xd_change(info, object, OS_SELECTED, TRUE);
	} else
		xd_oldbutt = -1;

	/* Now handle other stuff... */

	if (xd_selectable(tree, object) && ((flags & OF_SELECTABLE) || (flags & OF_TOUCHEXIT)))
	{
		oldstate = tree[object].ob_state;

		xd_focus(tree, object);
		xd_begmctrl();

		if (flags & OF_RBUTTON)
		{
			if (((parent = xd_obj_parent(tree, object)) >= 0) && !(oldstate & OS_SELECTED))
				xd_rbutton(info, parent, object);
		} else if (flags & OF_SELECTABLE)
		{
			XDEVENT events;

			_WORD evflags,
			 newstate,
			 state;

#if 0									/* there are currently no tristate objects in Teradesk */

			/* I_A changed to fit tristate-buttons! */

			if (!xd_is_tristate(tree + object))
				newstate = (oldstate & OS_SELECTED) ? oldstate & ~OS_SELECTED : oldstate | OS_SELECTED;
			else
			{
				/* switch tri-state button! */
				newstate = xd_get_tristate(oldstate);
				switch (newstate)
				{
				case TRISTATE_0:
					newstate = xd_set_tristate(oldstate, TRISTATE_1);
					break;
				case TRISTATE_1:
					newstate = xd_set_tristate(oldstate, TRISTATE_2);
					break;
				case TRISTATE_2:
					newstate = xd_set_tristate(oldstate, TRISTATE_0);
					break;
				}
			}
#endif

			newstate = oldstate ^ OS_SELECTED;

			xd_clrevents(&events);
			events.ev_mflags = MU_BUTTON | MU_TIMER;
			events.ev_mbclicks = 1;
			events.ev_mbmask = 1;
			events.ev_mm1flags = 1;

			xd_objrect(tree, object, (GRECT *) & events.ev_mm1);
			xd_change(info, object, newstate, TRUE);

			do
			{
				evflags = xe_xmulti(&events);

				if (evflags & MU_M1)
				{
					if (events.ev_mm1flags == 1)
					{
						events.ev_mm1flags = 0;
						state = oldstate;
					} else
					{
						events.ev_mm1flags = 1;
						state = newstate;
					}

					xd_change(info, object, state, TRUE);
				}

				events.ev_mflags = MU_BUTTON | MU_M1;
				events.ev_mtlocount = 0;
			} while (!(evflags & MU_BUTTON));
		}

		xd_endmctrl();

		if (flags & OF_TOUCHEXIT)
		{
			*result = object | ((clicks > 1) ? 0x8000 : 0);
			return FALSE;
		}

		if ((flags & OF_EXIT) && (tree[object].ob_state != oldstate))
		{
			*result = object;
			return FALSE;
		}

		evnt_button(1, 1, 0, &dummy, &dummy, &dummy, &dummy);
	}

	*result = (flags & OF_EDITABLE) ? object : 0;

	return TRUE;
}


/********************************************************************
 *																	*
 * Funkties voor form_do.											*
 *																	*
 ********************************************************************/

/* 
 * Funktie voor het zoeken van de button waarmee de dialoogbox
 * verplaatst kan worden. 
 * Find the "dragbox" object in a dialog.
 */

_WORD xd_movebutton(OBJECT *tree)
{
	OBJECT *c_obj;
	_WORD cur = 0;

	for (;;)
	{
		c_obj = &tree[cur];

		if (xd_xobtype(c_obj) == XD_DRAGBOX)
			return cur;

		if (c_obj->ob_flags & OF_LASTOB)
			return -1;

		cur++;
	}
}



/* 
 * Eigen form_do.
 * Note: this will return -1 if closed without any button. 
 */


_WORD xd_kform_do(XDINFO *info, _WORD start, userkeys userfunc, void *userdata)
{
	OBJECT *tree = info->tree;
	XDEVENT events;
	_WORD which, kr, db, nkeys, cont, next_obj, s, cmode;
	bool inw = TRUE;
	KINFO kinfo[MAXKEYS];

	cont = TRUE;
	next_obj = 0;
	s = start;
	cmode = -1;

	xd_begupdate();

	xd_fdo_flag = TRUE;
	db = xd_movebutton(tree);			/* find the dragbox */
	nkeys = xd_set_keys(tree, kinfo);

	if (info->dialmode != XD_WINDOW)
	{
		inw = FALSE;
		xd_begmctrl();
	}

	/* Set the parameters for events */

	xd_clrevents(&events);
	events.ev_mflags = MU_KEYBD | MU_BUTTON | MU_MESAG;
	events.ev_mbclicks = 2;
	events.ev_mbmask = 1;
	events.ev_mbstate = 1;
	next_obj = 0;

	/* Find the first editable object; if none found, return 'start' */

	s = xd_find_obj(tree, start, FMD_FORWARD);

	if (start == 0)
		start = s;

	xd_edit_init(info, start, cmode);

	/*
	 * The loop comes in effect only with the dragbox object ?
	 */

	while (cont)
	{
		/* 
		 * If an arrow button is still pressed, need to make 
		 * another loop after a timer expires. Otherwise, there is
		 * a chance that the arrow will not be redrawn proprely.
		 */

		if (xd_oldarrow(info))
		{
			events.ev_mflags |= MU_TIMER;
			events.ev_mtlocount = 20;
		} else
		{
			events.ev_mflags &= ~MU_TIMER;
			events.ev_mtlocount = 0;
		}

		xd_endupdate();

		which = xe_xmulti(&events);

		if ((which & MU_MESAG) != 0)
		{
			if (xd_usermessage != NULL && xd_usermessage(events.ev_mmgpbuf) != 0)
			{
				next_obj = -1;
				cont = FALSE;
			} else if ((events.ev_mmgpbuf[0] == WM_CLOSED) && inw && (events.ev_mmgpbuf[3] == info->window->xw_handle))
			{
				/* if "Closer" then act as if a cancel button was pressed */

				if ((next_obj = xd_find_cancel(tree)) > 0)
					cont = FALSE;
				else
					next_obj = 0;
			}
		}

		xd_begupdate();

		if ((which & MU_KEYBD) && cont)
		{
			_WORD object;

			if ((userfunc == (userkeys) 0) || (userfunc(info, userdata, events.xd_keycode) == 0))
			{
				if ((object = xd_find_key(tree, kinfo, nkeys, events.xd_keycode)) >= 0)
				{
					next_obj = object;
					cont = xd_form_button(info, next_obj, 1, &next_obj);
				} else
				{
					if ((cont = xd_form_keybd(info, next_obj, events.xd_keycode, &next_obj, &kr)) != FALSE)
						cmode = -1;

					if (kr)
						xd_edit_char(info, kr);
				}
			}
		}

		if ((which & MU_BUTTON) && cont)
		{
			if ((next_obj = objc_find(tree, ROOT, MAX_DEPTH, events.ev_mmox, events.ev_mmoy)) == -1)
			{
				bell();
				next_obj = 0;
				xd_oldbutt = -1;
			} else
			{
				if ((cont = xd_form_button(info, next_obj, events.ev_mbreturn, &next_obj)) != FALSE)
					cmode = events.ev_mmox;

				if ((next_obj & 0x7FFF) == db)	/* move by dragbox */
				{
					_WORD nx, ny;

					graf_dragbox(info->drect.g_w, info->drect.g_h, info->drect.g_x, info->drect.g_y,
								 xd_desk.g_x, xd_desk.g_y, xd_desk.g_w, xd_desk.g_h, &nx, &ny);

					xd_restore(info);
					xd_set_position(info, nx, ny);
					xd_save(info);
					xd_redraw(info, ROOT, MAX_DEPTH, &info->drect, XD_RDIALOG | XD_RCURSOR);

					cont = TRUE;
					next_obj = 0;
				}
			}
		}

		if (cont && (next_obj != 0))
		{
			xd_edit_init(info, next_obj, cmode);

			if ((events.ev_mbreturn == 2) && xd_get_scrled(tree, next_obj) != NULL)
				xd_edit_char(info, INSERT);

			next_obj = 0;
		}
	}

	if (!inw)
		xd_endmctrl();

	xd_edit_end(info);
	xd_fdo_flag = FALSE;
	xd_endupdate();
	return (next_obj);
}


_WORD xd_form_do(XDINFO *info, _WORD start)
{
	return xd_kform_do(info, start, (userkeys) 0, NULL) & 0x7FFF;
}


/*
 * Same as above but immediately change button state to normal and redraw
 */

_WORD xd_form_do_draw(XDINFO *info)
{
	_WORD button;

	button = xd_form_do(info, ROOT);
	xd_change(info, button, OS_NORMAL, 1);
	return button;
}

/********************************************************************
 *																	*
 * Funkties voor het initialiseren en deinitialiseren van een		*
 * dialoog.															*
 *																	*
 ********************************************************************/


/*
 * Open a dialog with or without zoom effects.
 * Note: it is asumed that  the dragbox "ear" always exists in a dialog!!!
 * 
 * Parameters:
 *
 * tree		- Object tree of the dialogbox
 * info		- Pointer to a XDINFO structure
 * funcs	- Pointer to a XD_NMFUNCS structure (nonmodal only).
 *       	  if this is not NULL it is assumed that the dialog is nonmodal.
 * start	- First edit object (as in form_do) (nonmodal only)
 * x		- x position where dialogbox should appear. If -1 the
 *			  library will calculate the position itself. (nonmodal only)
 * y		- y position where dialogbox should appear. If -1 the (nonmodal only)
 *			  library will calculate the position itself.
 * menu		- Optional pointer to a object tree, which should be used
 *			  as menu bar in the window. If NULL no menu bar will
 *			  appear in top of the window. (nonmodal only)
 * xywh		- Optional pointer to a GRECT structure. If this pointer
 *			  is not NULL and zoom is not 0, the library will draw
 *			  a zoombox from the rectangle in xywh to the window.
 * zoom		- see xywh
 * title	- Optional title. If not NULL this string is used as the
 *			  title of the window. Otherwise the program name is used (nonmodal only)
 */

static _WORD xd_open_wzoom(OBJECT *tree, XDINFO *info, XD_NMFUNC *funcs, _WORD start,
   OBJECT *menu,
#if _DOZOOM
   GRECT *xywh, _WORD zoom
#endif
    const char *title)
{
	XDINFO *prev;
	_WORD db, dialmode;
	int error;

	(void) menu;

	prev = xd_dialogs;
	dialmode = xd_dialmode;
	error = 0;

	db = xd_movebutton(tree);

	if (funcs)
	{
		prev = NULL;
		dialmode = XD_WINDOW;
	}

	xd_begupdate();

	info->tree = tree;
	info->prev = prev;

	if (!funcs)
		xd_dialogs = info;

	info->edit_object = -1;
	info->cursor_x = 0;
	info->curs_cnt = 1;
	info->func = funcs;

	xd_oldbutt = -1;

	xd_calcpos(info, prev, xd_posmode);

	if (dialmode == XD_WINDOW)
	{
		if ((prev == NULL) || (prev->dialmode == XD_WINDOW))
		{
			WINDOW *w;
			GRECT wsize;
			_WORD d;
			_WORD thetype;
			WD_FUNC *thefuncs;
			size_t thesize;
			OBJECT *themenu;

			tree[db].ob_flags |= OF_HIDETREE;	/* hide dragbox */

			if (funcs)
			{
				/* This is for nonmodal windowed dialogs */
				thetype = XW_NMDIALOG;
#if 0									/* currently there are no menus in dialogs */
				themenu = menu;
#endif
				thefuncs = &xd_nmwdfuncs;
				thesize = sizeof(XD_NMWINDOW);
			} else
			{
				/* This is for other windowed dialogs */
				thetype = XW_DIALOG;
#if 0									/* currently there are no menus in dialogs */
				themenu = NULL;
#endif
				thefuncs = &xd_wdfuncs;
				thesize = sizeof(WINDOW);
			}

			themenu = NULL;				/* while there are no menus in dialogs */

			xw_calc(WC_BORDER, XD_WDFLAGS, &info->drect, &wsize, themenu);

			/* Nicer looking window border */

			tree->ob_x -= 1;
			wsize.g_w -= 2;
			wsize.g_h -= 1;

#if 0									/* Currently this is not used anywhere in TeraDesk (See font.c only) */

			if (funcs && (x != -1) && (y != -1))
			{
				_WORD dx, dy;

				dx = x - wsize.g_x;

				info->drect.g_x += dx;
				tree->ob_x += dx;
				wsize.g_x = x;

				dy = y - wsize.g_y;

				info->drect.g_y += dy;
				tree->ob_y += dy;
				wsize.g_y = y;
			}
#endif
			/* Fit to screen */

			if (wsize.g_x < xd_desk.g_x)
			{
				d = xd_desk.g_x - wsize.g_x;
				info->drect.g_x += d;
				tree->ob_x += d;
				wsize.g_x = xd_desk.g_x;
			}

			if (wsize.g_y < xd_desk.g_y)
			{
				d = xd_desk.g_y - wsize.g_y;
				info->drect.g_y += d;
				tree->ob_y += d;
				wsize.g_y = xd_desk.g_y;
			}
#if _DOZOOM
			if (zoom && xywh)
			{
				graf_growbox(xywh->x, xywh->y, xywh->w, xywh->h, wsize.x, wsize.y, wsize.w, wsize.h);
				zoom = FALSE;
			}
#endif
			if ((w = xw_create(thetype, thefuncs, XD_WDFLAGS, &wsize, thesize, themenu, &error)) != NULL)
			{
				wind_set_str(w->xw_handle, WF_NAME, title ? title : xd_prgname);

				info->window = w;

				if (funcs)
				{
					info->prev = xd_nmdialogs;
					xd_nmdialogs = info;

					((XD_NMWINDOW *) w)->xd_info = info;
					((XD_NMWINDOW *) w)->nkeys = xd_set_keys(tree, ((XD_NMWINDOW *) w)->kinfo);

					if (start == 0)
						start = xd_find_obj(tree, 0, FMD_FORWARD);

					xd_edit_init(info, start, -1);

				} else if (prev == NULL)
					xd_enable_menu(OS_DISABLED);

				xw_open(w, &wsize);

#if _DOZOOM
				if (zoom && xywh)
				{
					graf_growbox(xywh->x, xywh->y, xywh->w, xywh->h, wsize.x, wsize.y, wsize.w, wsize.h);
					zoom = FALSE;
				}
#endif

				xd_endupdate();
			} else if (!funcs)
			{
				dialmode = XD_BUFFERED;	/* has no effect in nonmodal */
			}
		} else
		{
			dialmode = XD_BUFFERED;
		}
	}

	/* XD_WINDOW ? */
#if _DOZOOM
	if (zoom && xywh)
	{
		graf_growbox(xywh->x, xywh->y, xywh->w, xywh->h, info->drect.x, info->drect.y, info->drect.w, info->drect.h);
		zoom = FALSE;
	}
#endif

	info->dialmode = dialmode;

	if (dialmode == XD_BUFFERED)
	{
		long scr_size;

		scr_size = xd_initmfdb(&info->drect, &info->mfdb);

		/*
		 * Note: if xd_malloc were used here, great loss of memory
		 * would occur in some circumstances. E.g. open a large file
		 * in a text window, then manipulate some lists in dialogs,
		 * close dialogs, close text window 
		 */

		info->mfdb.fd_addr = (void *) Malloc(scr_size);

		if (info->mfdb.fd_addr == NULL)
		{
			xd_endupdate();
			return ENOMEM;
		}

		tree[db].ob_flags &= ~OF_HIDETREE;

		xd_save(info);
		xd_drawdeep(info, ROOT);
	}

	return error;
}


/* 
 * initialisatie voor dialoog. Berekent de positie van de box en redt
 * het scherm.
 *
 * tree		- objectboom,
 * info		- bevat informatie over buffer voor scherm en de positie
 *   			  van de dialoogbox.
 *
 * Should return negative value in case of failure.
 */


_WORD xd_open(OBJECT *tree, XDINFO *info)
{
#if _DOZOOM
	GRECT xywh;

	return xd_open_wzoom(tree, info, NULL, 0, /* 0, 0, */ NULL, &xywh, TRUE, NULL);
#else
	return xd_open_wzoom(tree, info, NULL, 0, /* 0, 0, */ NULL, NULL);
#endif
}


_WORD xd_nmopen(OBJECT *tree, XDINFO *info, XD_NMFUNC *funcs, _WORD start,
	OBJECT *menu,
#if _DOZOOM
	GRECT *xywh, _WORD zoom,
#endif
	const char *title)
{
	(void) menu;
	(void) start;
#if _DOZOOM
	return xd_open_wzoom(tree, info, funcs, 0, /* -1, -1, not used */ NULL, NULL, FALSE, wtitle);
#else
	return xd_open_wzoom(tree, info, funcs, 0, /* -1, -1, not used */ NULL, title);
#endif
}


static void xd_close_wzoom(XDINFO *info,
#if _DOZOOM
   GRECT *xywh, _WORD zoom,
#endif
   _WORD nmd)
{
	XDINFO *h, *prev;

	h = xd_nmdialogs;
	xd_oldbutt = -1;

	if (nmd)
	{
		prev = NULL;

		while (h && (h != info))
		{
			prev = h;
			h = h->prev;				/* eigenlijk h->next. */
		}

		if (h != info)
			return;
	} else
	{
		if (xd_dialogs != info)
			return;

		prev = info->prev;
	}


	if (info->dialmode == XD_WINDOW)
	{
		WINDOW *w = info->window;

		xd_begupdate();

		if (!(nmd || prev))
			xd_enable_menu(OS_NORMAL);

		xw_close(w);
		xw_delete(w);

		if (nmd)
		{
			if (prev)
				prev->prev = info->prev;
			else
				xd_nmdialogs = info->prev;
		} else if (prev)
		{
			xw_set_topbot(prev->window, WF_TOP);
		}
	}

	if (info->dialmode == XD_BUFFERED)
	{
		xd_restore(info);

		/* Can this be xd_free ? */

		Mfree(info->mfdb.fd_addr);
	}
#if _DOZOOM
	if (zoom && xywh)
	{
		graf_shrinkbox(xywh->x, xywh->y, xywh->w, xywh->h, info->drect.x, info->drect.y, info->drect.w, info->drect.h);
	}
#endif

	xd_endupdate();

	if (!nmd)
		xd_dialogs = prev;

#if 0									/*!!! why ?   btw. where is it defined */
	if ((prev != NULL) && (prev->dialmode == XD_WINDOW))
		xd_scan_messages(XD_EVREDRAW, NULL);
#endif
}


/* 
 * funktie die opgeroepen moet worden na het einde van de dialoog.
 * De funktie hersteld het scherm en geeft het geheugen van de
 * schermbuffer weer vrij. 
 */

void xd_close(XDINFO *info)
{
#if _DOZOOM
	GRECT xywh;

	xd_close_wzoom(info, &xywh, TRUE, FALSE);
#else
	xd_close_wzoom(info, FALSE);
#endif
}


/*
 * Close a nonmodal dialog
 */

void xd_nmclose(XDINFO *info)
{
#if _DOZOOM
	GRECT xywh;

	xd_close_wzoom(info, &xywh, TRUE, TRUE);
#else
	xd_close_wzoom(info, TRUE);
#endif
}


/********************************************************************
 *																	*
 * Hoog niveau funkties voor het uitvoeren van dialogen. Deze		*
 * funkties voeren alles wat nodig is voor een dialoog uit,			*
 * inclusief de form_do.											*
 *																	*
 ********************************************************************/

_WORD xd_kdialog(OBJECT *tree, _WORD start, userkeys userfunc, void *userdata)
{
	XDINFO info;
	_WORD exitcode;

	if ((exitcode = xd_open(tree, &info)) >= 0)
	{
		exitcode = xd_kform_do(&info, start, userfunc, userdata);
		xd_buttnorm(&info, exitcode);
		xd_close(&info);
	}

	return exitcode;
}


_WORD xd_dialog(OBJECT *tree, _WORD start)
{
	return xd_kdialog(tree, start, (userkeys) 0, NULL);
}


/********************************************************************
 *																	*
 * Funkties voor het zetten van de dialoogmode en de mode waarmee	*
 * de plaats van de dialoog op het scherm bepaald wordt.			*
 *																	*
 ********************************************************************/

_WORD xd_setdialmode(_WORD new, _WORD(*hndl_message) (_WORD *message), OBJECT *menu, _WORD nmnitems,
					 const _WORD *mnitems)
{
	_WORD old = xd_dialmode;

	/* Note: it is assumed that a correct dial mode is always given */

	xd_dialmode = new;
	xd_usermessage = hndl_message;
	xd_menu = menu;
	xd_nmnitems = nmnitems;
	xd_mnitems = mnitems;
	return old;
}


/*
 * Set dialog position mode. 
 * It is assumed that a valid mode is always given; there are no checks!
 */

_WORD xd_setposmode(_WORD new)
{
	_WORD old = xd_posmode;

	xd_posmode = new;
	return old;
}

/*
 * A small routine to set default colours in AES 4, 
 * in video modes with more than 4 colours.
 * Otherwise, these have been set earlier to WHITE and BLACK, respectively.
 */

static void xd_aes4col(void)
{
	if (xd_colaes)
	{
		xd_bg_col = G_LWHITE;			/* gray background for some objects */
		xd_sel_col = G_LBLACK;			/* dark gray for selected objects */
	}
}


static _WORD has_appl_getinfo(void)
{
    static _WORD has_agi = -1; /* do the check only once */
    
    /* check for appl_getinfo() being present */
    if (has_agi < 0)
    {
        has_agi = 0;
        /* AES 4.0? */
        if (gl_ap_version >= 0x400)
             has_agi = 1;
        else if (appl_find( "?AGI\0\0\0\0") >= 0)
            has_agi = 3;
    }
    return has_agi;
}

static _WORD appl_x_getinfo(_WORD type, _WORD *out1, _WORD *out2, _WORD *out3, _WORD *out4)
{
    _WORD ret;

    /* no appl_getinfo? return error code */
	if (!has_appl_getinfo() || (ret = appl_getinfo(type, out1, out2, out3, out4)) == 0)
	{
	    if (out1 != NULL)
	    	*out1 = 0;
	    if (out2 != NULL)
		    *out2 = 0;
		if (out3 != NULL)
		    *out3 = 0;
		if (out4 != NULL)
		    *out4 = 0;
		return 0;
	}
	return ret;
}

/********************************************************************
 *																	*
 * Funkties voor de initialisatie en deinitialisatie van de module.	*
 *																	*
 ********************************************************************/

_WORD init_xdialog(_WORD *vdi_handle, void *(*malloc_func) (unsigned long size),
				   void (*free_func) (void *block), const char *prgname, _WORD load_fonts, _WORD *nfonts)
{
	_WORD dummy, i;
	_WORD work_in[11];
	_WORD work_out[57];
	_WORD ag1, ag2, ag3, ag4;

	xd_malloc = malloc_func;
	xd_free = free_func;
	xd_prgname = prgname;

	/* 
	 * Note: Magic (V6.20 at least) returns AES version 3.99.
	 * In that case, after an inquiry ?AGI has been made,
	 * On the other hand, TOS 4.0 returns AES version 3.40
	 * but still is a "3D" AES
	 */

	xd_min_timer = 5;					/* Minimum time passed to xe_multi(); was 10 earlier */

	/* Don't use xw_get() here... */

	wind_get_grect(0, WF_WORKXYWH, &xd_desk);
	xd_vhandle = graf_handle(&xd_fnt_w, &xd_fnt_h, &dummy, &dummy);

	/* Open virtual workstation on screen */

	for (i = 0; i < 10; i++)
		work_in[i] = 1;
	work_in[10] = 2;

	v_opnvwk(work_in, &xd_vhandle, work_out);

	if (xd_vhandle == 0)
		/* Could not open */
		return XDVDI;

	/* It was successful */
	xd_pix_height = work_out[4];
	xd_ncolours = work_out[13];

	if (xd_ncolours >= 16)
		xd_colaes = 1;

	vq_extnd(xd_vhandle, 1, work_out);
	xd_nplanes = work_out[4];

	vsf_perimeter(xd_vhandle, 0);	/* no borders in v_bar() */

	if (load_fonts && vq_gdos())
		*nfonts = vst_load_fonts(xd_vhandle, 0);
	else
		*nfonts = 0;

	/* If appl_getinfo is supported, then get diverse information: */

	if (has_appl_getinfo())
	{
		/* Assume some colour settings */

		xd_aes4col();

		/* Information on "normal" AES font */

		appl_x_getinfo(AES_LARGEFONT, &xd_regular_font.size, &xd_regular_font.id, &dummy, &dummy);
#if _SMALL_FONT

		/* Information on "small" AES font */

		appl_x_getinfo(AES_SMALLFONT, &xd_small_font.size, &xd_small_font.id, &dummy, &dummy);

#endif
		/* Information on colour icons and supported rsc format */

		appl_x_getinfo(AES_SYSTEM, &ag1, &ag2, &colour_icons, &dummy);

		/* 
		 * Information on supported window handling capabilities;
		 * theoretically, inquiry types higher than 4 are supposed 
		 * to work only with AES higher than 4.0, but at least Geneva >= 4 
		 * (AES 4.0 ?) recognizes these inquiries anyway
		 */

		appl_x_getinfo(AES_WINDOW, &aes_wfunc, &dummy, &dummy, &dummy);

		/* Information on object handling capabilites */

		if (appl_x_getinfo(AES_OBJECT, &xd_has3d, &ag2, &ag3, &ag4))
		{
			if (ag4 & 0x04)					/* MagiC (OS_WHITEBAK) objects */
				aes_flags |= GAI_WHITEBAK;

			/* get 3D enlargement value and real background colour */

			if (xd_has3d && ag2)			/* 3D-objects and objc_sysvar() present? */
			{
				objc_sysvar(0, AD3DVAL, 0, 0, &aes_hor3d, &aes_ver3d);	/* 3D-enlargements   */
				objc_sysvar(0, BACKGRCOL, 0, 0, &xd_bg_col, &dummy);	/* background colour */
			}
		}

		/* Set some details of font specifications */

#if _SMALL_FONT

		if (xd_small_font.id > 0)	/* for some AESes (Milan) */
			xd_small_font.id = vst_font(xd_vhandle, xd_small_font.id);
		else
			xd_small_font.size = 8;
#endif
		if (xd_regular_font.id > 0)	/* for some AESes (Milan) */
		{
			xd_regular_font.id = vst_font(xd_vhandle, xd_regular_font.id);
		} else
		{
			_WORD dum;
			_WORD effects[3];
			_WORD d[5];

			vqt_fontinfo(xd_vhandle, &dum, &dum, d, &dum, effects);
			xd_regular_font.size = d[4];	/* celltop to baseline */
		}

		/* Is appl_control supported ? */

		if (appl_x_getinfo(AES_NAES, &ag1, &ag2, &ag3, &ag4))
			aes_ctrl = ag1;
	} else
	{
		/* 
		 * Appl_getinfo is not supported, so probably  this is AES < 4 
		 * except that TOS 4.0* (Falcon) identifies itself as AES 3.40 but
		 * is still a "3D" AES. That particular case is dealt with here, 
		 * hopefully without bad effects: AES4 is declared by force.
		 */

		if (get_tosversion() >= 0x400 && aes_version >= 0x330)
		{
			aes_hor3d = 2;
			aes_ver3d = 2;
			xd_aes4col();
		}

		/* Manually set  details of font specifications */

		vqt_attributes(xd_vhandle, work_out);

		xd_regular_font.id = 1;
#if 0									/* currently unused */
		xd_regular_font.type = 0;
#endif
		xd_regular_font.size = work_out[7] <= 7 ? 9 : 10;
#if _SMALL_FONT
		xd_small_font.id = 1;
#if 0									/* currently unused */
		xd_small_font.type = 0;
#endif
		xd_small_font.size = 8;
#endif
	}

#if _SMALL_FONT
	/* Some more settings of font sizes */

	xd_small_font.size = xd_fnt_point(xd_small_font.size, &xd_small_font.cw, &xd_small_font.ch);
#endif
	/* 
	 * In some cases (at least Magic low-res), a too-small font size
	 * is set above; to make matters worse, that information is not
	 * systematically applied, which makes dialogs unusable; 
	 * forcing regular font size to a minimum of 9pt seems to cure this
	 */

	if (xd_regular_font.size < 9)
		xd_regular_font.size = 9;

	xd_regular_font.size = xd_fnt_point(xd_regular_font.size, &xd_regular_font.cw, &xd_regular_font.ch);
	*vdi_handle = xd_vhandle;
	return 0;
}


/*
 * Close all dialogs and then the workstation as well
 */

void exit_xdialog(void)
{
	XDOBJDATA *h = xd_objdata;
	XDOBJDATA *next;

	/* Close the dialogs if there are any that are still open */

	while (xd_dialogs)
		xd_close(xd_dialogs);

	while (xd_nmdialogs)
#if _DOZOOM
		xd_nmclose(xd_nmdialogs, NULL, FALSE);
#else
		xd_nmclose(xd_nmdialogs);
#endif

	while (h)
	{
		next = h->next;
		xd_free(h);
		h = next;
	}

	xw_closeall();

	/* At the very end, close virtual workstation on screen */

	v_clsvwk(xd_vhandle);
}
