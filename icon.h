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

extern OBJECT 
	*icons,
	*desktop;

extern WINDOW 
	*desk_window;

extern int 
	iconw,	/* icon sell size */
	iconh,	/* icon cell size */
	n_icons;
 
extern INAME
	iname;

extern bool 
	noicons;

extern XDINFO
	icd_info;


void init_obj(OBJECT *obj, int otype);
bool dsk_init(void);
int dsk_load(XFILE *file);
void dsk_default(void);
void dsk_close(void);
void dsk_insticon(WINDOW *w, int n, int *list);
void dsk_chngicon(int n, int *list, bool dialog);
void dsk_draw(void);
void redraw_desk(RECT *r);
void regen_desktop(OBJECT *desk_tree);
void dsk_options(void);
void set_dsk_background(int pattern, int colour);
bool load_icons(void);
void free_icons(void);
void remove_icon(int object, bool draw);
int limcolour(int col);
int limpattern(int pat);
int dsk_defaultpatt(void);
void set_selcolpat(XDINFO *info, int obj, int col, int pat);
int rsrc_icon(const char *name);
bool isfile(ITMTYPE type);
void hideskip(int n, OBJECT *obj);
int trash_or_print(ITMTYPE type);
int icn_iconid(const char *name);
int rsrc_icon_rscid(int id, char *name );
int default_icon(ITMTYPE type);
void set_iselector(SLIDER *slider, bool draw, XDINFO *info);
void icn_sl_init(int line, SLIDER *sl);
int icn_dialog(SLIDER *sl_info, int *icon_no, int startobj, int bckpat, int bckcol);
void draw_icrects( WINDOW *w, OBJECT *tree, RECT *r1);
void start_rubberbox(void);
void rubber_rect(int x1, int x2, int y1, int y2, RECT *r);
void icn_coords(ICND *icnd, RECT *tr, RECT *ir);
void icn_fix_ictype(void);
void changestate(int mode, bool *newstate, int i, int selected, bool iselected, bool iselected4);

