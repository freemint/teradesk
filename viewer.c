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

#include <np_aes.h>	
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>
#include <xscncode.h>
#include <library.h>

#include "desk.h"
#include "error.h"
#include "font.h"
#include "resource.h"
#include "xfilesys.h"
#include "config.h"
#include "screen.h"
#include "window.h" /* moved before viewer.h */
#include "viewer.h"
#include "file.h"


#define FWIDTH			256 /* max.possible width of text window in text mode */


#define ASCII_PERC		80


WINFO textwindows[MAXWINDOWS]; 
RECT tmax;
FONT txt_font;

static void set_menu(TXT_WINDOW *w);

static void txt_closed(WINDOW *w);
static void txt_hndlmenu(WINDOW *w, int title, int item);


static WD_FUNC txt_functions =
{
	wd_type_hndlkey,
	wd_type_hndlbutton,
	wd_type_redraw,
	wd_type_topped,
	wd_type_topped,	
	txt_closed,	/* the only one remaining specific for text window */
	wd_type_fulled, 
	wd_type_arrowed,
	wd_type_hslider,
	wd_type_vslider,
	wd_type_sized,
	wd_type_moved,
	txt_hndlmenu,
	/* wd_type_top, */ 0L,
	wd_type_iconify,
	wd_type_uniconify
};


/********************************************************************
 *																	*
 * Hulpfuncties.													*
 *																	*
 ********************************************************************/

/*
 * Determine width of a text window; depending on whether the display is
 * in hex mode or text mode, it will be either a fixed value or a value
 * determined from the longest line in the text. Line lengths are calculated
 * from differences between pointers on line beginnings
 * (returned width will never be more than FWIDTH).
 */

int txt_width(TXT_WINDOW *w)
{
	long 
		i,				/* line counter */
		mw;				/* maximum line width */


	/* 
	 * If display is in hex mode, return a fixed width (HEXLEN); otherwise
	 * scan pointers to all lines and find greatest difference between
	 * two adjecent ones- that would be the maximum line length
	 */

	if ( w->hexmode == 1)
		return HEXLEN + 1;	/* fixed window width in hex mode */
	else
	{
		/* Length of last (and first, if there is only one) line in the text */

		mw = (long)(w->lines[0]) + w->size - (long)(w->lines[w->tlines - 1]);

		/* Is length of any other line greater than the above? */

		for ( i = 1; i < w->tlines; i++ ) 
			mw = lmax(mw, (long)(w->lines[i]) - (long)(w->lines[i - 1]));

		/* Add the right margin */

		mw += MARGIN;

		/* 
		 * Window width should be at least (arbitrary) 16 characters 
		 * but not more than FWIDTH characters 
		 */

		mw = lmax(16L, lmin(mw, FWIDTH));
		return (int)mw;
	}
}


/*
 * Redraw all text windows 
 */

void txt_draw_all(void)
{
	WINDOW *w = xw_first();

	while (w)
	{
		if (xw_type(w) == TEXT_WIND)
			wd_type_draw( (TYP_WINDOW *)w, TRUE );
		w = w->xw_next;
	}
}


/********************************************************************
 *																	*
 * Functies voor het tekenen van de inhoud van een tekstwindow.		*
 *																	*
 ********************************************************************/

/*
 * Functie voor het converteren van een hexadecimale digit in
 * een ASCII karakter.
 *
 * Parameters:
 *
 * x		- hexadecimaal getal
 *
 * Resultaat: ASCII karakter
 */

static char hexdigit(int x)
{
	return (x <= 9) ? x + '0' : x + 'A' - 10;
}


/*
 * Functie voor het uitlezen van een regel uit de buffer van een
 * window. Afhankelijk van de mode is de presentatie in ASCII of
 * hexadecimaal.
 *
 * Parameters:
 *
 * w		- Pointer naar WINFO structuur 
 * dest		- Buffer waarin de regel geplaatst moet worden
 * line		- Nummer van de regel
 *
 * Resultaat: Lengte van de regel
 */

