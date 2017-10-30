/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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


#define ICON_W		80
#define ICON_H		46

extern OBJECT *icons;
extern OBJECT *desktop;

extern WINDOW *desk_window;

extern _WORD iconw;	/* icon sell size */
extern _WORD iconh;	/* icon cell size */
extern _WORD n_icons;

extern INAME iname;

extern bool noicons;

extern XDINFO icd_info;
extern _WORD sl_noop;

void init_obj(OBJECT *obj, _WORD otype);
bool dsk_init(void);
_WORD dsk_load(XFILE *file);
void dsk_default(void);
void dsk_close(void);
void dsk_insticon(WINDOW *w, _WORD n, _WORD *list);
void dsk_chngicon(_WORD n, _WORD *list, bool dialog);
void dsk_draw(void);
void redraw_desk(GRECT *r);
void regen_desktop(OBJECT *desk_tree);
void dsk_options(void);
void set_dsk_background(_WORD pattern, _WORD colour);
bool load_icons(void);
void free_icons(void);
void remove_icon(_WORD object, bool draw);
_WORD limcolour(_WORD col);
_WORD limpattern(_WORD pat);
void set_selcolpat(XDINFO *info, _WORD obj, _WORD col, _WORD pat);
_WORD rsrc_icon(const char *name);
bool isfile(ITMTYPE type);
void hideskip(_WORD n, OBJECT *obj);
_WORD trash_or_print(ITMTYPE type);
_WORD icn_iconid(const char *name);
_WORD rsrc_icon_rscid(_WORD id, char *name );
_WORD default_icon(ITMTYPE type);
void set_iselector(SLIDER *slider, bool draw, XDINFO *info);
void icn_sl_init(_WORD line, SLIDER *sl);
_WORD icn_dialog(SLIDER *sl_info, _WORD *icon_no, _WORD startobj, _WORD bckpat, _WORD bckcol);
void draw_icrects( WINDOW *w, OBJECT *tree, GRECT *r1);
void start_rubberbox(void);
void rubber_rect(_WORD x1, _WORD x2, _WORD y1, _WORD y2, GRECT *r);
void icn_coords(ICND *icnd, GRECT *tr, GRECT *ir);
void icn_fix_ictype(void);
void changestate(_WORD mode, bool *newstate, _WORD i, _WORD selected, bool iselected, bool iselected4);
void dsk_config(XFILE *file, int lvl, int io, int *error);
