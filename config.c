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
#include <fcntl.h>

#include "desktop.h"
#include "desk.h"
#include "error.h"
#include "stringf.h"
#include "xfilesys.h"
#include "config.h"
#include "file.h"
#include "font.h"						/* because of windows.h */
#include "window.h"



/*
 * Some variables for checkout of integrity of configuration files being read.
 *
 * lastnest: last detected keyword of a CFG_NEST entry; useful when reporting 
 * the location of an illegal record. Before starting to use CfgLoad, contents
 * of the file header should be copied to lastnest 
 *
 * chklevel: current nesting level in configuration file. Starts from 0 and
 * should return to 0 at the end of configuration file.
 */

static const char *cname;				/* name of curently open file */
static const char *lastnest = "?";		/* last remembered nesting keyword */
int chklevel = 0;						/* summary nest level */


/* 
 * End of line: either <CR> <LF> or <LF> only.
 * Using <CR><LF> is more in line with TOS standard, but can produce
 * significantly larger .INF file. Using only <LF> produces smaller
 * files, is in line with mint standards, but can create problems 
 * when read with some programs -including the Pure-C editor!!!
 */

#if 0
static const char eol[3] = { '\r', '\n', 0 };	/* <CR><LF> */
#endif
static const char eol[2] = { '\n', 0 };	/* <LF>     */


/* 
 * Substitute all "%" in a string with  "$"  
 */

static void no_percent(char *s)			/* pointer to the string to be modified */
{
	while (*s)
	{
		if (*s == '%')
			*s = '$';

		s++;
	}
}


/*
 * Append format and line-end strings to a keyword 
 * depending on table entry type, then copy to output string.
 */

static void append_fmt(CFG_TYPE cfgtype,	/* entry type         */
					   char *dest,		/* destination string */
					   const char *src	/* input string       */
	)
{
	CFG_TYPE cftype = cfgtype & CFG_MASK;

	if (src != NULL)
		strcpy(dest, src);

	/* CFG_NOFMT can be tested for here if it becomes needed */

	switch (cftype)
	{
	case CFG_HDR:
		strcat(dest, "=");
		break;
	case CFG_BEG:
		strcpy(dest, "{");
		break;
	case CFG_END:
		strcpy(dest, "}");
		break;
	case CFG_ENDG:
		strcpy(dest, "}");
		strcat(dest, eol);
		break;
	case CFG_FINAL:
		strcpy(dest, "end");
		strcat(dest, eol);
		break;
#if 0									/* currently not used in TeraDesk- but may be used some day */
	case CFG_B:
	case CFG_C:
		strcat(dest, "=%c");
		break;
	case CFG_BD:
		strcat(dest, "=%d");
		break;
	case CFG_H:
		strcat(dest, "=%u");
		break;
#endif
	case CFG_D:
		strcat(dest, "=%d");
		break;
	case CFG_DDD:
		strcat(dest, "=%d,%d,%d");
		break;
	case CFG_X:
		strcat(dest, "=0x%4x");
		break;
	case CFG_L:
		strcat(dest, "=%ld");
		break;
	case CFG_S:
		strcat(dest, "=%s");
		break;
	default:
		break;
	}

	strcat(dest, eol);
}


/*
 * Write something with a number of preceding tabs 
 * (lvl= number of tabs). Return error code or else
 * if everything is OK, return number of bytes written.
 */

static _WORD fprintf_wtab(XFILE *fp,	/* pointer to open file parameters */
						  int lvl,		/* number of tabs, equal to current nesting level */
						  char *string, ...	/* string(s) to print */
	)
{
	_WORD error = 0;
	char s[MAX_CFGLINE];
	va_list argpoint;

	/* Print a number of tab characters */

	while ((lvl-- > 0) && (error >= 0))
		error = (_WORD) x_fwrite(fp, "\t", sizeof(char));

	/* Print whatever else is specified */

	if (error >= 0)
	{
		/* Note: if positive, error contains string length */

		va_start(argpoint, string);
		error = vsprintf(s, string, argpoint);
		va_end(argpoint);

		if (error > 0)
			error = (_WORD) x_fwrite(fp, s, (long) error);
	}

	return error;
}


