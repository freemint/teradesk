/* 
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                         2003, 2004, 2005  Dj. Vukovic
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
 * Trim the path to be displayed in order to remove 
 * item name and backslash, so that more of the path can fit into field
 * (insert a null character at the appropriate location).
 * Do not trim the backslash if it is a path to a root directory. 
 * Note: there is already a similar routine but it doesn't do 
 * quite the same thing.
 * This routine also handles the cases of '\.\' or '\.' in the path
 */

void path_to_disp ( char *fpath )
{
	/* find the last '\' */

	char *nend = strrchr( fpath, '\\' );
		
	if ( nend )
	{
		char *p, *q;

		/* If '\' is preceded by a ':' keep it, otherwise truncate */

		if ( nend[-1] == ':' ) 
			nend++;
		*nend = 0;

		/* Attempt to find '\.\' or a trailing '\.' */

		while((p = strstr( fpath, "\\.")) != NULL)
		{
			q = p + 2L;
	
			/* If found, remove it */

			if(*q == '\\' || *q == '\0')
			{
				*p = 0;
				strcat(fpath, q);
			}
		} 
	}
}


/*
 * Return a pointer to the last backslash in a name string;
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
 * This routine returns a pointer to the beginning of the name proper in
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
 * Extract the path part of a full name. Space is allocated for 
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

	if (((l = backsl - (char *)path) == 2) && (path[1] == ':'))
		l++;

	/* Allocate memory for the new path */

	if ((backsl = malloc_chk(l + 1)) == NULL)
		return NULL;

	/* Copy the relevant part of the path to the new location */

	strsncpy(backsl, path, l + 1);

	/* Return pointer to the new location */

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
 * Space is allocated for the result.
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

	l = backsl - (char *)oldname;			/* length of the path part */

	tl = l + strlen(newname) + 2L;			/* total new length */

	if ((path = malloc_chk(tl)) != NULL)	/* allocate space for the new */
	{
		strsncpy(path, oldname, l + 1);		/* copy the path */

		if ( (error = x_checkname( path, newname ) ) == 0 )
		{
			strcat(path, bslash);
			strcat(path, newname);
		}
		else
		{
			free(path);
			path = NULL;
		}
	}

	/* Note: failed malloc will be reported in malloc_chk */

	xform_error(error); /* Will ignore error >= 0 */

	return path;
}


/* This routine is not currently used anywhere in TeraDesk

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

*/



/* 
 * Check if path is to a root directory on drive.
 * This is determined only by analyzing the name.
 * No check of the actual directory is made. 
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
 * Locate an EXISTING file using the file-selector.
 * (for saving configuration and for printing, the file need not exist).
 * Return pointer to a full path+name specification (allocated inside).
 * If not found, return NULL.
 * Note: Types L_FILE and L_FOLDER are currently not used in TeraDesk
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

	static const int /* don't use const char here! indexes are large */
		titles[] = {FSTLFILE, FSTLPRG, FSTLFLDR, FSTLOADS, FSTSAVES, FSPRINT};


	cfgext = strdup( (type == L_PRINTF) ? PRN_EXT : CFG_EXT); /* so that strlwr can be done upon it */

#if _MINT_
	if (mint && cfgext)
		strlwr(cfgext);
#endif

	/* Beware of the sequence of L_*; valid for L_LOADCFG, L_SAVECFG, L_PRINTF */

	defext = (type >= L_LOADCFG) ? cfgext : fsdefext;

/* Currently not used

	if (type == L_FOLDER)
	{
		if ((fspec = fn_make_path(name, defext)) == NULL)
			return NULL; 

		fname[0] = 0;
		ex_flags = EX_DIR;
	}
	else
*/

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

/* Currently not used

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

*/
		{
			if ((type == L_PROGRAM) && !prg_isprogram(fname))
				alert_iprint(TPLFMT);
			else
			{
				if (((newname = fn_make_newname(newpath, fname)) != NULL) && (type != L_SAVECFG) && (type != L_PRINTF) )
				{
					result = x_exist(newname, ex_flags);
					if (!result)
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
	while (!result);

	free(newpath);

	return newname;
}


/*
 * Get a path or a name using a fileselector and insert into string.
 * This is used in scrolled editable text fields.
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
	title = get_freestring(FSTLANY);
	name[0] = 0;
	path = xfileselector( defext, name, title ); /* path is allocated here */

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
			goto freepath;
		}

		cc = NULL;
		c = result + (long)(*pos); /* part after the cursor */

		if ( (*pos < sl) && ( (cc = strdup(c)) == NULL ) )
			goto freepath; /* not decent, but... */

		strsncpy( c, path, pl + 1 ); /* must include null at end here! */

		if ( fl )
			strcat( result, name );

		if ( cc )
		{
			strcat( result, cc );
			free(cc);
		} 

		*pos = *pos + (int)tl;

		freepath:;
		free(path);
	}
}


