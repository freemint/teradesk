/*
 * Utility functions for Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren,
 *                                                     2002, 2003 H. Robbers,
 *                                               2003, 2004, 2005 Dj. Vukovic
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


#include <stdlib.h>
#include <string.h>
#include <tos.h>

/*
 * Convert a number (range 0:99 only!!!) into a string
 * (leading zeros shown). Attention: no termination 0 byte.
 */

void digit(char *s, int x)
{
	x = x % 100;
/*
	s[0] = x / 10 + '0';
	s[1] = x % 10 + '0';
*/
	s[0] = x / 10;
	s[1] = x - 10 * s[0] + '0'; /* is this faster than % ? */
	s[0] += '0';
}


/* 
 * Ring a bell
 */

void bell(void)
{
	Bconout(2, 7);
}


/* 
 * Free memory allocated to an item and set pointer to NULL 
 * (because in several places action is dependent on whether 
 * the pointer is NULL when memory is not allocated).
 * BUT: advantage of using this function is dubious;
 * little, if anything at all, is gained in code size; probably makes
 * sense to use it only if areas pointed to from structures are 
 * freed and set to null.
 */

void free_item( void **ptr )
{
	if ( *ptr != 0L )
	{
		free(*ptr);
		*ptr = 0L;
	}
}


/*
 * An improved substitute for toupper();
 * This should work for characters above 127 as well.
 */

int touppc(int c)
{
	if ( (c & 0x7F) > '?' )
		return c & 0xDF;
	else
		return c;
}


/*
 * Set contents of a memory area to zeros.
 * Save some bytes in size when speed is not critical
 */

void *memclr(void *s, size_t len) 
{
	return memset(s, 0, len);
}