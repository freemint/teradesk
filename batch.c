/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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
#include <string.h>
#include <stdlib.h>
#include <mint.h>

#include "desk.h"
#include "desktop.h"
#include "error.h"
#include "xfilesys.h"
#include "batch.h"
#include "config.h"
#include "file.h"

#if _BATFILE

extern char
	*infname, 
	*optname;


/*
 * Check if this character is a part of a non-blank string.
 * return TRUE if it is -not-
 */

static boolean eos(char c)
{
	if ((c != ' ') && (c != '\t') && (c != 0))
		return FALSE;
	else
		return TRUE;
}


static char *skipchar(char *p)
{
	char *i = p;

	while (eos(*i) == FALSE)
		i++;

	return i;
}


/*
 * Read a decimal integer from a string.
 * Return pointer to a place after the string
 */

static char *getint(char *p, int *result)
{
	char *i = p;
	int r = 0;

	while ((*i >= '0') && (*i <= '9'))
		r = r * 10 + (int) (*i++ - '0');

	*result = r;

	return (i == p) ? NULL : i;
}


/*
 * Execute the startup batch file
 */

void exec_bat(char *name)
{
	char line[256], *p, *s, *com, *tail;
	char *olddir;
	XFILE *bf;
	int x, y, cnt = 0, error;
	COMMAND comline;
	boolean syntx;

	if ((olddir = getdir(&error)) != NULL)
	{
		if ((bf = x_fopen(name, O_DENYW | O_RDONLY, &error)) != NULL) 
		{
			/* Read startup file */

			while ((error = x_fgets(bf, line, 256)) == 0)
			{
				cnt++;

				/* move to first nonblank character */

				p = nonwhite(line);

				syntx = FALSE;

				switch (*p)
				{
				case 0:
				case '*':
					/* comment ? */
					break;
				case '#':
					/* Specify inf file for a screen resolution */
					p = nonwhite(++p);
					if ((p = getint(p, &x)) == NULL)
					{
						syntx = TRUE;
						break;
					}
					p = nonwhite(p);
					if (*p != ',')
					{
						syntx = TRUE;
						break;
					}
					p = nonwhite(++p);
					if ((p = getint(p, &y)) == NULL)
					{
						syntx = TRUE;
						break;
					}
					p = nonwhite(p);

					if ((x == max_w) && (y == max_h))
					{
						long l = 0;

						while (eos(p[l]) == FALSE)
							l++;

						if (l > 0)
						{
							if ((s = malloc(l + 1)) != NULL)
							{
#if TEXT_CFG_IN
								free(infname);
								infname = s;
#else
								free(optname);
								optname = s;
#endif
								strsncpy(s, p, l + 1);
								p += l;
							}
							else
								xform_error(ENSMEM);
						}
					}
					break;
				default:
					com = p;
					p = skipchar(p);
					if (*p != 0)
					{
						*p++ = 0;
						p = nonwhite(p);
						tail = p;
					}
					else
						tail = "";

					/* Check if this is a command to change directory */

					if (strcmp(com, "cd") != 0)
					{
#if _MINT_
						boolean bg;
						if (mint)
						{
							int i;
	
							i = (int) strlen(tail) - 1;
							while (((tail[i] == ' ') || (tail[i] == '\t')) && (i > 0))
								i--;
							if (tail[i] == '&')
							{
								bg = TRUE;
								tail[i] = 0;
							}
							else
								bg = FALSE;
						}
#endif
						strcpy(comline.command_tail, tail);
						comline.length = (unsigned char) strlen(tail);
#if _MINT_
						if (mint)
							error = (int) x_exec((bg == FALSE) ? 0 : 100, com, &comline, NULL);
						else
#endif
							error = (int) x_exec(0, com, &comline, NULL);
					}
					else
						error = chdir(tail);

					if (error != 0)
						hndl_error(AEBATCH, error);
					break;
				}
				if (syntx)
					alert_printf(1, ABSYNTAX, cnt);

			}
			x_fclose(bf);
		}

		chdir(olddir);
		free(olddir);
	}

	if ((error != 0) && (error != EEOF) && (error != EFILNF))
		hndl_error(AEBATCH, error);

}

#endif
