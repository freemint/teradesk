/*
 * Utility functions for Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren,
 *                                                     2002, 2003 H. Robbers,
 *                                                     2003, 2004 Dj. Vukovic
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


/*
 * Note: to save space, do not use these fonctions to substirute 
 * costructs like "if ( x > y ) x = y;" with "x = min(x,y);", etc.
 */


/*
 * Return the smaller of two integers
 */

int min(int x, int y)
{
	return (x < y) ? x : y;
}


/* 
 * Return the larger of two integers
 */

int max(int x, int y)
{
	return (x > y) ? x : y;
}


/*
 * Return an integer within limits
 */

int minmax(int lo, int i, int hi)
{
	if ( i < lo )
		return lo;
	else
	{
		if ( i < hi )
			return i;
		else
			return hi;
	}
}


/*
 * Return the smaler of two long integers
 */

long lmin(long x, long y)
{
	return (x < y) ? x : y;
}


/*
 * Return the larger of two long integers
 */

long lmax(long x, long y)
{
	return (x > y) ? x : y;
}


/*
 * Return an long integer within limits
 */

long lminmax(long lo, long i, long hi)
{
	if ( i < lo )
		return lo;
	else
	{
		if ( i < hi )
			return i;
		else
			return hi;
	}
}
