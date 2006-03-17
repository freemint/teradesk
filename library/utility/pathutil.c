/*
 * Utility functions for Teradesk. Copyright             1993, 2002  W. Klaren
 *                                                       2002, 2003  H. Robbers,
 *                                           2003, 2004, 2005, 2006  Dj. Vukovic
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


#include <string.h>
#include <stddef.h>
#include <library.h>


/********************************************************************
 *																	*
 * Hulpfunkties voor dialoogboxen.									*
 * HR 151102 strip_name, cramped_name courtesy XaAES				*
 *																	*
 ********************************************************************/

/* 
 * Strip leading and trailing spaces from a string. 
 * Insert a zero byte at string end.
 * This routine actualy copies the the characters to a destination
 * which must already exist. The source is left unchanged. 
 * Note1: only spaces are considered, not tabs, etc.
 * Note2: destination can be at the same address as the source.
 * Note3: At most sizeof(XLNAME) - 1 characters will be considered
 */

void strip_name(char *to, const char *from)
{
	const char *last = from + strlen(from) - 1;	/* last nonzero */

	while (*from && *from == ' ')  from++;		/* first nonblank */

	if (*from)									/* if not empty string... */
	{
		size_t nc = 1;

		while (*last == ' ')					/* last nonblank */
			last--;

		while (from <= last && nc < sizeof(XLNAME))	/* now copy */
		{
			nc++;
			*to++ = *from++;
		}
	}

	*to = 0;									/* terminate with a null */
}


/* 
 * Fit a long filename or path (or any string into a shorter string;
 * a name should become e.g: c:\s...ng\foo.bar; 
 * s = source, t=target, ww= available target length
 * Note 1: "ww" accomodates the termination byte as well
 * Note 2: At most sizeof(XLNAME) - 1 characters will be considered
 * Source and destination can be at the same location.
 */


void cramped_name(const char *s, char *t, int ww)
{
	char 
		*p = t; 	/* pointer to a location in target string  */

	XLNAME
		ts;			/* temporary storage for the stripped name */

	int
		l,			/* input string length */ 
		d,			/* difference between input and output lengths */ 
		h;			/* length of the first part of the name (before "...") */


	strip_name(ts, s);		/* remove leading and trailing blanks; insert term. byte */
	l = (int)strlen(ts);	/* new (trimmed) string length */
	d = l - ww + 1;			/* new length difference */

	if (d <= 0)		/* (new) source is shorter than target (or same), so just copy */
		strcpy(t, ts);
	else			/* (new) source is longer than the target, must cramp */
	{
		if (ww < 13)				/* 8.3: destination is very short  */
		{
			strcpy(t, ts + d);	/* so copy only the last ch's */
			t[0] = '<';			/* cosmetic, to show it is a truncated name */
		}
		else					/* else replace middle of the string with "..." */
		{
			h = (ww - 4) / 2;	/* half of dest. length minus "..." */ 

			strncpy(t, ts, h);	/* copy first half to  destination */
			p += h;				/* add "..." */
			*p++ = '.';
			*p++ = '.';
			*p++ = '.';
 
			strcpy(p, ts + l - (ww - h - 4) );
		}
	}
}

