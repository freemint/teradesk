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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>			/* for wildcards */
#include <tos.h>
#include <mint.h>
#include <ctype.h>
#include <vdi.h>
#include <xdialog.h>
#include <library.h>

#include "desktop.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "config.h"
#include "file.h"
#include "prgtype.h"


#define CFG_EXT		"*.INF" /* default configuration-file extension */
#define PRN_EXT		"*.PRN" /* default print-file extension */


void wd_drawall(void);



/* 
 * Trim path to be displayed in order to remove 
 * item name and backslash, so that more of the path can fit into field
 * (insert a null character at the appropriate location).
 * Do not trim the backslash if it is a path to a root directory. 
 * Note: there is already a similar routine but it doesn't do 
 * quite the same thing
 */

void path_to_disp ( char *fpath )
{
	char *nend;
		
	nend = strrchr ( fpath, '\\' );
	if ( nend )
	{
		if ( *(nend -1L) == ':' ) 
			nend++;
		*nend = 0;
	}
}


/*
 * Return pointer to the last backslash in a name;
 * If a backslash does not exist, return pointer to the
 * beginning of the string
 */

char *fn_last_backsl(const char *fname)
{
	char *e;

	if ((e = strrchr(fname, '\\')) == NULL)
		e = (char *)fname;

	return e;
}


/* 
 * Geef een pointer terug die wijst naar het begin van de filenaam
 * in een padnaam.
 * This routine returns a pointer to the beginning of a name in
 * an existing fullname string. If there is no backslash in the
 * name, it returns pointer to the beginning of the string.
 * If the fullname string does not exist, returns NULL.
 * It doesn't allocate any memory for the name. 
 */

char *fn_get_name(const char *path)
{
	char *h;

	if (!path)
		return NULL;

	h = fn_last_backsl(path);
	if ( h != path || *path == '\\' )
		h++;

	return h;
}


/* 
 * Geef een pointer terug op een met malloc gereserveerd stuk
 * geheugen, waarin het pad van de padnaam van een file wordt gezet.
 * Extract the path part of the full name. Space is allocated for 
 * the extracted path here
 */

char *fn_get_path(const char *path)
{
	char *backsl;
	long l;


	/* If there is no backslash in the name, just take the whole path */

	backsl = fn_last_backsl(path);

	/* 
	 * Compute path length. Treat special case for the root path:
	 * add one to the length so that "\" is included
	 */

	if (((l = backsl - (char *) path) == 2) && (path[1] == ':'))
		l++;

	/* Allocate memory for the new path */

	if ((backsl = malloc_chk(l + 1)) == NULL)
		return NULL;

	/* Copy the relevant part of the path to the new location */

	strsncpy(backsl, path, l + 1);

	/* Return pointer o the new location */

	return backsl;
}


/*
 * Create a path+filename by concatenating two strings;
 * memory is allocated for the resulting string; return pointer to it.
 * If memory can not be allocated, display an alert box.
 */

char *fn_make_path(const char *path, const char *name)
{
	int error;
	char *result;

	if ((result = x_makepath(path, name, &error)) == NULL)
		xform_error(error);

	return result;
}


/* 
 * Geef een pointer terug op een met malloc gereserveerd stuk
 * geheugen, waarin het path van oldname is samengevoegd met
 * newnameen. 
 * Compose a new name from old fullname and a new name.
 */

char *fn_make_newname(const char *oldname, const char *newname)
{
	char 
		*backsl, 
		*path; 

	long 
		l, 
		tl; 

	int 
		error = 0;

	/* Find position of the last backslash  */

	backsl = fn_last_backsl(oldname);

	l = backsl - (char *)oldname;		/* length of the path part */

	tl = l + strlen(newname) + 2L;		/* total new length */

	if ((path = malloc_chk(tl)) != NULL)	/* allocate space for the new */
	{
		strsncpy(path, oldname, l + 1);	/* copy the path */

		if ( (error = x_checkname( path, newname ) ) == 0 )
		{
			strcat(path, bslash);
			strcat(path, newname);
		}
		else
			free_item(&((void *)path));
	}

	/* Note: failed malloc will be reported in malloc_chk */

	xform_error(error); /* Will ignore error >= 0 */

	return path;
}