static int txt_line
(
	TXT_WINDOW *w,	/* pointer to window being processed */ 
	char *dest,		/* pointer to destination (output) string */ 
	long line		/* line index */
)
{
	char 
		*s,			/* pointer to the source string- the real line */ 
		*d = dest;	/* pointer to a location in destination string */

	int 
		i,						/* counter */ 
		wpxm = w->px - MARGIN,
		m = w->columns,			/* rightmost column in the text */
		cnt = 0,				/* input character count */
		cntm = 0;				/* left margin visible character count */ 


	/* Don't process invisible lines */

	if (line >= w->nlines)
	{
		*dest = 0;
		return 0;
	}

	/* Left margin in text window, if visible */

	for ( i = w->px; i < MARGIN; i++ )
	{
		*d++ = ' ';	
		cntm++;
	}

	m -= cntm;

	if (w->hexmode == 0)
	{
		/* Create a line in text mode */

		char ch;
		int tabsize = w->tabsize;

		m += w->px; /* rightmost column in text */ 

		/* If this line does not exist at all, return NULL */

		if ((s = w->lines[line]) == NULL)
		{
			*dest = 0;
			return 0;
		}

		/* The real line */

		while (cnt < m)
		{
			if ((ch = *s++) == 0)
				continue;
			if (ch == '\n')	/* linefeed */
				break;
			if (ch == '\r')	/* carriage return */
				continue;
			if (ch == '\t')	/* tab (substitute) */
			{
				do
				{
					if (cnt >= wpxm )
						*d++ = ' ';
					cnt++;
				}
				while ((( cnt % tabsize) != 0) && (cnt < m));
				continue;
			}
			if ( cnt  >= wpxm )
				*d++ = ch;

			cnt++;
		}
	}
	else
	{
		/* Create a line in hex mode */

		long 
			a = 16L * line, 
			h;

		int 
			j;

		char 
			tmp[HEXLEN + 2], 
			*p = &w->buffer[a];

		if (w->px >= HEXLEN)
		{
			*dest = 0;
			return 0;
		}

		h = a;

		/* tmp[0] to tmp[6] display line offset from  file beginning */

		for (i = 5; i >= 0; i--)
		{
			tmp[i] = hexdigit((int) h & 0x0F);
			h >>= 4;
		}
		tmp[6] = ':';

		/* Hexadecimal (in h) and ASCII (in 57+i) representation of 16 bytes */

		for (i = 0; i < 16; i++)
		{
			j = 7 + i * 3;
			tmp[j] = ' ';
			if ((a + (long)i) < w->size)
			{
				tmp[j + 1] = hexdigit((p[i] >> 4) & 0x0F);
				tmp[j + 2] = hexdigit(p[i] & 0x0F);
				tmp[57 + i] = (p[i] == 0) ? '\020' : p[i];
			}
			else
			{
				tmp[j + 1] = ' ';
				tmp[j + 2] = ' ';
				tmp[57 + i] = ' ';
			}
		}

		tmp[55] = ' ';
		tmp[56] = ' ';
		tmp[73] = 0;

		s = &tmp[wpxm + cntm]; /* same as &tmp[w->px - (MARGIN-cntm)] */

		while (cnt < m)
		{
			if ((*d++ = *s++) == 0)
			{
				d--;
				break;
			}
			cnt++;
		}
	}

	*d = 0;
	return (int) (d - dest);
}


/*
 * Bereken de rechthoek die de te tekenen tekst omsluit.
 *
 * Parameters:
 *
 * tw		- Pointer naar window
 * line		- Regelnummer
 * strlen	- lengte van de string
 * work		- RECT structuur met de grootte van het werkgebied van het
 *			  window.
 */

static void txt_comparea(TXT_WINDOW *w, long line, int strlen, RECT *r, RECT *work)
{
	r->x = work->x;
	r->y = work->y + (int) (line - w->py) * txt_font.ch;
	r->w = strlen * txt_font.cw;
	r->h = txt_font.ch;
}


/*
 * Functie voor het afdrukken van 1 karakter van een regel. Deze
 * functie wordt bij het naar links en rechts scrollen van een
 * window gebruikt.
 *
 * Parameters:
 *
 * w		- Pointer naar window
 * column	- Kolom waarin het karakter wordt afgedrukt
 * line		- Regel waarin het karakter wordt afgedrukt
 * area		- Clipping rechthoek
 * work		- Werkgebied van het window
 */

static void txt_prtchar(TXT_WINDOW *w, int column, long line, RECT *area, RECT *work)
{
	RECT r, in;
	int len, c;
	char s[FWIDTH];

	c = column - w->px;

	len = txt_line(w, s, line);

	txt_comparea(w, line, len, &r, work);
	r.x += c * txt_font.cw;
	r.w = txt_font.cw;

	if (xd_rcintersect(area, &r, &in) == TRUE)
	{
		if (c < len)
		{
			s[c + 1] = 0;
			pclear ( & r );
			vswr_mode( vdi_handle, MD_TRANS );
			v_gtext(vdi_handle, r.x, r.y, &s[c]);
		}
		else
			pclear(&in); 
	}
}


