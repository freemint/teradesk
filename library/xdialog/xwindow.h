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
					int xw_handle;	/* window handle */		\
					int xw_ap_id;	/* app id of owner */	\
					int xw_bar;		/* menubar object ind */\
					int xw_mparent;	/* parent window id */	\
					int xw_type;	/* window type */		\
					int xw_flags;	/* flags for widgets */	\
					int xw_xflags	/* other diverse flags */

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
	int (*wd_hndlkey) (WINDOW *w, int scancode, int keystate);
	void (*wd_hndlbutton) (WINDOW *w, int x, int y, int n, int bstate, int kstate);
	void (*wd_redraw) 	(WINDOW *w, RECT *area);
	void (*wd_topped) 	(WINDOW *w);
	void (*wd_bottomed)	(WINDOW *w);
	void (*wd_newtop) 	(WINDOW *w);
	void (*wd_closed) 	(WINDOW *w, int mode);
	void (*wd_fulled) 	(WINDOW *w, int mbshift);
	void (*wd_arrowed) 	(WINDOW *w, int arrows);
	void (*wd_hslider) 	(WINDOW *w, int newpos);
	void (*wd_vslider) 	(WINDOW *w, int newpos);
	void (*wd_sized) 	(WINDOW *w, RECT *newsize);
	void (*wd_moved) 	(WINDOW *w, RECT *newpos);
	void (*wd_hndlmenu) (WINDOW *w, int title, int item);
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

extern WINDOW *xw_create(int type, WD_FUNC *functions, int flags,
						 RECT *msize, size_t wd_struct_size,
						 OBJECT *menu, int *error);
extern void xw_open(WINDOW *wd, RECT *size);
extern void xw_close(WINDOW *w);
extern void xw_delete(WINDOW *w);
extern void xw_closedelete(WINDOW *w);

extern void xw_set(WINDOW *w, int field,...);
extern void xw_get(WINDOW *w, int field, RECT *r);
extern void xw_setsize(WINDOW *w, RECT *size);
extern void xw_getsize(WINDOW *w, RECT *size);
extern void xw_getwork(WINDOW *w, RECT *size);
extern void xw_getfirst(WINDOW *w, RECT *size);
extern void xw_getnext(WINDOW *w, RECT *size);
extern void xw_calc(int w_ctype, int w_flags, RECT *input,
					RECT *output, OBJECT *menu);

extern void xw_nop1(WINDOW *w);
extern void xw_nop2(WINDOW *w, int i);
extern WINDOW *xw_find(int x, int y);
extern WINDOW *xw_hfind(int handle);
extern WINDOW *xw_top(void);
extern WINDOW *xw_bottom(void);
extern void xw_note_top(WINDOW *w);
extern void xw_note_bottom(WINDOW *w);
extern int xw_exist(WINDOW *w);
extern WINDOW *xw_first(void);
WINDOW *xw_last(void);

#if __USE_MACROS
#define xw_type(w)		((w)->xw_type)
#define xw_handle(w)	((w)->xw_handle)
#define xw_next(w) 		((w)->xw_next)
#define xw_prev(w) 		((w)->xw_prev)
#else
extern int xw_type(WINDOW *w);
extern int xw_handle(WINDOW *w);
extern WINDOW *xw_next(WINDOW *w);
extern WINDOW *xw_prev(WINDOW *w);
#endif

extern int xw_dosend;

extern void xw_cycle(void);
extern void xw_send(WINDOW *w, int messid);
extern void xw_send_rect(WINDOW *w, int messid, int pid, RECT *area);
extern void xw_send_redraw(WINDOW *w, int messid, RECT *area);
extern void xw_menu_icheck(WINDOW *w, int item, int check);
extern void xw_menu_ienable(WINDOW *w, int item, int enable);
extern void xw_menu_text(WINDOW *w, int item, const char *text);
extern void xw_menu_tnormal(WINDOW *w, int item, int normal);
extern void xw_redraw_menu(WINDOW *w, int object, RECT *r);

extern int xw_hndlbutton(int x, int y, int n, int bstate, int kstate);
extern int xw_hndlkey(int scancode, int keystate);
extern int xw_hndlmessage(int *message);

extern WINDOW *xw_open_desk(int type, WD_FUNC *functions,
							size_t wd_struct_size, int *error);
extern void xw_close_desk(void);
extern void xw_iconify(WINDOW *w, RECT *r); 
extern void xw_uniconify(WINDOW *w, RECT *r); 


#endif
