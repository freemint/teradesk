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


#include <np_aes.h>
#ifdef __PUREC__
 #include <tos.h>
 #include <vdi.h>
 #include <multitos.h>
#else
 #include <aesbind.h>
 #include <osbind.h>
 #include <vdibind.h>
 #include "applgeti.h"
#endif

#include <ctype.h>
#include <string.h>
#include <library.h>

#include "xscncode.h"
#include "xdialog.h"


/* 
 * It seems that information on the small font is not in fact needed,
 * and so that code is disabled by seting _SMALL_FONT to 0
 */

#define _SMALL_FONT 0 

int 
	aes_flags    = 0,	/* proper appl_info protocol (works with ALL Tos) */
	colour_icons = 0,   		/* result of appl_getinfo(2,,,)  */
	aes_wfunc    = 0,			/* result of appl_getinfo(11,,,) */
	aes_ctrl	 = 0,			/* result of appl_getinfo(65,,,) */
	xd_aes4_0,					/* flag that AES 4 is present    */
	xd_colaes = 0,				/* more than 4 colours available */
	xd_has3d = 0,				/* result of appl-getinfo(13...) */
	aes_hor3d    = 0,			/* 3d enlargement value */
	aes_ver3d    = 0,			/* 3d enlargement value */
	xd_fdo_flag = FALSE,		/* Flag voor form_do  */
	xd_bg_col = WHITE,			/* colour of background object */
	xd_ind_col = LWHITE,		/* colour of indicator object  */
	xd_act_col = LWHITE,		/* colour of activator object  */
	xd_sel_col = BLACK;			/* colour of selected object   */

int
	brd_l, brd_r, brd_u, brd_d; /* object border sizes */

extern int 
	xe_mbshift;

/* Window elements for xdialog wndows */

#define XD_WDFLAGS ( NAME | MOVER | CLOSER )

int 
	xd_dialmode = XD_BUFFERED,
	xd_posmode = XD_CENTERED,	/* Position mode */
	xd_vhandle,					/* Vdi handle for library functions */
	xd_nplanes,					/* Number of planes the current resolution */
	xd_ncolours,				/* Number of colours in the current resolution */
	xd_pix_height,				/* pixel size */
	xd_npatterns,				/* Number of available patterns */
	xd_nfills,					/* numbr of available fills */
	xd_min_timer;				/* Minimum time passed to xe_multi(). */

void *(*xd_malloc) (size_t size);
void (*xd_free) (void *block);

const char 
	*xd_prgname;			/* Name of program, used in title bar of windows */

char
	*xd_cancelstring;		/* Possible texts on cancel/abort/undo buttons */

OBJECT 
	*xd_menu = NULL;			/* Pointer to menu tree of the program */

static int 
	(*xd_usermessage) (int *message) = 0L,	/*
                                   function which is called during a 
                                   form_do, if the message is not for
                                   the dialog window */
	xd_nmnitems = 0,		/* Number of menu titles that have to be disabled */
	*xd_mnitems = 0,		/* Array with indices to menu titles in xd_menu,
                               which have to be disabled. The first index is
                               not a title, but the index of the info item. */
	xd_upd_ucnt = 0,		/* Counter for wind_update */
	xd_upd_mcnt = 0,		/* Counter for wind_update */
	xd_msoff_cnt = 0;		/* Counter for xd_mouse_on/xd_mouse_off */


int
	xd_oldbutt = -1;

XDOBJDATA 
	*xd_objdata = NULL;		/* Arrays with USERBLKs */

XDINFO 
	*xd_dialogs = NULL,		/* Chained list of modal dialog boxes. */
	*xd_nmdialogs = NULL;	/* List with non modal dialog boxes. */

RECT 
	xd_desk;				/* Dimensions of desktop background. */

XDFONT 
#if _SMALL_FONT
	xd_small_font,			/* small font definition */
#endif
	xd_regular_font;		/* system font definition */ 

static void __xd_redraw(WINDOW *w, RECT *area);
static void __xd_moved(WINDOW *w, RECT *newpos);

extern int __xd_hndlkey(WINDOW *w, int key, int kstate);
extern void __xd_hndlbutton(WINDOW *w, int x, int y, int n, int bstate, int kstate);
extern void __xd_hndlmenu(WINDOW *w, int title, int item);

extern void __xd_topped(WINDOW *w);
extern void __xd_closed(WINDOW *w, int mode);
extern void __xd_top(WINDOW *w);


int get_tosversion(void);
extern void get_fsel( XDINFO *info, char *result, int flags );

static WD_FUNC xd_wdfuncs =
{
	0L,				/* hndlkey */
	0L,				/* hndlbutton */
	__xd_redraw,	/* redraw */
	__xd_topped,	/* topped */
	xw_nop1,		/* bottomed */
	__xd_topped,	/* newtop */
	0L,				/* closed */
	0L,				/* fulled */
	xw_nop2,		/* arrowed */
	0L,				/* hslider */
	0L,				/* vslider */
	0L,				/* sized */
	__xd_moved,		/* moved */
	0L,				/* hndlmenu */
	0L,				/* top */
	0L,  			/* iconify */
	0L				/* uniconify */
};


static WD_FUNC xd_nmwdfuncs =
{
	__xd_hndlkey,
	__xd_hndlbutton,
	__xd_redraw,
	__xd_topped,
	0L,				/* bottomed */
	__xd_topped,
	__xd_closed,
	0L,				/* fulled */
	0L,				/* arrowed */
	0L,				/* hslid */
	0L,				/* vslid */
	0L,				/* sized */
	__xd_moved,
	0L, /* __xd_hndlmenu, currently there are no menus in nonmodal dialogs */
	__xd_top,
	0L, 			/* uniconify */
	0L 				/* iconify */
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
	MFDB
		source;

	int
		pxy[8];

	source.fd_addr = NULL;

	xd_rect2pxy(&info->drect, pxy);

	pxy[4] = 0;
	pxy[5] = 0;
	pxy[6] = info->drect.w - 1;
	pxy[7] = info->drect.h - 1;

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
	MFDB
		dest;

	int
		pxy[8];

	dest.fd_addr = NULL;

	pxy[0] = 0;
	pxy[1] = 0;
	pxy[2] = info->drect.w - 1;
	pxy[3] = info->drect.h - 1;

	xd_rect2pxy(&info->drect, &pxy[4]);
	xd_mouse_off();
	vro_cpyfm(xd_vhandle, S_ONLY, pxy, &info->mfdb, &dest);
	xd_mouse_on();
}