/* 
 * Strip filename (or path+filename) "fname" 
 * to get (path+)filename "name"  and extension "ext" 
 */

static void split_name(char *name, char *ext, const char *fname)
{
	char *s, *d, *e;

	e = fn_last_backsl(fname); 

	if ((e = strchr(e, '.')) == NULL)
	{
		strcpy(name, fname);
		*ext = 0;
	}
	else
	{
		s = (char *) fname;
		d = name;

		while (s != e)
			*d++ = *s++;
		*d = 0;

		strcpy(ext, e + 1);
	}
}


/* 
 * Check if path is to a root directory on drive.
 * This is determined by analyzing the name.
 * No check of the actual disk contents is made. 
 */

boolean isroot(const char *path)
{
	char *d = nonwhite((char *)path);

	long l = strlen(d);

	if ( l < 2 || l > 3 )
		return FALSE;

	l--;

	if (d[l] == '\\')
		l--;

	if (d[l] == ':' && l == 1 )
		return TRUE;

	return FALSE;
}


/* 
 * Locate an EXISTING file using file-selector.
 * (for saving configuration and for printing, the file need not exist).
 * Return pointer to a full path+name specification (allocated inside).
 * If not found, return NULL.
 */

char *locate(const char *name, int type)
{
	VLNAME
		fname; /* can this be a LNAME ? */

	char 
		*newpath, 
		*newname, /* local */ 
		*fspec, 
		*title,
		*cfgext,
		*defext;

	boolean 
		result = FALSE;

	int 
		ex_flags;

	static const int
		titles[] = {FSTLFILE, FSTLPRG, FSTLFLDR, FSTLOADS, FSTSAVES, FSPRINT};


	cfgext = strdup( (type == L_PRINTF) ? PRN_EXT : CFG_EXT); /* so that strlwr can be done upon it */

#if _MINT_
	if (mint && cfgext)
		strlwr(cfgext);
#endif

	defext = ((type == L_LOADCFG) || (type == L_SAVECFG) || (type == L_PRINTF) ) ? cfgext : fsdefext;

	if (type == L_FOLDER)
	{
		if ((fspec = fn_make_path(name, defext)) == NULL)
			return NULL; 

		fname[0] = 0;
		ex_flags = EX_DIR;
	}
	else
	{
		if ((fspec = fn_make_newname(name, defext)) == NULL)
				return NULL;

		strcpy(fname, fn_get_name(name));
		ex_flags = EX_FILE;
	}

	free(cfgext);
	title = get_freestring(titles[type]);

	do
	{
		newpath = xfileselector(fspec, fname, title);

		free(fspec);

		if (!newpath)
			return NULL;

		if (type == L_FOLDER)
		{
			if (((newname = fn_get_path(newpath)) != NULL) && isroot(newname))
			{
				alert_iprint(MNOROOT);
				free(newname);
			}
			else
				result = TRUE;
		}
		else
		{
			if ((type == L_PROGRAM) && (prg_isprogram(fname) == FALSE))
				alert_iprint(TPLFMT);
			else
			{
				if (((newname = fn_make_newname(newpath, fname)) != NULL) && (type != L_SAVECFG) && (type != L_PRINTF) )
				{
					result = x_exist(newname, ex_flags);
					if (result == FALSE)
					{
						alert_iprint(TFILNF);
						free(newname);
					}
				}
				else
					result = TRUE;
			}
		}
		fspec = newpath;
	}
	while (result == FALSE);

	free(newpath);

	return newname;
}


/*
 * Get a path or a name using a fileselector and insert into string
 */

