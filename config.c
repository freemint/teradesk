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

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <mint.h>
#include <string.h>
#include <np_aes.h>
#include <library.h>

#include "boolean.h"
#include "desk.h"
#include "error.h"
#include "sprintf.h"
#include "xfilesys.h"
#include "desktop.h"
#include "config.h"
#include "file.h"

typedef enum
{
	WD_UPD_DELETED,
	WD_UPD_COPIED,
	WD_UPD_MOVED,
	WD_UPD_ALLWAYS
} wd_upd_type;

void wd_set_update(wd_upd_type type, const char *name1, const char *name2);
void wd_do_update(void);

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

static char
	*cname = "?";			/* name of curently open file */


char
	*lastnest = "?";		/* last remembered nesting keyword */


int 
	chklevel = 0;		/* summary nest level */


/* 
 * End of line: either <cr> <lf> or <lf> only.
 * Using <cr><lf> is more in line with TOS standard, but can produce
 * significantly larger inf file. Using <lf> only produces smaller
 * files, is in line with mint standards, but can create problems 
 * when read with some programs -including Pure-C editor!!!
 */

char 
/*
	eol[3] = {'\r','\n', 0};
*/
	eol[3] = {'\n', 0, 0};

/* 
 * Substitute all "%" in a string with  "$" ?? 
 */

static boolean no_percent(char *s)
{
	bool perc = false;

	while (*s)
	{
		perc = (*s == '%');
		if (perc)
			*s = '$';
		s++;
	}
	return perc;
}

 
/*
 * Append format and lineend strings to a keyword 
 * depending on table entry type, then copy to output string.
 */

static void append_fmt
(
	CFG_TYPE cfgtype,	/* entry type         */
	char flag,			/* if CFG_NOFMT, don't add anything, just copy */
	char *dest,			/* destination string */
	char *src			/* input string       */
)
{

	if ( src != NULL )
{
		strcpy(dest, src);
}

	if ( (flag & CFG_NOFMT) == 0 )
	{
		switch(cfgtype)
		{
			case CFG_HDR:
				strcat(dest,"=");
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
/* currently not used in Teradesk- but may be used later 
			case CFG_B:
			case CFG_C:
				strcat(dest, "=%c");
				break;
*/
			case CFG_BD:
			case CFG_D:
			case CFG_H:
				strcat(dest, "=%d");
				break;
			case CFG_DDD:
				strcat(dest,"=%d,%d,%d");
				break;
			case CFG_X:
				strcat(dest, "=0x%4x");
				break;
			case CFG_L:
				strcat(dest, "=%ld");
				break;
			case CFG_S:
				strcat(dest, "=%s");
			default:
				break;
		}
		strcat( dest, eol );
	}
}


/*
 * Write something with a number of preceding tabs 
 * (lvl= number of tabs). Return error code or else
 * if everything is OK, return number of bytes written.
 */

static int fprintf_wtab(XFILE *fp, int lvl, char *string, ... )
{
	int error = 0;

	char s[MAX_CFGLINE];
	va_list argpoint;

	/* Print a number of tab characters */

	while ( (lvl-- > 0) && (error >= 0) )
		error = (int)x_fwrite( fp, "\t", sizeof(char) );

	/* Print whatever else is specified */

	if ( error >= 0 )
	{
		va_start(argpoint, string);

		/* Note: if positive, error contains string length */

		error = vsprintf(s, string, argpoint); 
		va_end(argpoint);

		if ( error > 0 )
			error = (int)x_fwrite(fp, s, (long)error );
	}

	return error;
}


/* 
 * Save a part of configuration defined by one table.
 * Option "emp" to only write field if non zero or non empty  
 */

