/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2013  Dj. Vukovic
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */


#include <library.h>
#include <xdialog.h>
#include <time.h>
#include <xscncode.h>
#include <fcntl.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h"
#include "file.h"
#include "copy.h"
#include "dir.h"
#include "events.h"
#include "printer.h"
#include "viewer.h"
#include "lists.h"
#include "slider.h"
#include "icon.h"

#define PBUFSIZ	1024L					/* Should be divisible by 16 !!! */

#define PTIMEOUT 2000					/* 2000 * 1/200s = 10s printer timeout */


XATTR pattr;							/* item attributes */

XFILE *printfile = NULL;				/* print file; if NULL print to port */

_WORD printmode;						/* text, hex, raw */


/*
 * Print a character through GEMDOS.
 */

static bool prtchar(char ch)
{
	long prttime;
	_WORD button, error;
	bool ready = FALSE;
	bool result = FALSE;
	char s;

	if (printfile)
	{
		s = ch;
		error = (_WORD) x_fwrite(printfile, &s, 1L);

		if (error < 1)
		{
			xform_error(error);
			result = TRUE;
		}
	} else
	{
		do
		{
			prttime = clock() + PTIMEOUT;

			while (clock() < prttime && Cprnos() == 0)
				(void) Syield();

			if (Cprnos() != 0)
			{
				(void) Cprnout(ch);
				result = FALSE;
				ready = TRUE;
			} else
			{
				button = alert_printf(2, APRNRESP);
				result = TRUE;
				ready = (button == 2);
			}
		} while (!ready);

	}

	return result;
}


/* 
 * print_eol() prints CR LF for end of line;
 * same as prtchar above, it returns FALSE when OK!
 */

static bool print_eol(void)
{
	bool status = FALSE;

	if ((status = prtchar((char) 13)) == FALSE)	/* CR */
		status = prtchar((char) 10);	/* LF */

	return status;
}


/* 
 * print_line prints a CR-LF terminated line for directory or hex-dump print 
 * If the line is longer than plinelen it is wrapped to the next printer line.
 * Function returns FALSE if ok, in style with other print functions
 */

static bool print_line(const char *dline	/* 0-terminated line to print */
	)
{
	const char *p = dline;				/* address of position in dline string */
	_WORD i = 0;						/* position in printer line */
	bool status = FALSE;				/* prtchar print status */

	while (!status && (*p != 0))
	{
		status = prtchar(*p);			/* beware: prtchar is false when OK ! */
		p++;							/* pointer to a char in the buffer */
		i++;							/* print line length */

		if (!status && ((*p == 0) || (i >= options.plinelen)))	/* end of line or line too long */
		{
			i = 0;						/* reset linelength counter */
			status = print_eol();		/* print CR-LF */
		}								/* if...    */
	}									/* while... */

	return status;						/* this one is FALSE if ok, too! */
}


/*
 * Print a complete file. 
 * The file is read PBUFSIZ characters at a time.
 * Return 0 if successfull, error code otherwise.
 */