/* 
 * Get name of the root directory of the current directory
 * (i.e. drive name, in the form such as "A:" ) 
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

	if (*path && ((d = path[0] & 0x5F) >= 'A') && (d <= 'Z') && (path[1] == ':'))
	{
		x_setdrv(d - 'A');
		h = (char *)path + 2;
		if (*h == 0)
			h = (char *)bslash;
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
 * Compare a string (e.g. a filename) against a wildcard pattern;
 * valid wildcards: * ? ! [<char><char>...] 
 * Return TRUE if matched. Comparison is NOT case-sensitive.
 * Note: speed could be improved by always preparing all-uppercase wildcards
 * (wildcards are stored in window icon lists, program type lists and 
 * filemask/documenttype lists, i.e. it is easy to convert to uppercase there).
 *
 * The first two wildcards (* and ?) are valid in single-TOS;
 * the other are valid only in mint.
 * 
 * Note: There were two alternative routines for matching wildcards,
 * depending on whether the Desktop was multitasking-capable or not:
 * match_pattern() and cmp_part(). The first one was used for 
 * multitasking-capable desktop and the other for single-tos only.
 * This has been changed now so that match_pattern() is always used.
 */

boolean match_pattern(const char *t, const char *pat)
{
	bool valid = true;
	char u;

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
			u = (char)touppc(*pat);
			while(*t &&(touppc(*t) != u))
				t++;
			break;
#if _MINT_
		case '!':			/* !X means any character but X */
			if (mint)
			{
				if (touppc(*t) != touppc(pat[1]))
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
				u = touppc(*t);
				while((*(++pat) != ']') && (u != touppc(*pat)));
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
			if (touppc(*t++) != touppc(*pat++))
				valid = false;
			break;
		}
	}

	return valid && (touppc(*t) == touppc(*pat));
}


/* 
 * Compare a filename against a wildcard pattern 
 */

boolean cmp_wildcard(const char *fname, const char *wildcard)
{
	boolean matched;
	char *dot = NULL;

#if _MINT_
	if (!mint)
#endif
	{
		dot = strchr(wildcard, '.');
		if (dot)
		{
			if (dot[1] == '*' && dot[2] == '\0' && strchr(fname, '.') == NULL)
				*dot = '\0';
		}
		else if (strchr(fname, '.') != NULL) 
			return FALSE;		
	}

	matched = match_pattern(fname, wildcard);

	if (dot)
		*dot = '.';

	return matched;
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
 * Force media change on drive contained in path.
 * This routine is used only so that there is no chance of
 * path being undefined or too short 
 */

void force_mediach(const char *path)
{
	int drive, p = *path;

	if ((p == 0) || !(isalpha(p) && path[1] == ':')) /* Valid path */
		return;
	drive = tolower(p) - 'a';

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
	const char 
		*s = source;			/* a location in the source string */ 

	char
		*dl = dest + 12,		/* In case a longer name slips through */
		*d = dest;				/* a location in the destination string */

	while (*s && (*s != '.') && d < dl )
		*d++ = *s++;

	if (*s)
	{
		while (d - dest < 8)
			*d++ = ' ';

		s++;

		while ( *s && d < dl )
			*d++ = *s++;
	}

	*d = 0;
}


/* 
 * Convert from a justified form string into TOS (8+3) filename.
 * Insert "." between the name and the extension (after 8 characters)
 */

static void cv_tos_form2fn(char *dest, const char *source)
{
	const char  
		*s = source;

	char
		*d = dest;

	while ((*s != 0) && (s - source < 8))
		if (*s != ' ')
			*d++ = *s++;
		else
			s++;

	if (*s)
	{
		*d++ = '.';

		while (*s != 0)
			if (*s != ' ')
				*d++ = *s++;
			else
				s++;
	}

	*d = 0;
}


/* 
 * Wrapper function for editable and possibly userdef fields. 
 * Fit a (possibly long) filename or path into a form (dialog) field
 * Note: now it is assumed that if the field is 12 characters long,
 * then it will be for a 8+3 format (see also routine tos_fnform in resource.c)
 * Note: if the name is too long, it will be trimmed 
 */

void cv_fntoform(OBJECT *tree, int object, const char *src)
{
	/* 
	 * Determine destination and what is the available length 
	 * Beware: "l" is the complete allocated length, 
	 * zero termination-byte must fit into it
	 */

	OBJECT *ob = tree + object;
	TEDINFO *ti = xd_get_obspecp(ob)->tedinfo;
	char *dst = ti->te_ptext;
	int l = ti->te_txtlen;

	/* 
	 * The only 12-chars long fields in TeraDesk should be 
	 * for 8+3 names, possibly even if Mint/Magic is around 
	 */

	if ( l < 13 )
		cv_tos_fn2form(dst, src);
	else
	{	
		if (ob->ob_flags & EDITABLE )
		{
			if(xd_xobtype(ob) == XD_SCRLEDIT)
			{
				l = sizeof(LNAME);
				xd_init_shift(ob, (char *)src); /* note: won't work ok if strlen(dest) > sizeof(LNAME) */
			}
			strsncpy(dst, src, (long)l); 		/* term. byte included in l */

			if ( strlen(src) > l - 1 )
				alert_iprint(TFNTLNG);
		}
		else
			cramped_name(src, dst, l); 			/* term. byte included */
	}	
} 


/* 
 * Convert filename from the dialog form into a convenient string.
 * This routine does not allocate space for destination. 
 */

void cv_formtofn(char *dest, OBJECT *tree, int object)
{
	TEDINFO *ti = xd_get_obspecp(tree + object)->tedinfo; 
	char *source = ti->te_ptext;
	int l = ti->te_txtlen;

	/* 
	 * The only 12-characters-long fields in TeraDesk should be 
	 * for 8+3 names, possibly even if Mint/Magic is around 
	 */

	if (  l < 13 )
		cv_tos_form2fn(dest, source);
	else
		strip_name(dest, source);
}
	