/* 
 * Save a part of configuration defined by one table.
 * Note: level is internally increased by one.  
 */

int CfgSave(XFILE *fp,					/* pointer to open file parameters */
			const CfgEntry *tab,		/* pointer to configuration table */
			int level0,					/* nesting (indent) level */
			bool emp					/* if true, write empty or zero-value fields */
	)
{
	int level = level0 + 1;
	int error = 0;
	char ts[MAX_CFGLINE];				/* temporary */
	char fmt[2 * MAX_KEYLEN];				/* "2 *" because of "end..." */
	CFG_TYPE tabtype;

	while (tab->type && (error >= 0))
	{
		int lvl = level;

		tabtype = tab->type & CFG_MASK;

		/* Append (or not) default formatting according to entry type */

		append_fmt(tab->type, fmt, tab->s);

		/* Now do output according to entry type */

		switch (tabtype)
		{
		case CFG_NEST:
			/* Go deeper, it is a nest, and all is specified */
			if ((tab->a.p != NULL) && (tab->s != NULL))
			{
				error = 0;
				(*tab->a.f) (fp, level, 1, &error);
			}
			break;

		case CFG_HDR:
			/* Write a nest header only if there is a keyword */
			if (tab->s != NULL)
				error = fprintf_wtab(fp, --lvl, fmt, tab->s);
			break;

		case CFG_BEG:
		case CFG_END:
		case CFG_ENDG:
			/* Change nesting level */
			lvl--;
			/* fall through */
		case CFG_FINAL:
			/* Write "end" to configuration file */
			error = fprintf_wtab(fp, lvl, fmt);
			break;

		default:
			if (!(tab->type & CFG_INHIB))
			{
				/* Write data according to type (check if data exist) */

				switch (tabtype)
				{
				case CFG_S:
					{
						/* Write string value */
						char *tp = ts;
						char *ss = tab->a.s;

						if (*ss || emp)
						{
							while (*ss)
							{
								if (*ss == '@')
									*tp++ = *ss;

								if (*ss == ' ')
									*ss = '@';

								*tp++ = *ss++;
							}

							*tp = 0;

							error = fprintf_wtab(fp, lvl, fmt, ts);
						}
					}
					break;
#if 0									/* currently not used */
				case CFG_C:
					{
						/* Write integer value (diverse formats) */
	
						unsigned char *v = tab->a.c;
	
						if (*v || emp)
							error = fprintf_wtab(fp, lvl, fmt, *v);
					}
					break;
				case CFG_BD:
					{
						/* Write integer value (diverse formats) */

						bool *v = tab->a.b;

						if (*v || emp)
							error = fprintf_wtab(fp, lvl, fmt, *v);
					}
					break;
#endif
				case CFG_D:
				case CFG_X:
					{
						/* Write integer value (diverse formats) */
						_UWORD *v = tab->a.u;

						if (*v || emp)
							error = fprintf_wtab(fp, lvl, fmt, *v);
					}
					break;
#if 0									/* Currently not used */
				case CFG_H:
				case CFG_B:
					{
						/* Write byte or unsigned char value */
						unsigned int v = *tab->a.c;

						if (v || emp)
							error = fprintf_wtab(fp, lvl, fmt, v);
					}
					break;
#endif
				case CFG_DDD:
					{
						/* Write a triplet of integers */
						_WORD *v = tab->a.i;

						if (v)
							error = fprintf_wtab(fp, lvl, fmt, v[0], v[1], v[2]);
					}
					break;
				case CFG_L:
					{
						/* Write a long value */
						long *v = tab->a.l;

						if (*v || emp)
							error = fprintf_wtab(fp, lvl, fmt, *v);
					}
					break;
				default:
					{
						/* Remove any format specifier and write as a string */

						no_percent(fmt);	/* safety check */
						error = fprintf_wtab(fp, lvl, fmt);

					}
					break;
				}
			}
			break;
		}
		tab++;
	}

	/* Positive return codes are not errors */

	if (error > 0)
		error = 0;

	return error;
}


