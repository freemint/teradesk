/*
 * Utility functions for Teradesk. Copyright 1993, 2002  W. Klaren
 *                                           2002, 2003  H. Robbers,
 *                                           2003, 2004  Dj. Vukovic
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
#include <stddef.h>
#include <library.h>


/* 
 * Concatenate a path+filename string "name" from a path "path"
 * and a filename "fname"; Add a "\" between the path and
 * the name if needed.
 * Space for resultant string has to be allocated before the call
 * Note: "name" and "path" can be at the same location
 */

void make_path( char *name, const char *path, const char *fname )
{
	long l;

	strcpy(name, path);
	l = strlen(name);
	if (l && (name[l - 1] != '\\'))
		name[l++] = '\\';
	strcpy(name + l, fname);
}


/*
 * Split a full (file)name (path+filename) into "path" and "fname" parts.
 * Space for path and fname has to be allocated beforehand.
 * Path is always finished with a "\"
 */

void split_path( char *path, char *fname, const char *name )
{
	char *backsl;

	backsl = strrchr(name,'\\');
	if (backsl == NULL)
	{
		*path = 0;
		strcpy(fname, name); /* no path, there is just filename */
	}
	else
	{
		strcpy(fname, backsl + 1);
		if (backsl == name)
			strcpy(path, "\\");
		else
		{
			long l = (backsl - (char *)name);

			strsncpy(path, name, l + 1);		/* secure copy */
			if ((l == 2) && (path[1] == ':'))
				strcat(path, "\\");
		}
	}
}
