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

#ifndef __XWINDOW_H__
#define __XWINDOW_H__

/*
 * Macro met alle variabelen die benodigd zijn voor de window
 * bibliotheek. Dit macro moet voor alle variabelen van de gebruiker
 * in de window structuur staan.
 */

#define XW_INTVARS	struct window *xw_prev;	\
					struct window *xw_next;	\
					struct wd_func *xw_func;				\
					OBJECT *xw_menu;/* menu object */		\
					RECT xw_size;	/* window size */		\
					RECT xw_work;	/* work area size */	\
					_WORD xw_handle;	/* window handle */		\
					_WORD xw_ap_id;	/* app id of owner */	\
					_WORD xw_bar;		/* menubar object ind */\
					_WORD xw_mparent;	/* parent window id */	\
					_WORD xw_type;	/* window type */		\
					_WORD xw_flags;	/* flags for widgets */	\
					_WORD xw_xflags	/* other diverse flags */

/*
 * Default window structuur, bevat alleen de door de window
 * bibliotheek benodigde variabelen. Is een gebruikers programma
 * eigen variabelen nodig, dan moet de gebruiker een eigen
 * window structuur maken (natuurlijk met een andere naam),
 * waarin de eigen variabelen volgen na XW_INTVARS.
 */

typedef struct window
{
	XW_INTVARS;
} WINDOW;

/*
 * Structuur met funkties die aan een window gekoppeld moeten
 * worden. Een pointer naar een dergelijke structuur moet opgegeven
 * worden bij het creeren van een window. De structuur bevat pointers
 * naar funkties, die worden aangeroepen door de event handler als
 * een event optreed als het window waaraan de funktie gekoppeld is
 * het bovenste is als het event optreedt. Als een bepaald event
 * niet voor kan komen, dan mag een NULL pointer ingevuld worden.
 * Als voor wd_hndlkey of wd_hndlbutton een NULL pointer wordt
 * ingevuld, dan zal, als een dergelijk event optreedt, het event
 * gewoon worden doorgegeven aan de applikatie, als resultaat van
 * xe_multi(). Zijn deze waardes ongelijk NULL, dan wordt het event
 * niet doorgegeven aan de applikatie. wd_hndlkey kan met het 
 * funktie resultaat aangeven of de toets voor het window is bestemd,
 * (TRUE) of dat de toets voor de applikatie bestemd is (FALSE).
 * Alleen in het laatste geval wordt de toets als resultaat van
 * xe_multi() teruggegeven.
 */

typedef struct wd_func
{
	_WORD (*wd_hndlkey) (WINDOW *w, _WORD scancode, _WORD keystate);
	void (*wd_hndlbutton) (WINDOW *w, _WORD x, _WORD y, _WORD n, _WORD bstate, _WORD kstate);
	void (*wd_redraw) 	(WINDOW *w, RECT *area);
	void (*wd_topped) 	(WINDOW *w);
	void (*wd_bottomed)	(WINDOW *w);
	void (*wd_newtop) 	(WINDOW *w);
	void (*wd_closed) 	(WINDOW *w, _WORD mode);
	void (*wd_fulled) 	(WINDOW *w, _WORD mbshift);
	void (*wd_arrowed) 	(WINDOW *w, _WORD arrows);
	void (*wd_hslider) 	(WINDOW *w, _WORD newpos);
	void (*wd_vslider) 	(WINDOW *w, _WORD newpos);
	void (*wd_sized) 	(WINDOW *w, RECT *newsize);
	void (*wd_moved) 	(WINDOW *w, RECT *newpos);
	void (*wd_hndlmenu) (WINDOW *w, _WORD title, _WORD item);
	void (*wd_top)     	(WINDOW *w);
	void (*wd_iconify)	(WINDOW *w, RECT *r);
	void (*wd_uniconify)(WINDOW *w, RECT *r);
} WD_FUNC;

