/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2011  H. Robbers,
 *                         2003 - 2016  Dj. Vukovic
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

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "config.h"
#include "file.h"
#include "prgtype.h"
#include "window.h"

#define CFGEXT		"*.IN?" /* default configuration-file extension */
#define PRNEXT		"*.PRN" /* default print-file extension */



/*
 * Concatenate a path+filename string "name" from a path "path" and
 * a filename "fname"; Add a "\" between the path and the name if needed.
 * Space for resultant string has to be allocated before the call
 * Note: "name" and "path" can be at the same location.
 * A check is made whether total path length is within VLNAME size
 */

int make_path( char *name, const char *path, const char *fname )
{
	size_t l = strlen(path);


	/* "-1" below because a backlash may be added to the string */

	if(l + strlen(fname) >= sizeof(VLNAME) - 1)
		return EPTHTL;

	strcpy(name, path);

	if (l && (name[l - 1] != '\\'))
		name[l++] = '\\';

	strcpy(name + l, fname);

	return 0;
}


/*
 * Return a pointer to the last backslash in a name string;
 * If a backslash does not exist, return pointer to the
 * beginning of the string
 */

static const char *fn_last_backsl(const char *fname)
{
	const char *e;

	if ((e = strrchr(fname, '\\')) == NULL)
		e = fname;

	return e;
}


/*
 * Split a full (file)name (path+filename) "name" into "path" and "fname" parts.
 * Space for path and fname has to be allocated beforehand.
 * Root path is always finished with a "\"
 * It is assumed that "path" is at least VLNAME and
 * that "fname" is at least LNAME.
 */

void split_path( char *path, char *fname, const char *name )
{
	long
		pl;		/* path length */

	const char *np = fn_get_name(name);


	pl = lmin((size_t)(np - name), sizeof(VLNAME));
	strsncpy(fname, np, sizeof(LNAME));
	*path = 0;

	if(pl > 0)
	{
		if(fn_last_backsl(name) == name + 2)
			pl++;

		strsncpy(path, name, pl);
	}
}


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


	if ( nend > fpath ) /* otherwise nend[-1] below would not be possible */
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
 * This routine returns a pointer to the beginning of the name proper
 * in an existing fullname string. It is identified as the string after
 * the last backslash in the name. If there is no backslash in the name,
 * it returns pointer to the beginning of the string. If the fullname
 * string does not exist, returns NULL.
 * It doesn't allocate any memory for the name.
 */

const char *fn_get_name(const char *path)
{
	if(path)
	{
		const char *h = fn_last_backsl(path); /* h == path if no backslash found */

		if ( h != path || *path == '\\' )
			h++;

		return h;
	}

	return NULL;
}


/*
 * Extract the path part of a full name.
 * Space is allocated for the extracted path here
 */

char *fn_get_path(const char *path)
{
	const char
		*backsl;	/* pointer to the last backslash */
	char *newpath;
	long l;			/* string length */


	/* If there is no backslash in the name, just take the whole path */

	backsl = fn_last_backsl(path);

	/*
	 * Compute path length. Treat special case for the root path:
	 * add one to the length so that "\" is included
	 */

	if (((l = backsl - path) == 2) && (path[1] == ':'))
		l++;

	l++; /* for the trailing zero byte */

	/*
	 * Allocate memory for the new path, then copy the relevant part of
	 * the path to the new location
	 */

	if ((newpath = malloc_chk(l)) != NULL)
		strsncpy(newpath, path, l);

	/* Return pointer to the new location, or NULL if failed */

	return newpath;
}


/*
 * Create a path+filename by concatenating two strings;
 * memory is allocated for the resulting string; return pointer to it.
 * If memory can not be allocated, display an alert box.
 */

char *fn_make_path(const char *path, const char *name)
{
	char
		*result;

	_WORD
		error;


	if ((result = x_makepath(path, name, &error)) == NULL)
		xform_error(error);

	return result;
}


/*
 * Compose a new name from old fullname "oldn" and a new name "newn".
 * I.e. concatenate the new name and the old path.
 * Space is allocated for the result.
 */

