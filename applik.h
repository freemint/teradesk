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

typedef struct appl
{
/* ---vvv--- compatible with PRGTYPE structure */
 /* ---vvv--- compatible with FTYPE and LSTYPE structures */
	SNAME shname;
	struct appl *next;
 /* ---^^^--- compatible with FTYPE and LSTYPE structures */
	ApplType appltype;
	boolean argv;
	boolean path;
	boolean single;
	long limmem;
	int flags;				/* temporary; for load/save same as argv + path + single */
/* ---^^^--- compatible with PRGTYPE structure */
	char *cmdline;
	char *name;
	int fkey;
	boolean edit;
	boolean autostart;
	FTYPE *filetypes;
} APPLINFO;

extern APPLINFO *applikations;

CfgNest app_config;

void app_init(void);
void app_default(void);

#if !TEXT_CFG_IN
int app_load(XFILE *file);
#endif

void app_install(void);
APPLINFO *app_find(const char *file);
APPLINFO *find_appl(APPLINFO **list, const char *program, int *pos);
APPLINFO *find_fkey(int fkey);
boolean app_exec(const char *program, APPLINFO *appl, WINDOW *w, int *sellist, int n, int kstate, boolean dragged);
void app_update(wd_upd_type type, const char *fname1, const char *fname2);
void app_autostart(void);
