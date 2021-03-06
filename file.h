/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2006  Dj. Vukovic
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


#define DEFAULT_EXT presets[0]
#define TOSDEFAULT_EXT presets[1]

/* Modes for locate(); must be in sequence */

#define L_FILE		0	/* file */
#define L_PROGRAM	1	/* program */
#define L_FOLDER	2	/* folder (currently not used) */
#define L_LOADCFG	3	/* load configuration */
#define L_SAVECFG	4	/* save configuration */
#define L_PRINTF	5	/* print file */

extern const char *fsdefext;

bool btst(long x, _WORD bit);
int make_path( char *name,const char *path,const char *fname );
void split_path( char *path,char *fname,const char *name );
void path_to_disp(char *path);
const char *fn_get_name(const char *path);
char *fn_get_path(const char *path);
char *fn_make_path(const char *path, const char *name);
char *fn_make_newname(const char *oldn, const char *newn);
bool isdisk(const char *path);
bool isroot(const char *path);
int chdir(const char *path);
long drvmap(void);
bool check_drive(_WORD drv);
bool cmp_wildcard(const char *fname, const char *wildcard);
bool cmp_part(const char *name, const char *wildcard);
bool match_pattern(const char *t, const char *pat);	
char *locate(const char *name, _WORD type);
void force_mediach(const char *path);
void cv_formtofn(char *dest, OBJECT *tree, _WORD object);
void cv_fntoform(OBJECT *tree, _WORD object, const char *source);
void cv_tos_fn2form(char *dest, const char *source);