char *fn_make_newname(const char *oldn, const char *newn)
{
	long
		l,
		tl;

	const char *backsl;
	char *path;

	_WORD
		error = 0;


	/* Find position of the last backslash  */

	backsl = fn_last_backsl(oldn);

	l = backsl - oldn;				/* length of the path part */
	tl = l + strlen(newn) + 3L;				/* total new length */

	if ((path = malloc_chk(tl)) != NULL)	/* allocate space for the new */
	{
		strsncpy(path, oldn, l + 1);		/* copy the path */

		if ( (error = x_checkname( path, newn ) ) == 0 )
		{
			if(*(backsl + 1) != '\0')
				strcat(path, bslash);

			strcat(path, newn);
		}
		else
		{
			free(path);
			path = NULL;
		}
	}

	/* Note: failed malloc will be reported in malloc_chk above */

	xform_error(error); /* Will ignore error >= 0 */

	return path;
}


/*
 * Check if the BEGINNING of a path is a valid disk name
 * This routine does -not- trim leading blanks.
 * Acceptable forms: X:<0> or X:\<0>
 */

bool isdisk(const char *path)
{
	const char *p = path;


	/*
	 * If address exists, index 0 can be examined
	 * If char at index 0 is not 0, index 1 can be examined
	 * If that is not 0, index 2 can be examined
	 */

	/*                 [0]       [1]             [2]            [2]     */

	if( p && isalpha(*(p++)) && *(p++) == ':' && (*p == '\\' || *p == '\0') )
		return TRUE;

	return FALSE;
}


/*
 * Check if a path is to a root directory on drive. This is determined only
 * by analyzing the name. No check of the actual directory is made.
 */

bool isroot(const char *path)
{
	const char *d = nonwhite(path);

	if( isdisk(d) && (d[2] == '\0' || (d[2] == '\\' && d[3] == '\0')) )
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

char *locate(const char *name, _WORD type)
{
	VLNAME
		fname; /* can this be a LNAME ? */

	char
		*newpath,
		*newn,
		*fspec,
		*title,
		*cfgext;
	const char *defext;

	_WORD
		ex_flags;

	bool
		result = FALSE;


	static const _WORD /* don't use const char here! indexes are large */
		titles[] = {FSTLFILE, FSTLPRG, FSTLFLDR, FSTLOADS, FSTSAVES, FSPRINT};


	cfgext = strdup( (type == L_PRINTF) ? PRNEXT : CFGEXT); /* so that strlwr can be done upon it */

#if _MINT_
	if (mint && cfgext)
		strlwr(cfgext);
#endif

	/* Beware of the sequence of L_*; valid for L_LOADCFG, L_SAVECFG, L_PRINTF */

	defext = (type >= L_LOADCFG) ? cfgext : fsdefext;

	if ((fspec = fn_make_newname(name, defext)) == NULL)
			return NULL;

	strcpy(fname, fn_get_name(name));
	ex_flags = EX_FILE;

	free(cfgext);
	title = get_freestring(titles[type]);

	do
	{
		newpath = xfileselector(fspec, fname, title);

		free(fspec);

		if (!newpath)
			return NULL;

		if ((type == L_PROGRAM) && !prg_isprogram(fname))
			alert_iprint(TPLFMT);
		else
		{
			if (((newn = fn_make_newname(newpath, fname)) != NULL) && (type != L_SAVECFG) && (type != L_PRINTF) )
			{
				result = x_exist(newn, ex_flags);

				if (!result)
				{
					alert_iprint(TFILNF);
					free(newn);
				}
			}
			else
				result = TRUE;
		}

		fspec = newpath;
	}
	while (!result);

	free(newpath);

	return newn;
}


/*
 * Get a path or a name using a fileselector and insert into string.
 * This is used in scrolled editable text fields.
 * Note: if flags & 0x8000 is set, path only will be passed.
 * If flags & 0x4000 is set, name only will be passed.
 * These flags are set in the resource as ob_flags; bits 15 and 14 for
 * the scrolled-text FTEXT object.
 */

void get_fsel
(
	XDINFO *info,	/* dialog data */
	char *result,	/* pointer to the string being edited */
	_WORD flags		/* sets whether pathonly or nameonly */
)
{
	char
		*c,			/* pointer to aft part of the string */
		*cc,		/* copy of the above */
		*path,		/* path obtained */
		*title;		/* Selector title */

	long
		pl = 0,			/* path length   */
		fl = 0,			/* name length   */
		tl,				/* total length of inserted string */
		sl,				/* string length */
		ml;				/* possible maximum for tl */

	_WORD
		tid = FSTLANY,
		err = EPTHTL;

	VLNAME			/* actually a LNAME should be enough */
		name;		/* name obtained */

	bool
		addname = ((flags & 0x8000) == 0);


	if(addname)
	{
		if((flags & 0x4000) != 0)
			tid = FSTLFILE;
	}
	else
		tid = FSTLFLDR;

	title = get_freestring(tid);

	name[0] = 0;

	/* Call the fileselector */

	path = xfileselector( fsdefext, name, title ); /* path is allocated here */

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

		if (*name && !isroot(path) && addname )
			strcat(path, bslash);

		fl = strlen(name);
		pl = strlen(path);
		sl = strlen(result);
		tl = 0;			/* length of the string to be inserted */
		ml = XD_MAX_SCRLED - sl - 1;

		cc = NULL;
		c = result + (long)(info->cursor_x); /* part after the cursor */

		if ( (info->cursor_x < sl) && ( (cc = strdup(c)) == NULL ) )
			err = 0; /* error already reported in strdup */
		else
		{
			*c = 0;

			if((flags & 0x4000) == 0)
			{
				tl += pl;

				if(tl > ml)
					goto freepath;

				strsncpy( c, path, pl + 1 ); /* must include null at end here! */
			}

			if ( fl && addname )
			{
				tl += fl;

				if(tl > ml)
					goto freepath;

				strcat( result, name );
			}

			if ( cc )
			{
				strcat( result, cc );
				free(cc);
			}

			info->cursor_x += (_WORD)tl;
			err = 0;
		}

		freepath:;
		xform_error(err);
		free(path);
	}
}


