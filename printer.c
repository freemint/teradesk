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

#include <np_aes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tos.h>
#include <vdi.h>
#include <xdialog.h>
#include <xscncode.h>
#include <mint.h>

#include "desk.h"
#include "resource.h"
#include "printer.h"
#include "error.h"
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h"
#include "file.h"
#include "dir.h"
#include "events.h"
#include "copy.h"

#define PBUFSIZ	1024L


#define plinelen options.V2_2.plinelen

XATTR pattr;		/* item attributes */

static boolean prtchar(char ch)
{
	long time;
	boolean ready = FALSE, result;
	int button;

	do
	{
		time = clock() + 1000;
		while ((clock() < time) && (Cprnos() == 0));
		if (Cprnos() != 0)
		{
			Cprnout(ch);
			result = FALSE;
			ready = TRUE;
		}
		else
		{
			button = alert_printf(2, APRNRESP);
			result = TRUE;
			ready = (button == 2) ? TRUE : FALSE;
		}
	}
	while (ready == FALSE);

	return result;
}


/* 
 * DjV 029 150203 
 * print_eol() prints CR LF for end of line;
 * same as prtchar above, it returns FALSE when OK!
 */

static boolean print_eol(void)
{
	boolean status = FALSE;
	if ( ( status = prtchar( (char)13 )	) == FALSE )	/* CR */
		status = prtchar( (char)10 );					/* LF */
	return status;
}


/* 
 * print_line prints a cr-lf terminated line for directory print 
 * If the line is longer than plinelen it is wrapped to the next printer line.
 * Function returns FALSE if ok, in style with other print functions
 */

static boolean print_line 
( 
	const char *dline 	/* 0-terminated line to print */
)
{
	const char *p;					/* address of position in dline string */
	int i;						/* position in printer line */
	boolean status = FALSE;		/* prtchar print status */

	p = dline;
	i = 0;
	while ( !status && (*p != 0) )
	{
		status = prtchar(*p); 		/* beware: prtchar is false when OK ! */
		p++;
		i++;
		if ( !status && ((*p == 0) || (i >= plinelen)) ) /* end of line or line too long */
		{
			i = 0;					/* reset linelength counter */
			status = print_eol();	/* print cr lf */
		}	/* if...    */ 
	} 		/* while... */

	return status;					/* this one is FALSE if ok, too! */
}

static int print_file(WINDOW *w, int item)
{
	int handle, error = 0, i, result = 0;
	char *buffer;
	const char *name;
	long l;			/* index in buffer[] */
	int ll = 0;		/* line length counter */
	boolean stop = FALSE;

	if ((name = itm_fullname(w, item)) == NULL)
		return XFATAL;


	/* Print a header here */

	if ( options.cprefs & P_HEADER )
	{
		if ( (stop = print_line(name)) == FALSE )	/* print header */
			stop = print_eol();	  					/* print blank line */

		if (stop)
		{
			free(name);
			return XFATAL;
		}
	}

	buffer = malloc(PBUFSIZ);

	if (buffer != NULL)
	{
		graf_mouse(HOURGLASS, NULL);

		if ((handle = x_open(name, O_DENYW | O_RDONLY)) >= 0)
		{
			do
			{
				if ((l = x_read(handle, PBUFSIZ, buffer)) >= 0)
				{
					for (i = 0; i < (int) l; i++)
					{
						/*  DjV 031 150203
						 * line wrap & new line handling;
						 */
						ll++;
						if ( (buffer[i] == (char)13) || (buffer[i] == (char)10) || (buffer[i] == (char)12) )
							ll = 0; /* reset linelength counter at CR, LF or FF */
						else if ( ll >= plinelen )
						{
							ll = 0;
							if (( stop = print_eol() ) == TRUE)
								break;
						}

						if ((stop = prtchar(buffer[i])) == TRUE)
							break;
					}

					if ( escape_abort(TRUE) )
						stop = TRUE;
				}
				else
					error = (int) l;
			}
			while ((l == PBUFSIZ) && (stop == FALSE));

			x_close(handle);
			print_eol();			/* print cr lf at end of file */
		}
		else
			error = handle;

		if (stop == TRUE)
			result = XABORT;

		if (error != 0)
			result = xhndl_error(MEPRINT, error, itm_name(w, item));

		graf_mouse(ARROW, NULL);
		free(buffer);
	}
	else
	{
		xform_error(ENSMEM);
		result = XFATAL;
	}

	free(name);

	
	if ( options.cprefs & P_HEADER )
		if ( prtchar( (char)12 ) )
			return XFATAL;

	return result;
}


/*
 * Check if an item can be printed
 */

