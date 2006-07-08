/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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



#define MARGIN			2        
#define HEXLEN			75 + MARGIN 
#define SUBST_DISP		127 /* DEL char, usually represented by a triangle */


/*
 * For structure compatibility reasons XW_INTVARS was substituted
 * by ITM_INTVARS (larger); it's a tradeoff for reusing a number of
 * routines
 * Note: take care of compatibility between 
 * TXT_WINDOW, DIR_WINDOW, TYP_WINDOW
 */


typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
	WD_VARS;					/* other common header data */

	/* three window-type structures are identical up to this point */

	char *buffer;				/* buffer met de tekst */
	char **lines;				/* lijst met pointers naar het begin van alle tekstregels */
	long size;					/* aantal bytes in de tekst */
	long tlines;				/* aantal regels in de tekst */
	int tabsize;				/* tab size */
	int *ntabs;					/* tabs per lines */
	int twidth;					/* text width incl. tab substitutes */
	int hexmode;				/* Hexmode flag. */

} TXT_WINDOW;

CfgNest text_one;

boolean txt_add_window(WINDOW *sw, int item, int kstate, char *thefile);
void txt_closed(WINDOW *w);
void txt_hndlmenu(WINDOW *w, int title, int item);

void txt_prtline(TXT_WINDOW *w, long line, RECT *area, RECT *work);
void txt_prtlines(TXT_WINDOW *w, RECT *area, RECT *work);
void txt_prtcolumn(TXT_WINDOW *w, int column, int nc, RECT *area, RECT *work);
int txt_read(TXT_WINDOW *w, boolean setmode);
boolean txt_reread( TXT_WINDOW *w, char *name, int px, long py);

int read_txtfile(const char *name, char **buffer, long *flength, long *tlines, char ***lines, int **ntabs); 
int read_txtf(const char *name, char **buffer, long *flength); 
void compare_files( WINDOW *w, int n, int *list );
void disp_hex( char *tmp, char *p, long a, long size, boolean toprint );

