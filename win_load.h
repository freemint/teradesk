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

static int load_windows(XFILE *file)
{
	int type, error;
	long n;


	do
	{
		if ((n = x_fread(file, &type, sizeof(int))) != sizeof(int))
			 return (n < 0) ? (int) n : EEOF;

		switch (type)
		{
		case -1:
			error = 0;
			break;
		case DIR_WIND :
			error = dir_load_window(file);
			if ( !error )
			{
				menu_ienable(menu, MSHOWINF, TRUE); 
				menu_ienable(menu, MPRINT, TRUE);
			}
			break;
		case TEXT_WIND :
			error = txt_load_window(file);
			if ( !error )								/* DjV 041 280303 */ 
			{
				menu_ienable(menu, MSHOWINF, FALSE);	/* DjV 041 280303 */ 
				menu_ienable(menu, MPRINT, TRUE);		/* DjV 029 290703 */
			}
			break;
		}

		if (error < 0)
			return error;
	}
	while (type >= 0);

	return 0;
}
