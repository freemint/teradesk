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

#if !TEXT_CFG_OUT

/* Save configuration of open windows */

static int save_windows(XFILE *file, boolean close)
{
	WINDOW *w;
	int dummy = -1, type, error;
	long n;

	w = xw_last();


	while (w != NULL)
	{
		switch(type = xw_type(w))
		{
		case DIR_WIND :
			if ((n = x_fwrite(file, &type, sizeof(int))) < 0)
				 return (int) n;
			if ((error = dir_save_window(file, w)) < 0)
				return error;
			if (close)
				dir_close(w, 1);
			break;
		case TEXT_WIND :
			if ((n = x_fwrite(file, &type, sizeof(int))) < 0)
				 return (int) n;
			if ((error = txt_save_window(file, w)) < 0)
				return error;
			if (close)
				txt_closed(w);
			break;
		default :
			if (close)
				return 1;
			break;
		}

		w = xw_prev();
	}

	return ((n = x_fwrite(file, &dummy, sizeof(int))) < 0) ? (int) n : 0;
}
#endif
