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

int dsk_load(XFILE *file)
{
	OLDICONINFO iconinfo;
	char *fname;
	long n;
	int error;

	rem_all_icons();

	if (options.version < 0x125)
		set_dsk_background((ncolors > 2) ? 7 : 4, 3);
	else
		set_dsk_background(options.dsk_pattern, options.dsk_color);

	do
	{
		if ((n = x_fread(file, &iconinfo, sizeof(OLDICONINFO))) != sizeof(OLDICONINFO))
			return (n < 0) ? (int) n : EEOF;

		if (iconinfo.type != 0)
		{
			int icon;
	
			if (isfile(iconinfo.type))
			{
				if ((fname = x_freadstr(file, NULL, sizeof(LNAME), &error)) == NULL)		/* HR 240103: max l */ /* HR 240203 */
					return error;
			}
			else
				fname = NULL;
			
			icon = rsrc_icon(iconinfo.icon_name);			/* HR 151102: find by name */
			if (icon < 0)
				icon = rsrc_icon_rscid ( FIINAME, iname );	/* DjV 024 140103 */ 
			if (icon >= 0)
				if (add_icon((ITMTYPE) iconinfo.type, icon,			/* HR 151102 */
							 iconinfo.label, iconinfo.drv + 'A', iconinfo.x,
							 iconinfo.y, FALSE, fname) < 0)
					return ERROR;
			
		}
	}
	while (iconinfo.type != 0);

	wind_set(0, WF_NEWDESK, desktop, 0);

	dsk_draw();

	return 0;
}
