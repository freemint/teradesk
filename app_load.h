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

/* Load applications definitions from config file */

typedef struct
{
	int appltype : 4;
	unsigned int path:1;
	unsigned int fkey:6;
	unsigned int argv:1;
	unsigned int single:1; /* DjV 047 040503 */
	int resvd1:3;          /* DjV 047 040503 changed 4 to 3 */
	int resvd2;
} OLDSINFO;


int app_load(XFILE *file)
{
	OLDSINFO appl;		/* flags for this application */
	char *name,		/* application name */ 
		*cmdline;	/* default command line for this application */
	long lim,		/* DjV 050 190503 */
		n;
	int error;		/* error code, if any */

	app_default(); /* clear the old applications list, if there is any */

	do
	{
		/* read application info flags */
		if ((n = x_fread(file, &appl, sizeof(OLDSINFO))) != sizeof(OLDSINFO))
			return (n < 0) ? (int) n : EEOF;

		if (appl.appltype != -1)
		{
			/* DjV 050 190503 ---vvv--- */

			/* Provide for reading of old confug files too, therefore "if" */

			if ( options.version >= 0x300 )
			{
				if ((n = x_fread(file, &lim, sizeof(lim))) != sizeof(lim))
				return (n < 0) ? (int) n : EEOF;
			}
			else
				lim = 0L;
			/* DjV 050 190503 ---^^^--- */

			/* read and malloc (inx_freadstr) application name */
			if ((name = x_freadstr(file, NULL, sizeof(LNAME), &error)) == NULL)		/* HR 240103: max l */ /* HR 240203 */
				return error;

			/* read and malloc command line for the application */
			if ((cmdline = x_freadstr(file, NULL, sizeof(LNAME), &error)) == NULL)		/* HR 240103: max l */ /* HR 240203 */
			{
				free(name);
				return error;
			}

			/* List of documenttypes is read and allocated here */
			awork.filetypes = NULL;

			if ( (error = ft_load(file, &(awork.filetypes) ) ) != 0 )
			{
				free(name);
				free(cmdline);
				return error;
			}

			/* Fill-in application info data */

			log_shortname( awork.shname, name );
			awork.name = name;
			awork.appltype = appl.appltype;
			awork.path = (boolean) appl.path;
			awork.fkey = appl.fkey;
			awork.argv = appl.argv;
			awork.single = appl.single;
			awork.limmem = lim;
			awork.edit = 0;	
			awork.autostart = 0;
			awork.cmdline = cmdline;

			/* add application to list */
			if ( lsadd( 
			             (LSTYPE **)(&applikations), 
			              sizeof(APPLINFO), 
			              (LSTYPE *)(&awork), 
			              END, 
			              copy_app 
			           ) == NULL 
			   ) /* DjV 047 040503 */
			{
				free(name);
				free(cmdline);
				rem_all((LSTYPE **)(&awork.filetypes), rem);
				return ENSMEM;
			}
		}
	}
	while (appl.appltype != -1);

	return 0;
}

