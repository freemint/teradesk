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

boolean item_copy(WINDOW *dw, int dobject, WINDOW *sw, int n,
				  int *list, int kstate);


#define CMD_COPY	 0
#define CMD_MOVE	 1
#define CMD_DELETE	 2
#define CMD_PRINT    3
#define CMD_PRINTDIR 4
#define CMD_TOUCH    8 

extern boolean cfdial_open;
extern boolean rename_files;

int open_cfdialog(long folders, long files, long bytes, int function);
void close_cfdialog(int button);
void upd_copyinfo(long folders, long files, long bytes);
void upd_copyname(const char *dest, const char *path, const char *name);
int copy_error(int error, const char *name, int function);

boolean itmlist_op(WINDOW *w, int n, int *list, const char *dest, int function);
int touch_file( const char *fullname, DOSTIME *time, int attr, boolean link );
int frename(const char *oldfname, const char *newfname);