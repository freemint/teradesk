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

/* Load list of defined  masks (or document types) from configuration file */

/* int ft_load(XFILE *file) DjV 047 260403 */
int ft_load(XFILE *file, FTYPE **list)
{
	/* SNAME name;			/* HR 240203 */ DjV 046 190503 */
	int error;

	/* rem_all_filetypes(); DjV 046 120503 */
	rem_all( (LSTYPE **)list, rem); /* DjV 046 120503 */

	do
	{
		/* if (x_freadstr(file, name, sizeof(name), &error) == NULL) DjV 046 190503 */		/* HR 240103: max l */
		if (x_freadstr(file, fwork.filetype, sizeof(fwork.filetype), &error) == NULL) /* DjV 046 190503 */
			return error;
		/* if ((strlen(name) != 0) && (add(name) == NULL)) DjV 046 210403 */
		if (   strlen(fwork.filetype) != 0
		    && lsadd(  (LSTYPE **)list,
		    			sizeof(FTYPE),
		                (LSTYPE *)(&fwork),
		                END,
		                copy_ftype)    == NULL   /* DjV 046 210403 */
		   )
			return ERROR;
	}
	/* while (strlen(name) != 0); DjV 046 190503 */
	while (strlen(fwork.filetype) != 0); /* DjV 046 190503 */

	return 0;
}