/*
 * Functie voor het afdrukken van een regel.
 *
 * Parameters:
 *
 * w		- Pointer naar window
 * line		- Regel die wordt afgedrukt
 * area		- Clipping rechthoek
 * work		- Werkgebied van het window
 */

void txt_prtline(TXT_WINDOW *w, long line, RECT *area, RECT *work)
{
	RECT r, in;
	int len;
	char s[FWIDTH];

	len = txt_line(w, s, line);
	txt_comparea(w, line, len, &r, work);
	if (rc_intersect2(area, &r) == TRUE)
	{
		pclear(&r);
		vswr_mode( vdi_handle, MD_TRANS );
		v_gtext(vdi_handle, r.x, r.y, s);
	}
	r.x += r.w;
	r.w = work->w - r.w;
	if (xd_rcintersect(&r, area, &in) == TRUE)
		pclear(&in); 
}


/*
 * Functie voor het afdrukken van alle regels die zichtbaar zijn in
 * een window.
 *
 * Parameters:
 *
 * w		- Pointer naar window
 * area		- Clipping rechthoek
 */

void txt_prtlines(TXT_WINDOW *w, RECT *area)
{
	long i;
	RECT work;

	set_txt_default(txt_font.id, txt_font.size);

	xw_get((WINDOW *) w, WF_WORKXYWH, &work);

	for (i = 0; i < w->rows; i++)
		txt_prtline(w, w->py + i, area, &work);
}


/********************************************************************
 *																	*
 * Funkties voor scrollen van tekstwindows.							*
 *																	*
 ********************************************************************/

/*
 * Teken een kolom van een window opnieuw.
 *
 * Parameters:
 *
 * w		- pointer naar window
 * column	- kolom
 * area		- clipping rechthoek
 * work		- werkgebied window
 */

void txt_prtcolumn(TXT_WINDOW *w, int column, RECT *area, RECT *work)
{
	int i;

	for (i = 0; i < w->rows; i++)
		txt_prtchar(w, column, w->py + i, area, work);
}


/********************************************************************
 *																	*
 * Functies voor het afhandelen van selecties van een menupunt in	*
 * een window menu.													*
 *																	*
 ********************************************************************/

/*
 * Functie voor het updaten van het window menu van een tekstwindow.
 */

static void set_menu(TXT_WINDOW *w)
{
	xw_menu_icheck((WINDOW *) w, VMHEX, w->hexmode);
}


/*
 * Functie voor het instellen van de ASCII of de HEXMODE van een
 * window.
 */

static void txt_mode(TXT_WINDOW *w)
{
	if (w->hexmode == 0)
	{
		/* Switch to hex mode */

		w->hexmode = 1;
		w->nlines = (w->size + 15L) / 16L;	/* 16 bytes per line */
	}
	else
	{
		/* Switch to text mode */

		w->hexmode = 0;
		w->nlines = w->tlines;
	}

	set_sliders((TYP_WINDOW *)w);
	wd_type_draw ( (TYP_WINDOW *)w, TRUE );
	set_menu(w);
}


/*
 * Functie voor het instellen van de tabultorgrootte van een
 * tekstwindow.
 */

static void txt_tabsize(TXT_WINDOW *w)
{
	int oldtab;
	char *vtabsize = stabsize[VTABSIZE].ob_spec.tedinfo->te_ptext;

	oldtab = w->tabsize;

	itoa(w->tabsize, vtabsize, 10);

	if (xd_dialog(stabsize, VTABSIZE) == STOK)
	{
		if ((w->tabsize = atoi(vtabsize)) < 1)
			w->tabsize = 1;

		if (w->tabsize != oldtab)
			wd_type_draw((TYP_WINDOW *)w,TRUE); 
	}
}


/********************************************************************
 *																	*
 * Funktie voor het vrijgeven van al het geheugen gebruikt door een	*
 * tekstwindow.														*
 *																	*
 ********************************************************************/

static void txt_rem(TXT_WINDOW *w)
{
	/* in free() there is a test if address is NULL, no need to check here */

	if (w != NULL )
	{
		free(w->lines);	
		free(w->buffer);

		free(w->name);

		w->winfo->used = FALSE;
		w->winfo->typ_window = NULL; 
		w->winfo->flags.iconified = 0;
	}
}


