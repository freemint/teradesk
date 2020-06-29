/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren.
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

/* DjV 017 150103 280103 ---vvv--- */
void item_showinfo(WINDOW *w, int n, int *list, int search);
void closeinfo (void);
int si_drive(const char *path, int *x, int *y);
int folder_info(const char *oldname, const char *fname, XATTR *attr);
int file_info(const char *oldname, const char *fname, XATTR *attr);
void path_to_disp(char *path);
/* DjV 017 150103 ---^^^--- */