int CfgSave(XFILE *fp, CfgEntry *tab, int level, bool emp) 
{
	int error = 0;
	char fmt[2*MAX_KEYLEN]; /* 2* because of "end..." */

	while( (tab->type) && (error >= 0) )
	{
		int lvl = level;

		/* Append (or not) default formatting according to entry type */

		append_fmt( tab->type, tab->flag, fmt, tab->s );

		/* Now do output according to entry type */

		switch(tab->type)
		{
			case CFG_NEST:
				/* Go deeper, it is a nest, and all is specified */
				if ( (tab->a != NULL) && (tab->s != NULL) )
				{
					error = 0;
					(*(CfgNest *)tab->a)(fp, tab->s, level, 1, &error); 
				}
				break;
			case CFG_HDR:
				/* Write a nest header only if there is a keyword */
				if ( tab->s != NULL )
					error = fprintf_wtab(fp, --lvl, fmt, tab->s);
				break;
			case CFG_BEG:
		    case CFG_END:
			case CFG_ENDG:
				/* Change nesting level */
				lvl--;
			case CFG_FINAL:
				/* Write "end" to configuration file */
				error = fprintf_wtab(fp, lvl, fmt);
				break;
			default:
			{
				if ( !(tab->flag &CFG_INHIB) )
				{
					/* Write data according to type (check if data exist) */

					switch(tab->type)
					{
						case CFG_S:
						{
							/* Write string value */
							char *ss = (char *)tab->a;
							if (*ss || emp)
							{
								while(*ss)
								{
									if (*ss == ' ')
										*ss = '@';
									ss++;
								}
								error = fprintf_wtab(fp, lvl, fmt, tab->a);
							}
							break;
						}
/* not used
						case CFG_C:
*/
						case CFG_BD:
						case CFG_D:
						case CFG_X:
						{
							/* Write integer value (diverse formats) */

							unsigned int *v = (unsigned int *)tab->a;
							if (*v || emp)
								error = fprintf_wtab(fp, lvl, fmt, *v);
							break;
						}
						case CFG_H:
						case CFG_B:
						{
							/* Write byte or unsigned char value */
							unsigned int v = *(unsigned char *)tab->a;
							if (v || emp)
								error = fprintf_wtab(fp, lvl, fmt, v);
							break;
						}
						case CFG_DDD: 
						{
							/* Write a triplet of integers */
							int *v = (int *)tab->a;
							if (v)
								error = fprintf_wtab(fp, lvl, fmt, v[0], v[1], v[2]); 
							break;
						}
						case CFG_L:
						{
							/* Write a long value */
							long *v = (long *)tab->a;
							if (*v || emp)
								error = fprintf_wtab(fp, lvl, fmt, *v);
							break;
						}
						default:
						{
							/* Remove any format specifier and write as string */

							no_percent(fmt);		/* safety check */
							error = fprintf_wtab(fp, lvl, fmt  );
							break;
						}
					} /* tab->type ? */
				} /* CFG_INHIB ? */
			} /* default */
		} /* tab->type */
		tab++;
	}

	/* Positive return codes are not errors */

	if ( error > 0 )
		error = 0;

	return error; 
}


/* 
 * Search for the first non-blank (and non-tab) character in a string; 
 * return pointer to it 
 */

char *nonwhite(char *s)
{
	while( ( (*s == '\t') || (*s == ' ') ) && (*s != 0) ) 
		s++;

	return s;
}


/* 
 * Strip <lf>, <cr> or <cr><lf> from line end (insert null-characters there) 
 */

static char *crlf(char *f)
{
	int i;
	
	i = (int)strlen(f) - 1;
	if (f[i] == '\n') 
		f[i--] = 0; 	/* <lf> */

	if (f[i] == '\r') 
		f[i  ] = 0; 	/* <cr> */

	return f;
}


/*
 * Strip the comment from a line
 */

static char *nocomment( char *f )
{
	char *s = strchr( f, ';' );

	if ( s != NULL )
		*s = 0;

	return f;
}
/* 
 * Copy not more than "x" characters from "s" to "d",
 * until blank, tab, end of string or comment character (";") reached,
 * substituting all "@" with " " 
 */

static void cfgcpy(char *d, char *s, int x)
{
	while ( 				/* loop until: */     
			x > 0			/* character count */
			&& *s != ' '	/* blank */
			&& *s != '\t'	/* tab */
			&& *s != ';'	/* comment */
			&& *s != 0		/* end of string */
		  )
	{
		if (*s == '@') 
			*s = ' ';

		*d++ = *s++; 
		x--;
	}

	*d = 0;
}




/* 
 * Load a segment of configuration defined by one table.
 * Note: if an invalid record or value is encountered, the rest 
 * of the level is skipped in the hope that it will be possible to 
 * recover.
 */