static _WORD print_file(WINDOW *w,		/* ponter to window in which the item has been selected */
						_WORD item		/* item index in the window */
	)
{
	long l;								/* index in buffer[] */
	char *buffer;						/* file is read into this */
	char *name;
	_WORD handle;
	_WORD i;
	_WORD error = 0;
	_WORD ll = 0;	/* line length counter */
	_WORD result = 0;
	bool stop = FALSE;

	if ((name = itm_fullname(w, item)) == NULL)
		return XFATAL;

	/* Print a header here */

	if (options.cprefs & P_HEADER)
	{
		if ((stop = print_line(name)) == FALSE)	/* print header */
			stop = print_eol();			/* print blank line */

		if (stop)
		{
			free(name);
			return XFATAL;
		}
	}

	/* Now print the file itself */

	buffer = malloc_chk(PBUFSIZ + 1L);

	if (buffer != NULL)
	{
		hourglass_mouse();

		if ((handle = x_open(name, O_DENYW | O_RDONLY)) >= 0)
		{
			long size = 0;
			long a = 0;

			do
			{
				/* Read no more than PBUFSIZ bytes (divisible by 16) */

				if ((l = x_read(handle, PBUFSIZ, buffer)) >= 0)
				{
					buffer[l] = 0;

					if (printmode == PM_HEX)
					{
						char tmp[HEXLEN + 2];

						ll = 0;

						size = size + l;

						for (i = 0; i < (((_WORD) l - 1) / 16 + 1); i++)
						{
							disp_hex(tmp, &buffer[ll], a, size, TRUE);

							if ((stop = print_line((const char *) (&tmp))) == TRUE)
								break;

							ll += 16;
							a += 16;
						}
					} else
					{
						char *buffi = buffer;

						for (i = 0; i < (_WORD) l; i++)
						{
							/* line wrap & new line handling */

							if (printmode == PM_TXT)	/* line wrap in text mode */
							{
								ll++;

								if ((*buffi == (char) 13) || (*buffi == (char) 10) || (*buffi == (char) 12))
									ll = 0;	/* reset linelength counter at CR, LF or FF */
								else if (ll >= options.plinelen)
								{
									ll = 0;

									if ((stop = print_eol()) == TRUE)
										break;
								}
							}

							if ((stop = prtchar(*buffi)) == TRUE)
								break;

							buffi++;
						}
					}

					/* Note: AV and termination messages will be processed here */

					if (escape_abort(TRUE))
						stop = TRUE;
				} else
				{
					error = (_WORD) l;
				}
			} while ((l == PBUFSIZ) && (stop == FALSE));

			x_close(handle);

			if (printmode != PM_RAW)
				stop = print_eol();		/* print CR-LF at end of file */
		} else
			error = handle;

		if (stop)
			result = XABORT;

		if (error != 0)
			result = xhndl_error(MEPRINT, error, itm_name(w, item));

		arrow_mouse();
		free(buffer);
	} else
		result = XFATAL;

	free(name);

	/* A formfeed at the end */

	if (options.cprefs & P_HEADER)
		if (prtchar((char) 12))
			return XFATAL;

	return result;
}


/*
 * Check if an item can be printed, depending on item type.
 * Drives, folders, network objects or unknown objects can not
 * be printed.
 * Note: parameter 'list' is locally modified
 */

bool check_print(WINDOW *w,				/* poiner to window in which items have been selected */
				 _WORD n,				/* number of selected items */
				 _WORD *list			/* list of item indices */
	)
{
	_WORD mes, i;
	ITMTYPE type;

	for (i = 0; i < n; i++)
	{
		mes = 0;
		type = itm_type(w, *list);

		switch (type)
		{
		case ITM_DRIVE:
			{
				mes = MDRIVE;
				break;
			}
		case ITM_FOLDER:
		case ITM_PREVDIR:
			{
				mes = MFOLDER;
				break;
			}
		default:
			{
				mes = trash_or_print(type);
				break;
			}
		}

		if (mes)
		{
			alert_cantdo(mes, MNOPRINT);
			return FALSE;
		}

		list++;
	}

	return TRUE;
}


/* 
 * Print a list of items selected in a window, or print a directory of an
 * open directory window. This routine currently does -not- recurse into
 * subdirectories, but prints only files in the current directory level.
 * Directory printout is performed by using lines as displayed in windows, 
 * and so sorting and display of diverse information is the same as for the 
 * window the directory of which is being printed.
 */

