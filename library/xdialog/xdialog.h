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

#ifndef __XDIALOG_H__
#define __XDIALOG_H__

#ifndef __PUREC__
#define cdecl
#endif

#include <np_aes.h>			/* HR 151102: modern */

#ifndef __XWINDOW_H__
 #include "xwindow.h"
#endif

/* Errorcodes. */

#define XDVDI			1		/* Geen vdi handle meer beschikbaar. */
#define XDNMWINDOWS		2		/* Geen windows meer. */
#define XDNSMEM			3		/* Niet voldoende geheugen. */

/* key-status constants */

#define XD_SHIFT		0x0100
#define XD_CTRL			0x0200
#define XD_ALT			0x0400
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
								   NORMAL / SELECTED / CHECKED */
#define XD_CYCLBUT		12		/* IA: cycling button. used with pop-ups mostly. */
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

#define TRISTATE_MASK	(CROSSED | CHECKED)
#define TRISTATE_0		CROSSED
#define TRISTATE_1		CHECKED
#define TRISTATE_2		(CROSSED | CHECKED)

extern unsigned char xd_emode_specs[XD_EMODECOUNT];

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
	int dialmode;			/* dialoog mode. */
	GRECT drect;			/* Maten van de dialoogbox. */
	WINDOW *window;			/* Indien in window mode pointer naar window structuur. */
	int edit_object;		/* Object waarop de cursor staat. */
	int cursor_x;			/* x positie in edit_object. */
	int curs_cnt;			/* cursor counter. */
	MFDB mfdb;				/* Indien in buffered mode structuur met data buffer. */
	struct xd_func *func;	/* Pointer naar de structuur met funkties van een niet modale dialoogbox. */
	struct xdinfo *prev;
} XDINFO;

typedef struct xd_func
{
	void (*dialbutton) (XDINFO *info, int object);
	void (*dialclose) (XDINFO *info);
	void (*dialmenu) (XDINFO *info, int title, int item);
	void (*dialtop) (XDINFO *info);
} XD_NMFUNC;

typedef struct
{
	int ev_mflags;
	int ev_mbclicks, ev_mbmask, ev_mbstate;
	int ev_mm1flags;
	GRECT ev_mm1;
	int ev_mm2flags;
	GRECT ev_mm2;
	unsigned int ev_mtlocount, ev_mthicount;

	int ev_mwhich;
	int ev_mmox, ev_mmoy;
	int ev_mmobutton;
	int ev_mmokstate;
	int ev_mkreturn;
	int ev_mbreturn;

	int ev_mmgpbuf[8];

	int xd_keycode;
} XDEVENT;

typedef int (*userkeys) (XDINFO *info, void *userdata, int scancode);

/* Funkties voor het openen en sluiten van een dialoog */

extern void xd_open(OBJECT *tree, XDINFO *info);
extern void xd_open_wzoom(OBJECT *tree, XDINFO *info, GRECT *xywh,
						  int zoom);
extern void xd_close(XDINFO *info);
extern void xd_close_wzoom(XDINFO *info, GRECT *xywh, int zoom);

/* Funkties voor het tekenen van objecten in een dialoogbox. */

extern void xd_draw(XDINFO *info, int start, int depth);
extern void xd_change(XDINFO *info, int object, int newstate,
					  int draw);

/* Funkties voor het uitvoeren van een dialoog. */

extern int xd_kform_do(XDINFO *info, int start, userkeys userfunc,
					   void *userdata);
extern int xd_form_do(XDINFO *info, int start);
extern int xd_kdialog(OBJECT *tree, int start, userkeys userfunc,
					  void *userdata);
extern int xd_dialog(OBJECT *tree, int start);

/* Funkties voor initialisatie van een resource. */

extern int xd_gaddr(int type, int index, void *addr);
extern void xd_fixtree(OBJECT *tree);
extern void xd_set_userobjects(OBJECT *tree);

/* Funkties voor het zetten van de verschillende modes */

extern int xd_setdialmode(int new, int (*hndl_message) (int *message),
						  OBJECT *menu, int nmnitems, int *mnitems);
extern int xd_setposmode(int new);

/* Funkties voor initialisatie bibliotheek */

extern int init_xdialog(int *vdi_handle, void *(*malloc) (unsigned long size),
						void (*free) (void *block), const char *prgname,
						int load_fonts, int *nfonts);
extern void exit_xdialog(void);

/* Hulpfunkties */

extern int xd_rcintersect(GRECT *r1, GRECT *r2, GRECT *intersection);
extern int xd_inrect(int x, int y, GRECT *r);

extern long xd_initmfdb(GRECT *r, MFDB *mfdb);

extern void xd_objrect(OBJECT *tree, int object, GRECT *r);

extern void xd_userdef(OBJECT *object, USERBLK *userblk,
					   int cdecl(*code) (PARMBLK *parmblock));

extern void xd_rect2pxy(GRECT *r, int *pxy);

extern int xd_obj_parent(OBJECT *tree, int object);

extern int xd_wdupdate(int mode);
extern void xd_mouse_off(void);
extern void xd_mouse_on(void);

extern int xd_get_rbutton(OBJECT *tree, int rb_parent);
extern void xd_set_rbutton(OBJECT *tree, int rb_parent, int object);

extern long xd_get_obspec(OBJECT *object);
extern void xd_set_obspec(OBJECT *object, long obspec);

extern int xd_set_tristate(int ob_state, int state);
extern int xd_get_tristate(int ob_state);
extern int xd_is_tristate(OBJECT *tree);

extern void xd_clip_on(GRECT *r);
extern void xd_clip_off(void);

/* Event funkties */

extern int xe_keycode(int scancode, int kstate);
extern int xe_xmulti(XDEVENT *events);
int xe_button_state(void);
int xe_mouse_event(int mstate, int *x, int *y, int *kstate);

/* Funkties voor niet modale dialoogboxen. */

int xd_nmopen(OBJECT *tree, XDINFO *info, XD_NMFUNC *funcs,
			  int start, int x, int y, OBJECT *menu, GRECT *xywh,
			  int zoom, const char *title);
void xd_nmclose(XDINFO *info, GRECT *xywh, int zoom);

#endif
