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

#ifndef __XDIALOG_H__
#define __XDIALOG_H__

#ifndef __PUREC__
#define cdecl
#endif

#ifndef __XWINDOW_H__
 #include "xwindow.h"
#endif

#define _DOZOOM 0

typedef enum
{
	GAI_WDLG = 0x0001,		/* wdlg_xx()-Funktionen vorhanden */
	GAI_LBOX = 0x0002,		/* lbox_xx()-Funktionen vorhanden */
	GAI_FNTS = 0x0004,		/* fnts_xx()-Funktionen vorhanden */
	GAI_FSEL = 0x0008,		/* neue Dateiauswahl vorhanden */

	GAI_MAGIC= 0x0100,		/* MagiC-AES vorhanden */
	GAI_INFO = 0x0200,		/* appl_getinfo() vorhanden */
	GAI_3D   = 0x0400,		/* 3D-Look vorhanden */
	GAI_CICN = 0x0800,		/* Color-Icons vorhanden */
	GAI_APTERM = 0x1000,	/* AP_TERM wird unterstÿtzt */
	GAI_GSHORTCUT = 0x2000,	/* Objekttyp G_SHORTCUT wird unterstÿtzt */
	GAI_WHITEBAK = 0x4000	/* WHITEBAK objects */
} GAI;

/* Errorcodes. Take care to use same values as in XERROR.H */

#define ENSMEM		-39			/* From xerror.h */
#define XDVDI		-4096		/* Geen vdi handle meer beschikbaar. */
#define XDNMWINDOWS -4097		/* Geen windows meer. */
#define XDNSMEM		ENSMEM		/* Niet voldoende geheugen. */

/* key-status constants */

#define XD_SHIFT		0x0100
#define XD_CTRL			0x0200
#define XD_ALT			0x0400
#define XD_SENDKEY		0x2000	/* mark that this is passed through AV */
#define XD_SCANCODE		0x8000

/* Extended types (1 t/m 63) */

#define XD_EMODECOUNT	64		/* Number of extended types. */

/* 4 is gereserveerd voor speciale functies (bij voorbeeld markering)
   XUSERBLK. */

#define XD_DRAGBOX		5		/* button rechtsboven waarmee dialoogbox verplaatst kan worden. */
#define XD_ROUNDRB		6		/* ronde radiobutton. */
#define XD_RECTBUT		7		/* rechthoekige button. */
#define XD_BUTTON		8		/* gewone button, met toetsbediening. */
#define XD_RBUTPAR		9		/* Rechthoek met titel. */
#define XD_TITLE		10		/* IA: Underlined title */
#define XD_RECTBUTTRI	11		/* IA: rectangle button: tri-state!
								   this means cycle between:
								   OS_NORMAL / OS_SELECTED / OS_CHECKED */
#define XD_CYCLBUT		12		/* IA: cycling button. used with pop-ups mostly. */
#define XD_SCRLEDIT		13		/* HR 021202: scrolling editable text fields. */
#define XD_FONTTEXT		14		/* font sample text */
#define XD_BCKBOX		15		/* window background box */
#define XD_UP			52		/* Codes voor buttons met bediening cursortoetsen */
#define XD_DOWN			53
#define XD_LEFT			54
#define XD_RIGHT		55
#define XD_SUP			56
#define XD_SDOWN		57
#define XD_SLEFT		58
#define XD_SRIGHT		59
#define XD_CUP			60
#define XD_CDOWN		61
#define XD_CLEFT		62
#define XD_CRIGHT		63

/* Constantes voor positie mode */

#define XD_CENTERED	0
#define XD_MOUSE	1
#define XD_CURRPOS	2

/* Constantes voor dialoog mode */

#define XD_NORMAL	0
#define XD_BUFFERED	1
#define XD_WINDOW	2

/* I_A: constants for ob_state when using 'tri-state' buttons */

#define TRISTATE_MASK	(OS_CROSSED | OS_CHECKED)
#define TRISTATE_0		OS_CROSSED
#define TRISTATE_1		OS_CHECKED
#define TRISTATE_2		(OS_CROSSED | OS_CHECKED)

extern const unsigned char xd_emode_specs[XD_EMODECOUNT];
extern char *xd_cancelstring;	/* from xdialog; possible 'Cancel' texts */