bool print_list(WINDOW *w,				/* pointer to window in which itemsh have been selected */
				_WORD n,				/* number of seleced items */
				_WORD *list,			/* list of item indices */
				long *folders,			/* count of selected files */
				long *files,			/* count of selected files */
				LSUM * bytes,			/* total size of selected items */
				_WORD function			/* operation code: CMD_PRINT / CMD_PRINTDIR */
	)
{
	XATTR attr;							/* Enhanced file attributes information */
	XLNAME dline;						/* sufficiently long string for a complete directory line */
	char *path;							/* Item's path */
	const char *name;					/* Item's name */
	_WORD *item;						/* (pointer to) item index */
	_WORD i;							/* counter */
	_WORD amode;						/* attribute finding mode; 0= follow links */
	_WORD error;						/* error code */
	_WORD result;						/* TRUE if operation successful */
	ITMTYPE type;						/* item type (file/folder...) */
	ITMTYPE tgttype;						/* link target type */
	bool printerror = FALSE;			/* true if there is an error in printing */

	/* If this is a directory printout, then maybe print direcory title */

	if (function == CMD_PRINTDIR && (options.cprefs & P_HEADER))
	{
		strcpy(dline, get_freestring(TDIROF));	/* Get "Directory of " string */
		strcat(dline, ((DIR_WINDOW *) w)->title);	/* Append window title */

		if ((printerror = print_line(dline)) == FALSE)
			printerror = print_eol();
	}

	if (printerror)
		return FALSE;

	/* Repeat print or dir-line print operation for each item in the list */

	item = list;

	for (i = 0; i < n; i++)
	{
		if (*item >= 0)
		{
			name = itm_name(w, *item);

			if ((path = itm_fullname(w, *item)) == NULL)
			{
				result = copy_error(ENSMEM, name, function);
			} else
			{
				type = itm_type(w, *item);
				tgttype = itm_tgttype(w, *item);
				amode = 0;

				/* Dont follow links for directory printout or network items */

				if (function == CMD_PRINTDIR || tgttype == ITM_NETOB)
					amode = 1;

				/* Now do whatever is needed to "print" an item */

				if ((error = itm_attrib(w, *item, amode, &attr)) == 0)	/* follow links */
				{
					if (function == CMD_PRINT)
					{
						/* 
						 * Only files can be printed, ignore everything else. 
						 * Executable files (programs) are hex-dumped.
						 */

						*folders = 0;

						if (isfileprog(type))
						{
							_WORD oldmode = printmode;

							result = 0;

							if (tgttype != ITM_NETOB)
							{
								if (type == ITM_PROGRAM && printmode == PM_TXT)
									printmode = PM_HEX;	/* hex-dump */

								upd_copyname(NULL, NULL, name);
								result = print_file(w, *item);

								printmode = oldmode;
							}

							*files -= 1;
							sub_size(bytes, attr.st_size);

							upd_copyname(NULL, NULL, empty);
						}
					} else
					{
						/* Printing of a directory line (all kinds of items) */

						dir_line((DIR_WINDOW *) w, dline, *item);

						if (dline[1] == (char) 7)
						{
							dline[1] = '\\';	/* Mark folders with "\" */
							*folders -= 1;
						} else
						{
							*files -= 1;
							sub_size(bytes, attr.st_size);
						}

						printerror = print_line(dline);

						if (escape_abort(cfdial_open))
							printerror = TRUE;

					}

				} else
				{
					result = copy_error(error, name, function);
				}
				free(path);

				/* If something is wrong, get out of the loop */

				if (printerror)
					result = XFATAL;

				/* Update information on the number of folders/files/bytes remaining */

				upd_copyinfo(*folders, *files, bytes);
			}

			/* Check for user abort (ESC key pressed) */

			check_opabort(&result);

			if (mustabort(result))
				break;
		}

		item++;
	}

	/* Print directory summary, if needed */

	if (!mustabort(result) && (function == CMD_PRINTDIR) && (options.cprefs & P_HEADER))
	{
		strcpy(dline, ((DIR_WINDOW *) w)->info);

		if ((printerror = print_eol()) == FALSE)	/* print blank line */
			if ((printerror = print_line(dline)) == FALSE)	/* print directory total */
				if ((printerror = print_eol()) == FALSE)	/* print blank line */
					printerror = prtchar(12);	/* print formfeed */
	}

	return (result == XFATAL || printerror) ? FALSE : TRUE;
}
