/*
 * Utility functions for Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                                                     2002, 2003  H. Robbers,
 *                                                     2003, 2004  Dj. Vukovic
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
 * Copy a string and right-justify in a field with the length 'len'
 * Termination zero byte is added -after- length 'len'.
 * Length of 's' must not be grater than 'len'.
 * Note: currently this routine is not used anywhere in teraDesk
 * because using it (as opposed to direcy coding) gives a marginal 
 * gain in program size (just several bytes) but a penalty in speed.
 * Therefore this code is commented out
 */

/*
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