#define __XD_IS_ELEMENT		0x01
#define __XD_IS_SPECIALKEY	0x02
#define __XD_IS_SELTEXT		0x04/* element that may employ 'shortcuts' */
#define __XD_IS_NOTYETDEF	0x08/* element type that's not yet defined (how sad ;-)) */

#define xd_is_xtndbutton(emode)		\
	((xd_emode_specs[(emode)] & __XD_IS_SELTEXT) != 0)
#define xd_is_xtndelement(emode)	\
	((xd_emode_specs[(emode)] & __XD_IS_ELEMENT) != 0)
#define xd_is_xtndspecialkey(emode)	\
	((xd_emode_specs[(emode)] & __XD_IS_SPECIALKEY) != 0)

typedef struct xdinfo
{
	OBJECT *tree;			/* pointer naar object boom. */
	_WORD dialmode;			/* dialoog mode. */
	GRECT drect;			/* Maten van de dialoogbox. */
	WINDOW *window;			/* Indien in window mode pointer naar window structuur. */
	_WORD edit_object;		/* Object waarop de cursor staat. */
	_WORD cursor_x;			/* x positie in edit_object. */
	_WORD curs_cnt;			/* cursor counter. */
	MFDB mfdb;				/* Indien in buffered mode structuur met data buffer. */
	struct xd_func *func;	/* Pointer naar de structuur met funkties van een niet modale dialoogbox. */
	struct xdinfo *prev;
} XDINFO;

typedef struct xd_func
{
	void (*dialbutton) (XDINFO *info, _WORD object);
	void (*dialclose) (XDINFO *info);
	void (*dialmenu) (XDINFO *info, _WORD title, _WORD item);
	void (*dialtop) (XDINFO *info);
} XD_NMFUNC;

typedef struct
{
	_WORD ev_mflags;
	_WORD ev_mbclicks, ev_mbmask, ev_mbstate;
	_WORD ev_mm1flags;
	GRECT ev_mm1;
	_WORD ev_mm2flags;
	GRECT ev_mm2;
	unsigned short ev_mtlocount, ev_mthicount;

	_WORD ev_mwhich;
	_WORD ev_mmox, ev_mmoy;
	_WORD ev_mmobutton;
	_WORD ev_mmokstate;
	_WORD ev_mkreturn;
	_WORD ev_mbreturn;

	_WORD ev_mmgpbuf[8];

	_WORD xd_keycode;
} XDEVENT;

typedef _WORD (*userkeys) (XDINFO *info, void *userdata, _WORD scancode);

extern _WORD xd_aes4_0;
extern _WORD xd_colaes;
extern _WORD aes_hor3d;
extern _WORD aes_ver3d;
extern _WORD xd_rbdclick;
extern _WORD colour_icons;

extern _WORD aes_wfunc;	/* result of appl_getinfo(11, ...) */
extern _WORD aes_ctrl;


/* Funkties voor het openen en sluiten van een dialoog */

_WORD xd_open(OBJECT *tree, XDINFO *info);
void xd_close(XDINFO *info);
void xd_enable_menu(_WORD state);

/* Funkties voor het tekenen van objecten in een dialoogbox. */

void xd_draw(XDINFO *info, _WORD start, _WORD depth);
void xd_drawdeep(XDINFO *info, _WORD start);
void xd_drawthis(XDINFO *info, _WORD start);
void xd_change(XDINFO *info, _WORD object, _WORD newstate, _WORD draw);
void xd_buttnorm(XDINFO *info, _WORD button);
void xd_drawbuttnorm(XDINFO *info, _WORD button);
void xd_own_xobjects( bool setit );
void clr_object(GRECT *r, _WORD colour, _WORD pattern);
void draw_xdrect(_WORD x, _WORD y, _WORD w, _WORD h);
void xd_vswr_trans_mode(void);
void xd_vswr_repl_mode(void);

/* Funkties voor het uitvoeren van een dialoog. */

_WORD xd_kform_do(XDINFO *info, _WORD start, userkeys userfunc, void *userdata);
_WORD xd_form_do(XDINFO *info, _WORD start);
_WORD xd_form_do_draw(XDINFO *info);
_WORD xd_kdialog(OBJECT *tree, _WORD start, userkeys userfunc, void *userdata);
_WORD xd_dialog(OBJECT *tree, _WORD start);