boolean check_print(WINDOW *w, int n, int *list)
{
	int i;
	boolean noerror;
	char *mes = "";


	for (i = 0; i < n; i++)
	{
		noerror = FALSE;

		switch (itm_type(w, list[i]))
		{
		case ITM_PRINTER:
			rsrc_gaddr(R_STRING, MPRINTER, &mes);
			break;
		case ITM_TRASH:
			rsrc_gaddr(R_STRING, MTRASHCN, &mes);
			break;
		case ITM_DRIVE:
			rsrc_gaddr(R_STRING, MDRIVE, &mes);
			break;
		case ITM_FOLDER:
			rsrc_gaddr(R_STRING, MFOLDER, &mes);
			break;
		case ITM_PROGRAM:
			rsrc_gaddr(R_STRING, MPROGRAM, &mes);
			break;
		case ITM_FILE:
			noerror = TRUE;
			break;
		}
		if (noerror == FALSE)
			break;
	}
	if (noerror == FALSE)
		alert_printf(1, ANOPRINT, mes);

	return noerror;
}


/* 
 * Similar to print_file() but returns TRUE if successful 
 */

boolean prt_file(WINDOW *w, int item)
{
	return (print_file(w, item) == 0) ? TRUE : FALSE;
}


/*
 * Routine item_print() didn't  work correctly, but printed only the first file!!!- 
 * seems like a part of the code (a loop) disappeared somehow, sometime; Anyway,
 * now replaced by a simpler routine print_list(), more in style with other 
 * file-list-dealing routines. So: item_print() removed.
 */ 
 

/* 
 * Print a list of items selected in a window, or print a directory. 
 * This routine currently does -not- recurse into subdirectories, 
 * but prints only files in the current directory level.
 * Directory printout is performed by using window lines, and so
 * sorting and display of diverse information is the same as for the 
 * directory window the directory of which is being printed.
 */

boolean print_list
( 
	WINDOW *w, 
	int n, 
	int *list, 
	long *folders, 
	long *files, 
	long *bytes,
	int function
)
{
	int 
		i,			/* counter */ 
		item,		/* item type (file/folder) */ 
		error,		/* error code */ 
		result;		/* TRUE if operation successful */

	ITMTYPE 
		type;		/* Item type (file/folder...) */

	const char 
		*path,		/* Item's path */ 
		*name;		/* Item's name */

	XATTR
		attr;		/* Enhanced file attributes information */

	char
		dline[256];	/* sufficiently long string for a complete directory line */

	boolean
		noerror = TRUE;		/* true if there is no error */


	/* If this is a directory printout, then maybe print direcory title */

	if ( function == CMD_PRINTDIR && (options.cprefs & P_HEADER) )
	{
		strcpy ( dline, get_freestring(TDIROF) ); 		/* Get "Directory of " string */
		get_dir_line( w, &dline[strlen(dline)], -1 );	/* Append window title */

		if ( (noerror = !print_line(dline) ) == TRUE )
			noerror = !print_eol();
	}

	if ( !noerror )
		return FALSE;

	/* Repeat print or dir-line print operation for each item in the list */

	for (i = 0; i < n; i++)
	{
		if ((item = list[i]) == -1)
			continue;

		name = itm_name(w, item);

		if ((path = itm_fullname(w, item)) == NULL)
			result = copy_error(ENSMEM, name, function);
		else
		{
			type = itm_type(w, item);
			if ((error = itm_attrib(w, item, 0, &attr)) == 0)
			{
				if ( function == CMD_PRINT )
				{
					/* Only files can be printed, ignore everything else */

					*folders = 0; 

					if ( type == ITM_FILE )
					{
						upd_copyname(NULL, NULL, name);

						result = print_file(w, list[i]);
						*bytes -= attr.size;
						*files -= 1;

						upd_copyname(NULL, NULL, "");
					}

				}
				else
				{
					/* Printing of a directory line (all kinds of items) */

					get_dir_line( w, dline, list[i] );

					if ( dline[1] == (char)7 )
					{
						dline[1] = '\\';	/* Mark folders with "\" */
						*folders -= 1;
					}
					else
					{
						*files -= 1;
						*bytes -= attr.size;
					}

					noerror = !print_line(dline);
					if ( escape_abort( cfdial_open ) )
					noerror = FALSE;

				} /*  function */

			}
			else
				result = copy_error(error, name, function);

			free(path);

			/* If something is wrong, get out of the loop */

			if ( !noerror )
				result = XFATAL;

			/* Update information on the number of folders/files/bytes remaining */

			upd_copyinfo(*folders, *files, *bytes);

		}

		/* Check for user abort */

		if (result != XFATAL)
		{
			if ( escape_abort(cfdial_open) )
				result = XABORT;
		}

		if ((result == XABORT) || (result == XFATAL))
			break;
	}


	/* Print directory summary, if needed */

	if ( result !=XABORT && result != XFATAL && (function == CMD_PRINTDIR) && (options.cprefs & P_HEADER) )
	{
 		get_dir_line ( w, dline, -2 );						/* get directory info line */
		if ( (noerror = !print_eol()) == TRUE )				/* print blank line */
			if ( ( noerror = !print_line(dline) ) == TRUE )	/* print directory total */
				if ( (noerror = !print_eol()) == TRUE )		/* print blank line */
					noerror = prtchar( (char)12 );			/* print formfeed */		
	}

	return ((result == XFATAL || !noerror) ? FALSE : TRUE);
}
