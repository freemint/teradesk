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

#define DEFAULT_EXT "*"
#define TOSDEFAULT_EXT "*.*"

/* Modes voor locate */

#define L_FILE		0
#define L_PROGRAM	1
#define L_FOLDER	2
#define L_LOADCFG	3
#define L_SAVECFG	4

void path_to_disp(char *path);
char *fn_get_name(const char *path);
char *fn_get_path(const char *path);
char *fn_make_path(const char *path, const char *name);
char *fn_make_newname(const char *oldname, const char *newname);

void getroot(char *root);
boolean isroot(const char *path);
char *getdir(int *error);
int chdir(const char *path);


int cnt_items(const char *path, long *folders, long *files, long *bytes, int attrib, boolean search);

long drvmap(void);
boolean check_drive(int drv);

boolean cmp_wildcard(const char *fname, const char *wildcard);
boolean cmp_part(const char *name, const char *wildcard);
bool match_pattern(const char *t, const char *pat);	

char *locate(const char *name, int type);

void force_mediach(const char *path);

void cv_formtofn(char *dest, OBJECT *ob);
void cv_fntoform(OBJECT *ob, const char *source);
void cramped_name(const char *s, char *t, int w);
void strip_name (char *dst, const char *src);