/********************************************************************
 *																	*
 * 'Object' functies voor tekstwindows.								*
 *																	*
 ********************************************************************/


/*
 * Window closed handler voor tekstwindows.
 *
 * Parameters:
 *
 * w			- Pointer naar window
 */

void txt_closed(WINDOW *w)
{
	txt_rem((TXT_WINDOW *) w);
	xw_closedelete(w);
}


/*
 * Window menu handler voor tekstwindows.
 *
 * Parameters:
 *
 * w			- Pointer naar window
 * title		- Menutitel die geselekteerd is
 * item			- Menupunt dat geselekteerd is
 */

static void txt_hndlmenu(WINDOW *w, int title, int item)
{
	switch (item)
	{
	case VMTAB:
		txt_tabsize((TXT_WINDOW *) w);
		break;
	case VMHEX:
		txt_mode((TXT_WINDOW *) w);
		break;
	}

	xw_menu_tnormal(w, title, 1);
}

 
/********************************************************************
 *																	*
 * Funkties voor het laden van files in een window.					*
 *																	*
 ********************************************************************/

/* Funkties voor het tellen van de regels van een file */

static long count_lines(char *buffer, long length)
{
	char cr = '\n', *s = buffer;
	long cnt = length, n = 1;

	do
	{
		if (*s++ == cr)
			n++;
	}
	while (--cnt > 0);

	return n;
}


/* 
 * Funktie voor het maken van een lijst met pointers naar het begin
 * van elke regel in de file. 
 *
 * Line end is detected by "\n" character (linefeed)
 * so this will be ok for TOS/DOS and Unix files but not for Mac files
 * (there, lineend is carriage-return only) 
 */

static void set_lines(char *buffer, char **lines, long length)
{
	char cr = '\n', *s = buffer, **p = lines;
	long cnt = length;

	*p++ = s;
	do
	{
		if (*s++ == cr)
			*p++ = s;
	}
	while (--cnt > 0);
}


/*
 * Check if a character is printable
 * 
 * Acceptable range: ' ' to '~' (32 to 126), <tab>, <cr>, <lf> and 'ÿ'
 * Also, characters higher than 127 (c < 0) are assumed to be printable
 * for the sake of codepages for other languages
 */

static boolean isascii(char c)
{
	if (((c >= ' ') && (c <= '~')) || (c == '\t') || (c == '\r') ||
		(c == '\n') || (c == 'ÿ') || (c < 0) )
		return TRUE;
	else
		return FALSE;
}


#if _SHOWFIND
extern long 
	search_nsm,
	find_offset;
#endif

/*
 * Load a text file for the purpose of displaying in a window, 
 * or for the purpose of searching to find a string in the file
 * or for any other purpose where found convenient.
 * Function returns error code, if there is any.
 * If parameter "tlines" is NULL, it is assumed that the routine will
 * be used for string searching; then lines will not be counted and set.
 * Note: for safety reasons, the routine allocates one byte more than
 * needed.
 */

int read_txtfile
(
	const char *name,	/* name of file to read */
	char **buffer,		/* location of the buffer to receive text */
	long *flength,		/* size of the file being read */
	long *tlines,		/* number of text lines in the file */
	char ***lines		/* pointers to beginnings of text lines */
)
{
	int 
		handle,		/* file handle */ 
		error;		/* error code  */

	long 
		read,		/* number of bytes read from the file */ 
		fl,			/* file length [bytes] */
		length;		/* length of space reserved for file  */

	XATTR 
		attr;		/* file attributes */


	/* Get file attributes (follow the link in x_attr)  */

	if ((error = (int)x_attr(0, name, &attr)) == 0)
	{
		*flength = attr.size;	/* output file length */
		fl = *flength;			/* for local use      */

		/* Open file */

		if ((handle = x_open(name, O_DENYW | O_RDONLY)) >= 0)
		{

			char *msg_endfile;

			/* 
			 * Get "end of file" string for text window display;
			 * this is not needed if reading is not for the purpose
			 * of a display.
			 */

			length = fl + 2; /* why was this needed? */

			if ( tlines != NULL )
			{
				rsrc_gaddr(R_STRING, MENDFILE, &msg_endfile);
				length = length + strlen(msg_endfile);
			}

			/* Allocate buffer for complete file; read file into it  */
		
			if ((*buffer = malloc(length + 1L)) == NULL)		
				error = ENSMEM;
			else
			{
				read = x_read(handle, fl, *buffer);

				/* If completely read with success... */

				if (read == fl)
				{				
					/* And this has been read for display in a window */

					if ( tlines != NULL )
					{
						/* Append "End of file" string to end of data read */

						strcpy( *buffer + fl, msg_endfile);

						/* Count text lines in the file (in fact, in the buffer) */

						fl = fl + 2;

						*tlines = count_lines(*buffer, fl);

						/* 
						 * Allocate memory for pointers to beginnings of lines,
						 * then find those beginnings
						 */

						if ((*lines = malloc(*tlines * sizeof(char *))) != NULL)
							set_lines(*buffer, *lines, fl);
						else
						{
							error = ENSMEM;
							free(*buffer);
						}
					}
				}
				else
				{
					error = (read < 0) ? (int) read : EREAD;
					free(*buffer);
				}
			}
			x_close(handle);
		}
		else
			error = handle;
	}
	return error;
}


