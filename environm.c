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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <boolean.h>

#include "environm.h"


/*
 * Determine the length of the environment, including
 * two terminating zeros. Return length of environment,
 * including two trailing zero bytes.
 */

long envlen(void)
{
	char *p = _BasPag->p_env;
	long l = 0;

	do
	{
		while (*p++)
			l++;		/* count nonzero bytes */
		l++;			/* and a following zero byte */
	}
	while (*p);			/* while the next byte after a zero is nonzero */

	l++;				/* count the last trailing zero */

	return l;			/* return total length */
}


/*
 * Find an environmental variable
 */

static char *findvar(const char *var)
{
	char *p;
	long l;
	boolean found = FALSE;

	l = strlen(var);
	p = _BasPag->p_env;

	/* 
	 * Find an "=" then retrace by "l" and compare name 
	 * Note: this may not work correctly with the variables
	 * which have the same trailing substrings (e.g. PERA= and OPERA=) !!!
	 */

	while ((*p) && (found == FALSE))
	{
		if ((p[l] == '=') && (strncmp(p, var, l) == 0))
			found = TRUE;
		else
			while (*p++);
	}
	return (found) ? p : NULL;
}


/*
 * Get the value of an environmental variable.
 * This routine is also able to find the value of 
 * (the 1st part of) ARGV, i.e. if the first character following 
 * a "=" is "\0", then the next character is pointed to.
 */

char *getenv(const char *var)
{
	char *p;

	/* Variable not found */

	if ((p = findvar(var)) == NULL)
		return NULL;

	/* Find the character following the next "=" */

	p = strchr(p, '=') + 1;

	if (*p == 0)
	{
		if (p[1] == 0)
			return p;
		if (strchr(p + 1, '=') == NULL)
			return p + 1;
	}
	return p;
}


/*
 * Add new environment variable before or after the old ones.
 * where=1: add before old environment; where =2: add after old environment. 
 * Return NULL if memory allocation failed.
 * If size = 0, just copy the old environment.
 * This routine always creates an environment string,
 * be it only "\0\0" if nothing else exists.
 */

char *new_env
(
	const char *newvar,	/* form: NEWVAR=VALUE */ 
	size_t size,	 	/* size of above, INCLUDING the trailing zero byte */
	int where,			/* 1: add at head; 2: add at tail */
	size_t *newsize		/* new environment string size including two trailing zeros */
)
{
	long l = envlen() - 1; /* this will include one trailing zero */
	char 
		*new,
		*newto,
		*oldto,
		*p = _BasPag->p_env;

	*newsize = 0;

	/* Can this ever happen- that there is no global environment ? */

	if (!p || !l)
	{
		p = "\0";
		l = 2;
	}

	/* Allocate space for the new environment */

	if ( (new = malloc(l + size + 4L)) != NULL )
	{
		oldto = newto = new;

		if ( where == 1 )
			oldto += size;
		else	
			newto += l;

		memcpy(oldto, p, l);
	
		if ( size )
			memcpy(newto, newvar, size);

		new[l + size] = '\0';		/* second trailing zero */

		*newsize = l + size + 1;
	}

	return new;
}


/*
 * Clear (unset) ARGV environmental variable: put a 0 instead of "A"
 */

void clr_argv(void)
{
	char *p;

	if ((p = findvar("ARGV")) != NULL)
		*p = 0;
}


/*
 * Build an environment string with an ARGV variable- or any other string.
 * If "program" is not NULL, it is assumed that ARGV is built.
 * ARGV environment is different in that there is a zero byte after "ARGV=",
 * and also a zero byte after each subsequent parameter.
 * Result: pointer to environment or NULL if an error occured.
 * Also return final string size in "size"
 * Created environment string (and size) includes two trailing zeros.
 */

char *make_argv_env(const char *program, const char *cmdline, size_t *size)
{
	long 
		argvl = 0;			/* Size of allocated space */

	char 
		h, 					/* Location in command line string */
		*d, 				/* Current location in output string */
		*envp;				/* String being built */

	const char
		*name = "ARGV=", 	/* name of ARGV variable */
		*s;					/* current location in input strings */

	boolean 
		q = FALSE;			/* quoting flag */

	
	argvl = strlen(cmdline) + 2L; 

	if ( program )
		argvl += sizeof(name) + strlen(program) + 1L;
	
	*size = 0;

	/* Allocate new environment space */

	if ((envp = malloc(argvl)) != NULL)
	{
		d = envp;

		if ( program )
		{
			/* Add ARGV variable. */

			s = name;
			while (*s)
				*d++ = *s++;
			*d++ = 0;				/* Delimiting zero  after ARGV= */

			/* Add program name. */

			s = program;
			while (*s)
				*d++ = *s++;
			*d++ = 0;
		}

		/* 
		 * Add the command line (or the environment string)
		 * Strip spaces and insert zeros except if spaces are quoted 
		 */

		s = cmdline;
		while ((h = *s++) != 0)
		{
			if ((h == ' ') && !q )
			{
				*d++ = 0;
				while (*s == ' ')
					s++;
			}
			else if (h == 34 || h == 39) /* 34= double quote, 39=single quote */
			{
				if (*s == 34 || *s == 39)
					*d++ = *s++;
				else
					q = !q;
			}
			else
				*d++ = h;
		}

		*d++ = 0; /* first trailing zero  */
		*d = 0;	  /* second trailing zero */

		*size = (d - envp + 1L);

	}

	return envp;
}