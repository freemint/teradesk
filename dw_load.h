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

typedef struct
{
	int index;
	int px;
	int py;
} OLDSINFO2;


int dir_load_window(XFILE *file)
{
	OLDSINFO2 sinfo;
	char *path, *fspec;
	int error;
	long n;

	if ((n = x_fread(file, &sinfo, sizeof(OLDSINFO2))) != sizeof(OLDSINFO2))
		return (n < 0) ? (int) n : EEOF;

	if ((path = x_freadstr(file, NULL, sizeof(LNAME), &error)) == NULL)		/* HR 240103: max l */ /* HR 240203 */
		return error;

	if ((fspec = x_freadstr(file, NULL, sizeof(LNAME), &error)) == NULL)		/* HR 240103: max l */ /* HR 240203 */
	{
		free(path);
		return error;
	}

	if (dir_do_open(&dirwindows[sinfo.index], path, fspec, sinfo.px, sinfo.py, &error) == NULL)
	{
		if ((error == EPTHNF) || (error == EFILNF) || (error == ERROR))
			return 0;
		return error;
	}

	return 0;
}

