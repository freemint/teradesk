/*
 * Teradesk. Copyright (c)  1993 - 2002  W. Klaren,
 *                          2002 - 2003  H. Robbers,
 *                          2003 - 2013  Dj. Vukovic
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */


#include <library.h>
#include <xdialog.h>
#include <xscncode.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "font.h"
#include "xfilesys.h"
#include "config.h"
#include "screen.h"
#include "window.h"
#include "viewer.h"
#include "file.h"
#include "stringf.h"


#define FWIDTH			256				/* max.possible width of text window in text mode */

#define ASCII_PERC		80				/* if so many % of printable chars, it is a text file */


WINFO textwindows[MAXWINDOWS];

GRECT tmax;

XDFONT txt_font;

static _WORD displen = -1;



/*
 * Determine number of lines in a text window
 */

static long txt_nlines(TXT_WINDOW *w)
{
	return (w->hexmode) ? ((w->size + 15L) / 16) : w->tlines;
}


/*
 * Determine width of a text window; depending on whether the display is
 * in hex mode or text mode, it will be either a fixed value or a value
 * determined from the longest line in the text. Line lengths are calculated
 * from differences between pointers to line beginnings
 * (returned width will never be more than FWIDTH).
 */

static _WORD txt_width(TXT_WINDOW *w, _WORD hexmode)
{
	long i;							/* line counter */
	long ll;						/* current line width */
	long mw;						/* maximum line width */
	char **wl;
	char *p;						/* pointer to the beginning of the current line */
	char *p1;						/* same, but for the next line */

	/* 
	 * If display is in hex mode, return a fixed width (HEXLEN); otherwise
	 * scan pointers to all lines and find greatest difference between
	 * two adjecent ones- that would be the maximum line length.
	 * To each line length is added the total length of tab substitutes.
	 */

	if (hexmode == 1)
		return HEXLEN + 1;				/* fixed window width in hex mode */
	/* text is scanned only if width had been reset to 0 */

	if (w->twidth == 0)
	{
		_WORD ts = w->tabsize;

		if (w->tlines > 100)
			hourglass_mouse();

		wl = w->lines;
		mw = 0;

		for (i = 0; i < w->tlines; i++)
		{
			p = *wl++;
			p1 = *wl;
			ll = 0;

			do
			{
				ll++;

				if (*p++ == '\t')
				{
					ll += (ts - (ll % ts));
				}
			} while (p < p1);

			if (ll > mw)
				mw = ll;
		}

		/* Add the right margin */

		mw += MARGIN;

		arrow_mouse();
	} else
	{
		mw = w->twidth;
	}
	
	/* 
	 * Window width should be at least (arbitrary) 16 characters 
	 * but not more than FWIDTH characters 
	 */

	mw = lminmax(16L, mw, FWIDTH);

	return (_WORD) mw;
}


/********************************************************************
 *                                                                  *
 * Functions for drawing a textwindow content.                      *
 *                                                                  *
 ********************************************************************/

/*
 * Function for converting a hexadecimal digit
 * to an ASCII character
 *
 * Parameters:
 *
 * x		- hexadecimal number
 *
 * Resultt: ASCII character
 */

static char hexdigit(_WORD x)
{
	x &= 0x000F;

	/* this is shorter than using a table */

	return (x < 10) ? x + '0' : x + ('A' - 10);
}


/*
 * Create a line in hex mode for window display or printing.
 * Note: offset from file beginning will not be displayed correctly
 * for files larger than 256MB.
 */

void disp_hex(char *tmp,				/* output buffer tmp[HEXLEN + 2] with data in hex mode  */
			  char *p,					/* pointer to the beginning of the 16-byte line of text */
			  long a,					/* index of 1st byte of the line from the file beginning */
			  long size,				/* file size */
			  bool toprint				/* true if file is being printed */
	)
{
	long h = a;
	_WORD i;
	char *tmpj, pi;

	if (a >= size)
	{
		*tmp = 0;
		return;
	}

	/* tmp[0] to tmp[6] display line offset from file beginning */

	for (i = 6; i >= 0; i--)
	{
		tmp[i] = hexdigit((_WORD) h);
		h >>= 4;
	}

	tmpj = &tmp[7];

	*tmpj++ = ':';

	/* Hexadecimal (in h) and ASCII (in 58+i) representation of 16 bytes */

	for (i = 0; i < 16; i++)
	{
		*tmpj++ = ' ';

		if ((a + (long) i) < size)
		{
			pi = p[i];
			*tmpj++ = hexdigit((_WORD) (pi >> 4));
			*tmpj++ = hexdigit((_WORD) pi);

			if ((pi == 0) || ((toprint == TRUE) && (pi < ' ')))
				pi = '.';
		} else
		{
			*tmpj++ = ' ';
			*tmpj++ = ' ';
			pi = ' ';
		}

		tmp[58 + i] = pi;
	}

	tmp[56] = ' ';
	tmp[57] = ' ';
	tmp[74] = 0;
}


