/*
 * Utility functions for Teradesk. Copyright 1993, 2002  W. Klaren
 *                                           2002, 2003  H. Robbers,
 *                                           2003, 2004  Dj. Vukovic
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


/* 
 * Concatenate a path+filename string "name" from a path "path"
 * and a filename "fname"; Add a "\" between the path and
 * the name if needed.
 * Space for resultant string has to be allocated before the call
 * Note: "name" and "path" can be at the same location
 */

void make_path( char *name, const char *path, const char *fname )
{
	long l;

	strcpy(name, path);
	l = strlen(name);
	if (l && (name[l - 1] != '\\'))
		name[l++] = '\\';
	strcpy(name + l, fname);
}


/*
 * Split a full (file)name (path+filename) into "path" and "fname" parts.
 * Space for path and fname has to be allocated beforehand.
 * Path is always finished with a "\"
 */

void split_path( char *path, char *fname, const char *name )
{
	char *backsl;

	backsl = strrchr(name,'\\');
	if (backsl == NULL)
	{
		*path = 0;
		strcpy(fname, name); /* no path, there is just filename */
	}
	else
	{
		strcpy(fname, backsl + 1);
		if (backsl == name)
			strcpy(path, "\\");
		else
		{
			long l = (backsl - (char *)name);

			strsncpy(path, name, l + 1);		/* secure copy */
			if ((l == 2) && (path[1] == ':'))
				strcat(path, "\\");
		}
	}
}


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
 */

void strip_name(char *to, const char *fro)
{
	const char *last = fro + strlen(fro) - 1;

	/* Find first nonblank */

	while (*fro && *fro == ' ')  fro++;

	/* If there is a nonzero character... */

	if (*fro)
	{
		while (*last == ' ')				/* strip tail */
			last--;

		while (*fro && (fro != last + 1) )	/* now copy */
			*to++ = *fro++;
	}

	*to = 0;								/* terminate with a null */
}


/* 
 * Fit a long filename or path into a shorter string;
 * a name should become e.g: c:\s...ng\foo.bar; 
 * s = source, t=target, w= available target length
 * Note 1: "ww" accomodates the termination byte as well
 * Note 2: it is assumed that source string will never be longer
 * than 255 bytes; there is no length checking.
 */

void cramped_name(const char *s, char *t, int ww)
{
	const char 
		*q = s;		/* pointer to a location in source string */

	char 
		*p = t, 	/* pointer to a location in target string  */
		tus[256];	/* temporary storage to form the cramped name */

	int
		w = ww - 1,	/* width -1 for termination byte */ 
		l,			/* input string length */ 
		d,			/* difference between input and output lengths */ 
		h;			/* length of the first part of the name (before "...") */


	strip_name(tus, s);		/* remove leading and trailing blanks; insert term. byte */
	q = tus;				/* location of the new (trimmed) source */
	l = (int)strlen(tus);	/* new (trimmed) string length */
	d = l - w;				/* new length difference */

	if (d <= 0)		/* (new) source is shorter than target (or same), so just copy */
		strcpy(t, s);
	else			/* (new) source is longer than the target, must cramp */
	{
		if (w < 12)				/* 8.3: destination is very short  */
		{
			strcpy(t, q + d);  	/* so copy only the last ch's */
			t[0] = '<';			/* cosmetic, to show truncated name */
		}
		else					/* else replace middle of the string with "..." */
		{
			h = (w - 3) / 2;	/* half of dest. length minus "..." */ 
			strncpy(p, q, h);	/* copy first half to  destination */
			p += h;				/* add "..." */
			*p++ = '.';
			*p++ = '.';
			*p++ = '.';

			strcpy(p, q + l - (w - h - 4) );
		}
	}
}