/* 
 * Enable all items in a menu (NORMAL or DISABLED).
 * First menu item will always be set to NORMAL in order to enable
 * access to accessorries.
 */

void xd_enable_menu(int state)
{
	if (xd_menu)
	{
		int i;

		for (i = 0; i < xd_nmnitems; i++)
			xd_menu[xd_mnitems[i]].ob_state = (i == 1) ? NORMAL : state;

		menu_bar(xd_menu, 1);
	}
}


/* 
 * Funktie die ervoor zorgt dat een dialoogbox binnen het scherm valt.
 * Find the intersection of two areas for clipping ? 
 */

static void xd_clip(XDINFO *info, RECT *clip)
{
	if (info->drect.x < clip->x)
		info->drect.x = clip->x;

	if (info->drect.y < clip->y)
		info->drect.y = clip->y;

	if ((info->drect.x + info->drect.w) > (clip->x + clip->w))
		info->drect.x = clip->x + clip->w - info->drect.w;

	if ((info->drect.y + info->drect.h) > (clip->y + clip->h))
		info->drect.y = clip->y + clip->h - info->drect.h;
}


/* 
 * Funktie voor het verplaatsen van een dialoog naar een nieuwe positie 
 */

static void xd_set_position(XDINFO *info, int x, int y)
{
	int
		dx,
		dy;

	OBJECT
		*tree = info->tree;


	dx = x - info->drect.x;
	dy = y - info->drect.y;
	tree->ob_x += dx;
	tree->ob_y += dy;
	info->drect.x = x;
	info->drect.y = y;
}


/* 
 * Get the values of size enlargement of a bordered object.
 * This function sets (in brd_l, brd_r, etc.) separate values for 
 * enlargements on all four sides of an object.
 */

static void xd_border(OBJECT *tree)
{
	int
		old_x,
		old_y,
		x,
		y,
		w,
		h;


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

void xd_calcpos(XDINFO *info, XDINFO *prev, int pmode)
{
	int
		dummy;

	OBJECT
		*tree = info->tree;


	/* 
	 * Find border sizes. Attention: proper use of this data relies on
	 * an assumption that ALL dialogs opened one from another
	 * will have identical borders!!!
	 */

	xd_border(tree);

	/* Now position the dialog appropriately */

	if ((pmode == XD_CENTERED) && (prev == NULL))
		form_center(tree, &info->drect.x, &info->drect.y, &info->drect.w, &info->drect.h);
	else
	{
		info->drect.w = tree->ob_width + brd_l + brd_r;
		info->drect.h = tree->ob_height + brd_u + brd_d;

		switch (pmode)
		{
		case XD_CENTERED:
			info->drect.x = prev->drect.x + (prev->drect.w - info->drect.w) / 2;
			info->drect.y = prev->drect.y + (prev->drect.h - info->drect.h) / 2;
			if ( xd_desk.w > 400 && xd_desk.h > 300 )
			{
				/* 
				 * If there is room on the screen,
				 * stack dialogs a little to the right & down, for nicer looks 
				 */
				info->drect.x += 16;
				info->drect.y += 16;
			}
			break;
		case XD_MOUSE:
			graf_mkstate(&info->drect.x, &info->drect.y, &dummy, &dummy);
			info->drect.x -= info->drect.w / 2;
			info->drect.y -= info->drect.h / 2;
			break;
		case XD_CURRPOS:
			info->drect.x = tree->ob_x - brd_l;
			info->drect.y = tree->ob_y - brd_u;
			break;
		}

		xd_clip(info, &xd_desk);

		tree->ob_x = info->drect.x + brd_l;
		tree->ob_y = info->drect.y + brd_u;
	}
}


/*
 * Toggle on/off and redraw a radiobutton within a parent object
 */

static void xd_rbutton(XDINFO *info, int parent, int object)
{
	int
		i,
		prvstate,
		newstate;

	OBJECT
		*tree = info->tree,
		*treei;
	

	i = tree[parent].ob_head;

	if (info->dialmode == XD_WINDOW)
		xd_cursor_off(info);

	do
	{
		treei = &tree[i];

		if (treei->ob_flags & RBUTTON)
		{
			prvstate = treei->ob_state;
			newstate = (i == object) ? treei->ob_state | SELECTED : treei->ob_state & ~SELECTED;

			if (newstate != prvstate)
				xd_change(info, i, newstate, TRUE);
		}

		i = treei->ob_next;
	}
	while (i != parent);

	if (info->dialmode == XD_WINDOW)
		xd_cursor_on(info);
}


/********************************************************************
 *																	*
 * Funktie ter vervanging van wind_update.							*
 *																	*
 ********************************************************************/


/* This is never used in TeraDesk

/*
 * This routine takes care of multiple-level wind-updates.
 * Only on the first level is actual update done.
 */

int xd_wdupdate(int mode)
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
	}

	return wind_update(mode);
}

*/

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

int xd_set_keys(OBJECT *tree, KINFO *kinfo)
{
	int
		i = 0,
		cur = 0;

	while(1)
	{	
		char 
			*h = NULL;

		OBJECT
			*c_obj = &tree[cur];

		int
			etype = xd_xobtype(c_obj), state = c_obj->ob_state;


		/* Use AES 4 extended object state if it is there. */

		if ( xd_is_xtndbutton(etype) || (c_obj->ob_type & 0xff) == G_BUTTON)
		{
			if (state & WHITEBAK)
			{
				char 
					*p = xd_get_obspecp(c_obj)->free_string;

				int
					und = (state << 1) >> 9;

				if (und >= 0)
				{
					und &= 0x7f;
					if (und < strlen(p))
						h = p + und;
				}
			}
/* Currently not used anywhere in teradesk
			else if ( xd_is_xtndbutton(etype))
			{
				/* I_A: changed to let '#' through if doubled! */
	
				/* find single '#' */
				for (h = xd_get_obspecp(c_obj)->free_string;
					 (h = strchr(h, '#')) != 0 && (h[1] == '#');
					 h += 2)
					;
	
				if (h) 
					h++;	/* pinpoint exactly */
			}
*/
		}

		if (h)		/* one of the above options. */
		{
			int 
				ch = touppc((int)*h); /* toupper() does not work above code 127 */

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

		if ((c_obj->ob_flags & LASTOB) || (i == MAXKEYS))
			return i;

		cur++;
	}
}


