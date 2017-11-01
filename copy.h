/*
 * Teradesk. Copyright (c)       1993, 1994, 2002  W. Klaren,
 *                               2002, 2003, 2011  H. Robbers,
 *                         2003, 2004, 2005, 2011  Dj. Vukovic
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


#ifndef __COPY_H__
#define __COPY_H__ 1

#define CMD_COPY	 0
#define CMD_MOVE	 1
#define CMD_DELETE	 2
#define CMD_PRINT    3
#define CMD_PRINTDIR 4
#define CMD_TOUCH    8


#define mustabort(x) ((x == XABORT) || (x == XFATAL))

typedef struct
{
	long kbytes;
	long bytes;
}LSUM;

extern _DOSTIME now;
extern _DOSTIME optime;

#if _MINT_
extern _WORD opmode;
extern _WORD opuid;
extern _WORD opgid;
#endif
extern _WORD opattr;

extern bool cfdial_open;
extern bool rename_files;

void add_size(LSUM *nbytes, long fsize);
void sub_size(LSUM *nbytes, long fsize);
void size_sum(long *total, LSUM *bytes);
_WORD cnt_items(const char *path, long *folders, long *files, LSUM *bytes, _WORD attrib, bool search);
void check_opabort (_WORD *result);
bool item_copy(WINDOW *dw, _WORD dobject, WINDOW *sw, _WORD n, _WORD *list, _WORD kstate);
_WORD open_cfdialog(long folders, long files, LSUM *bytes, _WORD function);
void close_cfdialog(_WORD button);
void upd_copyinfo(long folders, long files, LSUM *bytes);
void upd_copyname(const char *dest, const char *path, const char *name);
_WORD copy_error(_WORD error, const char *name, _WORD function);
bool itmlist_op(WINDOW *w, _WORD n, _WORD *list, const char *dest, _WORD function);
bool itmlist_wop(WINDOW *w, _WORD n, _WORD *list, _WORD function);
_WORD touch_file( const char *fullname, _DOSTIME *time, XATTR *attr, bool link);
_WORD frename(const char *oldfname, const char *newfname, XATTR *attr);

#endif /* __COPY_H__ */