int CfgLoad
(
	XFILE *fp,			/* pointer to file definition structure */ 
	CfgEntry *cfgtab, 	/* pointer to configuration table */
	int maxs,			/* maximum length of value string (after "=") */ 
	int level			/* nesting (indent) level */
) 
{
	int 
		v,						/* aux, for type conversion */
		error = 0,				/* error code */
		tel = 0;				/* error counter */

	char 
		r[MAX_CFGLINE], 		/* string read from the file */
		*s;						/* pointer to a positon in the above */


	boolean
		skip = FALSE;			/* true while recovering from errors */


	/* Loop while needed. Get next record from the file */

	while ( (error = (int)x_fgets(fp, s = r, sizeof(r))) == 0 )
	{
		CfgEntry *tab = cfgtab;

		/* Strip line end and comment , then move to first nonblank */

		crlf(s);
		nocomment(s);
		s = nonwhite(s);

		/* Skip empty lines */

		if (*s == 0)
			continue;

		/* Check total nesting level in .inf file */

		if (*s == '{' )				/* start of group */
		{
			chklevel++;
			continue;
		}

		if (*s == '}' )				/* end of group; break from the loop */
		{
			chklevel--;
			if (level > chklevel) 	/* made so for recovery from errors */
				break;
		}

		/* Check for the end of configuration file */

		if ( strncmp(s, "end", 3) == 0 )	/* end of everything */
		{
			if ( chklevel == 0 )
				chklevel = -999;	/* "end" is where it should be */
			else
				error = EEOF;		/* "end" came too soon */
			break;
		}

		/* If recovery from error is in progress, skip all the rest */

		if ( skip )
			continue;

		/* Loop through the table until finding the tab or the end */

		while( (tab->type != CFG_LAST) && (error >= 0) )
		{
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

			if ( tab->type > CFG_FINAL )
			{
				while ( (tab->s[v] > ' ') && (tab->s[v] != '=') )
					v++;
			}

			/* Compare first "v" characters or string and keyword */

			if ( v > 0 && strncmp(s, tab->s, v) == 0 )
			{
				/* Match found; move to "=" */

				s = strchr(s, '=');

				/* If "=" was found, now move to first next nonblank */

				if (*s == '=')
					s = nonwhite(++s);

				/* Now do whatever is appropriate to interpret data */

				switch(tab->type)
				{
					case CFG_NEST: 
						/* It is a nest, go one level deeper */
						/* also remember this keyword for possible error output */
						lastnest = tab->s;
						(*(CfgNest *)tab->a)(fp, tab->s, level, 0, &error);

						if ( error == EFRVAL )
						{
	 						alert_printf(1, AVALIDCF, lastnest);
							tel++;
							error = 0;
						}
						break;
					case CFG_S:
						/* Decode a string */
						cfgcpy(tab->a, s, maxs);
						break;

/* currently not used in Teradesk but may be used some day
					case CFG_C:
						/* Interprete string as uint */
						*(uint *)tab->a = *s++;
						break;
					case CFG_B:
						/* Decode a character value */
						*(char *)tab->a = *s++;
						break;
*/
					case CFG_H:
						/* Decode a positive decimal byte value */
						*(char *)tab->a = (char)max(atoi(s), 0);
						break;
					case CFG_BD:
					case CFG_D:
						/* Decode a positive decimal integer value */
						*(int *)tab->a = max(atoi(s), 0);
						if ( (tab->type == CFG_BD) && (*(int *)tab->a > 0) )
							*(int *)tab->a = 1;
						break;
					case CFG_DDD:
					{
						/* Decode a triplet of positive decimal integer values */
						int *vv = (int *)tab->a;
						char *s2;
						s2 = --s;
						v = 0;
						do
						{
							*vv++ = atoi(++s2);
						}
						while ( (s2 = strchr(s2,',')) != NULL && ++v < 3 );
						break;	
					}					
					case CFG_X:
						/* Decode a hex integer value */
						*(int *)tab->a = (int)strtol(s, NULL, 16); 
						break;
					case CFG_L:
						/* Decode a positive decimal long int value */
						*(long *)tab->a = lmax(atol(s), 0L);
						break;
					default:
						break;

				} /* switch */
	
				break; /* break from further searching, because key found */
			}
			tab++;
		}
	
		/* 
		 * Increase error count if table entry not found 
		 * Note: currently this count is valid only within
		 * one nesting (not very good, the same error can repeat in
		 * other nests many times without an abort)
		 */

		if ( (tab->type == CFG_LAST) || (tab->s == NULL) )
		{
			tel++;
			alert_printf( 1, AUNKRTYP, s, lastnest );
		}

		/* If enough errors found, skip to the end of this level */

		if (tel > 3)	/* Permit a maximum of three errors in the group */
		{
			alert_iprint( BADCFG ); /* may not be a config file at all */
			skip = TRUE;
		}

		/* 
		 * Has there been some fatal errors? Break from the loop
		 */

		if ( error < 0 ) 
			break;
	}

	/* 
	 * Ignore positive return values possibly generated somewhere;
	 * Also ignore end-of-file if it is complete
	 */

	if ( (error > 0) || (error == EEOF && chklevel == -999) )
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

int handle_cfg
(
	XFILE *fp, 			/* open file definition data */
	CfgEntry *cfgtab, 	/* pointer to configuration table used */
	int maxs, 			/* maximum string length to be read */
	int level,			/* nesting level */ 
	boolean emp, 		/* if TRUE, save empty fields */
	int io,				/* CFG_SAVE or 1= save data;  0= load data */
	void *ini,			/* initial setup routine */
	void *def			/* default setup routine */
) 
{
	int error;

	void 
		(*initial_setup)(void) = ini,
		(*default_setup)(void) = def;

	if ( io == CFG_SAVE )
		return CfgSave( fp, cfgtab, level, emp );
#if TEXT_CFG_IN
	else
	{
		if ( ini != NULL )
			initial_setup();

		error = CfgLoad( fp, cfgtab, maxs, level );

		if ( error == EFRVAL && def != NULL )
		{
		    alert_printf(1, ALOADCFG, cname, get_message(error));
			if ( ini != NULL )
				initial_setup();
			default_setup();
		}
		return error;
	}
#else
return 0;
#endif
}


/*
 * Handle complete reading or writing from/to a configuration
 * or palette file. Routine is supposed to correctly trace
 * nesting names up to one level of recursion (i.e. a palette file
 * created within creation of inf file).
 */

int handle_cfgfile
(
	char *name,		/* name of configuration file to read/write */
	CfgEntry *tab,	/* table which has to be handled */
	char *ident,	/* Identification header t the file */
	int io			/* 1=save, 0=read */
)
{
	XFILE *file;

	int 
		n, 
		h, 
		error;

	char 
		*savecname,
		*savelastnest;

	int
		etext,
		savechklevel;

	static char
		*fmt1 = "%s%s%s";


	/* Remember last nest name for error tracing */

	savelastnest = lastnest;
	savechklevel = chklevel;

	/* Find name only; if there is mint, it is lowercase */

	savecname = cname;
	cname = fn_get_name(name);

#if _MINT_
	if ( magx || !mint )
#endif
		strupr(cname);

	/* Proceed to load or save */

	if ( io == CFG_SAVE )
	{
		etext = ASAVECFG;

		if ((file = x_fopen(name, O_DENYRW | O_WRONLY, &error)) != NULL)
		{
			/* Write file identification header */

			error = fprintf_wtab(file, 0, fmt1, ident, eol, eol);

			/* Write the "don't edit" warning */

			error = fprintf_wtab(file, 0, fmt1, get_freestring(TDONTEDI), eol, eol);

			/* Write complete configuration */
		
			error = CfgSave(file, tab, 0, CFGEMP); 

			if (((h = x_fclose(file)) < 0) && (error == 0))
				error = h;
			
			/* Update the window into which the file was written */

			wd_set_update(WD_UPD_COPIED, cname, NULL);
			wd_do_update();

		}
	}
	else
	{
		etext = ALOADCFG;

		if ((file = x_fopen(name, O_DENYW | O_RDONLY, &error)) != NULL)
		{
			char identbuf[MAX_CFGLINE];

			if ((n = x_fgets(file, identbuf, MAX_CFGLINE - 1) ) == 0)
			{
				lastnest = ident;
				chklevel = 0;

				/* 
				 * Check the file identifier header. If it is OK, 
				 * then read complete configuration 
				 */

				if (strncmp(identbuf, ident, strlen(ident)) == 0)
					error = CfgLoad(file, tab, MAX_CFGLINE, 0); 
				else
					error = EFRVAL;
			}
			else
			{
				error = (n < 0) ? (int) n : EEOF;
			}
		
			x_fclose(file);
		}
	}

	/* Display error information */

	if ( (error < 0) && (error != ENOMSG) )
		alert_printf(1, etext, cname, get_message(error));

	/* Restore previous filename, nest name and level if any */

	cname = savecname;
	lastnest = savelastnest;
	chklevel = savechklevel;

	return error;
}	


