/*
 * Utility functions for Teradesk. Copyright 1993, 2002 W. Klaren.
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

#include <aes.h>
#include <stdio.h>
#include <stdarg.h>
#include <library.h>

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