void get_fsel
(
	XDINFO *info,	/* dialog data */
	char *result,	/* pointer to string being edited */
	int len,		/* max. length of field          */
	int *pos		/* cursor position in the string */
)
{
	long
		pl = 0,		/* path length   */
		fl = 0,		/* name length   */
		tl,			/* total length of inserted string */
		sl;			/* string length */

	char
		*c,			/* pointer to aft part of the string */
		*cc,		/* copy of the above */
		*path,		/* path obtained */
		*title,		/* Selector title */
		*defext;	/* default file extension */

	LNAME
		name;		/* name obtained */


	defext = fsdefext;

	title = get_freestring(FSTLFILE);

	name[0] = 0;

	path = xfileselector( defext, name, title );

	/* Some fileselectors do not redraw what was below them... */

	wd_drawall();
	xd_drawdeep( info, ROOT );

	/* If a path is specified, get rid of wildcards in it */

	if ( path ) 
	{
		/* 
		 * There will always be something after the last "\"
		 * so, after trimming in path_to_disp, it is safe to add
		 * a "\" again, if a name has to be concatenated
		 */

		path_to_disp(path);

		if (*name && !isroot(path))
			strcat(path, bslash);

		fl = strlen(name);
		pl = strlen(path);

		tl = fl + pl;

		sl = strlen(result);
		if ( sl + tl >= len )
		{
			alert_iprint(TFNTLNG);
			return;
		}

		cc = NULL;
		c = result + (long)(*pos); /* part after the cursor */

		if ( *pos < sl )
		{
			if ( (cc = strdup(c)) == NULL )
				return;
		}

		strsncpy( c, path, pl + 1 ); /* must include null at end here! */
		free(path);

		if ( fl )
			strcat( result, name );

		if ( cc )
		{
			strcat( result, cc );
			free(cc);
		} 

		*pos = *pos + (int)tl;
	}
}


/* 
 * Get name of the root directory (i.e. drive name, in the form such as "A:" ) 
 */

void getroot(char *root)
{
	root[0] = x_getdrv() + 'A';
	root[1] = ':';
	root[2] = 0;
}


/* 
 * Get name (i.e. path + name) of the current directory 
 */

char *getdir(int *error)
{
	return x_getpath(0, error);
}


/* 
 * Change current directory to "path" 
 */

int chdir(const char *path)
{
	int error;
	char *h;
	char d;

	h = (char *)path;
	if (*path && ((d = path[0] & 0xDF) >= 'A') && (d <= 'Z') && (path[1] == ':'))
	{
		x_setdrv(d - 'A');
		h = (char *)path + 2;
		if (*h == 0)
			h = bslash;
	}
	error = x_setpath(h);

	return error;
}


/* 
 * Get bitflags for existing drives 
 */

long drvmap(void)
{
	return (x_setdrv(x_getdrv()));
}


/* 
 * Check if drive "drv" exists.
 * Note: 26 drives are supported (A to Z),
 * but defined by numbers 0 to 25.
 */

boolean check_drive(int drv)
{
	if ((drv >= 0) && (drv < 26) && (btst(drvmap(), drv)))
		return TRUE;

	alert_iprint(MDRVEXIS);

	return FALSE;
}


/* 
 * There are (were) two alternative routines for matching wildcards,
 * depending on whether the Desktop was multitasking-capable or not:
 * match_pattern() and cmp_part() .The first one was used for 
 * multitasking-capable desktop and the other for single-tos only.
 * This has been changed so that match_pattern() is always used.
 */

/*
#if _MINT_
*/

/* 
 * Compare string (.e.g. filename) against a wildcard pattern;
 * valid wildcards: * ? ! [<char><char>...] 
 * Return TRUE if matched. Comparison is NOT case-sensitive.
 * Note: speed could be improved by always preparing all-uppercase wildcards
 * (wildcards are stored in window icon lists, program type lists and 
 * filemask/documenttype lists, i.e. it is easy to convert to uppercase there)
 */

