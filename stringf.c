/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2013  Dj. Vukovic
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

#include "desktop.h"
#include "desk.h"
#include "error.h"

#if defined(__GNUC__) && (__GNUC__ >= 6)
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#endif


/*
 * Duplicate 's', returning an identical malloc'd string.
 * This is an adjusted replacement for the library routine:
 * it displays an alert if memory can not be allocated.
 * Also, if the source is NULL, NULL is returned.
 */

char *strdup(const char *s)
{
	char *new = NULL;

	if(s != NULL)
	{
		size_t ls = strlen(s) + 1;

		if ((new = malloc_chk(ls)) != NULL)
			memcpy(new, s, ls);
	}

	return new;
}


/*
 * "Write" into a string. A substitute for the library function.
 * Note: This routine is able to perform only basic formatting,
 * but sufficient for the needs of TeraDesk.
 * Recognized formats are -ONLY- : %d %x %ld %lx %s.
 * Maximum width of NUMERIC output should not exceed 15 characters.
 * Maximum width of TEXT output should not exceed 255 characters.
 * Width specifier is supported but in a simplified form- specifiers 
 * like %nn are ok but not %n.n; Output is a null-terminated string.
 */

int vsprintf(char *buffer, const char *format, va_list argpoint)
{
	const char *s;	/* pointer to a location in input format */ 
	char *d;		/* pointer to a location in output buffer */ 
	char *h;		/* pointer to a location in input string */ 
	char fill;		/* padding character */
	char tmp[16];	/* temporary buffer */

	bool 
		lng, 		/* true if a numeric variable is of a long type */
		ready;		/* true when conversionn is finished */

	int 
		radix,		/* decimal or hexadecimal base for numeric output */
		maxl,		/* maximum output string length */ 
		ls,			/* string length */
		i;			/* counter */


	s = format;
	d = buffer;

	while (*s)
	{
		if (*s == '%')
		{
			/* Beginning of a format specifier detected... */

			s++;
			lng = ready = FALSE;
			maxl = 0;
			radix = 10;
			fill = ' ';

			while (!ready)
			{
				/* What is next */

				switch (*s)
				{
					case 's':
					{
						/* alphanumeric string format */
	
						h = va_arg(argpoint, char *);
						goto copyit;
					}
					case 'l':
					{
						/* next numeric output will be of a 'long' variable */
	
						lng = TRUE;
						break;
					}
					case 'x':
					{
						/* override radix and fill for hexadecimal output */
						radix = 16;
						fill = '0';
					}
					case 'd':
					{
						/* decimal or hexadecimal numeric output */
	
						if ( lng )					
							ltoa(va_arg(argpoint, long), tmp, radix);
						else
							itoa(va_arg(argpoint, int), tmp, radix);
	
						h = tmp;
	
						copyit:;
	
						ls = (int)strlen(h);
	
						if(maxl == 0)
							maxl = ls;
	
						maxl = min(maxl, 255 - (int)(d - buffer)); 
	
						/* pad with zeros or blanks */
	
						i = maxl - ls;
						ls = 0;
	
						if (maxl && i) /* use maxl for d as well */
						{
							while (i--)
							{
								*d++ = fill;
								ls++;
							}
						}
	
						/* copy data to output */
	
						while (*h && (ls < maxl))
						{
							*d++ = *h++;
							ls++;
						}
	
						ready = TRUE;
						break;
					}
					default:
					{
						/* interpret length specifier if given */
	
						if (isdigit(*s))
							maxl = maxl * 10 + (int)(*s - '0');
						else
							ready = TRUE;
					}
				}
				s++;
			}
		}
		else
			*d++ = *s++;
	}

	*d = 0;	

	return (int)(d - buffer);
}


/*
 * Write into a string- substitute a standard function
 */

int sprintf(char *buffer, const char *format,...)
{
	int r;

	va_list argpoint;
	va_start(argpoint, format);
	r = vsprintf(buffer, format, argpoint);
	va_end(argpoint);

	return r;
}
