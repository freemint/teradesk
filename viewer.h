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


/*
 * For structure compatibility reasons XW_INTVARS was substituted
 * by ITM_INTVARS (larger); it's a tradeoff for reusing a number of
 * routines
 */

/* Note: take care of compatibility between TXT_WINDOW, DIR_WINDOW, TYP_WINDOW */

typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
	WD_VARS;					/* other common header data */

	struct winfo *winfo;		/* pointer naar WINFO structuur. */

	/* three window-type structures are identical up to this point */

	int tabsize;
	const char *name;
	char *buffer;				/* buffer met de tekst */
	long size;					/* aantal bytes in de tekst */
	long tlines;				/* aantal regels in de tekst */
	char **lines;				/* lijst met pointers naar het begin van alle tekstregels */
	unsigned int hexmode : 1;	/* Hexmode flag. */

} TXT_WINDOW;


#define MARGIN			2        
#define HEXLEN			74 + MARGIN 

void txt_init(void);
void txt_default(void);


int text_save(XFILE *file, WINDOW *w, int lvl);

boolean txt_add_window(WINDOW *sw, int item, int kstate, char *thefile);
void txt_closed(WINDOW *w);

int txt_width(TXT_WINDOW *w); 
void txt_draw_all(void);
void txt_prtline(TXT_WINDOW *w, long line, RECT *area, RECT *work);
void txt_prtlines(TXT_WINDOW *w, RECT *area);
void txt_prtcolumn(TXT_WINDOW *w, int column, RECT *area, RECT *work);
void txt_title(TXT_WINDOW *w);

int read_txtfile(const char *name, char **buffer, long *flength, long *tlines, char ***lines); 
void compare_files( WINDOW *w, int n, int *list );