/*
 * Set title to a text window 
 * See also dir_title() in dir.c
 */

void txt_title(TXT_WINDOW *w)
{
	int columns;

	/* 
	 * How long can the title be? 
	 * Note: "-5" takes into account window gadgets left/right of the title */

	columns = min( w->scolumns - 5, (int)sizeof(w->title) );

	/* Cramp it to fit window */

	cramped_name(w->name, w->title, columns);

	/* Set window */

	xw_set((WINDOW *) w, WF_NAME, w->title);
}


/* 
 * Funktie voor het laden van een file in het geheugen.
 * Bij een leesfout wordt een waarde ongelijk 0 terug gegeven.
 * Alle buffers zijn dan vrijgegeven. 
 */

static int txt_read(TXT_WINDOW *w, boolean setmode)
{
	int error;

	graf_mouse(HOURGLASS, NULL);

	/* Read complete file */

	error = read_txtfile(w->name, &(w->buffer), &(w->size), &(w->tlines), &(w->lines) );

	graf_mouse(ARROW, NULL);

	if (error != 0)
	{
		w->lines = NULL;
		w->buffer = NULL;
	}
	else
	{
		if (setmode == TRUE)
		{
			char *b;
			int i, e, n = 0;

			b = w->buffer;
			e = (int) lmin(w->size, 256L); /* longest possible line */

			for (i = 0; i < e; i++)
			{
				if (isascii(b[i]) != FALSE)
					n++;
			}

			n = (n > 0) ? (n * 100) / e : 100;

			w->hexmode = (n > ASCII_PERC) ? 0 : 1;

			set_menu(w);
		}

		/* Calculate number of lines for display */

		if (w->hexmode == 0)
			w->nlines = w->tlines;
		else
			w->nlines = (w->size + 15) / 16;

		if ((w->py + w->nrows) > w->nlines)
			w->py = lmax(w->nlines - (long)w->nrows, 0L);


		/* Set display title */

		txt_title( w );

		set_sliders ((TYP_WINDOW *)w); 
	}

	return error;
}


/* 
 * Compare contents of two buffers; return index of first difference.
 * If buffers are identical, return buffer length
 */

long  compare_buf( char *buf1, char*buf2, long len )
{
	long i;

	for ( i = 0; i < len; i++ )
		if ( buf1[i] != buf2[i] )
			return i;

	return len;
}


/*
 * Copy a number of bytes from source to destination, starting from
 * index "pos" substituting all nulls with something else; Not more than DISPLEN - 2
 * bytes will be copied, or less if buffer end is encountered first.
 * Add a 0 char at the end.
 * Note 1: "pos" should never be larger than "length" !!!! no checking done !
 * ("length" is length from the beginning of source, not counted from "pos").
 * Note 2: there is a similar substitution of zeros in showinfo.c;
 * take care to use the same substitute (maybe define a macro)
 * Note 3: It turned out that characters "[", "]" and "|" must be substituted too,
 * otherwise they will cause unwanted formatting of alertbox text.
 */

#define DISPLEN 20

static void copy_unnull( char *dest,  char *source, long length, long pos )
{
	long i, n;

	n = lmin( length - pos, DISPLEN - 1 );

	for ( i = 0; i < n; i++ )
	{
		dest[i] = source[i + pos];

		/* Some characters can't be printed */

		if ( (dest[i] == 0) || (dest[i] == '|') || (dest[i] == '[') || (dest[i] == ']') )
			dest[i] = 127; /* [DEL], usually represented by a triangle */		
	}
	dest[n] = 0;
}


