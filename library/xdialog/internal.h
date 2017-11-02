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

#ifndef __XD_INTERNAL_H__
#define __XD_INTERNAL_H__

/* Vlaggen voor xd_redraw() */

#define XD_RDIALOG	0x1
#define XD_RCURSOR	0x2

/* 
 * Maximum size of scrolled editable text should be a little less
 * than the size of VLNAME
 */

#define XD_MAX_SCRLED sizeof(VLNAME) - 1

/* Vlaggen voor xd_form_button() */

#define FMD_FORWARD		0
#define FMD_BACKWARD	1
#define FMD_DEFLT		2

#define MAXKEYS		48

/* Macro's for 3D-buttons. */

#define AES3D_1			0x200
#define AES3D_2			0x400

#define GET_3D(flags)	(flags & (AES3D_1 | AES3D_2))
#define IS_ACT(flags)	(GET_3D(flags) == (AES3D_1 | AES3D_2))
#define IS_IND(flags)	(GET_3D(flags) == AES3D_1)
#define IS_BG(flags)	(GET_3D(flags) == AES3D_2)

/* Defines for internal fill style. */

#ifndef __PUREC__
#define FIS_HOLLOW		0
#define FIS_SOLID		1
#define FIS_PATTERN		2
#define FIS_HATCH		3
#define FIS_USER		4
#endif

/* Macro to check if an extended user-defined structure is used. */

#define IS_XUSER(userblk)	(userblk->ub_parm == (long)userblk)

/* Extended user-defined structure. */

typedef struct xuserblk
{
	PARMBLKFUNC ub_code;
	struct xuserblk *ub_parm;		/* Pointer to itself */
	_WORD ob_type;					/* Original object type */
	_WORD ob_flags;					/* Original object flags */
	OBSPEC ob_spec;					/* Original object specifier */
	union
	{ 
		void *ptr;					/* a handy pointer to anything */
		_WORD ob_shift;				/* For scrledit: position of letterbox. */
		struct
		{
			_WORD pattern;			/* background fill pattern */
			_WORD colour;				/* background fill colour */
		} fill;
	}uv;
} XUSERBLK;

typedef struct xdobjdata
{
	struct xdobjdata *next;
} XDOBJDATA;

typedef struct
{
	_WORD key;
	_WORD object;
} KINFO;

typedef struct
{
	XW_INTVARS;
	XDINFO *xd_info;
	_WORD nkeys;
	KINFO kinfo[MAXKEYS];
} XD_NMWINDOW;

typedef struct
{
	_WORD id;

/* currently unused
	_WORD type;
*/
	_WORD size;
	_WORD colour;
	_WORD effects;
	_WORD cw;
	_WORD ch;
} XDFONT;

extern _WORD xd_vhandle;
extern _WORD xd_nplanes;
extern _WORD xd_ncolours;
extern _WORD xd_fnt_w;
extern _WORD xd_fnt_h;
extern _WORD xd_pix_height;
extern _WORD xd_posmode;
extern _WORD xd_min_timer;
extern _WORD aes_flags;
extern _WORD xd_fdo_flag;

extern const _WORD ckeytab[];

extern _WORD xd_has3d;
extern _WORD xd_bg_col;
extern _WORD xd_ind_col;
extern _WORD xd_act_col;
extern _WORD xd_sel_col;

extern _WORD brd_l;
extern _WORD brd_r;
extern _WORD brd_u;
extern _WORD brd_d; /* object border sizes */

extern const char *xd_prgname;

extern void *(*xd_malloc) (size_t size);
extern void (*xd_free) (void *block);
extern XDOBJDATA *xd_objdata;
extern XDINFO *xd_dialogs;		/* Lijst met modale dialoogboxen. */
extern XDINFO *xd_nmdialogs;	/* Lijst met niet modale dialoogboxen. */
extern OBJECT *xd_menu;
extern GRECT xd_screen;
extern GRECT xd_desk;
extern XDFONT xd_regular_font;
extern XDFONT xd_small_font;
extern _WORD xe_mbshift;
extern _WORD xd_vhandle;

_WORD __xd_hndlkey(WINDOW *w, _WORD key, _WORD kstate);
void __xd_hndlbutton(WINDOW *w, _WORD x, _WORD y, _WORD n, _WORD bstate, _WORD kstate);
void __xd_hndlmenu(WINDOW *w, _WORD title, _WORD item);

void __xd_topped(WINDOW *w);
void __xd_closed(WINDOW *w, _WORD mode);
void __xd_top(WINDOW *w);

void set_linedef(_WORD colour);
_WORD xd_movebutton(OBJECT *tree);
_WORD xd_abs_curx(OBJECT *tree, _WORD object, _WORD curx);
void xd_cursor_on(XDINFO *info);
void xd_cursor_off(XDINFO *info);
_WORD xd_hndlmessage(_WORD *message, _WORD flag);
_WORD xd_scan_messages(_WORD flag, _WORD *mes);
void xd_redraw(XDINFO *info, _WORD start, _WORD depth, GRECT *area, _WORD flags);
XDINFO *xd_find_dialog(WINDOW *w);
_WORD xd_form_button(XDINFO *info, _WORD object, _WORD clicks, _WORD *result);
_WORD xd_find_obj(OBJECT *tree, _WORD start, _WORD which);
void xd_edit_init(XDINFO *info, _WORD object, _WORD curx);
void xd_edit_end(XDINFO *info);
void xd_calcpos(XDINFO *info, XDINFO *prev, _WORD pmode);
_WORD xd_edit_char(XDINFO *info, _WORD key);
_WORD xd_form_keybd(XDINFO *info, _WORD kobnext, _WORD kchar, _WORD *knxtobject, _WORD *knxtchar);
_WORD xd_find_key(OBJECT *tree, KINFO *kinfo, _WORD nk, _WORD key);
_WORD xd_set_keys(OBJECT *tree, KINFO *kinfo);
void xw_closeall(void);
void xd_xuserdef(OBJECT *object, XUSERBLK *userblk, PARMBLKFUNC code);
_WORD xd_oldarrow(XDINFO *info);

#endif