/* 
 * Strip <lf>, <cr> or <cr><lf> from line end (insert null-characters there)
 */

static void crlf(char *f)
{
	int i;
	int j;

	for (j = 0; j < 2; j++)
	{
		i = (int) strlen(f) - 1;

		if ((i >= 0) && ((f[i] == '\n') || (f[i] == '\r')))
			f[i] = '\0';
	}
}


/*
 * Strip any comments from the end of a line (everything after ";")
 */

static char *nocomment(char *f)			/* pointer to string being modified */
{
	char *s = strchr(f, ';');

	if (s != NULL)
		*s = 0;

	return f;
}


/* 
 * Copy not more than "x" characters from "s" to "d",
 * until blank, tab, end of string or comment character (";") reached,
 * substituting all "@" with " " 
 */

static void cfgcpy(char *d, const char *s, int x)
{
	while (								/* loop until: */
			  x > 0						/* character count */
			  && *s != ' '				/* blank */
			  && *s != '\t'				/* tab */
			  && *s != ';'				/* comment */
			  && *s != 0				/* end of string */
		)
	{
		char c = *s;

		if (c == '@')
		{
			if (s[1] == c)
				s++;
			else
				c = ' ';
		}

		*d++ = c;
		s++;
		x--;
	}

	*d = 0;
}


/* 
 * Load a segment of configuration defined by one table.
 * Note: if an invalid record or value is encountered, the rest 
 * of the level is skipped in the hope that it will be possible to 
 * recover.
 * Note: level is internally increased by 1
 */