/*
 * Compare two files. Display  an alert box with appropriate text.
 * Note: both files are completely read into memory 
 * (can be inconvenient for large files).
 * If only one file is selected, a file selector is activated
 * to select the second file.
 * 
 * This routine attemts to compare files with some intelligence: 
 * if a difference is found, the routine tries to resynchronize the
 * contents of the files after a difference. Synchronization window
 * is currently fixed to about 1/2 of the length of the strings which are
 * displayed to show a difference (i.e.  synchronization window length 
 * is about 9 characters). This should be sufficient to resynchronize
 * the contents of the files after, e.g. an additional empty line, or
 * an additional (not too long) word in one of the files.
 * In principle, sync window width might be made configurable
 * through use of "sw" and "swx" below.
 */

#define sw   DISPLEN / 2 	/* width of sync window */
#define swx2 DISPLEN - 1	/* (almost) twice the above */

void compare_files( WINDOW *w, int n, int *list )
{
	char 
		*name1 = NULL,			/* name of the 1st file */
		*name2 = NULL,			/* Name of the 2nd file */
		*buf1 = NULL,			/* buffer for file #1   */
		*buf2 = NULL,			/* buffer for file #2   */
		show1[DISPLEN],			/* to display file differences */
		show2[DISPLEN];			/* to display file differences */

	int
		button,					/* response in the alert box     */
		error;					/* error code from reading files */

	long
		size1,				/* size of file #1 */
		size2,				/* size of file #2 */
		i1, 				/* index in the first buffer  */
		i2, 				/* index in the second buffer */
		in, 				/* next start index for comparison */
		i1n,				/* new i1 */
		i2n,				/* new i2 */
		i2o,				/* previous i2 */
		nc, 				/* number of bytes to compare */
		ii;					/* counter of synchronization attempts */

	boolean
		sync = TRUE, 		/* false while attempting to resynchronize */
		diff = FALSE;		/* true if not equal files */

	char
		fname[20];			/* (part of) filename to display in the alert box */


	/* Obtain full names of two files; first the first one */

	name1 = itm_fullname( w, list[0] ); /* note: must free name1 later */
	
	if (name1)
	{
		/* Prepare a short name form for display purposes */

		cramped_name(itm_name(w, list[0]), fname, (int)sizeof(fname) );

		/* 
		 * If exactly two files are selected, take the name of the second one; 
		 * else, ask through the fileselector
		 */

		if ( n == 2 )
			name2 = itm_fullname( w, list[1] );
		else
		{
			name2 = locate( "*.*", L_FILE ); /* same for name2 */
			wd_drawall();
		}

		if ( name2 )
		{
			/* Don't do anything if second file is the same as the first one */

			if ( strcmp(name1, name2) == 0 )
				alert_iprint( MNOCOMP ); /* a file is identical to itself */
			else
			{
				/* OK; start comparison */ 

				graf_mouse(HOURGLASS, NULL);

				/* Read the files into buffers */

				error = read_txtfile( name1, &buf1, &size1, NULL, NULL );
				if ( error == 0 )
					error = read_txtfile( name2, &buf2, &size2, NULL, NULL );

				if ( error == 0 )
				{
					/* Current indices: at the beginning */

					i1 = 0; 
					i2 = 0;

					/* Scan until the ends of files */

					while ( (i1 < size1) && (i2 < size2) )
					{
						/* How many bytes to compare and not exceed the ends */

						nc = lmin( (size1 - i1), (size2 - i2) );

						/* How many bytes are equal ? Move to end of that */

						in = compare_buf(&buf1[i1], &buf2[i2], nc);

						i1n = i1 + in;
						i2n = i2 + in;

						/* If difference is found before file ends, report */

						if ( (in < nc) || ((in >=nc) && ( (i1n < size1) || (i2n < size2) ))  )
						{
							/* 
							 * If there is a difference, display an alert;
							 * Do so also if files have different lengths
							 * and the end of one is reached
							 */

							if ( sync || (in > swx2) )
							{
								graf_mouse(ARROW, NULL);
								sync = FALSE;	
								diff = TRUE;

								i1 = i1n;
								i2 = i2n;
								i2o = i2;

								/* Prepare something to display */

								copy_unnull( show1, buf1, size1, i1 );
								copy_unnull( show2, buf2, size2, i2 );

								button = alert_printf(1, ACOMPDIF, i1, fname, show1, show2 );	

								/* Stop further comparison ? */

								if ( button == 2 ) 
									break;

								graf_mouse(HOURGLASS, NULL);

								/* 
								 * Advance a little further and try to resync;
								 * it is reasonable that this advance stays
								 * inside the displayed area
								 */

								i1 += sw; /* currently half of displayed length */
								ii = 0;
							}
							else
							{
								ii++;
								if ( ii == swx2 )
								{
									sync = TRUE;
									i2 = i2o;
								}
							}
						}
						else
						{
							i1 = i1n;
							i2 = i2n;
							sync = TRUE;
						}

						if ( sync )
							i1++;
						i2++;
					}

					graf_mouse(ARROW, NULL);

					/* Report that files are identical */

					if ( !diff ) 
						alert_iprint(MCOMPOK);
				}
				else

					/* Something went wrong while reading the files */

					xform_error(error);
			}

			/* Free allocated blocks */

			free(name2);
			free(buf1);
			free(buf2);
		}
		free(name1);
	}

}



