/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2008  Dj. Vukovic
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


#define DP_PATHMAX	2
#define DP_NAMEMAX	3

/* TOS file attributtes */

#define FA_PARDIR       0x40 /* pseudo attribute for parent dir FIXME: used by MagiC */

#define FA_ANY  (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_SUBDIR | FA_ARCHIVE)

#define DEFAULT_DIRMODE (0777)
#define DEFAULT_MODE    (0666)
#define EXEC_MODE		(S_IXUSR | S_IXGRP | S_IXOTH)

/* Filesystem characteristics bitflags */

#define FS_TOS 0x0000	/* plain TOS/DOS FAT filesystem */
#define FS_LFN 0x0001	/* supports long filenames */
#define FS_LNK 0x0002	/* supports symbolic links */
#define FS_UID 0x0004	/* supports user/group IDs */
#define FS_CSE 0x0008	/* supports case-sensitive names */
#define FS_ANY 0x000f	/* bitmask for all */
#define FS_INQ 0x0100	/* inquire about the filesystem */

/* Modes voor x_exist */

#define EX_FILE		1
#define EX_DIR		2
#define EX_LINK		4	/* don't follow the link */

typedef struct          /* used by Pexec */
{
	unsigned char length;
	char command_tail[128];
} COMMAND;


typedef struct
{
	char *path;
	union
	{
		long handle;
		struct
		{
			int first;			/* 1 = eerste file lezen, 0 = huidige lezen. */
			_DTA *old_dta;
			_DTA dta;
		} gdata;
	} data;
#if _MINT_
	int type;
#endif
} XDIR;

typedef struct
{
	int handle;
	int mode;
	int bufsize;
	int read;
	int write;
	char *buffer;
	unsigned int eof : 1;
	unsigned int memfile : 1;
} XFILE;

#define dos_mtime(st)   (((DOSTIME *)&(st)->st_mtime)->time)
#define dos_mdate(st)   (((DOSTIME *)&(st)->st_mtime)->date)
#define dos_atime(st)   (((DOSTIME *)&(st)->st_atime)->time)
#define dos_adate(st)   (((DOSTIME *)&(st)->st_atime)->date)
#define dos_ctime(st)   (((DOSTIME *)&(st)->st_ctime)->time)
#define dos_cdate(st)   (((DOSTIME *)&(st)->st_ctime)->date)

/* niet GEMDOS en MiNT funkties */

int x_checkname(const char *path, const char *name);
char *x_makepath(const char *path, const char *name, int *error);
bool x_exist(const char *file, int flags);
bool x_netob(const char *name);
char *x_fullname(const char *file, int *error);

/* Directory funkties */

int x_setpath(const char *path);
char *x_getpath(int drive, int *error);
int x_mkdir(const char *path);
int x_rmdir(const char *path);
int x_mklink(const char *linkname, const char *refname);
int x_rdlink(size_t tgtsize, char *tgt, const char *linkname );
char *x_pathlink( char *tgtname, char *linkname );
char *x_fllink( char *linkname );
int x_dfree(_DISKINFO *diskinfo, int drive);
int x_getdrv(void);
long x_setdrv(int drive);
int x_getlabel(int drive, char *label);
int x_putlabel(int drive, char *label);

/* File funkties */

int x_rename(const char *oldn, const char *newn);
int x_unlink(const char *file);
int x_fattrib(const char *file, XATTR *attr);
int x_datime(_DOSTIME *time, int handle, int wflag);
int x_open(const char *file, int mode);
int x_create(const char *file, XATTR *attr);
int x_close(int handle);
long x_read(int handle, long count, char *buf);
long x_write(int handle, long count, char *buf);
long x_seek(long offset, int handle, int seekmode);

/* Funkties voor het lezen van een directory */

XDIR *x_opendir(const char *path, int *error);
long x_xreaddir(XDIR *dir, char **buffer, size_t len, XATTR *attrib); 
long x_rewinddir(XDIR *dir);
long x_closedir(XDIR *dir);
long x_attr(int flag, int fs_type, const char *name, XATTR *attrib);

/* Configuratie funkties */

long x_pathconf(const char *path, int which);

/* Funkties voor het uitvoeren van programma's */

long x_exec(int mode, void *ptr1, void *ptr2, void *ptr3);

/* GEM funkties */

char *xshel_find(const char *file, int *error);
char *xfileselector(const char *path, char *name, const char *label);

/* Vervangers voor de standaard bibliotheek. */

XFILE *x_fopen(const char *file, int mode, int *error);
XFILE *x_fmemopen(int mode, int *error);
int x_fclose(XFILE *file);
long x_fread(XFILE *file, void *ptr, long length);
long x_fwrite(XFILE *file, void *ptr, long length);
long x_fseek(XFILE *file, long offset, int mode);
char *x_freadstr(XFILE *file, char *string, size_t max, int *error);
int x_fwritestr(XFILE *file, const char *string);
int x_fgets(XFILE *file, char *string, int n);
int x_fprintf(XFILE *file, char *format, ...);
bool x_feof(XFILE *file);
int x_inq_xfs(const char *path);
long x_pflags(char *filename);

void x_init(void);
