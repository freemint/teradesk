/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                                     2003  Dj. Vukovic
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
extern int n_icons;
extern OBJECT *desktop;
extern WINDOW *desk_window;
extern INAME iname;

boolean dsk_init(void);
int dsk_load(XFILE *file);
void dsk_default(void);
void dsk_close(void);

void dsk_insticon(void);
void dsk_remicon(void);
void dsk_chngicon(void);

void dsk_draw(void);

void dsk_options(void);

boolean load_icons(void);
void free_icons(void);

void remove_icon(int object, boolean draw);
void set_dsk_background(int pattern, int color);

int rsrc_icon(const char *name);
boolean isfile(ITMTYPE type);

int rsrc_icon_rscid(int id, char *name );
void set_iselector(SLIDER *slider, boolean draw, XDINFO *info);
void regen_desktop(OBJECT *desk_tree);
void draw_icrects( WINDOW *w, OBJECT *tree, RECT *r1);

