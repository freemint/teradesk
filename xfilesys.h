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

#ifndef __XFILESYS_H__
#define __XFILESYS_H__ 1

#define DP_PATHMAX	2
#define DP_NAMEMAX	3

extern const char *presets[];

/* Modes voor x_open en x_fopen */

#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_ACCMODE
#undef O_DENYRW
#undef O_DENYW
#undef O_DENYR
#undef O_DENYNONE
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_ACCMODE   3
#define O_DENYRW	0x10
#define O_DENYW		0x20
#define O_DENYR		0x30
#define O_DENYNONE	0x40

/* TOS file attributtes */

#define FA_PARDIR       0x40 /* pseudo attribute for parent dir FIXME: used by MagiC */

#define FA_ANY  (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_DIR | FA_CHANGED)

#define DEFAULT_DIRMODE (0777)
#define DEFAULT_MODE    (0666)
#define EXEC_MODE		(S_IXUSR | S_IXGRP | S_IXOTH)

/* The requests for Dpathconf() */
#define DP_IOPEN	0	/* internal limit on # of open files */
#define DP_MAXLINKS	1	/* max number of hard links to a file */
#define DP_PATHMAX	2	/* max path name length */
#define DP_NAMEMAX	3	/* max length of an individual file name */
#define DP_ATOMIC	4	/* # of bytes that can be written atomically */
#define DP_TRUNC	5	/* file name truncation behavior */
#	define	DP_NOTRUNC	0	/* long filenames give an error */
#	define	DP_AUTOTRUNC	1	/* long filenames truncated */
#	define	DP_DOSTRUNC	2	/* DOS truncation rules in effect */
#define DP_CASE		6
#	define	DP_CASESENS	0	/* case sensitive */
#	define	DP_CASECONV	1	/* case always converted */
#	define	DP_CASEINSENS	2	/* case insensitive, preserved */
#define DP_MODEATTR		7
#	define	DP_ATTRBITS	0x000000ffL	/* mask for valid TOS attribs */
#	define	DP_MODEBITS	0x000fff00L	/* mask for valid Unix file modes */
#	define	DP_FILETYPS	0xfff00000L	/* mask for valid file types */
#	define	DP_FT_DIR	0x00100000L	/* directories (always if . is there) */
#	define	DP_FT_CHR	0x00200000L	/* character special files */
#	define	DP_FT_BLK	0x00400000L	/* block special files, currently unused */
#	define	DP_FT_REG	0x00800000L	/* regular files */
#	define	DP_FT_LNK	0x01000000L	/* symbolic links */
#	define	DP_FT_SOCK	0x02000000L	/* sockets, currently unused */
#	define	DP_FT_FIFO	0x04000000L	/* pipes */
#	define	DP_FT_MEM	0x08000000L	/* shared memory or proc files */
#ifndef DP_XATTRFIELDS
#define DP_XATTRFIELDS 8		/* information about supported extended attributes */
#  define   DP_INDEX    (0x0001)    /* index field unique for every file on the fs */
#  define   DP_DEV      (0x0002)    /* device field valid */
#  define   DP_RDEV     (0x0004)    /* rdev field valid (and not identical to dev) */
#  define   DP_NLINK    (0x0008)    /* number of links valid */
#  define   DP_UID      (0x0010)    /* user id valid */
#  define   DP_GID      (0x0020)    /* group id valid */
#  define   DP_BLKSIZE  (0x0040)    /* block size valid */
#  define   DP_SIZE     (0x0080)    /* size field valid (and meaningful!) */
#  define   DP_NBLOCKS  (0x0100)    /* number of blocks valid */
#  define   DP_ATIME    (0x0200)    /* file system has last access time */
#  define   DP_CTIME    (0x0400)    /* file system has last status change time */
#  define   DP_MTIME    (0x0800)    /* file system has last modification time */
#endif

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
	char command_tail[128]; /* FIXME: should that be 127? */
} COMMAND;