int CfgLoad(XFILE *fp,					/* pointer to file definition structure */
			const CfgEntry *cfgtab,	/* pointer to configuration table */
			int maxs,					/* maximum length of value string (after "="), incl. termination */
			int level0					/* nesting (indent) level */
	)
{
	int level = level0 + 1;				/* internally, level + 1 is used */
	int v;								/* aux, for type conversion */
	int error = 0;						/* error code */
	int tel = 0;						/* error counter */
	char r[MAX_CFGLINE];				/* string read from the file */
	const char *s;						/* pointer to a positon in the above */
	CFG_TYPE tabtype = 0;
	bool skip = FALSE;					/* true while recovering from errors */

	/* Loop while needed. Get next record from the file */

	while ((error = x_fgets(fp, r, (int) sizeof(r))) == 0)
	{
		const CfgEntry *tab = cfgtab;

		/* Strip line end and comment, then move to first nonblank */

		crlf(r);
		nocomment(r);
		s = r;
		s = nonwhite(s);

		/* Skip empty lines */

		if (*s == 0)
			continue;

		/* Check total nesting level in .inf file */

		if (*s == '{')					/* start of group */
		{
			chklevel++;
			continue;
		}

		if (*s == '}')					/* end of group; break from the loop */
		{
			chklevel--;

			if (level > chklevel)		/* made so for recovery from errors */
				break;
		}

		/* Check for the end of configuration file */

		if (strcmp(s, "end") == 0)		/* end of everything */
		{
			if (chklevel == 0)
				chklevel = -999;		/* "end" is where it should be */
			else
				error = EEOF;			/* "end" came too soon */
			break;
		}

		/* If recovery from error is in progress, skip all the rest */

		if (skip)
			continue;

		/* Loop through the table until finding the tab or the end */

		while ((tab->type != CFG_LAST) && (error >= 0))
		{
			tabtype = tab->type & CFG_MASK;

			/* 
			 * How long is the keyword? Search until "=" found 
			 * It is assumed that it starts with a nonblank.
			 * Note 1: no need to look for some entry types 
			 * (CFG_LAST, _HDR, _BEG, _END, _ENDG, _FINAL)
			 * ASSUMING they are the first ones in the list
			 * Note 2: those types do not have defined tab->s
			 * so further code will crash if attempted for those types
			 */

			v = 0;

			if (tabtype > CFG_FINAL)
			{
				while ((tab->s[v] > ' ') && (tab->s[v] != '='))
					v++;
			}

			/* Compare first "v" characters or string and keyword */

			if (v > 0 && strncmp(s, tab->s, v) == 0)
			{
				/* Match found; move to "=" */

				s = strchr(s, '=');

				/* If "=" was found, now move to first next nonblank */

				if (*s == '=')
					s = nonwhite(++s);

				/* Now do whatever is appropriate to interpret data */

				switch (tabtype)
				{
				case CFG_NEST:
					/* It is a nest, go one level deeper */
					/* also remember this keyword for possible error output */
					lastnest = tab->s;
					(*tab->a.f) (fp, level, 0, &error);

					if (error == EFRVAL)
					{
						alert_printf(1, AVALIDCF, lastnest);
						tel++;
						error = 0;
					}
					break;
				case CFG_S:
					/* Decode a string */
					cfgcpy(tab->a.s, s, maxs - 1);
					break;
#if 0									/* currently not used in Teradesk but may be used some day */
				case CFG_C:
					/* Interpret string as unsigned char */
					*tab->a.c = *s++;
					break;
				case CFG_B:
					/* Decode a character value */
					*tab->a.c = *s++;
					break;
				case CFG_H:
					/* Decode a positive decimal byte value */
					*tab->a.c = (char) max(atoi(s), 0);
					break;
				case CFG_BD:
					/* Decode a positive decimal integer value */
					*tab->a.b = max(atoi(s), 0) > 0;
					break;
#endif
				case CFG_D:
					/* Decode a positive decimal integer value */
					*tab->a.i = max(atoi(s), 0);
					break;
				case CFG_DDD:
					{
						/* Decode a triplet of positive decimal integer values */
						_WORD *vv = tab->a.i;

						const char *s2;

						s2 = --s;
						v = 0;

						do
						{
							*vv++ = atoi(++s2);
						} while ((s2 = strchr(s2, ',')) != NULL && ++v < 3);
					}
					break;
				case CFG_X:
					/* Decode a hex integer value */
					*tab->a.u = (_UWORD) strtol(s, NULL, 16);
					break;
				case CFG_L:
					/* Decode a positive decimal long int value */
					*tab->a.l = lmax(atol(s), 0L);
					break;
				default:
					break;
				}

				break;					/* break from further searching, because key found */
			}

			tab++;
		}

		/* 
		 * Increase error count if table entry not found 
		 * Note: currently this count is valid only within
		 * one nesting (not very good, the same error can repeat in
		 * other nests many times without an abort)
		 */

		if ((tabtype == CFG_LAST) || (tab->s == NULL))
		{
			tel++;
			alert_printf(1, AUNKRTYP, s, lastnest);
		}

		/* If enough errors found, skip to the end of this level */

		if (tel > 3)					/* Permit a maximum of three errors in one group */
		{
			alert_iprint(BADCFG);		/* may not be a config file at all */
			skip = TRUE;
		}

		/* 
		 * Has there been some fatal errors? Break from the loop
		 */

		if (error < 0)
			break;
	}

	/* 
	 * Ignore positive return values possibly generated somewhere;
	 * Also ignore end-of-file if it is complete
	 */

	if ((error > 0) || (error == EEOF && chklevel == -999))
		error = 0;

	/*
	 * If there has been too many errors, return status of invalid value read
	 */

	return (skip) ? EFRVAL : error;
}


/* 
 * Load or save a part of Teradesk configuration defined by one table.
 * In case of an error when loading, make default setup instead 
 */