/*
 * Change current directory to "path"
 */

int chdir(const char *path)
{
	const char *h = path;

	_WORD
		error;


	if(isdisk(path))
	{
		x_setdrv((path[0] & 0x5F) - 'A');

		h = path + 2;

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
	return x_setdrv(x_getdrv());
}


bool btst(long x, _WORD bit)
{
	return (x & (1L << bit)) != 0;
}


/*
 * Check if drive "drv" exists. Display an alert if it does not.
 * Note: 26 drives are supported (A to Z),
 * but defined by numbers 0 to 25.
 */

bool check_drive(_WORD drv)
{
	if ((drv >= 0) && (drv <= ('Z' - 'A')) && (btst(drvmap(), drv)))
		return TRUE;

	alert_iprint(MDRVEXIS);

	return FALSE;
}


/*
 * Compare a string (e.g. a filename) against a wildcard pattern;
 *
 * valid wildcards:
 *     *                 = string of any characters
 *     ?                 = any single character
 *     [<char><char>...] = any single character from the enclosed list
 *     !<char>           = any single character except next character
 *     ~                 = negate match (must be the first char in pattern)
 *
 * Return TRUE if matched. Comparison is NOT case-sensitive.
 * Note: speed could be improved by always preparing all-uppercase wildcards
 * (wildcards are stored in window icon lists, program type lists and
 * filemask/documenttype lists, i.e. it is easy to convert to uppercase there).
 *
 * The first two wildcards (* and ?) are legal in single-TOS;
 * the other are legal only in mint. However, TeraDesk permits the
 * use od these other wildcards, within some restictions that are
 * imposed by the validation string of the editable text fields in dialogs.
 *
 * Note: In early versions here were two alternative routines for matching
 * wildcards, depending on whether the Desktop was multitasking-capable
 * or not: match_pattern() and cmp_part(). The first one was used for
 * multitasking-capable desktop and the other for single-tos version only.
 * This was changed so that match_pattern() is always used now.
 */

bool match_pattern(const char *t, const char *pat)
{
	const char *d; 				/* difference in positions */
	const char *pe = NULL;		/* pointer to pattern end */
	const char *te = NULL; 		/* pointer to name end */
	const char *pa = NULL;		/* pointer to last astarisk */
	char u; 					/* uppercased character in pat */
	char tu;					/* uppercased *t */

	bool
		inv = FALSE,
		valid = TRUE;


	/* Is this an exception mask? */

	if(*pat == '~')
	{
		pat++;
		inv = TRUE;
	}

	/* Now match the pattern */

	while(valid && ((*t && *pat) || (!*t && *pat == '*')))	/* HR: catch empty that should be OK */
	{
		switch(*pat)
		{
			case '?':			/* ? means any single character */
			{
				t++;
				pat++;
				break;
			}
			case '*':			/* * means a string of any character */
			{
				if(!te)
					te = t + strlen(t);	/* find the end */

				if(!pe)
				{
					pe = pat;

					while(*pe)	/* find the end and the last asterisk */
					{
						if(*pe == '*')
							pa = pe;

						pe++;
					}
				}

				pat++;

				if(pat > pa)	/* skip irelevant part */
				{
					d = te - (pe - pat);

					if(d > t)
						t = d;
				}

				/*
				 * First character that must be matched after the '*'
				 * this code could be smaller, but slower
				 */

				if(*pat == '!')
				{
					pat++;

					if(*pat)	/* guard against invalidly specified masks */
					{
						u = (char)touppc(pat[1]);	/* the first one after that */

						while(((tu = touppc(*t)) != 0) && touppc(t[1]) != u)
							t++;

						goto exceptch;
					}
				}
				else if(*pat == '[')
				{
					d = strchr(pat,']');

					if(d)	/* guard against invalidly specified masks */
					{
						u = (char)touppc(d[1]);

						while(((tu = touppc(*t)) != 0) && touppc(t[1]) != u)
							t++;

						goto anych;
					}
				}
				else
				{
					u = (char)touppc(*pat);

					while(*t &&(touppc(*t) != u))
						t++;

					break;
				}

				valid = FALSE;
				break;
			}
			case '!':			/* !X means any character but X */
			{
				tu = touppc(*t);
				pat++;

				exceptch:;

				u = (char)touppc(*pat);

				if (u && tu != u)
				{
					t++;
					pat++;
					break;
				}

				valid = FALSE;
				break;
			}
			case '[':			/* [<chars>] means any one of <chars> */
			{
				tu = touppc(*t);
				d = strchr(pat, ']');

				if(d)
				{
					anych:;

					while(++pat < d && (tu != touppc(*pat)));

					if(pat < d)
					{
						pat = d;
						pat++;
						t++;
						break;
					}
				}

				valid = FALSE;
				break;
			}
			default:	/* exact match on anything else, case insensitive */
			{
				if (touppc(*t++) == touppc(*pat++))
					break;

				valid = FALSE;
			}
		}
	}

	return (valid && (touppc(*t) == touppc(*pat))) ^ inv;
}


/*
 * Compare a filename against a wildcard pattern
 * Some special considerations of the '.' character in single-TOS
 */

bool cmp_wildcard(const char *fname, const char *pat)
{
	char
		*dot = NULL;

	bool
		matched;


#if _MINT_
	if (!mint)
#endif
	{
		/* Is there a dot in the wildcard pattern? */

		dot = strchr(pat, '.');

		if (dot)
		{
			/*
			 * If there is a dot and...
			 * the rest of the wildcard is '*', and
			 * there is no dot in the name, ignore the dot
			 * so that names without it can still be matched
			 */

			if (dot[1] == '*' && dot[2] == '\0' && strchr(fname, '.') == NULL)
				*dot = '\0';
		}
		else if (strchr(fname, '.') != NULL)
			return FALSE;
	}

	/* Now match the pattern */

	matched = match_pattern(fname, pat);

	if (dot)
		*dot = '.'; /* restore the dot which was removed earlier */

	return matched;
}


static _WORD chdrv;
static long cdecl (*Oldgetbpb) (_WORD);
static long cdecl (*Oldmediach) (_WORD);
static long cdecl (*Oldrwabs) (_WORD, void *, _WORD, _WORD, _WORD, long);

#define hdv_bpb              ( *( long cdecl (**)( _WORD dev ) ) 0x472L )
#define hdv_rw               ( *( long cdecl (**)( _WORD rwflag,void *buf,_WORD cnt,_WORD recnr,_WORD dev,long lrecno)) 0x476L )
#define hdv_mediach  ( *( long cdecl (**)( _WORD dev ) ) 0x47EL )

/* HR: The AHCC generated code uses a6, which wasnt good on my MILAN Tos */
/*     04'10 Coldfire */

#ifdef __AHCC__
static long __asm__ cdecl Newgetbpb(_WORD d)
{
	move.l	d2,-(a7)
	move.l	d3,-(a7)
	move	12(sp),d3		; 4(sp) + 8
	cmp		chdrv.l,d3
	bne.s	L44
	lea 	1138,a0
	move.l	Oldgetbpb.l,(a0)
	lea 	1142,a0
	move.l	Oldrwabs.l,(a0)
	lea 	1150,a0
	move.l	Oldmediach.l,(a0)
L44:
	move	d3,-(a7)
	movea.l	Oldgetbpb.l,a0
	jsr		(a0)
	addq.l	#2,a7
	move.l	(a7)+,d3
	move.l	(a7)+,d2
	rts
}
#else
static long cdecl Newgetbpb(_WORD d)
{
	if (d == chdrv)
	{
		hdv_bpb = Oldgetbpb;		/*  *((Func *)0x472L)  */
		hdv_rw = Oldrwabs;			/*  *((Func *)0x476L)  */
		hdv_mediach = Oldmediach;	/*  *((Func *)0x47eL)  */
	}

	return (*Oldgetbpb)(d);
}
#endif


/*
 * HR:
 * The following 2 functions didnt suffer, but I have done the same
 * and then disabled. Rwabs is a very dangerous function :-)
 */

#if 0 /* __AHCC__ */
static long __asm__ cdecl Newmediach(_WORD d)
{
	movem.l	d2-d3,-(a7)
	move	12(sp),d3		; 4(sp) + 8
	cmp		chdrv.l,d3
	bne.s	L94
	moveq	#2,d0
L86:
	movem.l	(a7)+,d2-d3
	rts

L94:
	move	d3,-(a7)
	movea.l	Oldmediach.l,a0
	jsr		(a0)
	addq	#2,a7
	bra.s	L86
}
#else
static long cdecl Newmediach(_WORD d)
{
	if (d == chdrv)
		return 2;
	else
		return (*Oldmediach)(d);
}
#endif


#if 0 /* __AHCC__ */
static long __asm__ cdecl Newrwabs(_WORD d, void *buf, _WORD a, _WORD b, _WORD c, long l)
{
	move.l	sp,a0			; stackframe pointer --> a0
	movem.l	d2-d3,-(a7)
	move	12(sp),d3		; 4(sp) + 8
	cmp		chdrv.l,d3
	bne.s	L158
	moveq	#-14,d0
L150:
	movem.l	(a7)+,d2-d3
	rts

L158:
	move.l	16(a0),-(a7)	; l
	move	14(a0),-(a7)	; c
	move	12(a0),-(a7)	; b
	move	10(a0),-(a7)	; a
	move.l	 6(a0),-(a7)	; buf
	move	 4(a0),-(a7)	; d
	movea.l	Oldrwabs.l,a0
	jsr		(a0)
	lea 	16(a7),a7
	bra.s	L150
}
#else
static long cdecl Newrwabs(_WORD d, void *buf, _WORD a, _WORD b, _WORD c, long l)
{
	if (d == chdrv)
		return MEDIA_CHANGE;
	else
		return (*Oldrwabs)(d, buf, a, b, c, l);
}
#endif


/*
 * Force media change on drive contained in path.
 * This routine is used un such a way that there is no chance
 * of path being undefined or too short
 */

void force_mediach(const char *path)
{
	_WORD
		drive,
		p = *path;

	if(!isdisk(path))
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

#if 0 /* replaced with equivalent code below */
		Oldrwabs = *((Func *)0x476L);
		Oldgetbpb = *((Func *)0x472L);
		Oldmediach = *((Func *)0x47eL);
#endif
		Oldrwabs = hdv_rw;
		Oldgetbpb = hdv_bpb;
		Oldmediach = hdv_mediach;

#if 0 /* replaced with equivalent code below */
		*((Func *)0x476L) = Newrwabs;
		*((Func *)0x472L) = Newgetbpb;
		*((Func *)0x47eL) = Newmediach;
#endif
		hdv_rw = Newrwabs;
		hdv_bpb = Newgetbpb;
		hdv_mediach = Newmediach;

		fname[0] = drive + 'A';
		r = Fopen(fname, 0);

		if (r >= 0)
			Fclose((_WORD)r);

		if (*((void **)0x476L) == (void *)Newrwabs)
		{
#if 0 /* replaced with equivalent code below */
			*((Func *)0x472L) = Oldgetbpb;
			*((Func *)0x476L) = Oldrwabs;
			*((Func *)0x47eL) = Oldmediach;
#endif
			hdv_bpb = Oldgetbpb;
			hdv_rw = Oldrwabs;
			hdv_mediach = Oldmediach;
		}

		Super(stack);
	}
}