/********************************************************************
 *																	*
 * Funkties voor het openen van een tekst window.					*
 *																	*
 ********************************************************************/

/* 
 * Open een window. file moet een gemallocde string zijn. Deze mag
 * niet vrijgegeven worden door de aanroepende funktie. Bij een fout
 * wordt de string vrijgegeven.
 */

static WINDOW *txt_do_open(WINFO *info, const char *file, int px,
						   long py, int tabsize, boolean hexmode,
						   boolean setmode, int *error) 
{
	TXT_WINDOW *w;
	RECT size;
	int errcode;

	wd_in_screen( info );

	if ((w = (TXT_WINDOW *)xw_create(TEXT_WIND, &txt_functions, TFLAGS, &tmax,
									 sizeof(TXT_WINDOW), viewmenu, &errcode)) == NULL)
	{
		if (errcode == XDNMWINDOWS)
		{
			alert_iprint(MTMWIND);
			*error = ERROR;
		}
		else if (errcode == XDNSMEM)
			*error = ENSMEM;
		else
			*error = ERROR;

		free(file);

		return NULL;
	}

	info->typ_window = (TYP_WINDOW *)w; 
	w->px = px;
	w->py = py;
	w->name = file;
	w->buffer = NULL;
	w->lines = NULL;
	w->nlines = 0L;
	w->tabsize = tabsize;
	w->hexmode = hexmode;
	w->winfo = info;

	wd_calcsize(info, &size); 

	graf_mouse(HOURGLASS, NULL);
	*error = txt_read(w, setmode);
	graf_mouse(ARROW, NULL);

	if (*error != 0)
	{
		txt_rem(w);
		xw_delete((WINDOW *) w);		/* after txt_rem (MP) */
		return NULL;
	}
	else
	{
		wd_iopen((WINDOW *)w, &size); 
		info->used = TRUE;
		return (WINDOW *) w;
	}
}


/*
 * Add a text window specified by item selected in a window
 * or by a filename (if not NULL, filename has priority)
 */

boolean txt_add_window(WINDOW *w, int item, int kstate, char *thefile)
{
	int j = 0, error;
	const char *file;

	while ((j < MAXWINDOWS - 1) && (textwindows[j].used != FALSE))
		j++;

	if (textwindows[j].used == TRUE)
		return alert_iprint(MTMWIND), FALSE;

	if ( thefile != NULL )
		file = thefile;
	else if ((file = itm_fullname(w, item)) == NULL)
		return FALSE;

	if (txt_do_open(&textwindows[j], file, 0, 0, options.tabsize, FALSE, TRUE, &error) == NULL)
		return xform_error(error), FALSE;

#if _SHOWFIND	
	/* 
	 * If there was a search for a string, position sliders to show string 
	 */

	if ( search_nsm > 0 )
	{
		long
			ppy = 0;	/* line in which the string is   */

		TXT_WINDOW *tw = (TXT_WINDOW *)(textwindows[j].typ_window);
		search_nsm = 0; /* this search is finished; cancel the finds */

		/* Find in which line and column the found string is */

		for ( ppy = 0; ppy < tw->tlines; ppy++ )
		{
			if ( tw->lines[ppy] - tw->lines[0] > find_offset )
				break;
		}

		/* Note: "ppy" is now equal to line number + 1 */

		tw->py = ppy - 1;
		tw->px = find_offset - tw->lines[tw->py];
		w_page((TYP_WINDOW *)tw, HORIZ | VERTI);
	}
#endif

	return TRUE;

}


