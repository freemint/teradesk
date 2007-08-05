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


void screen_size(void);
boolean clip_desk(RECT *r);
void clipdesk_on(void);
void pclear(RECT *r);
void invert(RECT *r);
boolean rc_intersect2(RECT *r1, RECT *r2);
void move_screen(RECT *dest, RECT *src);
void set_txt_default(XDFONT *f);
int *get_colours(void);
void set_colours(int *colours);
void handle_colours(int io);
void set_rect_default(void);
void draw_rect(int x1, int y1, int x2, int y2);
