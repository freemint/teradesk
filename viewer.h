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
	_WORD tabsize;				/* tab size */
	_WORD twidth;					/* text width incl. tab substitutes */
	_WORD hexmode;				/* Hexmode flag. */

} TXT_WINDOW;


extern XDFONT txt_font;
extern WINFO textwindows[MAXWINDOWS];	/* some information about open windows */
extern GRECT tmax;

bool txt_add_window(WINDOW *sw, _WORD item, _WORD kstate, char *thefile);
void txt_closed(WINDOW *w);
void txt_hndlmenu(WINDOW *w, _WORD title, _WORD item);
void txt_prtline(TXT_WINDOW *w, long line, GRECT *area, GRECT *work);
void txt_prtcolumn(TXT_WINDOW *w, _WORD column, _WORD nc, GRECT *area, GRECT *work);
bool txt_reread( TXT_WINDOW *w, char *name, _WORD px, long py);
_WORD read_txtf(const char *name, char **buffer, long *flength); 
void compare_files( WINDOW *w, _WORD n, _WORD *list );
void disp_hex( char *tmp, char *p, long a, long size, bool toprint );
void copy_unnull(char *dest, const char *source, long length, long pos, _WORD dl);
void view_config(XFILE *file, int lvl, int io, int *error);
void text_one(XFILE *file, int lvl, int io, int *error);
