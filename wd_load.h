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

/* Load configuration of directory windows, text windows and open windows */

int wd_load(XFILE *file)
{
	int error;

	/* Positions of all dir windows and text windows */

	if ((error = wd_type_load(file, DIR_WIND)) < 0)		/* DjV 041 210303 */
		return error;
	if ((error = wd_type_load(file, TEXT_WIND)) < 0)	/* DjV 041 210303 */
		return error;

	if ((error = edit_load(file)) < 0)
		return error;

	/* Configuration of open windows */

	if ((error = load_windows(file)) < 0)
		return error;

	return 0;
}