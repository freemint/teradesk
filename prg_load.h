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

int prg_load(XFILE *file)
{
	OLDSINFO pt;
	/* DjV 047 010503 ---vvv--- */
	/*
	SNAME name;				/* HR 240203 */
	boolean path;
	ApplType type;
	*/
	long lim;
	/* DjV 047 010503 ---^^^--- */
	long n;
	int error;

	rem_all_prgtypes();

	do
	{
		if ((n = x_fread(file, &pt, sizeof(OLDSINFO))) != sizeof(OLDSINFO))
			return (n < 0) ? (int) n : EEOF;

		if (pt.appl_type != -1)
		{
			/* DjV 047 010503 ---vvv--- */
			/*
			if (x_freadstr(file, name, sizeof(name), &error) == NULL)	/* HR 240103: max l */
				return error;

			type = (ApplType) pt.appl_type;
			path = (boolean) pt.path;

			if (x_freadstr(file, (char *)(&p.name), sizeof(p.name), &error) == NULL)
			*/
			/* DjV 047 010503 ---^^^--- */

			/* DjV 050 170503 ---vvv--- NEW CFG */
			if ( options.version >= 0x0300 ) /* From TeraDesk 3 on */
			{
				if ((n = x_fread(file, &lim, sizeof(lim))) != sizeof(lim))
				return (n < 0) ? (int) n : EEOF;
			}
			else
				lim = 0L;
			/* DjV 050 170503 ---^^^--- */

			/* DjV 047 010503 ---vvv--- */

			if (x_freadstr(file, (char *)(&pwork.name), sizeof(pwork.name), &error) == NULL) 
				return error;

			pwork.appl_type = (ApplType)pt.appl_type;
			pwork.argv = (boolean)pt.argv;
			pwork.path = (boolean)pt.path;
			pwork.single = (boolean)pt.single; 	/* NEW CFG */
			pwork.limmem = lim; /* NEW CFG */

			/* if (add(name, type, (boolean) pt.argv, path, END) == NULL) */

			if (lsadd(  (LSTYPE **)(&prgtypes), 
			            sizeof(PRGTYPE), 
			            (LSTYPE *)&pwork, 
			            END, 
			            copy_prgtype
					 ) == NULL)
				return ERROR;

			/* DjV 047 010503 ---^^^--- */

		}
	}
	while (pt.appl_type != -1);

	return 0;
}