int handle_cfg(XFILE *fp,				/* open file definition data */
			   CfgEntry *cfgtab,		/* pointer to configuration table used */
			   int level,				/* nesting level */
			   int emp,					/* if 0x0001 save empty fields, if 0x0002 skip all */
			   int io,					/* CFG_SAVE or 1= save data;  0= load data */
			   void *ini,				/* initial setup routine */
			   void *def				/* default setup routine */
	)
{
	void (*initial_setup) (void) = ini;
	void (*default_setup) (void) = def;
	int error = 0;

	if (io == CFG_SAVE)
	{
		/* 
		 * Save configuration. 
		 * If CFGEMP is set, always save. 
		 * Otherwise save only if CFGSKIP is NOT set
		 */

		if (emp != CFGSKIP)
			error = CfgSave(fp, cfgtab, level, (emp & 0x0001));
	} else
	{
		/* 
		 * Load configuration, but first do initial setup:
		 * usually clear all existing entries 
		 */

		if (ini)
			initial_setup();

		error = CfgLoad(fp, cfgtab, MAX_KEYLEN, level);

		if (error == EFRVAL && (def != NULL))
		{
			alert_printf(1, ALOADCFG, cname, get_message(error));

			/* 
			 * In case of error, again perform default setup 
			 * It always contains initial setup as well
			 */

			if (def)
				default_setup();
		}
	}

	return error;
}


/*
 * Handle complete reading or writing from/to a configuration
 * or palette file. Routine is supposed to correctly trace
 * nesting names up to one level of recursion (i.e. a palette file
 * created within creation of inf file).
 */

int handle_cfgfile(const char *name,	/* name of configuration file to read/write */
				   const CfgEntry *tab,	/* table which has to be handled */
				   const char *ident,	/* Identification header for this file */
				   int io				/* 1=save, 0=read */
	)
{
	XFILE *file;
	const char *savecname;
	const char *savelastnest;
	static char *fmt1 = "%s%s%s";
	_WORD n, error, savechklevel;

	/* Remember last nest name for error tracing */

	savelastnest = lastnest;
	savechklevel = chklevel;

	/* Find name only; if there is mint, name is in lowercase */

	savecname = cname;
	cname = fn_get_name(name);

	/* Proceed to load or save */

	if (io == CFG_SAVE)
	{
		if ((file = x_fopen(name, O_DENYRW | O_WRONLY, &error)) != NULL)
		{
			/* Write file identification header */

			error = fprintf_wtab(file, 0, fmt1, ident, eol, eol);

			/* Write the "don't edit" warning */

			error = fprintf_wtab(file, 0, fmt1, get_freestring(TDONTEDI), eol, eol);

			/* Write complete configuration (set level-1 here) */

			error = CfgSave(file, tab, -1, CFGEMP);

			if (((n = x_fclose(file)) < 0) && (error == 0))
				error = n;

			/* Update the window into which the file was written */

			wd_set_update(WD_UPD_COPIED, cname, NULL);
			wd_do_update();
		}
	} else
	{
		if ((file = x_fopen(name, O_DENYW | O_RDONLY, &error)) != NULL)
		{
			char identbuf[MAX_CFGLINE];

			if ((n = x_fgets(file, identbuf, MAX_CFGLINE - 1)) == 0)
			{
				lastnest = ident;
				chklevel = 0;

				/* 
				 * Check the file identifier header. If it is OK, 
				 * then read complete configuration (use level-1 here)
				 */

				if (strncmp(identbuf, ident, strlen(ident)) == 0)
					error = CfgLoad(file, tab, MAX_CFGLINE, -1);
				else
					error = EFRVAL;
			} else
			{
				if (n < 0)
					error = (int) n;
				else
					error = EEOF;		/* if no error, it is end of file */
			}

			x_fclose(file);
		}
	}

	/* Display error information */

	if ((error < 0) && (error != ENOMSG))
		alert_printf(1, ALOADCFG, cname, get_message(error));

	/* Restore previous filename, nest name and level if any */

	cname = savecname;
	lastnest = savelastnest;
	chklevel = savechklevel;

	return error;
}