/*
 * Function for reading a line from the buffer of a
 * window. Depending on the mode, the presentation is in ASCII or
 * hexadecimal.
 *
 * Parameters:
 *
 * w		- Pointer to WINFO structur 
 * dest		- Buffer in which the line should be placed
 * line		- Number of the line
 *
 * Result:   Length of the line
 */

static _WORD txt_line(TXT_WINDOW *w,	/* pointer to window being processed */
					  char *dest,		/* pointer to destination (output) string */
					  long line			/* line index */
	)
{
	char *s;							/* pointer to the source string- the real line */
	char *d = dest;						/* pointer to a location in destination string */

	_WORD i;							/* counter */
	_WORD wpxm = w->px - MARGIN, m = w->columns;	/* rightmost column in the text */
	_WORD cnt = 0;						/* input character count */
	_WORD cntm = 0;						/* left margin visible character count */

	/* Don't process invisible or nonexistent lines */

	if (line >= w->nlines)
	{
		*dest = 0;
		return 0;
	}

	/* Left margin in text window, if visible */

	for (i = w->px; i < MARGIN; i++)
	{
		*d++ = ' ';
		cntm++;
	}

	m -= cntm;

	if (w->hexmode == 0)
	{
		/* Create a line in text mode */
		char ch;
		_WORD tabsize = w->tabsize;

		m += w->px;						/* rightmost column in text */

		/* If this line does not exist at all, return NULL */

		if ((s = w->lines[line]) == NULL)
		{
			*dest = 0;
			return 0;
		}

		/* The real line */

		while (cnt < m)
		{
			if ((ch = *s++) == 0)		/* null: ignore */
				continue;

			if (ch == '\n')				/* linefeed: end of line */
				break;

			if (ch == '\r')				/* carriage return: ignore */
				continue;

			if (ch == '\t')				/* tab: substitute */
			{
				do
				{
					if (cnt >= wpxm)
						*d++ = ' ';

					cnt++;
				} while (((cnt % tabsize) != 0) && (cnt < m));

				continue;
			}

			if (cnt >= wpxm)			/* if righter than left edge... */
				*d++ = ch;				/* set that character */

			cnt++;
		}
	} else
	{
		/* Create a line in hex mode */
		long a = 16L * line;

		char tmp[HEXLEN + 2],
		*p = &w->buffer[a];

		if (w->px >= HEXLEN)
		{
			*dest = 0;
			return 0;
		}

		disp_hex(tmp, p, a, w->size, FALSE);

		s = &tmp[wpxm + cntm];			/* same as &tmp[w->px - (MARGIN - cntm)] */

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

	/* Finish the line with a null */

	*d = 0;
	return (_WORD) (d - dest);
}


/*
 * Calculate the rectangle that encloses the text to be drawn.
 */

static void txt_comparea(TXT_WINDOW *w,	/* pointer to window */
						 long line,		/* line number */
						 _WORD str_len,	/* length of the string */
						 GRECT *r, GRECT *work	/* GRECT structure with the size of the window's work area */
	)
{
	r->g_x = work->g_x;
	r->g_y = work->g_y + (_WORD) (line - w->py) * txt_font.ch;
	r->g_w = str_len * txt_font.cw;
	r->g_h = txt_font.ch;
}


/*
 * Function for printing 1 character of a line. This one
 * function is scrolled to the left and right of one
 * window used.
 */

static void txt_prtchar(TXT_WINDOW *w,	/* pointer to window */
						_WORD column,	/* column in which the character is printed */
						_WORD nc,		/* number of columns */
						long line,		/* line in which the character is printed */
						GRECT *area,		/* clipping rectangle */
						GRECT *work		/* Workspace of the window */
	)
{
	GRECT r, in;
	_WORD len, c;
	char s[FWIDTH];

	c = column - w->px;

	len = txt_line(w, s, line);
	txt_comparea(w, line, len, &r, work);

	r.g_x += c * txt_font.cw;
	r.g_w = nc * txt_font.cw;

	if (xd_rcintersect(area, &r, &in))
	{
		pclear(&in);

		if (c < len)
		{
			s[min(c + nc, len)] = 0;
			w_transptext(r.g_x, r.g_y, &s[c]);
		}
	}
}


/*
 * Function for printing a line.
 */

void txt_prtline(TXT_WINDOW *w,		/* Pointer to window */
				 long line,			/* Line that will be printed */
				 GRECT *area,		/* Clipping rectangle */
				 GRECT *work			/* Workspace of the window */
	)
{
	GRECT r, r2;
	_WORD len;
	char s[FWIDTH];

	len = txt_line(w, s, line);
	txt_comparea(w, line, len, &r, work);

	r2 = r;
	r2.g_w = work->g_w;

	if (rc_intersect(area, &r2))
		pclear(&r2);

	if (rc_intersect2(area, &r))
		w_transptext(r.g_x, r.g_y, s);
}


/********************************************************************
 *																	*
 * Functions for scrolling text windows.							*
 *																	*
 ********************************************************************/

/*
 * Retype a column of a window.
 */

void txt_prtcolumn(TXT_WINDOW *w,		/* pointer naar window */
				   _WORD column,		/* first column */
				   _WORD nc,			/* number of columns */
				   GRECT *area,			/* clipping rechthoek */
				   GRECT *work			/* werkgebied window */
	)
{
	_WORD i;

	for (i = 0; i < w->rows; i++)
		txt_prtchar(w, column, nc, w->py + i, area, work);
}


/********************************************************************
 *																	*
 * Functions for handling selections of a menu item                 *
 * a window menu.                                                   *
 *																	*
 ********************************************************************/

/*
 * Function for updating the window menu of a text window.
 * If a macro is used instead of a function, a couple of bytes
 * are saved
 */
#define set_menu(x) xw_menu_icheck((WINDOW *)(x), VMHEX, x->hexmode)


/*
 * Function for setting the ASCII or HEXMODE
 * of a window. This routine toggles hexmode on/off
 * upon each call.
 */

static void txt_mode(TXT_WINDOW *w)
{
	w->hexmode ^= 1;
	w->nlines = txt_nlines(w);
	w->columns = txt_width(w, w->hexmode);

	wd_type_sldraw((WINDOW *) w);
	set_menu(w);						/* check the hexmode menu item */
}


/*
 * Function for setting tabulator size of one
 * textwindow
 */
static void txt_tabsize(TXT_WINDOW *w)
{
	_WORD oldtab, newtab;
	char *vtabsize = stabsize[VTABSIZE].ob_spec.tedinfo->te_ptext;

	oldtab = w->tabsize;

	itoa(oldtab, vtabsize, 10);

	/* Act only if there was an OK in the dialog */

	if (chk_xd_dialog(stabsize, VTABSIZE) == STOK)
	{
		if ((newtab = atoi(vtabsize)) < 1)
			newtab = 1;

		/* Do it only if tab width has changed */

		if (newtab != oldtab)
		{
			w->tabsize = newtab;

			w->twidth = 0;				/* force txt_width */

			w->twidth = txt_width(w, 0);	/* in text mode, slower */
			w->columns = txt_width(w, w->hexmode);
			wd_type_sldraw((WINDOW *) w);
		}
	}
}


/*
 * Aux. size-saving function- free some allocations related to text windows
 */

static void txt_free(TXT_WINDOW *w)
{
	free(w->lines);
	free(w->buffer);
}


/*
 * Remove a text window and delete window structure
 */

static void txt_rem(TXT_WINDOW *w)
{
	txt_free(w);
	free(w->path);
	w->winfo->used = FALSE;
	w->winfo->typ_window = NULL;

	xw_delete((WINDOW *) w);
}


/*
 * Window close handler for text windows.
 *
 * Parameters:
 *
 * w			- Pointer to window
 */

void txt_closed(WINDOW *w)
{
	xw_close(w);
	txt_rem((TXT_WINDOW *) w);
}


/*
 * Window menu handler for text windows.
 *
 * Parameters:
 *
 * w			- Pointer to window
 * title		- Selected menu title
 * item			- Selected menu item
 */

void txt_hndlmenu(WINDOW *w, _WORD title, _WORD item)
{
	/* A switch structue is used in case new menu items are introduced */

	switch (item)
	{
	case VMTAB:
		txt_tabsize((TXT_WINDOW *) w);
		break;
	case VMHEX:
		txt_mode((TXT_WINDOW *) w);
		break;
	}

	wd_type_nofull(w);
	xw_menu_tnormal(w, title, 1);
}


/********************************************************************
 *                                                                  *
 * Functions for loading files in a window.                         *
 *                                                                  *
 ********************************************************************/

/* 
 * Funkties voor het tellen van de regels van een file.
 * Return number of lines counted. 
 */

static long count_lines(char *buffer,	/* pointer to buffer contaning file */
						long length		/* buffer (file) length */
	)
{
	long cnt = length;
	long n = 1;
	char *s = buffer;

	do
	{
		if (*s++ == '\n')
			n++;
	} while (--cnt > 0);

	return n;
}


/* 
 * Funktie voor het maken van een lijst met pointers naar het begin
 * van elke regel in de file. 
 *
 * Line end is detected by "\n" character (linefeed)
 * so this will be ok for TOS/DOS and Unix files but not for Mac files
 * (there, lineend is carriage-return only).
 */

static void set_lines(char *buffer,		/* pointer to buffer containing file */
					  char **lines,		/* pointer to array of pointers to line beginnings */
					  long length		/* file buffer length */
	)
{
	char *s = buffer;
	char **p = lines;
	long cnt = length;

	*p++ = s;

	do
	{
		if (*s++ == '\n')				/* find lineends */
		{
			*p++ = s;
		}
	} while (--cnt > 0);

	*p = buffer + length;
}


/*
 * Check if a character is printable.
 * This is a (sort of) substitute for the simpler Pure-C function isprint().
 * BEWARE that this function accepts an extended range of characters
 * compared to isprint(); 
 * 
 * Acceptable range: ' ' to '~' (32 to 126), <tab>, <cr>, <lf>
 * Also, characters higher than 127 (c < 0) are assumed to be printable
 * for the sake of codepages of other languages.
 */

static bool isxprint(signed char c)
{
	if ((c >= ' ' && c <= '~') || c == '\t' || c == '\r' || c == '\n' || c < 0)
		return TRUE;
	return FALSE;
}


/*
 * Load a text file for the purpose of displaying in a window, 
 * or for the purpose of searching to find a string in the file
 * or for any other purpose where found convenient.
 * Function returns error code, if there is any, or 0 for OK.
 * If pointer parameter "tlines" is NULL, it is assumed that the routine will
 * be used for non-display purpose; then lines will not be counted and set.
 * Note: for safety reasons, the routine allocates several bytes 
 * more than needed.
 */

static _WORD read_txtfile(const char *name,	/* name of file to read */
						  char **buffer,	/* location of the buffer to receive text */
						  long *flength,	/* size of the file being read */
						  long *tlines,	/* number of text lines in the file */
						  char ***lines	/* pointer to array of pointers to beginnings of text lines */
	)
{
	_WORD handle;						/* file handle */
	_WORD error;						/* error code  */
	long read;							/* number of bytes read from the file */
	XATTR attr;							/* file attributes */

	*flength = 0L;

	/* Get file attributes (follow the link in x_attr)  */

	if ((error = (_WORD) x_attr(0, FS_INQ, name, &attr)) == 0)
	{
		*flength = attr.st_size;		/* output param. file length */

		/* Open the file */

		if ((attr.st_mode & S_IFMT) != S_IFREG)
		{
			error = ENODEV;
		} else if ((handle = x_open(name, O_DENYW | O_RDONLY)) >= 0)
		{
			/* Allocate buffer for complete file; read file into it  */

			if ((*buffer = malloc(*flength + 4L)) == NULL)
			{
				error = ENOMEM;
			} else
			{
				read = x_read(handle, *flength, *buffer);

				/* If completely read with success... */

				if (read == *flength)
				{
					/* And this has been read for a display in a window */
					if (tlines != NULL)
					{
						/* 
						 * There must be a linefeed -after- the end of file
						 * or the display routine will not know when to stop
						 */

						(*buffer)[*flength] = '\n';

						/* Count text lines in the file (in fact, in the buffer) */

						*tlines = count_lines(*buffer, *flength);

						/* 
						 * Allocate memory for pointers to beginnings of lines,
						 * then find those beginnings. 
						 */

						if (((*lines = malloc((*tlines + 1) * sizeof(char *))) != NULL))
							set_lines(*buffer, *lines, *flength);
						else
						{
							error = ENOMEM;
							free(*lines);
							free(*buffer);
						}
					}
				} else
				{
					error = (read < 0) ? (_WORD) read : EIO;
					free(*buffer);
				}
			}
			x_close(handle);
		} else
		{
			error = handle;
		}
	}
	return error;
}


/*
 * A shorter form of the above, with smaller number of arguments.
 */

_WORD read_txtf(const char *name,		/* name of file to read */
				char **buffer,			/* location of the buffer to receive text */
				long *flength			/* size of the file being read */
	)
{
	return read_txtfile(name, buffer, flength, NULL, NULL);
}


/* 
 * Funktie voor het laden van een file in het geheugen.
 * Bij een leesfout wordt een waarde ongelijk 0 terug gegeven.
 * Alle buffers zijn dan vrijgegeven. 
 * File name is contained in w->path;
 * ASCII or Hex mode is determined from the first 256 bytes of file.
 * Af at least 80% of those characters are ASCII printable ones,
 * it is assumed that text mode should be applied.
 */

static _WORD txt_read(TXT_WINDOW *w,	/* pointer to window where the file will be displayed */
					  bool setmode		/* if true, determine whether to use text or hex mode */
	)
{
	_WORD error;

	hourglass_mouse();

	/* Read complete file */

	error = read_txtfile(w->path, &(w->buffer), &(w->size), &(w->tlines), &(w->lines));

	arrow_mouse();

	if (error != 0)
	{
		w->lines = NULL;
		w->buffer = NULL;
	} else
	{
		w->twidth = 0;					/* force txt_width to do something */

		w->twidth = txt_width(w, 0);	/* width in text mode (slower) */

		if (setmode)
		{
			char *b;
			_WORD i;
			_WORD e;
			_WORD n = 0;

			b = w->buffer;
			e = (_WORD) lmin(w->size, 256L);	/* longest possible line */

			for (i = 0; i < e; i++)
			{
				if (isxprint(*b++))
					n++;
			}

			n = (n * 100) / (e + 1);	/* e can be 0 */

			if (n > ASCII_PERC)
				w->hexmode = 0;
			else
				w->hexmode = 1;

			set_menu(w);
		}

		/* Determine window text length and width */

		w->nlines = txt_nlines(w);
		w->columns = txt_width(w, w->hexmode);
	}

	return error;
}


/*
 * Read or reread a file into the same window.
 * If filename is not given (is NULL), keep old name and reread old file.
 */

bool txt_reread(TXT_WINDOW *w,			/* pointer to window into which the file is reread */
				char *name,				/* file name */
				_WORD px,				/* position of horizontal slider (column index) */
				long py					/* position of vertical slider (line index) */
	)
{
	txt_free(w);

	if (name)
	{
		free(w->path);
		w->path = name;
	}

	if (txt_read(w, TRUE) >= 0)
	{
		w->px = px;
		w->py = py;
		wd_type_title((TYP_WINDOW *) w);	/* set title AND sliders */
		wd_type_draw((TYP_WINDOW *) w, FALSE);
		return TRUE;
	}

	return FALSE;
}


/* 
 * Compare contents of two buffers; return index of first difference.
 * If buffers are identical, return buffer length
 */

static long compare_buf(char *buf1,		/* pointer to buffer #1 */
						char *buf2,		/* pointer to buffer #2 */
						long len		/* how many bytes to compare */
	)
{
	long i;

	for (i = 0; i < len; i++)
	{
		if (*buf1++ != *buf2++)
			break;
	}

	return i;
}


/*
 * Copy a number of bytes from source to destination, starting from
 * index "pos", substituting all nulls with something else that is printable; 
 * Not more than "dl" bytes will be copied, or less if buffer end (nul char)
 * is encountered first. Add a nul char at the end. In order for all this
 * to work, "dl" must first be given a positive value.
 *
 * Note: "pos" should never be larger than "length" !!!! no checking done !
 * ("length" is length from the beginning of source, not counted from "pos").
 */

void copy_unnull(char *dest,			/* pointer to destination */
				 const char *source,	/* pointer to source */
				 long length,			/* length of source */
				 long pos,				/* starting position in the soruce */
				 _WORD dl				/* length of the display field */
	)
{
	_WORD i, n;
	const char *s = source + pos;
	char *d = dest;

	n = (_WORD) lmin(length - pos, (long) dl);

	for (i = 0; i < n; i++)
	{
		*d = *s++;

		/* Some characters can't be printed; substitute them */

		if (*d == 0)
			*d = SUBST_DISP;

		d++;
	}

	*d = 0;
}


/*
 * Compare two files. Display differences found.
 * Note: both files are completely read into memory 
 * (can be inconvenient for large files).
 * 
 * This routine attempts to compare files with some intelligence: 
 * if a difference is found, the routine tries to resynchronize the
 * contents of the files after a difference. 
 */

void compare_files(WINDOW *w, _WORD n, _WORD *list)
{
	char *name;							/* name(s) of the file(s) */
	char *buf1 = NULL;					/* buffer for file #1   */
	char *buf2 = NULL;					/* buffer for file #2   */

	_WORD i;							/* a counter */
	_WORD sw;							/* size of compare window */
	_WORD swx2;							/* almost twice of above */
	_WORD button;							/* response in the alert box     */
	_WORD error = 0;						/* error code from reading files */

	long size1;							/* size of file #1 */
	long size2;							/* size of file #2 */
	long i1;							/* index in the first buffer  */
	long i2;							/* index in the second buffer */
	long in;							/* next start index for comparison */
	long i1n;							/* new i1 */
	long i2n;							/* new i2 */
	long i2o;							/* previous i2 */
	long nc;							/* number of bytes to compare */
	long ii;							/* counter of synchronization attempts */

	XDINFO info;						/* dialog info structure */

	bool sync = TRUE;					/* false while attempting to resynchronize */
	bool diff = FALSE;					/* true if not equal files */


	/* 
	 * Set initial values for some dialog elements. Length of display is 
	 * determined only in the first call, when the display contains the
	 * model from the resource file.
	 */

	if (displen < 0)
		displen = (_WORD) strlen(compare[DTEXT1].ob_spec.tedinfo->te_ptext);

	compare[CFILE1].ob_flags |= OF_EDITABLE;
	compare[CFILE2].ob_flags |= OF_EDITABLE;
	compare[COMPWIN].ob_flags |= OF_EDITABLE;
	obj_hide(compare[CMPIBOX]);

	if (options.cwin == 0)
		options.cwin = 15;				/* default value */

	itoa(options.cwin, compare[COMPWIN].ob_spec.tedinfo->te_ptext, 10);

	/* If names were selected, use them */

	for (i = 0; i < 2; i++)
	{
		if (i < n)						/* i.e. if it is in the list */
			name = itm_fullname(w, list[i]);
		else
			name = strdup(empty);

		cv_fntoform(compare, CFILE1 + i, name);
		free(name);
	}

	/* Open the dialog, then loop while needed */

	if (chk_xd_open(compare, &info) >= 0)
	{
		do
		{
			button = xd_form_do_draw(&info);

			if (button == COMPOK)
			{
				/* Take match-window width from the dialog */

				sw = atoi(compare[COMPWIN].ob_spec.tedinfo->te_ptext);
				swx2 = 2 * sw - 1;

				/* File names and window width must be given properly */

				strip_name(cfile1, cfile1);
				strip_name(cfile2, cfile2);

				if (*cfile1 <= ' ' || *cfile2 <= ' ' || sw <= 0)
				{
					alert_iprint(MINVSRCH);
					continue;
				}

				options.cwin = sw;

				/* Fields are not editable anymore */

				compare[CFILE1].ob_flags &= ~OF_EDITABLE;
				compare[CFILE2].ob_flags &= ~OF_EDITABLE;
				compare[COMPWIN].ob_flags &= ~OF_EDITABLE;

				/* A file need not be compared to itself */

				if (strcmp(cfile1, cfile2) == 0)
					break;

				/* Start real work */

				hourglass_mouse();		/* this will take some time... */

				/* Read the files into respective buffers */

				if ((error = read_txtf(cfile1, &buf1, &size1)) == 0)
					error = read_txtf(cfile2, &buf2, &size2);

				/* All ok, now start comparing */

				if (error == 0)
				{
					/* Current indices: at the beginning(s) */

					i1 = 0;
					i2 = 0;

					/* Scan until the end of the shorter file */

					while ((i1 < size1) && (i2 < size2))
					{
						/* How many bytes to compare and not exceed the ends */

						nc = lmin((size1 - i1), (size2 - i2));

						/* How many bytes are equal ? Move to end of that */

						in = compare_buf(&buf1[i1], &buf2[i2], nc);

						i1n = i1 + in;
						i2n = i2 + in;

						/* If a difference is found before file ends, report */

						if ((in < nc) || ((in >= nc) && ((i1n < size1) || (i2n < size2))))
						{
							/* 
							 * If there is a difference, display strings;
							 * Do so also if files have different lengths
							 * and the end of one is reached
							 */

							if (sync || (in > swx2))
							{
								arrow_mouse();

								sync = FALSE;
								diff = TRUE;

								i1 = i1n;
								i2 = i2n;
								i2o = i2;

								/* Prepare something to display */

								copy_unnull(compare[DTEXT1].ob_spec.tedinfo->te_ptext, buf1, size1, i1, displen);
								copy_unnull(compare[DTEXT2].ob_spec.tedinfo->te_ptext, buf2, size2, i2, displen);
								rsc_ltoftext(compare, COFFSET, i1);

								obj_unhide(compare[CMPIBOX]);
								xd_drawdeep(&info, CMPIBOX);
								button = xd_form_do_draw(&info);

								/* Stop further comparison ? */

								if (button == COMPCANC)
									break;

								hourglass_mouse();

								/* 
								 * Advance a little further and try to resync;
								 * it is reasonable that this advance stays
								 * inside the displayed area, but need not
								 * be so (i.e. is optimum that sw is half of
								 * display length)
								 */

								i1 += sw;
								ii = 0;
							} else
							{
								ii++;
								if (ii == swx2)
								{
									sync = TRUE;
									i2 = i2o;
								}
							}
						} else
						{
							i1 = i1n;
							i2 = i2n;
							sync = TRUE;
						}

						if (sync)
							i1++;
						i2++;

					}					/* while */

					arrow_mouse();
					break;

				} /* error ? */
				else
				{

					/* Something went wrong while reading the files */

					arrow_mouse();
					xform_error(error);
					button = COMPCANC;
				}
			} else
				error = 1;				/* in order not to show alert below */
		} while (button != COMPCANC);

		/* Close the dialog */

		xd_close(&info);

		free(buf1);
		free(buf2);

		/* Display info if files are identical */

		if (!diff && (error == 0))
			alert_iprint(MCOMPOK);
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

static WINDOW *txt_do_open(WINFO *info, char *file, _WORD px,
						   long py, _WORD tabsize, bool hexmode, bool setmode, int *error)
{
	TXT_WINDOW *w;
	GRECT oldsize;
	WDFLAGS oldflags;

	wd_in_screen(info);

	if ((w = (TXT_WINDOW *) xw_create(TEXT_WIND, &wd_type_functions, TFLAGS, &tmax,
									  sizeof(TXT_WINDOW), viewmenu, error)) == NULL)
	{
		free(file);
		return NULL;
	}

	/* Note: complete window structure content set to zeros in xw_create */

	hourglass_mouse();

	info->typ_window = (TYP_WINDOW *) w;
	w->px = px;
	w->py = py;
	w->path = file;
	w->tabsize = tabsize;
	w->hexmode = hexmode;
	w->winfo = info;

	oldsize = info->pos;				/* remember old size */

	wd_restoresize(info);

	oldflags = info->flags;

	wd_setnormal(info);

	/* Read text file */

	*error = txt_read(w, setmode);

	arrow_mouse();

	if (*error != 0)
	{
		txt_rem(w);
		return NULL;
	} else
	{
		wd_iopen((WINDOW *) w, &oldsize, &oldflags);
		return (WINDOW *) w;
	}
}


/*
 * Add a text window specified by item selected in a window
 * or by a filename (if not NULL, filename has priority)
 */

bool txt_add_window(WINDOW *w, _WORD item, _WORD kstate, char *thefile)
{
	_WORD j = 0;
	int error;
	char *file;
	WINFO *textwj = textwindows;

	(void) kstate;
	while ((j < MAXWINDOWS - 1) && (textwj->used != FALSE))
	{
		j++;
		textwj++;
	}

	if (textwj->used)
		return alert_iprint(MTMWIND), FALSE;

	if (thefile)
		file = thefile;
	else if ((file = itm_fullname(w, item)) == NULL)
		return FALSE;

	if (txt_do_open(textwj, file, 0, 0, options.tabsize, FALSE, TRUE, &error) == NULL)
	{
		xform_error(error);
		return FALSE;
	}
#if _SHOWFIND
	/* If there was a search for a string, position sliders to show string */

	if (search_nsm > 0)
	{
		long ppy = 0;					/* line in which the string is */
		TXT_WINDOW *tw = (TXT_WINDOW *) (textwj->typ_window);

		search_nsm = 0;					/* this search is finished; cancel the finds */

		/* Find in which line and column the found string is */

		for (ppy = 0; ppy < tw->tlines; ppy++)
		{
			if (tw->lines[ppy] - tw->lines[0] > find_offset)
				break;
		}

		/* Note: "ppy" is now equal to line number + 1 */

		w_page((TYP_WINDOW *) tw, (_WORD) (find_offset - tw->lines[tw->py]), ppy - 1L);
	}
#endif

	return TRUE;
}


/********************************************************************
 *																	*
 * Funkties voor het initialisatie, laden en opslaan.				*
 *																	*
 ********************************************************************/

/* 
 * Configuration table for one open text window 
 */

static CfgEntry txtw_table[] = {
	CFG_HDR("text"),
	CFG_BEG(),
	CFG_D("indx", that.index),
	CFG_S("name", that.path),
	CFG_D("xrel", that.px),
	CFG_L("yrel", that.py),
	CFG_D("hexm", that.hexmode),
	CFG_D("tabs", that.tabsize),
	CFG_END(),
	CFG_LAST()
};

/* 
 * Save or load configuration for one open text window 
 */
void text_one(XFILE *file, int lvl, int io, int *error)
{
	if (io == CFG_SAVE)
	{
		_WORD i = 0;
		TXT_WINDOW *tw;
		
		/* Identify window's index in WINFO by the pointer to that window */
		while ((tw = (TXT_WINDOW *) textwindows[i].typ_window) != (TXT_WINDOW *) that.w)
			i++;
		that.index = i;
		that.px = tw->px;
		that.py = tw->py;
		that.hexmode = tw->hexmode;
		that.tabsize = tw->tabsize;
		strsncpy(that.path, tw->path, sizeof(that.path));
		*error = CfgSave(file, txtw_table, lvl, CFGEMP);
	} else
	{
		memclr(&that, sizeof(that));
		*error = CfgLoad(file, txtw_table, (_WORD) sizeof(VLNAME), lvl);
		if ((*error == 0) && (that.path[0] == 0 || that.index >= MAXWINDOWS || that.tabsize > 40))
		{
			*error = EFRVAL;
		}
		if (*error == 0)
		{
			char *name = malloc(strlen(that.path) + 1);

			if (name)
			{
				strcpy(name, that.path);
				/* 
				 * Window's index in WINFO is always taken from the file;
				 * Otherwise, positions and size would not be preserved
				 * Note: in case of error, name is deallocated in txt_do_open 
				 */
				txt_do_open(&textwindows[that.index], name, that.px, that.py, that.tabsize, that.hexmode, FALSE, error);
				/* If there is an error, display an alert */
				wd_checkopen(error);
			} else
			{
				*error = ENOMEM;
			}
		}
	}
}


/*
 * Configuration table for one window type
 */

static CfgEntry const vtype_table[] = {
	CFG_HDR("views"),
	CFG_BEG(),
	CFG_NEST("font", cfg_wdfont),
	CFG_NEST("pos", positions),							/* Repeating group */
	CFG_END(),
	CFG_LAST()
};


/*
 * Save or load configuration for text (viewer) windows
 */

void view_config(XFILE *file, int lvl, int io, int *error)
{
	if (io == CFG_LOAD)
	{
		memclr(&thisw, sizeof(thisw));
		memclr(&that, sizeof(that));
	}
	
	cfg_font = &txt_font;
	thisw.windows = &textwindows[0];
	*error = handle_cfg(file, vtype_table, lvl, CFGEMP, io, NULL, NULL);
}
