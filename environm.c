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

#include "desktop.h"
#include "environm.h"
#include "desk.h"

#ifdef __GNUC__
# define _BasPag _base
#endif


/*
 * Determine the length of the environment, including
 * two terminating zeros. Return this length of environment
 * (including two trailing zero bytes).
 */

long envlen(void)
{
	const char *p = _BasPag->p_env;
	long l = 0;

	do
	{
		while (*p++)
			l++;						/* count nonzero bytes */
		l++;							/* and a following zero byte */
	} while (*p);						/* while the next byte after a zero is nonzero */

	l++;								/* count the last trailing zero */

	return l;							/* return total length */
}


/*
 * Add new environment variable before or after the old ones.
 * where=1: add before old environment; where =2: add after old environment. 
 * Return NULL if memory allocation failed.
 * If size = 0, just copy the old environment.
 * This routine always creates an environment string,
 * be it only "\0\0" if nothing else exists.
 */

char *new_env(const char *newvar,		/* form: NEWVAR=VALUE */
			  size_t size,				/* size of above, INCLUDING the trailing zero byte */
			  int where,				/* 1: add at head; 2: add at tail */
			  size_t * newsize			/* new environment string size including two trailing zeros */
	)
{
	long l = envlen() - 1;				/* this will include one trailing zero */
	char *new;							/* allocated  space for the new enviro */
	char *newto;						/* where to put the old string */
	char *oldto;						/* where to put the new string */
	const char *p = _BasPag->p_env;		/* where is the old environment */

	*newsize = 0;

	/* Can this ever happen- that there is no global environment ? */

	if (!(p && l))
	{
		p = empty;
		l = 2;
	}

	/* Allocate space for the new environment */

	if ((new = malloc_chk(l + size + 4L)) != NULL)
	{
		oldto = newto = new;

		if (where == 1)
			oldto += size;				/* where to put the old string */
		else
			newto += l;					/* where to put the new string */

		memcpy(oldto, p, l);

		if (size)
			memcpy(newto, newvar, size);

		new[l + size] = '\0';			/* second trailing zero */

		*newsize = l + size + 1;
	}

	return new;
}


/*
 * Clear (unset) ARGV environmental variable in the environment string
 * for TeraDesk itself: put a 0 instead of "A" in "ARGV". As there is
 * already a 0 in front of "ARGV", now there will be two consecutive 0s
 * which will mean the end of the environment area (ARGV should always
 * be the last variable in the pool).
 * But is the program's environment string always allowed to write into ???
 */

void clr_argv(void)
{
	char *p;

	/* Find the location for the value of ARGV, then retrace until "ARGV=" found */

	if ((p = getenv("ARGV")) != NULL)
	{
		while (strstr(p, "ARGV=") == NULL)
		{
			p--;
		}

		*p = '\0';						/* Destroy ARGV, this is now the end of environment */
	}
}


/*
 * Build an environment string with an ARGV variable- or any other string.
 * If "program" is not NULL, it is assumed that ARGV is built.
 * ARGV environment is different in that there is a zero byte after "ARGV=",
 * and also a zero byte after each subsequent parameter.
 * Result: pointer to new environment or NULL if an error occured.
 * Also return final string size in "size"
 * Created environment string (and size) includes two trailing zeros.
 * Arguments containing spaces are expected to be quoted using either the
 * single-quote or the double-quote characters. Quotes in the string are
 * expected to be doubled.
 */

char *make_argv_env(const char *program,	/* program name */
					const char *cmdl,	/* command line or an environment string */
					size_t * size		/* resultant size of the string */
	)
{
	long argvl = 0;						/* Size of allocated space */
	char *envp;							/* String being built */
	const char *name = "ARGV=";			/* name of ARGV variable */
	const char *s;						/* current location in input strings */

	/* 
	 * Length of command line + two trailing zeros. If the command line
	 * contains quotes, this length will be longer than necessary
	 * because the quotes will in fact be removed from the string
	 */

	argvl = strlen(cmdl) + 2L;

	/* 
	 * Length of "ARGV=" + length of program name + zero byte after "="
	 * + zero byte after program name 
	 */

	if (program)
		argvl += strlen(program) + 7L;

	*size = 0;

	/* 
	 * Allocate new environment space. Note that if the command line
	 * contains quotes, this allocation may be somewhat larger than 
	 * necessary because quotes will be removed from the string
	 */

	if ((envp = malloc_chk(argvl)) != NULL)
	{
		char *d = envp;					/* current location in the string */

		if (program)
		{
			/* Add ARGV variable. */

			s = name;

			while (*s)
			{
				*d++ = *s++;
			}

			*d++ = 0;					/* Delimiting zero  after ARGV= */


			/* Add program name and a zero after it */

			s = program;

			while (*s)
			{
				*d++ = *s++;
			}

			*d++ = 0;
		}

		/* 
		 * Add the command line (or an environment string)
		 * Strip spaces and insert zeros instead, except if spaces are quoted.
		 * Note: quoted strings will be unquoted here: quotes will not be
		 * passed because, in ARGV, arguments -can- contain spaces. 
		 * Code below attempts to parse a possible mixture of single-
		 * or double quotes
		 */

		d = strcpyuq(d, cmdl);

		*d = 0;							/* second trailing zero */

		/* This is the actual length of the environment string */

		*size = (d - envp + 1L);
	}

	return envp;
}
