/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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


typedef struct appl
{
	/* ---vvv--- compatible with PRGTYPE structure */
	/* ---vvv--- compatible with FTYPE and LSTYPE structures */
	SNAME shname;
	struct appl *next;
	/* ---^^^--- compatible with FTYPE and LSTYPE structures */
	ApplType appltype;
	long limmem;
	_WORD flags;				/* argv + path + single */
	/* ---^^^--- compatible with PRGTYPE structure */
	char *cmdline;
	char *localenv;
	char *name;
	FTYPE *filetypes;
	_WORD fkey;
} APPLINFO;

extern APPLINFO 
	awork,
	*applikations;

extern _WORD naap;



void app_config(XFILE *file, int lvl, int io, int *error);

void app_init(void);
void app_default(void);
void app_install(_WORD use, APPLINFO **applist);
APPLINFO *app_find(const char *file, bool dial);
APPLINFO *find_fkey(_WORD fkey);
bool app_exec(const char *program, APPLINFO *appl, WINDOW *w, _WORD *sellist, _WORD n, _WORD kstate);
void app_update(wd_upd_type type, const char *fname1, const char *fname2);
_WORD app_specstart(_WORD flags, WINDOW *w, _WORD *list, _WORD n, _WORD kstate);
bool app_checkspec(_WORD flag, _WORD pos);
void log_shortname( char *dest, const char* appname ); 
char *app_find_name(const char *path, bool full);
