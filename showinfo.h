/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2013  Dj. Vukovic
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

#define MFINDS 999  /* max. number of found instances of searched-for string */

#if _SHOWFIND
extern _WORD search_nsm;
extern long find_offset;
#endif

void item_showinfo(WINDOW *w, _WORD n, _WORD *list, bool search); 
void closeinfo (void);
_WORD si_drive(const char *path);
_WORD object_info(ITMTYPE type, const char *oldname, const char *fname, XATTR *attr);
bool searched_found( const char *path, const char *name, XATTR *attr);
