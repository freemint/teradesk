/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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


#include <np_aes.h>	
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <boolean.h>
#include <ctype.h>


/*
 * "Write" into a string. 
 * Note: This routine is able to perform only basic formatting,
 * but sufficient for the needs of TeraDesk.
 * Recognized formats are -ONLY- : %d %x %ld %lx %s.
 * Maximum width of NUMERIC output should not exceed 15 characters.
 * Width specifier is supported. Output is a null-terminated string.
 */

int vsprintf(char *buffer, const char *format, va_list argpoint)
{
	char 
		*s, 
		*d, 
		*h, 
		fill,
		tmp[16];

	boolean 
		lng, 
		ready;

	int 
		radix,
		maxl, 
		i;

	s = (char *) format;
	d = buffer;

	while (*s)
	{
		if (*s == '%')
		{
			s++;
			lng = ready = FALSE;
			maxl = 0;

			while (ready == FALSE)
			{
				switch (*s)
				{
				case 's':
					h = va_arg(argpoint, char *);

					i = 0;
					if (maxl == 0 || maxl > 255)
						maxl = 256 - (int)(d - buffer);	

					while ((h[i]) && (i < maxl))
						*d++ = h[i++];
					ready = TRUE;
					break;
				case 'l':
					lng = TRUE;
					break;
				case 'd':
				case 'x':
					if ( *s == 'x' )
					{
						radix = 16;
						fill = '0';
					}
					else
					{
						radix = 10;
						fill = ' ';
					}
					if ( lng )					
						ltoa(va_arg(argpoint, long), tmp, radix);
					else
						itoa(va_arg(argpoint, int), tmp, radix);

					h = tmp;
					i = (int)strlen(tmp);
					if (maxl && i < maxl) /* use maxl for d as well */
					{
						i = maxl - i;
						while (i--)
							*d++ = fill;
					}
					while (*h)
						*d++ = *h++;
					ready = TRUE;
					break;
				default:
					if (isdigit(*s))
						maxl = maxl * 10 + (int) (*s - '0');
					else
						ready = TRUE;
					break;
				}
				s++;
			}
		}
		else
			*d++ = *s++;
	}

	*d = 0;	

	return (int) (d - buffer);
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


int vaprintf( int def,const char *string,va_list argpoint )
{
	char s[256];

	vsprintf(s,string,argpoint);
	return form_alert(def,s); 
}


int aprintf( int def,const char *string, ... )
{
	va_list argpoint;
	int button;

	va_start(argpoint,string);
	button = vaprintf(def,string,argpoint);
	va_end(argpoint);

	return button;
}

