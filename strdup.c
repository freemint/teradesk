/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                                     2003  Dj. Vukovic
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
#include <string.h>

#include "desk.h"
#include "error.h"

/*
 * Duplicate 's', returning an identical malloc'd string.
 * This is a replacement for a library routine; this one
 * displays an alert if memory can not be allocated
 */

char *strdup(const char *s)
{
	size_t l;
	char *new;

	l = strlen(s) + 1;

	if ((new = malloc(l)) != NULL)
		memcpy(new, s, l);
	else
		xform_error(ENSMEM);

	return new;
}