/*
 * Constantes voor het xw_type veld in de window structuur. De
 * waardes 0 t/m 15 en negatieve waardes zijn gereserveerd en
 * mogen niet door de gebruiker gebruikt worden. Alle andere
 * waardes mogen door de gebruiker gebruikt worden voor het
 * definieren van eigen window types.
 */

#define XW_NOT_DEF		0		/* Gebruiker mag dit opgegeven
								   aan wd_create() als hij niet
								   van window types wil gebruik
								   maken. */
#define XW_DIALOG		1		/* Gewone dialoogbox. */
#define XW_NMDIALOG		2		/* Niet modale dialoogbox. */


/*
 * Some bit flags for xw_xflags
 */

#define XWF_ICN 1	/* is iconified */
#define XWF_SIM 2	/* is a simulated window */
#define XWF_OPN 4	/* window has been opened */

/*
 * Declaratie van de voor de gebruiker beschikbare funkties.
 */

WINDOW *xw_create(_WORD type, WD_FUNC *functions, _WORD flags,
						 RECT *msize, size_t wd_struct_size,
						 OBJECT *menu, int *error);
void xw_open(WINDOW *wd, RECT *size);
void xw_close(WINDOW *w);
void xw_delete(WINDOW *w);
void xw_closedelete(WINDOW *w);

void xw_set(WINDOW *w, _WORD field,...);
void xw_get(WINDOW *w, _WORD field, RECT *r);
void xw_setsize(WINDOW *w, RECT *size);
void xw_getsize(WINDOW *w, RECT *size);
void xw_getwork(WINDOW *w, RECT *size);
void xw_getfirst(WINDOW *w, RECT *size);
void xw_getnext(WINDOW *w, RECT *size);
void xw_calc(_WORD w_ctype, _WORD w_flags, RECT *input,
					RECT *output, OBJECT *menu);

void xw_nop1(WINDOW *w);
void xw_nop2(WINDOW *w, _WORD i);
WINDOW *xw_find(_WORD x, _WORD y);
WINDOW *xw_hfind(_WORD handle);
WINDOW *xw_top(void);
WINDOW *xw_bottom(void);
void xw_note_top(WINDOW *w);
void xw_note_bottom(WINDOW *w);
_WORD xw_exist(WINDOW *w);
WINDOW *xw_first(void);
WINDOW *xw_last(void);

#define xw_type(w)		((w)->xw_type)
#define xw_handle(w)	((w)->xw_handle)
#define xw_next(w) 		((w)->xw_next)
#define xw_prev(w) 		((w)->xw_prev)

extern _WORD xw_dosend;
extern WINDOW *xw_deskwin; 			/* pointer to desktop window */

void xw_cycle(void);
void xw_send(WINDOW *w, _WORD messid);
void xw_send_rect(WINDOW *w, _WORD messid, _WORD pid, const RECT *area);
void xw_send_redraw(WINDOW *w, _WORD messid, const RECT *area);
void xw_menu_icheck(WINDOW *w, _WORD item, _WORD check);
void xw_menu_ienable(WINDOW *w, _WORD item, _WORD enable);
void xw_menu_text(WINDOW *w, _WORD item, const char *text);
void xw_menu_tnormal(WINDOW *w, _WORD item, _WORD normal);
void xw_redraw_menu(WINDOW *w, _WORD object, RECT *r);

_WORD xw_hndlbutton(_WORD x, _WORD y, _WORD n, _WORD bstate, _WORD kstate);
_WORD xw_hndlkey(_WORD scancode, _WORD keystate);
_WORD xw_hndlmessage(_WORD *message);

WINDOW *xw_open_desk(_WORD type, WD_FUNC *functions, size_t wd_struct_size, _WORD *error);
void xw_close_desk(void);
void xw_iconify(WINDOW *w, RECT *r); 
void xw_uniconify(WINDOW *w, RECT *r); 


#endif