/* 
 * Return TRUE if an object is selectable; otherwise return FALSE
 * if this object or any of its ancestors is hidden or disabled. 
 */

static int xd_selectable(OBJECT *tree, int object)
{
	int
		parent;

	OBJECT
		*o = &tree[object];

	if ((o->ob_flags & HIDETREE) || (o->ob_state & DISABLED))
		return FALSE;

	parent = xd_obj_parent(tree, object);

	while (parent > 0)
	{
		o = tree + parent;

		if ((o->ob_flags & HIDETREE) || (o->ob_state & DISABLED))
			return FALSE;

		parent = xd_obj_parent(tree, parent);
	}

	return TRUE;
}


/* 
 * Find if a key is a hotkey 
 */

int xd_find_key(OBJECT *tree, KINFO *kinfo, int nk, int key)
{
	int
		i,
		k= (key & 0xFF);


	/* Create "uppercase" of characters above 127 for other codepages */

	if ( k > 127 )
		key = ( (key & 0xFF00) | touppc(key & 0xFF) );

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
 * Find a dialog which is relaed to a window
 */

XDINFO *xd_find_dialog(WINDOW *w)
{
	XDINFO *info = xd_dialogs;	/* look among normal dialogs first */


	while ((info != NULL) && (info->window != w))
		info = info->prev;

	if (info == NULL)			/* if not found there... */
	{
		info = xd_nmdialogs;	/* ...look among nonmodal dialogs */

		while ((info != NULL) && (info->window != w))
			info = info->prev;
	}

	return info;
}


/* 
 * Funktie voor het afhandelen van een redraw event. 
 */

static void __xd_redraw(WINDOW *w, RECT *area)
{
	XDINFO *info = xd_find_dialog(w);

	xd_begupdate();
	xd_redraw(info, ROOT, MAX_DEPTH, area, XD_RDIALOG | XD_RCURSOR);
	xd_endupdate();
}


/* 
 * Funktie voor het afhandelen van een window moved event. 
 */

static void __xd_moved(WINDOW *w, RECT *newpos)
{
	XDINFO *info = xd_find_dialog(w);

/* no need if there are no menus in dialog windows 
	RECT work;

	xw_setsize(w, newpos);
	xw_getwork(w, &work); /* work area may be modified by menu height */
	xd_set_position(info, work.x, work.y);
*/
	xw_setsize(w, newpos);
	xd_set_position(info, w->xw_work.x, w->xw_work.y);
}


/*
 * Set focus to an object in a dialog
 * (has to be done to enable keyboard navigation in XaAES)
 * Focus is set by simulating a mouse click upon an object,
 * but the number of clicks is 0, which seems to work.
 */

void xd_focus(OBJECT *tree, int obj)
{
	int d;

	if(obj > 0 && xd_xobtype(tree + obj) != XD_DRAGBOX && xd_oldbutt < 0)
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

int xd_abs_curx(OBJECT *tree, int object, int curx)
{
	XUSERBLK
		*blk = xd_get_scrled(tree, object);

	char
		*tmplt,
		*s,
		*h;


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

		return (int) (s - tmplt);
	}

	return 0;
}


/* 
 * Funktie die uit de positie van de cursor in de template string
 * de positie in de text string bepaald. 
 */

static int xd_rel_curx(OBJECT *tree, int edit_obj, int curx)
{
	char
		*tmplt = xd_get_obspecp(tree + edit_obj)->tedinfo->te_ptmplt;

	int
		i,
		x = 0;


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

static void str_delete(char *s, int pos)
{
	char
		*h = &s[pos],
		ch;


	if (*h)
	{
		do
		{
			ch = h[1];
			*h++ = ch;
		}
		while (ch);
	}
}


/* 
 * Funktie voor het tussenvoegen van een karakter in een string. 
 */

static void str_insert(char *s, int pos, int ch, int curlen, int maxlen)
{
	int
		i,
		m;


	if ( curlen >= maxlen  )
		bell();
	else if (pos < maxlen)
	{
		m = curlen + 1;

		for (i = m; i > pos; i--)
			s[i] = s[i - 1];

		s[pos] = (char)ch;
	}
}


/*
 * Return 1 if a key represents a printable character; otherwise return 0.
 * This should cure some problems with German and similar keyboards.
 */


static int xd_ischar(int key)
{
	int k = key & 0xFF;


	if
	( 
		(key & (XD_SCANCODE | XD_CTRL)) ||
		(
			(key & XD_ALT) && 
			!(k == 64 || k == 91 || k == 92 || k == 93 || k == 123 || k == 125)
		) ||
		key == SHFT_TAB	/* for moving backwards in dialogs */
	)
		return 0;

	return 1;
}


/* 
 * Funktie die controleert of een toets is toegestaan op positie in string. 
 */

static int xd_chk_key(char *valid, int pos, int key)
{
	int 
		ch = key & 0xFF, 
		cch = key & 0xDF;	/* uppercase, but valid only  above '?' */

	char 
		cvalid = valid[pos];


	if(xd_ischar(key))
	{
		if(cvalid == 'X')						/* anything */
			return ch;
		else if ( cvalid == 'x' )				/* anything uppercase */
		{
			if (isupper(cch) || (key & 0x80))
				return cch;
			else
				return ch;
		}

		if (ch < 0x80)							/* more detailed validation */
		{
			switch (cvalid)
			{
			case 'N':
			case 'n':
			case '9':
				if(isdigit(ch))
					return ch;
				if (cvalid == '9')
					break;
			case 'A':
			case 'a':
				if (ch == ' ')
					return ch;
				if(isupper(cch))
					return (cvalid & 0x20) ? ch : cch;
				break;
			case 'F':
			case 'P':
			case 'p':
			case '?':
				if(isupper(cch))	/* A-Z ONLY */
					return cch;
				if 
				(
					(strchr("_:œ-0123456789", ch) != 0) || /* additional permitted characters */
					((cvalid == '?') && strchr("*?~![]", ch) != 0) ||				/* wildcards */
					((cvalid == 'P' || cvalid == 'p') && strchr("\\.", ch) != 0)	/* characters for paths */
				)
					return ch;
			}
		}
	}

	return 0;
}


static int xd_chk_skip(OBJECT *tree, int edit_obj, int key)
{
	char 
		*s = xd_get_obspecp(tree + edit_obj)->tedinfo->te_ptmplt,
		*h;


	if(!xd_ischar(key))
		return 0;

	if (((h = strchr(s, key & 0xFF)) != NULL) && ((h = strchr(h, '_')) != NULL))
		return xd_rel_curx(tree, edit_obj, (int) (h - s));

	return 0;
}


/*
 * Check if the object is a scrolled editable text field, and
 * return a pointer to XUSERBLK if it is
 */

void *xd_get_scrled(OBJECT *tree, int edit_obj)
{
	if(xd_xobtype(&tree[edit_obj]) == XD_SCRLEDIT)
		return (XUSERBLK *)tree[edit_obj].ob_spec.userblk->ub_parm;

	return NULL;
}


/* 
 * Initialize shift position for scrolled text field so as to
 * agree with initial cursor position at the end of the string
 * (this amounts to initially displaying the -trailing- end of the text).
 * Note: use carefully, there is no checking of object type
 */

void xd_init_shift(OBJECT *obj, char *text)
{
	int
		tl = (int)strlen(text), 			/* real string length */
		vl = (int)strlen(xd_pvalid(obj));	/* form length */


	/* offset of first visible char */
	
	((XUSERBLK *)(obj->ob_spec.userblk->ub_parm))->uv.ob_shift = max(tl - vl, 0);
}


/* 
 * HR 021202: scrolling in scrolling editable texts. 
 * Note: "pos" is position in the edited text, not in the visible field.
 * flen is the length of the editable field.
 * The object need not be of the scrolled type.
 */

static boolean xd_shift(XUSERBLK *blk, int pos, int flen, int clen)
{
	if (blk)
	{
		int shift = blk->uv.ob_shift;

		if (pos == clen)			/* trailing end of text is visible */
			shift = clen - flen;
		if (shift < 0)				/* can't go off the first character */
			shift = 0;
		if (pos < shift)			/* scroll left */
			shift = pos;
		if (pos > shift + flen)		/* scroll right */
			shift = pos - flen;
		if (shift != blk->uv.ob_shift)	/* there was a change in shift amount */
			return blk->uv.ob_shift = shift, true;
	}

	return false;
}


/*
 * Edit an editable text. This need not be of the scrolled type. 
 */

int xd_edit_char(XDINFO *info, int key)
{
	TEDINFO 
		*tedinfo;

	RECT 
		clip;

	XUSERBLK 
		*blk;

	char
		*val,
		*str;

	OBJECT 
		*tree;

	int 
		edit_obj, 
		newpos,	/* new position in edited string (in tedinfo->te_ptext) */ 
		oldpos,	/* previous position in same */ 
		curlen,	/* length of edited string   */ 
		maxlen,	/* maximum possible length of editable string */ 
		flen,	/* length of editable field (of tedinfo->te_pvalid) */ 
		pos, 
		ch,
		result = TRUE,
		m_on = FALSE;


	tree  = info->tree;

	if ((edit_obj = info->edit_object) <= 0)
		return FALSE;

	tedinfo = xd_get_obspecp(tree + edit_obj)->tedinfo;
	str = tedinfo->te_ptext;
	val = tedinfo->te_pvalid;
	blk = xd_get_scrled(tree, edit_obj);
	oldpos = newpos = info->cursor_x;
	curlen = (int)strlen(str);
	flen = (int)strlen(val);

	objc_offset(tree, edit_obj, &clip.x, &clip.y);
	clip.h = xd_regular_font.ch;
	clip.w = xd_regular_font.cw * tedinfo->te_tmplen;

	if (blk)
	{
		/* only scrolled-text fields are handled here */

		clip.x -= xd_regular_font.cw + 2;		/* HR: This temporary until ub_scrledit() handles templates properly. */
		clip.w += 2 * xd_regular_font.cw + 4;
		clip.h += 4;
		maxlen = XD_MAX_SCRLED;
	}
	else
		maxlen = flen;

	if(xd_ischar(key))
		key &= 0xFF;

	switch (key)
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
		if(oldpos >= curlen)
			break;
			goto doshift;
	case BACKSPC:
		if( oldpos <= 0 )
			break;
	case ESCAPE:

		doshift:;

		xd_mouse_off();
		xd_cursor_off(info);

		if(key == ESCAPE)
		{
			*str = 0;
			info->cursor_x = 0;
			curlen = 0;
		}
		else
		{
			if(key == BACKSPC)
				info->cursor_x--;
			str_delete(str, info->cursor_x);
			curlen--;		
		}
		m_on = TRUE;
		goto shift_redraw;
	case INSERT: /* call the fileselector */
		if(blk)
		{
			xd_cursor_off(info);
			get_fsel(info, str, tree[edit_obj].ob_flags);
			curlen = (int)strlen(str);
			goto shift_redraw;
		}
	case HOME:
	case HELP:
		/* These codes have no effect */
		break;
	default:

		pos = oldpos;
		if(oldpos == maxlen)
			pos--;

		/* For a scrolled editable text use only the first validation character */

		if ((ch = xd_chk_key(val, (blk) ? 0 : pos, key))== 0 && !blk)
			pos = xd_chk_skip(tree, edit_obj, key);

		if (ch != 0 || pos > 0 )
		{
			xd_mouse_off();
			xd_cursor_off(info);

			if (ch != 0)
			{
				info->cursor_x = pos + 1;
				str_insert(str, pos, ch, curlen, maxlen);
			}
			else
			{
				int i;
				info->cursor_x = pos;

				for (i = oldpos; i < pos; i++)
					str[i] = ' ';
				str[pos] = 0;
			}

			m_on = TRUE;
			curlen++;

			shift_redraw:;

			xd_shift(blk, info->cursor_x, flen, curlen); /* scrolling editable texts. */
			xd_redraw(info, edit_obj, 0, &clip, XD_RDIALOG);
			xd_cursor_on(info);

			if(m_on)
				xd_mouse_on();
		}
		else
			result = FALSE;
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


void xd_edit_init(XDINFO *info, int object, int curx)
{
	OBJECT
		*tree = info->tree;


	if ((object > 0) && xd_selectable(tree, object))
	{
		XUSERBLK *blk = xd_get_scrled(tree, object);
		TEDINFO *ted = xd_get_obspecp(tree + object)->tedinfo;
		int x, dummy, maxlen;

		xd_focus(tree, object);

		maxlen = (int)strlen(ted->te_ptext);

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
		}
		else if ((curx >= 0) && (x != info->cursor_x))
		{
			xd_cursor_off(info);
			info->cursor_x = x;
			xd_cursor_on(info);
		}

		xd_shift(blk, info->cursor_x, (int)strlen(ted->te_pvalid), maxlen);
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

int xd_find_obj(OBJECT *tree, int start, int which)
{
	int 
		theflag,
		obj = 0,
		flag = EDITABLE,
		inc = ((tree[start].ob_flags & LASTOB) != 0) ? 0 : 1;


	switch (which)
	{
	case FMD_BACKWARD:
		inc = -1;
	case FMD_FORWARD:
		obj = start + inc;
		break;
	case FMD_DEFLT:
		flag = DEFAULT;
		break;
	}

	while (obj >= 0)
	{
		theflag = tree[obj].ob_flags;
		if ((theflag & flag) && xd_selectable(tree, obj))
			return obj;
		if (theflag & LASTOB)
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

static int xd_find_cancel(OBJECT *ob)
{
	int 
		f = 0;

	char
		*s = NULL, 
		t[16] = {'|'}; /* for "cancel" words up to 13 characters long */

	do
	{
		t[1] = 0;

		/* Consider only normal buttons and buttons with underlined text */

		if 
		(   
			((ob[f].ob_type) == G_BUTTON || (xd_xobtype(&ob[f]) == XD_BUTTON))
			&& (ob[f].ob_flags & (SELECTABLE | TOUCHEXIT | EXIT)) != 0 
		)
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

			if ( strstr(xd_cancelstring, t) != NULL )
				return f;
		}

	}
	while ( !(ob[f++].ob_flags & LASTOB));

	return -1;
}


int xd_form_keybd(XDINFO *info, int kobnext, int kchar, int *knxtobject, int *knxtchar)
{
	OBJECT 
		*tree = info->tree;

	int 
		i = xd_find_cancel(tree), 
		mode = FMD_FORWARD;


	*knxtobject = kobnext;
	*knxtchar = 0;

	if(xd_ischar(kchar))
		kchar &= 0xFF;

	switch (kchar)
	{
		case CURUP:
		case SHFT_TAB:
			mode = FMD_BACKWARD;
		case CURDOWN:
		case TAB:
			if ((i = xd_find_obj(tree, info->edit_object, mode)) > 0)
				*knxtobject = i;
			break;
		case RETURN:
			i = xd_find_obj(tree, 0, FMD_DEFLT);
		case UNDO:
			if(i > 0)
			{
				xd_change(info, i, SELECTED, TRUE);
				*knxtobject = i;
				return FALSE;
			}
			break;
		default:
			*knxtchar = kchar;
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

int xd_oldarrow(XDINFO *info)
{
	if(xd_oldbutt > 0)
	{
		int
			event;

		XDEVENT
			tev;

		xd_clrevents(&tev);
		tev.ev_mflags =  MU_BUTTON | MU_TIMER;
		tev.ev_mbmask = 1;
		tev.ev_mtlocount = 200;		/* 200ms delay while arrow pressed */

		do
		{
			event = xe_xmulti(&tev);	
		}
		while(!(event & (MU_TIMER | MU_BUTTON)));

		if(event & MU_BUTTON)
		{
			xd_drawbuttnorm(info, xd_oldbutt);
			xd_oldbutt = -1;
		}
		else
			return 1;	/* still pressed */
	}

	return 0;	/* not pressed anymore */
}


int xd_form_button(XDINFO *info, int object, int clicks, int *result)
{
	OBJECT 
		*tree = info->tree;

	int 
		parent, 
		oldstate, 
		dummy,
		flags = tree[object].ob_flags, 
		etype = xd_xobtype(&tree[object]);


	/* Handle arrow (boxchar) buttons */

	if(etype >= XD_CUP && etype <= XD_CRIGHT)
	{
		xd_oldbutt = object;
		xd_change(info, object, SELECTED, TRUE);
	}
	else
		xd_oldbutt = -1;

	/* Now handle other stuff... */

	if (xd_selectable(tree, object) && ((flags & SELECTABLE) || (flags & TOUCHEXIT)))
	{
		oldstate = tree[object].ob_state;

		xd_focus(tree, object);
		xd_begmctrl();

		if (flags & RBUTTON)
		{
			if (((parent = xd_obj_parent(tree, object)) >= 0) && !(oldstate & SELECTED))
				xd_rbutton(info, parent, object);
		}
		else if (flags & SELECTABLE)
		{
			XDEVENT events;
			int evflags, newstate, state;

/* there are currently no tristate objects in Teradesk

			/* I_A changed to fit tristate-buttons! */

			if (!xd_is_tristate(tree + object))
				newstate = (oldstate & SELECTED) ? oldstate & ~SELECTED : oldstate | SELECTED;
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
*/
			newstate = oldstate ^ SELECTED;

			xd_clrevents(&events);
			events.ev_mflags = MU_BUTTON | MU_TIMER;
			events.ev_mbclicks = 1;
			events.ev_mbmask = 1;
			events.ev_mm1flags = 1;

			xd_objrect(tree, object, &events.ev_mm1);
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
					}
					else
					{
						events.ev_mm1flags = 1;
						state = newstate;
					}

					xd_change(info, object, state, TRUE);
				}

				events.ev_mflags = MU_BUTTON | MU_M1;
				events.ev_mtlocount = 0;
			}
			while (!(evflags & MU_BUTTON));
		}

		xd_endmctrl();

		if (flags & TOUCHEXIT)
		{
			*result = object | ((clicks > 1) ? 0x8000 : 0);
			return FALSE;
		}

		if ((flags & EXIT) && (tree[object].ob_state != oldstate))
		{
			*result = object;
			return FALSE;
		}

		evnt_button(1, 1, 0, &dummy, &dummy, &dummy, &dummy);
	}

	*result = (flags & EDITABLE) ? object : 0;

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

int xd_movebutton(OBJECT *tree)
{
	OBJECT
		*c_obj;

	int
		cur = 0;


	while(1)
	{
		c_obj = &tree[cur];

		if(xd_xobtype(c_obj) == XD_DRAGBOX)
			return cur;

		if (c_obj->ob_flags & LASTOB)
			return -1;

		cur++;
	}
}



/* 
 * Eigen form_do.
 * Note: this will return -1 if closed without any button. 
 */


int xd_kform_do(XDINFO *info, int start, userkeys userfunc, void *userdata)
{
	OBJECT
		*tree = info->tree;

	XDEVENT
		events;

	int
		which,
		kr,
		db,
		nkeys,
		cont,
		next_obj,
		s,
		cmode;

	boolean
		inw = TRUE;

	KINFO
		kinfo[MAXKEYS];


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

	if(start == 0)
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
		}
		else
		{
			events.ev_mflags &= ~MU_TIMER;
			events.ev_mtlocount = 0;
		}

		xd_endupdate();

		which = xe_xmulti(&events);

		if ( (which & MU_MESAG) != 0 ) 
		{
			if (xd_usermessage != 0L && xd_usermessage(events.ev_mmgpbuf) != 0)
			{
				next_obj = -1;
				cont = FALSE;
			}
			else if 
			( 
				(events.ev_mmgpbuf[0] == WM_CLOSED) && inw &&
				(events.ev_mmgpbuf[3] == info->window->xw_handle ) 
			)
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
			int object;

			if ((userfunc == (userkeys)0) || (userfunc(info, userdata, events.xd_keycode) == 0))
			{
				if ((object = xd_find_key(tree, kinfo, nkeys, events.xd_keycode)) >= 0)
				{
					next_obj = object;
					cont = xd_form_button(info, next_obj, 1, &next_obj);
				}
				else
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
			}
			else
			{
				if ((cont = xd_form_button(info, next_obj, events.ev_mbreturn, &next_obj)) != FALSE)
					cmode = events.ev_mmox;

				if ((next_obj & 0x7FFF) == db) /* move by dragbox */
				{
					int nx, ny;

					graf_dragbox(info->drect.w, info->drect.h, info->drect.x, info->drect.y,
								 xd_desk.x, xd_desk.y, xd_desk.w, xd_desk.h, &nx, &ny);

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


int xd_form_do(XDINFO *info, int start)
{
	return xd_kform_do(info, start, (userkeys)0, NULL) & 0x7FFF;
}


/*
 * Same as above but immediately change button state to normal and redraw
 */

int xd_form_do_draw(XDINFO *info)
{
	int button;
	button = xd_form_do(info, ROOT);
	xd_change( info, button, NORMAL, 1);
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
 * xywh		- Optional pointer to a RECT structure. If this pointer
 *			  is not NULL and zoom is not 0, the library will draw
 *			  a zoombox from the rectangle in xywh to the window.
 * zoom		- see xywh
 * title	- Optional title. If not NULL this string is used as the
 *			  title of the window. Otherwise the program name is used (nonmodal only)
 */

static int xd_open_wzoom
(
	OBJECT *tree, 
	XDINFO *info,

	XD_NMFUNC *funcs,
	int start, 

/* currently not used anywhere
	int x, 
	int y, 
*/

	OBJECT *menu,

#ifdef _DOZOOM
	RECT *xywh, 
	int zoom
#endif

	const char *title
)
{
	XDINFO 
		*prev;

	int 
		db,
		dialmode,
		error;


	prev = xd_dialogs;
	dialmode = xd_dialmode;
	error = 0;
	
	db = xd_movebutton(tree);

	if(funcs)
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
			RECT wsize;
			int d, thetype;
			WD_FUNC *thefuncs;
			size_t thesize;
			OBJECT *themenu;

			tree[db].ob_flags |= HIDETREE; /* hide dragbox */

			if ( funcs )
			{
				/* This is for nonmodal windowed dialogs */
				thetype = XW_NMDIALOG;
/* currently there are no menus in dialogs
				themenu = menu;
*/
				thefuncs = &xd_nmwdfuncs;
				thesize = sizeof(XD_NMWINDOW);
			}
			else
			{
				/* This is for other windowed dialogs */
				thetype = XW_DIALOG;
/* currently there are no menus in dialogs
				themenu = NULL;
*/
				thefuncs = &xd_wdfuncs;
				thesize = sizeof(WINDOW);
			}

			themenu = NULL; /* while there are no menus in dialogs */

			xw_calc(WC_BORDER, XD_WDFLAGS, &info->drect, &wsize, themenu );

			/* Nicer looking window border */

			tree->ob_x -= 1; 
			wsize.w -= 2;
			wsize.h -= 1;

/* Currently this is not used anywhere in TeraDesk (See font.c only)

			if ( funcs && (x != -1) && (y != -1) )
			{ 
				int dx, dy;

				dx = x - wsize.x;

				info->drect.x += dx;
				tree->ob_x += dx;
				wsize.x = x;

				dy = y - wsize.y;

				info->drect.y += dy;
				tree->ob_y += dy;
				wsize.y = y;
			}
*/
			/* Fit to screen */

			if (wsize.x < xd_desk.x)
			{
				d = xd_desk.x - wsize.x;
				info->drect.x += d;
				tree->ob_x += d;
				wsize.x = xd_desk.x;
			}

			if (wsize.y < xd_desk.y)
			{
				d = xd_desk.y - wsize.y;
				info->drect.y += d;
				tree->ob_y += d;
				wsize.y = xd_desk.y;
			}

#ifdef _DOZOOM 
			if (zoom && xywh)
			{
				graf_growbox(xywh->x, xywh->y, xywh->w, xywh->h,
							 wsize.x, wsize.y, wsize.w, wsize.h);
				zoom = FALSE;
			}
#endif
			if ((w = xw_create( thetype, thefuncs, XD_WDFLAGS,
							   &wsize, thesize, themenu, &error)) != NULL)
			{
				xw_set(w, WF_NAME, (title) ? title : xd_prgname);

				info->window = w;

				if ( funcs )
				{
					info->prev = xd_nmdialogs;
					xd_nmdialogs = info;

					((XD_NMWINDOW *)w)->xd_info = info;
					((XD_NMWINDOW *)w)->nkeys = xd_set_keys(tree, ((XD_NMWINDOW *)w)->kinfo);

					if ( start == 0 )
						start = xd_find_obj(tree, 0, FMD_FORWARD);

					xd_edit_init(info, start, -1);

				}
				else 
					if (prev == NULL)
						xd_enable_menu(DISABLED);

				xw_open(w, &wsize);

#if _DOZOOM
				if (zoom && xywh)
				{
					graf_growbox(xywh->x, xywh->y, xywh->w, xywh->h,
								 wsize.x, wsize.y, wsize.w, wsize.h);
					zoom = FALSE;
				}
#endif

				xd_endupdate();
			}
			else
				if (!funcs)
					dialmode = XD_BUFFERED; /* has no effect in nonmodal */
		}
		else
			dialmode = XD_BUFFERED;

	} /* XD_WINDOW ? */


#ifdef _DOZOOM
	if (zoom && xywh)
	{
		graf_growbox(xywh->x, xywh->y, xywh->w, xywh->h,
		info->drect.x, info->drect.y, info->drect.w, info->drect.h);
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

		info->mfdb.fd_addr = (void *)Malloc(scr_size);

		if ( info->mfdb.fd_addr == NULL )
		{
			xd_endupdate();
			return XDNSMEM;
		}

		tree[db].ob_flags &= ~HIDETREE; 

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


int xd_open(OBJECT *tree, XDINFO *info)
{
#ifdef _DOZOOM
	RECT xywh;
	return xd_open_wzoom(tree, info, NULL, 0, /* 0, 0, */ NULL,   &xywh, TRUE,  NULL);
#else
	return xd_open_wzoom(tree, info, NULL, 0, /* 0, 0, */ NULL, NULL);
#endif
}


int xd_nmopen
(OBJECT *tree, XDINFO *info, XD_NMFUNC *funcs, int start, 
/* int x, int y, not used */ 
OBJECT *menu, 
#if _DOZOOM
RECT *xywh, int zoom, 
#endif
const char *title
)
{
#if _DOZOOM
	return xd_open_wzoom(tree, info, funcs, 0, /* -1, -1, not used */ NULL, NULL, FALSE, wtitle);
#else
 	return xd_open_wzoom(tree, info, funcs, 0, /* -1, -1, not used */ NULL, title);
#endif
}


static void xd_close_wzoom
(
	XDINFO *info,

#ifdef _DOZOOM
RECT *xywh, int zoom,
#endif

	int nmd
)
{
	XDINFO 
		*h,
		*prev;


	h = xd_nmdialogs;
	xd_oldbutt = -1;

	if ( nmd )
	{
		prev = NULL;

		while (h && (h != info))
		{
			prev = h;
			h = h->prev;	/* eigenlijk h->next. */
		}

		if (h != info)
			return;
	}
	else
	{
		if (xd_dialogs != info)
			return;

		prev = info->prev;
	}


	if (info->dialmode == XD_WINDOW)
	{
		WINDOW *w = info->window;

		xd_begupdate();

		if (!(nmd || prev) )
			xd_enable_menu(NORMAL);

		xw_close(w);
		xw_delete(w);

		if ( nmd )
		{
			if ( prev )
				prev->prev = info->prev;
			else
				xd_nmdialogs = info->prev;
		}
		else
			if (prev)
				xw_set(prev->window, WF_TOP);
	}

	if (info->dialmode == XD_BUFFERED)
	{
		xd_restore(info);

/* Can this be xd_free ? */

		Mfree(info->mfdb.fd_addr);
	}

#ifdef _DOZOOM
	if (zoom && xywh)
	{
		graf_shrinkbox(xywh->x, xywh->y, xywh->w, xywh->h,
		info->drect.x, info->drect.y, info->drect.w, info->drect.h);
	}
#endif

	xd_endupdate();

	if (!nmd)
		xd_dialogs = prev;

/*!!! why ?	btw. where is it defined
	if ((prev != NULL) && (prev->dialmode == XD_WINDOW))
		xd_scan_messages(XD_EVREDRAW, NULL);
*/
}


/* 
 * funktie die opgeroepen moet worden na het einde van de dialoog.
 * De funktie hersteld het scherm en geeft het geheugen van de
 * schermbuffer weer vrij. 
 */

void xd_close(XDINFO *info)
{
#ifdef _DOZOOM
	RECT xywh;
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
#ifdef _DOZOOM
	RECT xywh;
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

int xd_kdialog(OBJECT *tree, int start, userkeys userfunc, void *userdata)
{
	XDINFO
		info;

	int
		exit;

	if((exit = xd_open(tree, &info)) >= 0)
	{
		exit = xd_kform_do(&info, start, userfunc, userdata);
		xd_buttnorm(&info, exit);
		xd_close(&info);
	}
	return exit;
}


int xd_dialog(OBJECT *tree, int start)
{
	return xd_kdialog(tree, start, (userkeys) 0, NULL);
}


/********************************************************************
 *																	*
 * Funkties voor het zetten van de dialoogmode en de mode waarmee	*
 * de plaats van de dialoog op het scherm bepaald wordt.			*
 *																	*
 ********************************************************************/

int xd_setdialmode(int new, int (*hndl_message) (int *message),
				   OBJECT *menu, int nmnitems, int *mnitems)
{
	int old = xd_dialmode;

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

int xd_setposmode(int new)
{
	int old = xd_posmode;
	xd_posmode = new;
	return old;
}

/*
 * A small routine to set default colours in AES 4, 
 * in video modes with more than 4 colours.
 * Otherwise, these have been set earlier to WHITE and BLACK, respectively.
 */

void xd_aes4col(void)
{
	if ( xd_colaes )
	{
		xd_bg_col = LWHITE;		/* gray background for some objects */		
		xd_sel_col = LBLACK;	/* dark gray for selected objects */
	}
}


/********************************************************************
 *																	*
 * Funkties voor de initialisatie en deinitialisatie van de module.	*
 *																	*
 ********************************************************************/

int init_xdialog(int *vdi_handle, void *(*malloc) (unsigned long size),
				 void (*free) (void *block), const char *prgname,
				 int load_fonts, int *nfonts)
{
	int dummy, i, work_in[11], work_out[58];

#ifndef __PUREC__
	extern short _global[];
#endif

	xd_malloc = malloc;
	xd_free = free;
	xd_prgname = prgname;

	/* 
	 * Note: Magic (V6.20 at least) returns AES version 3.99.
	 * In that case, after an inquiry ?AGI has been made,
	 * xd_aes4_0 is set to true.
	 * On the other hand, TOS 4.0 returns AES version 3.40
	 * but still is a "3D" AES
	 */

	xd_aes4_0  = (get_aesversion() >= 0x400);

	xd_min_timer = 5;	/* Minimum time passed to xe_multi(); was 10 earlier */

	/* Don't use xw_get() here... */

	wind_get(0, WF_WORKXYWH, &xd_desk.x, &xd_desk.y, &xd_desk.w, &xd_desk.h);
	xd_vhandle = graf_handle(&dummy, &dummy, &dummy, &dummy);

	/* Open virtual workstation on screen */

	for (i = 0; i < 10; i++)
		work_in[i] = 1;

	work_in[10] = 2;

	v_opnvwk(work_in, &xd_vhandle, work_out);

	if (xd_vhandle == 0)

		/* Could not open */

		return XDVDI;
	else
	{
		/* It was successful */

		xd_pix_height = work_out[4];
		xd_ncolours = work_out[13];

		/* 
		 * It is not clear which is the available number of fill designs.
		 * Here are summed the  number of fillpatterns and the number 
		 * of hatches. Still, it sems that one design more is available
		 * than this sum would indicate?
		 */

		xd_npatterns = work_out[11];
		xd_nfills = xd_npatterns + work_out[12] + 1;

		if ( xd_ncolours >= 16 ) 
			xd_colaes = 1;

		vq_extnd(xd_vhandle, 1, work_out);
		xd_nplanes = work_out[4];

		vsf_perimeter(xd_vhandle, 0);		/* no borders in v_bar() */

#ifdef __GNUC_INLINE__
		if (load_fonts && vq_vgdos())
#else
		if (load_fonts && vq_gdos())
#endif
			*nfonts = vst_load_fonts(xd_vhandle, 0);
		else
			*nfonts = 0;

		/* 
		 * Proper appl_getinfo protocol, works also with MagiC:
		 *
		 * Some AESses (at least Geneva 4, AES 4.1, NAES 1.1...) 
		 * do not react to "?AGI" but in fact support appl_getinfo;
		 * So below is forcing to use appl_getinfo with any AES 4.
		 * In fact this is not so bad; a document on TOS recommends:
		 *	has_agi = ((_GemParBlk.global[0] == 0x399 && get_cookie ("MagX", &dummy))
         *	|| (_GemParBlk.global[0] == 0x400 && type < 4)
         *	|| (_GemParBlk.global[0] > 0x400)
         *	|| (appl_find ("?AGI") >= 0));
		 */

		/* if ( appl_find( "?AGI" ) == 0 )	*/	/* appl_getinfo() supported? */

		if ( xd_aes4_0 || (appl_find("?AGI") == 0) )
			aes_flags |= GAI_INFO;

		/* If appl_getinfo is supported, then get diverse information: */

		if (aes_flags & GAI_INFO)
		{
			int ag1, ag2, ag3, ag4;

			/* This is surely (or hopefully?) some sort of "3D" AES 4 */

			xd_aes4_0 = TRUE;

			/* Assume some colour settings */

			xd_aes4col();

			/* Information on "normal" AES font */

			appl_getinfo(0, &xd_regular_font.size, &xd_regular_font.id,
						 &dummy, &dummy);
#if _SMALL_FONT

			/* Information on "small" AES font */

			appl_getinfo(1, &xd_small_font.size, &xd_small_font.id,
						 &dummy, &dummy);

#endif
			/* Information on colour icons and supported rsc format */

			appl_getinfo(2, &ag1, &ag2, &colour_icons, &dummy);

			/* 
			 * Information on supported window handling capabilities;
			 * theoretically, inquiry types higher than 4 are supposed 
			 * to work only with AES higher than 4.0, but at least Geneva >= 4 
			 * (AES 4.0 ?) recognizes these inquiries anyway
			 */

			appl_getinfo(11, &aes_wfunc, &dummy, &dummy, &dummy );

			/* Information on object handling capabilites */

			if ( appl_getinfo( 13, &xd_has3d, &ag2, &ag3, &ag4 ))
			{
				if ( ag4 & 0x08 )				/* G_SHORTCUT untersttzt ? */
					aes_flags |= GAI_GSHORTCUT;
				if ( ag4 & 0x04 )				/* MagiC (WHITEBAK) objects */
					aes_flags |= GAI_WHITEBAK;

				/* get 3D enlargement value and real background colour */

				if ( xd_has3d && ag2 )					/* 3D-Objekte und objc_sysvar() vorhanden? */
				{
					objc_sysvar( 0, AD3DVAL, 0, 0, &aes_hor3d, &aes_ver3d );	/* 3D-enlargements   */
					objc_sysvar( 0, BACKGRCOL, 0, 0, &xd_bg_col, &dummy); 		/* background colour */
				}
			}
			
			/* Set some details of font specifications */

#if _SMALL_FONT

			if (xd_small_font.id > 0)		/* for some AESes (Milan) */
				xd_small_font.id = vst_font(xd_vhandle, xd_small_font.id);
			else
				xd_small_font.size = 8;
#endif
			if (xd_regular_font.id > 0)		/* for some AESes (Milan) */
				xd_regular_font.id = vst_font(xd_vhandle, xd_regular_font.id);
			else
			{
				int dum, effects[3], d[5];
				vqt_fontinfo(xd_vhandle, &dum, &dum, d, &dum, effects);
				xd_regular_font.size = d[4];		/* celltop to baseline */
			}

			/* Is appl_control supported ? */

			if (appl_getinfo(65, &ag1, &ag2, &ag3, &ag4))
				aes_ctrl = ag1;
		}
		else
		{
			/* 
			 * Appl_getinfo is not supported, so probably  this is AES < 4 
			 * except that TOS 4.0* (Falcon) identifies itself as AES 3.40 but
			 * is still a "3D" AES. That particular case is dealt with here, 
			 * hopefully without bad effects: AES4 is declared by force.
			 */

			if ( get_tosversion() >= 0x400 && get_aesversion() >= 0x330 )
			{
				xd_aes4_0 = TRUE; 
				aes_hor3d = 2;
				aes_ver3d = 2;
				colour_icons = TRUE;
				xd_aes4col();
			}

			/* Manually set  details of font specifications */

			vqt_attributes(xd_vhandle, work_out);

			xd_regular_font.id = 1;
/* currently unused
			xd_regular_font.type = 0;
*/
			xd_regular_font.size = (work_out[7] <= 7) ? 9 : 10;
#if _SMALL_FONT
			xd_small_font.id = 1;
/* currently unused
			xd_small_font.type = 0;
*/
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

		if ( xd_regular_font.size < 9 )
			xd_regular_font.size = 9;	

		xd_regular_font.size = xd_fnt_point(xd_regular_font.size, &xd_regular_font.cw, &xd_regular_font.ch);
		*vdi_handle = xd_vhandle;
		return 0;
	}
}


/*
 * Close all dialogs and then the workstation as well
 */

void exit_xdialog(void)
{
	XDOBJDATA *h = xd_objdata, *next;

	/* Close the dialogs if there are any that are still open */

	while (xd_dialogs)
		xd_close(xd_dialogs);

	while (xd_nmdialogs)
#ifdef _DOZOOM
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