boolean match_pattern(const char *t, const char *pat)
{
	bool valid = true;

	while(valid && ((*t && *pat) || (!*t && *pat == '*')))	/* HR: catch empty that should be OK */
	{
		switch(*pat)
		{
		case '?':			/* ? means any single character */
			t++;
			pat++;
			break;
		case '*':			/* * means a string of any character */
			pat++;
			while(*t && (toupper(*t) != toupper(*pat)))
				t++;
			break;
#if _MINT_
		case '!':			/* !X means any character but X */
			if (mint)
			{
				if (toupper(*t) != toupper(pat[1]))
				{
					t++;
					pat += 2;
				} 
				else
					valid = false;
				break;
			}
		case '[':			/* [<chars>] means any one of <chars> */
			if (mint)
			{
				while((*(++pat) != ']') and (toupper(*t) != toupper(*pat)));
				if (*pat == ']')
					valid = false;
				else
					while(*++pat != ']');
				pat++;
				t++;
				break;
			}
#endif
		default:			/* exact match on anything else, case insensitive */
			if (toupper(*t++) != toupper(*pat++))
				valid = false;
			break;
		}
	}

	return valid && toupper(*t) == toupper(*pat);
}



/* let's not use it anymore
#else

/* 
 * Compare part of a filename (i.e. extension or name itself)
 * against a wildcard pattern. Comparison is case-insensitive.
 * Used only in TOS-only (i.e. short name) version
 */

boolean cmp_part(const char *name, const char *wildcard)
{
	int i = -1, j = -1;

	do
	{
		j++;
		i++;
		switch (wildcard[i])
		{
		case '?':
			if (name[j] == 0)
				return FALSE;
			break;
		case '*':
			if (wildcard[i + 1] == 0)
				return TRUE;
			else
			{
				i++;
				while (name[j] != 0)
				{
					if (cmp_part(name + j, wildcard + i))
						return TRUE;
					j++;
				}
				return FALSE;
			}
		default:
			if (tolower(name[j]) != tolower(wildcard[i])) /* case insensitive */
				return FALSE;
			break;
		}
	}
	while (wildcard[i] != 0);
	return TRUE;
}
#endif
*/


/* 
 * Compare a filename against a wildcard pattern 
 */

boolean cmp_wildcard(const char *fname, const char *wildcard)
{
#if _MINT_

		if ( mint )
        	return match_pattern(fname, wildcard);	
		else
#endif
		{
			/*
			 * For TOS & Geneva (short filenames) only: split filename into
			 * name+extension. Compare name against wildcard name,
			 * compare extension against wildcard extension.
			 */

			char name[10], ext[4], wname[10], wext[4];

			split_name(name, ext, fname);      /* name and extension */
			split_name(wname, wext, wildcard); /* wildcard name and extension */
			if (match_pattern(name, wname) == FALSE)
				return FALSE;
			return match_pattern(ext, wext);
		}
}


typedef long cdecl (*Func)();

static int chdrv;
static long cdecl (*Oldgetbpb) (int);
static long cdecl (*Oldmediach) (int);
static long cdecl (*Oldrwabs) (int, void *, int, int, int, long);

static long cdecl Newgetbpb(int d)
{
	if (d == chdrv)
	{
		*((Func *)0x472L) = Oldgetbpb;
		*((Func *)0x476L) = Oldrwabs;
		*((Func *)0x47eL) = Oldmediach;
	}

	return (*Oldgetbpb)(d);
}


static long cdecl Newmediach(int d)
{
	if (d == chdrv)
		return 2;
	else
		return (*Oldmediach)(d);
}


static long cdecl Newrwabs(int d, void *buf, int a, int b, int c, long l)
{
	if (d == chdrv)
		return MEDIA_CHANGE;
	else
		return (*Oldrwabs)(d, buf, a, b, c, l);
}


/* 
 * Force media change on drive contained in path 
 */

void force_mediach(const char *path)
{
	int drive, p = *path;

	if ((p == 0) || !(isalnum(p) && path[1] == ':'))	/* alnum */
		return;

	drive =   isalpha(p) ? tolower(p) - 'a' : p - '0' + 'z' - 'a'  + 2;

#if _MINT_
	if (mint)
	{
		if (Dlock(1, drive) == 0)
			Dlock(0, drive);
	}
	else
#endif
	{
		void *stack;
		static char fname[] = "X:\\X";
		long r;

		stack = (void *)Super(0L);

		chdrv = drive;
		Oldrwabs = *((Func *)0x476L);
		Oldgetbpb = *((Func *)0x472L);
		Oldmediach = *((Func *)0x47eL);

		*((Func *)0x476L) = Newrwabs;
		*((Func *)0x472L) = Newgetbpb;
		*((Func *)0x47eL) = Newmediach;

		fname[0] = drive + 'A';
		r = Fopen(fname, 0);
		if (r >= 0)
			Fclose((int)r);

		if (*((Func *)0x476L) == Newrwabs)
		{
			*((Func *)0x472L) = Oldgetbpb;
			*((Func *)0x476L) = Oldrwabs;
			*((Func *)0x47eL) = Oldmediach;
		}

		Super(stack);
	}
}



