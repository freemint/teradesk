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
	int x;
	int y;
	int w;
	int h;
	WDFLAGS flags;
	int resvd;
} OLDSINFO1;


static int wd_type_load(XFILE *file, int wtype)
{
	int i, n;
	OLDSINFO1 sinfo;
	WINFO *tyw;
	long s;
	FDATA font;


	if ((s = x_fread(file, &font, sizeof(FDATA))) != sizeof(FDATA))
		return (s < 0) ? (int) s : EEOF;
	if ( wtype == TEXT_WIND )
		fnt_setfont(font.id, font.size, &txt_font);
	else
		fnt_setfont(font.id, font.size, &dir_font);

	n = MAXWINDOWS;

	for (i = 0; i < n; i++)
	{
		if ( wtype == TEXT_WIND )
			tyw = &textwindows[i];
		else
			tyw = &dirwindows[i];

		if ((s = x_fread(file, &sinfo, sizeof(OLDSINFO1))) != sizeof(OLDSINFO1))
			return (s < 0) ? (int) s : EEOF;

		tyw->x = min(sinfo.x, screen_info.dsk.w - 32); 
		tyw->y = min(sinfo.y, screen_info.dsk.h - 32); /* maybe better 16pxl instead of 32 ?*/

		tyw->flags = sinfo.flags;
		tyw->w = sinfo.w;
		tyw->h = sinfo.h;
	}
	return 0;
}
