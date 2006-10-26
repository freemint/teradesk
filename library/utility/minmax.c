/*
 * Utility functions for Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren,
 *                                                     2002, 2003 H. Robbers,
 *                                               2003, 2004, 2006 Dj. Vukovic
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
 * Note: to save space, do not use these functions to substitute 
 * costructs like "if ( x > y ) x = y;" with "x = min(x,y);", etc.
 * they are only useful if three variables are present, e.g. z = min(x, y)
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
 * Return an integer within limits.
 * It is somewhat problematic what to do when 
 * high limit is lower than low limit
 */

int minmax(int lo, int i, int hi)
{
/*
	if ( i < lo )
		return lo;
	else
		return(i < hi) ? i : hi;
*/
	if( i < lo )
		return lo;
	else
	{
		if(hi < lo)
			hi = lo;

		return(i < hi) ? i : hi;
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
 * Return a long integer within limits
 * It is somewhat problematic what to do when 
 * high limit is lower than low limit
 */

long lminmax(long lo, long i, long hi)
{
/*
	if ( i < lo )
		return lo;
	else
		return (i < hi) ? i : hi;
*/
	if( i < lo )
		return lo;
	else
	{
		if(hi < lo)
			hi = lo;

		return(i < hi) ? i : hi;
	}
}