/********************************************************************
 *																	*
 * Funkties voor het initialisatie, laden en opslaan.				*
 *																	*
 ********************************************************************/

void txt_init(void)
{
	RECT work;

	xw_calc(WC_WORK, TFLAGS, &screen_info.dsk, &work, viewmenu);
	tmax.x = screen_info.dsk.x;
	tmax.y = screen_info.dsk.y;
	tmax.w = work.w - (work.w % screen_info.fnt_w);
	tmax.h = work.h - (work.h % screen_info.fnt_h);
}


/*
 * Calculate default positions and sizes of text windows;
 * windows get stacked rightwards and downwards
 */
 
void txt_default(void)
{
	int i;

	txt_font = def_font;

	for (i = 0; i < MAXWINDOWS; i++)
	{
		textwindows[i].x = (i + 1) * screen_info.fnt_w - screen_info.dsk.x;
		textwindows[i].y = screen_info.dsk.y + i * screen_info.fnt_h - screen_info.dsk.y;
		textwindows[i].w = (screen_info.dsk.w * 9) / (10 * screen_info.fnt_w);
		textwindows[i].h = (screen_info.dsk.h * 8) / (10 * screen_info.fnt_h);
		textwindows[i].flags.fulled = 0;
		textwindows[i].flags.iconified = 0;
		textwindows[i].flags.resvd = 0;
		textwindows[i].used = FALSE;
	}
}


typedef struct
{
	long 
		py;
	int 
		px, 
		index,
	    hexmode, 
		tabsize;
	LNAME 
		name;
	WINDOW 
		*w;
} SINFO2;

static SINFO2 that;


/* 
 * Configuration table for one open text window 
 */

static CfgEntry txtw_table[] =
{
	{CFG_HDR, 0, "text" },
	{CFG_BEG},
	{CFG_D,   0, "indx", &that.index	},
	{CFG_S,   0, "name",  that.name	    },
	{CFG_D,   0, "xrel", &that.px		},
	{CFG_L,   0, "yrel", &that.py		},
	{CFG_BD,  0, "hexm", &that.hexmode	},
	{CFG_D,   0, "tabs", &that.tabsize	},
	{CFG_END},
	{CFG_LAST}
};


/* 
 * Save or load configuration for one open text window 
 */


CfgNest text_one
{
	if (io == CFG_SAVE)
	{
		int i = 0;
		TXT_WINDOW *tw;
	
		while ((WINDOW *) textwindows[i].typ_window != that.w)
			i++;
	
		tw = (TXT_WINDOW *)textwindows[i].typ_window;
		that.index = i;
		that.px = tw->px;
		that.py = tw->py;
		that.hexmode = tw->hexmode;
		that.tabsize = tw->tabsize;
		strcpy(that.name, tw->name);
	
		*error = CfgSave(file, txtw_table, lvl + 1, CFGEMP);
	}
	else
	{
		memset(&that, 0, sizeof(that));

		*error = CfgLoad(file, txtw_table, (int)sizeof(LNAME), lvl + 1);

		if (*error == 0 )
		{
			if 
			(   that.name[0] == 0
				|| that.index >= MAXWINDOWS
				|| that.tabsize > 40
				|| that.px > 1000
			)
				*error = EFRVAL;
		}

		if (*error == 0)
		{
			char *name = malloc(strlen(that.name) + 1);
			if (name)
			{
				strcpy(name, that.name);

				/* Note: in case of error, name is deallocated in txt_do_open */

				txt_do_open(   &textwindows[that.index],
								name,
								that.px,
								that.py,
								that.tabsize,
								that.hexmode,
								FALSE,
								error
							);
				that.index++;
			}
			else
				*error = ENSMEM;
		}
	}
}


int text_save(XFILE *file, WINDOW *w, int lvl)
{
	int error = 0;

	that.w = w;
	text_one(file, "text", lvl + 1, true, &error );

	return error;
}


CfgNest view_config
{
	if ( io == CFG_LOAD )
	{
		memset(&thisw, 0, sizeof(thisw));
		memset(&that, 0, sizeof(that));
	}

	cfg_font = &txt_font;
	wtype_table[0].s = "views";
	thisw.windows = &textwindows[0];

	*error = handle_cfg(file, wtype_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, NULL, NULL );
}


