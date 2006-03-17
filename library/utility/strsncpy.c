/*
 * Utility functions for Teradesk. Copyright (c)        1993, 1994, 2002  W. Klaren,
 *                                                            2002, 2003  H. Robbers,
 *                                                2003, 2004, 2005, 2006  Dj. Vukovic
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


/* 
 * Safe string copy- never more than len-1 characters, and
 * always terminated with a nul char. Nul char is included in length "len"
 */

char *strsncpy(char *dst, const char *src, size_t len)	
{
	*dst = 0; /* just in case strncpy doesn't copy empty string */
	strncpy(dst, src, len - 1);		/* len is typical: sizeof(achararray) */
	*(dst + len - 1) = 0;
	return dst;
}


/*

/*
 * Copy a string 's' and right-justify in a field 'd' with length 'len'. 
 * Termination zero byte is added -after- length 'len'.
 * Length of 's' must not be grater than 'len'.
 * Note: currently this routine is not used anywhere in TeraDesk
 * because using it (as opposed to directly coding) gives a marginal 
 * gain in program size (just several bytes) but a penalty in speed.
 * Therefore this code is commented out
 */

char *strcpyj(char *d, const char *s, size_t len)
{
	size_t l, b;
	int i = 0;

	l = strlen(s);
	if (l > len)
		l = len;
	b = len - l;

	memset(d, ' ', b);
	memcpy(d + b, s, l);
	d[len] = 0;
	return d;
}
*/


/*
 * Calculate minimum safe buffer length for a name containing
 * quotes or spaces which will have to be quoted.
 * Space calculated here may be a little longer than actually needed,
 * depending on which quotes are used, and also three bytes reserve
 * is included here.
 */

size_t strlenq(const char *name)
{
	size_t l = 3;
	char *p = (char *)name;
	int q = 0;

	while(*p)
	{
		l++;

		if(*p == ' ')
			q = 1;					/* quote if space found */

		if(*p == 39 || *p == 34)
		{
			q = 1;					/* quote if embedded quote found */
			l++;					/* and it has to be doubled */
		}

		p++; /* don't put this in the 'if' above ! */
	}

	if(q)
		l += 2;						/* add two for enclosing quotes */

	return l;						/* return string length */
}


/*
 * Copy a string to destination. If it contains embedded blanks,
 * put it between quotes, using character qc. If it contains quotes, 
 * double them. This routine returns a pointer to the -end- of 
 * the string (to the null termination byte after it)
 */

char *strcpyq(char *d, const char *s, char qc)
{
	char q = 0;

	/* If there are embedded blanks or quotes, start quoting */

	if(strchr(s, ' ') || strchr(s, 34) || strchr(s, 39) )
	{
		*d++ = qc;
		q = 1; 
	}

	/* Transfer all characters; double any embedded quote */

	while(*s)
	{
		*d++ = *s;
		if(*s == qc)
			*d++ = qc;
		s++;
	}

	/* If quoting has been started, finish it (unquote) */

	if(q)
		*d++ = qc;

	/* Add a zero termination byte */

	*d = 0;

	return d;
}


/*
 * Copy a string removing the quotes. The first of the single- or
 * double-quote characters encountered in interpreted as the quote
 * character. If the string contains two consecutive quotes, onlt
 * one will be left. Any unquoted spaces will be replaced by zeros.
 * The routine will return a pointer to the -end- of the string.
 * The source and the destination can be the same, because the
 * resulting string will always be shorter than the source, and
 * copying starts from the beginning of the string.
 */

char *strcpyuq(char *d, char *s)
{
	char h, fqc = 0, q = 0;

	while ((h = *s++) != 0)
	{
		if ((h == ' ') && !q )
		{
			/* If not between quotes, substitute blanks with a single 0 */

			*d++ = 0;
			while (*s == ' ')
				s++;
		}

		/* Is this a quote character (see also va_start_prg() in va.c) */

		else if ((h == fqc) || (!fqc && (h == 39 || h == 34))) /* 34= double quote, 39=single quote */
		{
			/* two consequtive quotes mean that one is part of the string */

			if (*s == h)
				*d++ = *s++;	/* transfer quote as part of the string */
			else
			{
				fqc = 0;		/* reset quote character, just in case */

				if(!q)
					fqc = h;	/* First encountered quote character */

				q = !q;			/* start or end the quote */
			}
		}
		else
			*d++ = h;
	}

	*d++ = 0; /* a trailing zero (termination) byte */

	return d;
}


/*
 * Copy a string from s to d substituting the quotes character if 
 * necessary. Return a pointer to the -end- of string.
 * This function will e.g. convert a string containing items between
 * single-quotes into a string containing items between double quotes -
 * or v.v. Any appearance of the char qc between quotes will be doubled.
 * This routine will also find the first blank character that is not 
 * quoted.
 */

char *strcpyrq(char *d, const char *s, char qc, char **fb)
{
	char
		q = 0,				/* nonzero if quoting in effect */
		fqc = 0,			/* first encountered quote character */
 		*p = (char *)s,		/* a location in source string */
		*t = d;				/* a location in destination string */
		*fb = 0L;			/* no blanks found yet */


	while(*p)
	{
		if( ((*p == fqc) || (!fqc && (*p == 39 || *p == 34))) && p[1] != *p)
		{
			/* This is one quote character; start or end quoting */

			fqc = 0;

			if(!q)
				fqc = *p;

			q = !q;
			*t = qc;
		}
		else
		{
			/* 
			 * this may be one single/double quote enclosed in 
			 * different (double/single) quotes 
			 * or a duplicated current quote character
			 */

			if(q)
			{
				if(*p == qc && p[1] != qc)
					*t++ = qc;

				/* or a doubled quote */

				if(*p == fqc && p[1] == fqc)
				{
					if(fqc == qc)
						*t++ = qc;

					p++;			
				}
			}
			else
			{
				if(*p == ' ' && *fb == 0L) /* first unquoted blank */
					*fb = t;
			}		

			/* or any other character... */

			*t = *p;
		} 

		p++;
		t++;
	}

	*t = 0;

	return t;
}