/*
 * Convert a TOS filename (8+3) into a string suitable for display
 * (with left-justified file extension, e.g.: FFFF    XXX without ".")
 */

static void cv_tos_fn2form(char *dest, const char *source)
{
	int 
		s = 0,	/* a location in the source string */ 
		d = 0;	/* a location in the destination string */

	while ((source[s] != 0) && (source[s] != '.'))
		dest[d++] = source[s++];
	if (source[s] == 0)
		dest[d++] = 0;
	else
	{
		while (d < 8)
			dest[d++] = ' ';
		s++;
		while (source[s] != 0)
			dest[d++] = source[s++];
		dest[d++] = 0;
	}
}


/* 
 * Convert from a justified form string into TOS (8+3) filename.
 * Insert "." between the name and the extension (after 8 characters)
 */

static void cv_tos_form2fn(char *dest, const char *source)
{
	int s = 0, d = 0;

	while ((source[s] != 0) && (s < 8))
		if (source[s] != ' ')
			dest[d++] = source[s++];
		else
			s++;
	if (source[s] == 0)
		dest[d++] = 0;
	else
	{
		dest[d++] = '.';
		while (source[s] != 0)
			if (source[s] != ' ')
				dest[d++] = source[s++];
			else
				s++;
		dest[d++] = 0;
	}
}


/* 
 * wrapper function for editable and possibly userdef fields. 
 * Fit a (possibly long) filename or path into a form (dialog) field
 * Note: now it is assumed that if the field is 12 characters long,
 * then it will be for a 8+3 format (see also routine tos_fnform in resource.c)
 * Note: if the name is too long, it will be trimmed 
 */

void cv_fntoform(OBJECT *ob, const char *src)
{
	/* 
	 * Determine destination and what is the available length 
	 * Beware: "l" is the complete allocated length, 
	 * zero termination-byte must fit into it
	 */

	char *dst = xd_get_obspec(ob).tedinfo->te_ptext;
	int  l    = xd_get_obspec(ob).tedinfo->te_txtlen;

	/* 
	 * The only 12-chars long fields in TeraDesk should be 
	 * for 8+3 names, possibly even if mint is around 
	 */

	if ( l < 14 )
		cv_tos_fn2form(dst, src);
	else
	{	
		if (ob->ob_flags & EDITABLE )
		{	
			if (( (ob->ob_type >> 8) & 0xff) == XD_SCRLEDIT)
			{
				l = sizeof(LNAME);
				xd_init_shift(ob, (char *)src); 	/* note: won't work ok if strlen(dest) > sizeof(LNAME) */
			}
			strsncpy(dst, src, (long)l); 	/* term. byte included in l */

			if ( strlen(src) > l - 1 )
				alert_iprint(TFNTLNG);
		}
		else
			cramped_name(src, dst, l); /* term. byte included */
	}	
} 


/* 
 * Convert filename from the dialog form into a convenient string.
 * This routine does not allocate space for destination. 
 */

void cv_formtofn(char *dest, OBJECT *ob)
{
	char *source = xd_get_obspec(ob).tedinfo->te_ptext;
	int  l    = xd_get_obspec(ob).tedinfo->te_txtlen; /* this includes the term. byte */

	/* 
	 * The only 12-characters-long fields in TeraDesk should be 
	 * for 8+3 names, possibly even if mint is around 
	 */

	if (  l < 14 )
		cv_tos_form2fn(dest, source);
	else
		strip_name(dest, source);
}
	