typedef struct
{
	const char *path;
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
	_WORD handle;
	_WORD mode;
	_WORD bufsize;
	_WORD read;
	_WORD write;
	char *buffer;
	unsigned int eof : 1;
	unsigned int memfile : 1;
} XFILE;

#define dos_mtime(st)   ((st)->st_mtim.u.d.time)
#define dos_mdate(st)   ((st)->st_mtim.u.d.date)
#define dos_atime(st)   ((st)->st_atim.u.d.time)
#define dos_adate(st)   ((st)->st_atim.u.d.date)
#define dos_ctime(st)   ((st)->st_ctim.u.d.time)
#define dos_cdate(st)   ((st)->st_ctim.u.d.date)

/* niet GEMDOS en MiNT funkties */

_WORD x_checkname(const char *path, const char *name);
char *x_makepath(const char *path, const char *name, _WORD *error);
bool x_exist(const char *file, _WORD flags);
bool x_netob(const char *name);
char *x_fullname(const char *file, _WORD *error);

/* Directory funkties */

_WORD x_setpath(const char *path);
char *x_getpath(_WORD drive, _WORD *error);
_WORD x_mkdir(const char *path);
_WORD x_rmdir(const char *path);
_WORD x_mklink(const char *linkname, const char *refname);
_WORD x_rdlink(size_t tgtsize, char *tgt, const char *linkname );
char *x_pathlink( char *tgtname, const char *linkname );
char *x_fllink( const char *linkname );
_WORD x_dfree(_DISKINFO *diskinfo, _WORD drive);
_WORD x_getdrv(void);
long x_setdrv(_WORD drive);
_WORD x_getlabel(_WORD drive, char *label);
_WORD x_putlabel(_WORD drive, char *label);

/* File funkties */

_WORD x_rename(const char *oldn, const char *newn);
_WORD x_unlink(const char *file);
_WORD x_fattrib(const char *file, XATTR *attr);
_WORD x_datime(_DOSTIME *time, _WORD handle, _WORD wflag);
_WORD x_open(const char *file, _WORD mode);
_WORD x_create(const char *file, XATTR *attr);
_WORD x_close(_WORD handle);
long x_read(_WORD handle, long count, char *buf);
long x_write(_WORD handle, long count, char *buf);
long x_seek(long offset, _WORD handle, _WORD seekmode);

/* Funkties voor het lezen van een directory */

XDIR *x_opendir(const char *path, _WORD *error);
long x_xreaddir(XDIR *dir, char **buffer, size_t len, XATTR *attrib); 
long x_rewinddir(XDIR *dir);
long x_closedir(XDIR *dir);
long x_attr(_WORD flag, _WORD fs_type, const char *name, XATTR *attrib);

/* Configuratie funkties */

long x_pathconf(const char *path, _WORD which);

/* Funkties voor het uitvoeren van programma's */

long x_exec(_WORD mode, const char *fname, void *ptr2, void *ptr3);

/* GEM funkties */

char *xshel_find(const char *file, _WORD *error);
char *xfileselector(const char *path, char *name, const char *label);

/* Vervangers voor de standaard bibliotheek. */

XFILE *x_fopen(const char *file, _WORD mode, _WORD *error);
XFILE *x_fmemopen(_WORD mode, _WORD *error);
_WORD x_fclose(XFILE *file);
long x_fread(XFILE *file, void *ptr, long length);
long x_fwrite(XFILE *file, const void *ptr, long length);
long x_fseek(XFILE *file, long offset, _WORD mode);
char *x_freadstr(XFILE *file, char *string, size_t max, _WORD *error);
_WORD x_fwritestr(XFILE *file, const char *string);
_WORD x_fgets(XFILE *file, char *string, _WORD n);
bool x_feof(XFILE *file);
_WORD x_inq_xfs(const char *path);
long x_pflags(const char *filename);

void x_init(void);

#endif