/* Funkties voor initialisatie van een resource. */

_WORD xd_gaddr(_WORD index, void *addr);
void xd_fixtree(OBJECT *tree);
void xd_set_userobjects(OBJECT *tree);
char *xd_set_srcl_text(OBJECT *tree, _WORD item, char *txt);

/* Funkties voor het zetten van de verschillende modes */

_WORD xd_setdialmode(_WORD new, _WORD (*hndl_message) (_WORD *message), OBJECT *menu, _WORD nmnitems, const _WORD *mnitems);
_WORD xd_setposmode(_WORD new);

/* Funkties voor initialisatie bibliotheek */

_WORD init_xdialog(_WORD *vdi_handle, void *(*malloc) (unsigned long size),
						void (*free) (void *block), const char *prgname,
						_WORD load_fonts, _WORD *nfonts);
void exit_xdialog(void);


/* Hulpfunkties */

_WORD xd_isrect(GRECT *r);
_WORD xd_rcintersect(GRECT *r1, GRECT *r2, GRECT *intersection);
_WORD xd_inrect(_WORD x, _WORD y, GRECT *r);
long xd_initmfdb(GRECT *r, MFDB *mfdb);
void xd_objrect(OBJECT *tree, _WORD object, GRECT *r);
void xd_userdef(OBJECT *object, USERBLK *userblk, _WORD cdecl(*code) (PARMBLK *parmblock));
void xd_rect2pxy(GRECT *r, _WORD *pxy);
_WORD xd_obj_parent(OBJECT *tree, _WORD object);
_WORD xd_xobtype(OBJECT *tree);
_WORD xd_wdupdate(_WORD mode);
void xd_begupdate(void);
void xd_endupdate(void);
void xd_begmctrl(void);
void xd_endmctrl(void);
void xd_mouse_off(void);
void xd_mouse_on(void);
void xd_screensize(void);

_WORD xd_get_rbutton(OBJECT *tree, _WORD rb_parent);
_WORD xd_set_rbutton(OBJECT *tree, _WORD rb_parent, _WORD object);
void xd_set_child(OBJECT *tree, _WORD rb_parent, _WORD enab);

OBSPEC *xd_get_obspecp(OBJECT *object);
char *xd_pvalid(OBJECT *object);
char *xd_ptext(OBJECT *object);
void xd_zerotext(OBJECT *object);
void xd_set_obspec(OBJECT *object, OBSPEC *obspec);
void *xd_get_scrled(OBJECT *tree, _WORD edit_obj);
void xd_init_shift(OBJECT *obj, const char *text);

/* Currently not used anywhere in Teradesk
_WORD xd_set_tristate(_WORD ob_state, _WORD state);
_WORD xd_get_tristate(_WORD ob_state);
_WORD xd_is_tristate(OBJECT *tree);
*/

void xd_clip_on(GRECT *r);
void xd_clip_off(void);

_WORD xd_vst_point(_WORD height, _WORD *ch);
_WORD xd_fnt_point(_WORD height, _WORD *cw, _WORD *ch);

/* Event funkties */

void xd_clrevents(XDEVENT *ev);
_WORD xe_keycode(_WORD scancode, _WORD kstate);
_WORD xe_xmulti(XDEVENT *events);
_WORD xe_button_state(void);
_WORD xe_mouse_event(_WORD mstate, _WORD *x, _WORD *y, _WORD *kstate);

/* Funkties voor niet modale dialoogboxen. */

_WORD xd_nmopen(OBJECT *tree, XDINFO *info, XD_NMFUNC *funcs, _WORD start, 
/* _WORD x, _WORD y, not used */ 
OBJECT *menu, 
#if _DOZOOM
GRECT *xywh, _WORD zoom, 
#endif
const char *title);


void xd_nmclose(XDINFO *info
#if _DOZOOM
,GRECT *xywh, _WORD zoom
#endif
);

void get_fsel(XDINFO *info, char *result, _WORD flags);
void opn_hyphelp (void);

_WORD cdecl ub_bckbox(PARMBLK *pb);

#include "internal.h"

#endif
