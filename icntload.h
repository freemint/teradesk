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

/* Load the list of icon-to-filetype assignments from config file */

static int load_list(XFILE *file, ICONTYPE **list)
{
	ITYPE it;

	/* DjV 046 100503 ---vvv--- */
	/*
	SNAME name;
		  icon_name;			/* HR 240203 */ 
	*/
	/* DjV 046 100503 ---^^^--- */

	long n;
	int error;

	/* free_list(list); DjV 047 280403 */
	rem_all((LSTYPE **)list, rem); /* DjV 047 280403 */

	do
	{
		if ((n = x_fread(file, &it, sizeof(ITYPE))) != sizeof(ITYPE))
			return (n < 0) ? (int) n : EEOF;

		if (it.icon != -1)
		{
			/* DjV 046 050503 ---vvv--- */
			/*
			int icon;
			if (x_freadstr(file, name, sizeof(name), &error) == NULL)		/* HR 240103: max l */
				return error;
			if (x_freadstr(file, icon_name, sizeof(icon_name), &error) == NULL)		/* HR 240103: max l */
				return error;

			icon = rsrc_icon(icon_name);		/* HR 151102: find icons by name, not index */
			if (icon >= 0)
				if (add(list, name, icon, END) == NULL)
			*/
			if (x_freadstr(file, iwork.type, sizeof(iwork.type), &error) == NULL)		/* HR 240103: max l */
				return error;
			if (x_freadstr(file, iwork.icon_name, sizeof(iwork.icon_name), &error) == NULL)		/* HR 240103: max l */
				return error;

			iwork.icon = rsrc_icon(iwork.icon_name);		

			/* Ignore icons which do not exist (iwork.icon = -1) */

			if (iwork.icon >= 0)
				if (lsadd(	(LSTYPE **)list,
							sizeof(ICONTYPE),
							(LSTYPE *)&iwork,
							END,
							copy_icntype     ) == NULL
				   ) 							/* DjV 047 040503 */
			/* DjV 046 050503 ---^^^--- */
					return ERROR;
		}
	}
	while (it.icon != -1);
	return 0;
}


/* Load lists of assigned icons (for files and foledrs */

int icnt_load(XFILE *file)
{
	int error;

	if ((error = load_list(file, &files)) < 0)
		return error;
	return load_list(file, &folders);
}