/*
 * Convert a TOS filename (8+3) into a string suitable for display
 * (with left-justified file extension, e.g.: FFFF    XXX without ".")
 * This routine is also used to create 8-characters-long program name
 * when searching with appl_find().
 */

void cv_tos_fn2form(char *dest, const char *source)
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
	{
		if (*s != ' ')
			*d++ = *s++;
		else
			s++;
	}

	if (*s)
	{
		*d++ = '.';

		while (*s != 0)
		{
			if (*s != ' ')
				*d++ = *s++;
			else
				s++;
		}
	}

	*d = 0;
}


/*
 * Wrapper function for editable and possibly userdef fields.
 * Fit a (possibly long) filename or path into a form (dialog) field
 * Note: now it is assumed that if the field is 12 characters long,
 * then it will be for a 8+3 format (see also routine tos_fnform in resource.c)
 * Note: if the name is too long, it will be trimmed.
 */

void cv_fntoform(OBJECT *tree, _WORD object, const char *src)
{
	/*
	 * Determine destination and what is the available length
	 * Beware: "l" is the complete allocated length,
	 * zero termination-byte must fit into it
	 */

	OBJECT
		*ob = tree + object;

	TEDINFO
		*ti = xd_get_obspecp(ob)->tedinfo;

	char
		*dst = ti->te_ptext;

	long
		l = (long)ti->te_txtlen;


	/*
	 * The only 12-chars long fields in TeraDesk should be
	 * for 8+3 names, possibly even if Mint/Magic is around
	 */

	if ( l < 13L)
		cv_tos_fn2form(dst, src);
	else
	{
		if (ob->ob_flags & OF_EDITABLE )
		{
			if(xd_xobtype(ob) == XD_SCRLEDIT)
			{
				l = (long)sizeof(VLNAME);
				xd_init_shift(ob, src); /* will not work if strlen(dest) > sizeof(VLNAME) */
			}

			strsncpy(dst, src, (size_t)l); 		/* term. byte included in l */

			if ( strlen(src) >= l )
				alert_iprint(TFNTLNG);
		}
		else
			cramped_name(src, dst, l); 	/* term. byte included */
	}
}


/*
 * Convert filename from the dialog form into a convenient string.
 * This routine does not allocate any space for destination.
 */

void cv_formtofn(char *dst, OBJECT *tree, _WORD object)
{
	TEDINFO
		*ti = xd_get_obspecp(tree + object)->tedinfo;

	char
		*src = ti->te_ptext;


	/*
	 * The only 12-characters-long fields in TeraDesk should be
	 * for 8+3 names, possibly even if Mint/Magic is around
	 */

	if(ti->te_txtlen < 13)
		cv_tos_form2fn(dst, src);
	else
		strip_name(dst, src);
}
