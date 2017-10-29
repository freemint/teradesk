/*
 * Utility functions for Teradesk. Copyright (c) 1993 - 2002 W. Klaren,
 *                                               2002 - 2003 H. Robbers,
 *                                               2003 - 2007 Dj. Vukovic
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
#include <library.h>
#include <tos.h>

/*
 * Convert a number (range 0:99 only!!!) into a string
 * two characters long. (leading zeros shown). 
 * Return a poiner to the byte after the end of string.
 * Attention: no termination 0 byte.
 */

char *digit(char *s, _WORD x)
{
	char c;
	
	x = x % 100;
	c = *s++ = x / 10;
	*s++ = x - 10 * c + '0'; /* is this faster than % ? */
	s[-2] += '0';
	return s;
}


/* 
 * Ring a bell
 */

void bell(void)
{
	Bconout(2, 7);
}


/*
 * An improved substitute for toupper();
 * This should work for character ASCII codes above 127 as well.
 * For a certain range of characters it returns characters
 * with ASCII codes shifted down (or left) by 32.
 */

_WORD touppc(_WORD c)
{
	if ( (c & 0x7F) > '?' && c != 0x7F)
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
