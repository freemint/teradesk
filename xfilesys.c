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


#include <library.h>
#include <xdialog.h>
#include <mint/cookie.h>
#include <fcntl.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"
#include "prgtype.h"

#define XBUFSIZE	2048L				/* size of a buffer used in file reading/writing */


/* 
 * If _CHECK_RWMODE is set to 0, there will be no checks whether xfile->mode
 * is exactly O_RDONLY or O_WRONLY; 
 * if it is not O_WRONLY, O_RDONLY will be assumed
 */

#define _CHECK_RWMODE 0

static bool flock;



/* 
 * Aux. function for size optimization when returning results
 * of some GEMDOS operations.
 */

static long x_retresult(long result)
{
	return (result < 0) ? (long) xerror((_WORD) result) : result;
}


/*
 * Check if a filename or path + name is valid in this OS.
 * Also check if it fits into buffers.
 * Return 0 if OK.
 * 'path' must exist even if empty; 'name' can be NULL or empty.
 */

_WORD x_checkname(const char *path, const char *name)
{
	long nl = 0;
	long mp = sizeof(VLNAME);

	if (!x_netob(path))					/* only if this is not a network object... */
	{
		if (*path)						/* and a path is given */
		{
			if (!isdisk(path))			/* if it is not on a disk, return error */
				return ENOTDIR;

			mp = x_pathconf(path, DP_PATHMAX);	/* maximum possible length */

			if (mp < 0)
				return xerror((_WORD) mp);
		}

		nl = (long) strlen((name) ? name : fn_get_name(path));	/* name length */

		/* Avoid needless interrogation of drive if no name given */

		if ((*path && nl && nl > x_pathconf(path, DP_NAMEMAX)) || nl >= (long) sizeof(LNAME))
			return EFNTL;
	}

	if ((long) (strlen(path) + nl + 2L) > lmin(mp, (long) sizeof(VLNAME)))
		return ENAMETOOLONG;

	return 0;
}


/* 
 * Create a path+filename string from path "path" and filename "name".
 * Return a pointer to the created string (being allocated in this routine)
 */

char *x_makepath(const char *path, const char *name, _WORD *error)
{
	char *p;

	if ((p = malloc(strlen(path) + strlen(name) + 2)) != NULL)
	{
		*error = make_path(p, path, name);

		if (*error != 0)
		{
			free(p);
			p = NULL;
		}
	} else
	{
		*error = ENOMEM;
	}
	
	return p;
}


/* 
 * Check if a file or folder or link exists. Return true if it exists.
 * If EX_LINK is set in flags, check for the link itself;
 * otherwise follow the link and check for target existence. 
 */

bool x_exist(const char *file, _WORD flags)
{
	XATTR attr;
	unsigned short itype;
	_WORD theflag;

#if _MINT_
	if (x_attr((flags & EX_LINK) ? 1 : 0, FS_INQ, file, &attr) < 0)
#else
	if (x_attr(1, FS_INQ, file, &attr) < 0)
#endif
	{
		return FALSE;					/* x_attr can't find target */
	} else
	{
		itype = attr.st_mode & S_IFMT;

		switch (itype)
		{
		case S_IFDIR:
			theflag = EX_DIR;
			break;
#if _MINT_
		case S_IFLNK:
			theflag = EX_LINK;
			break;
#endif
		default:
			theflag = EX_FILE;
			break;
		}

		if ((flags & theflag) == 0)
			return FALSE;

		return TRUE;
	}
}


/*
 * Check if a name points to a network object, to be accessed
 * as http:, https:, ftp:, mailto: or telnet: target. Return TRUE if it is.
 * Comparison is case insensitive. If a new network object type is added,
 * take care to change loop exit count below.
 */

bool x_netob(const char *name)
{
	_WORD i;
	static const char *pfx[] = { "http:", "https:", "ftp:", "mailto:", "telnet:" };

	if (*name && name[1] != ':')		/* don't check further if not necessary */
	{
		for (i = 0; i < 5; i++)
		{
			if (strnicmp(name, pfx[i], strlen(pfx[i])) == 0)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


/* 
 * Set a directory path. This works on current drive only, and
 * 'path' hould bagin with a backslash, not wioth a drive letter
 */

_WORD x_setpath(const char *path)
{
	return xerror(Dsetpath(path));
}


/* 
 * Get current default path on the specified drive, return pointer to
 * this new-allocated string. If drive is specified as 0, also get 
 * default drive. The resulting path will not be longer than VLNAME.
 * Drive 1 = A: 2 = B: 3 = C: , etc. Beware: for x_getdrv() 0=A: 1=B; 2=C: ...
 */

char *x_getpath(_WORD drive, _WORD *error)
{
	VLNAME tmp;
	char *buffer = NULL;
	char *t = tmp;
	long e;

	/* Put drive id at the beginning of the temporary buffer */

	*t++ = (char) (((drive == 0) ? x_getdrv() : (drive - 1)) + 'A');
	*t++ = ':';
	*t = 0;								/* t is now tmp + 2 */

#if _MINT_
	if (mint)
	{
		/* Use Dgetcwd so that length can be limited */

		e = Dgetcwd(t, drive, (_WORD) sizeof(VLNAME) - 2);

		if (e == EBADARG)
			e = ENAMETOOLONG;
	} else
#endif
	{
		/* In single-TOS this should be safe */

		e = Dgetpath(t, drive);
	}

	*error = xerror((_WORD) e);

	/* Create output buffer only if there are no errors */

	if (*error == 0)
		buffer = strdup(tmp);

	return buffer;
}


/* 
 * Create a directory 
 */

_WORD x_mkdir(const char *path)
{
	return xerror(Dcreate(path));
}


/* 
 * Remove a directory 
 */

_WORD x_rmdir(const char *path)
{
	return xerror(Ddelete(path));
}


/*
 * Modify a name string so that it represents a full path + name.
 * The returned string can be longer than the initial one, so take
 * care to have a sufficient buffer. 
 * The resulting name will never be longer than VLNAME.
 */

static _WORD _fullname(char *buffer)
{
	_WORD error = 0, drive = 0;
	bool d = isdisk(buffer);

	if (!d || strlen(buffer) < 3)
	{
		/* No drive name contained in the name, or no complete path */
		char *def;
		char *save;
		char *n = buffer;

		/* Save the name so that it can be copied back */

		if (d)
		{
			drive = *buffer - 'A' + 1;
			n += 2;						/* here begins the name (after the ':') */
		}

		save = strdup(n);				/* save name only */
		def = x_getpath(drive, &error);	/* get default path incl. drive name */

		/* Compose fullname */

		if (def && save && !error)
			make_path(buffer, def, save);

		free(save);
		free(def);
	}

	return error;
}


/*
 * Create a new name string with maybe a path prepended to it so that
 * it represents a full name 
 */

char *x_fullname(const char *file, _WORD *error)
{
	char *buffer;

	if ((buffer = malloc(sizeof(VLNAME))) == NULL)
	{
		*error = ENOMEM;
	} else
	{
		strsncpy(buffer, file, sizeof(VLNAME));

		if ((*error = _fullname(buffer)) != 0)
		{
			free(buffer);
			buffer = NULL;
		}
	}

	return buffer;
}


#if _MINT_

/*
 * Create a symbolic link. "linkname" will point to a real object "refname"
 */

_WORD x_mklink(const char *linkname, const char *refname)
{
	if (x_exist(linkname, EX_LINK))
		return EACCES;

	return xerror((_WORD) Fsymlink(refname, linkname));
}


/*
 * Read target name of a symbolic link.
 * Any '/' characters are converted to '\', otherwise other routines
 * may not find this target. 
 * Do not make this conversion for network objects.
 */

_WORD x_rdlink(size_t tgtsize, char *tgt, const char *linkname)
{
	char *slash;
	_WORD err = EACCES;

	if (!x_netob(linkname))
		err = xerror((_WORD) Freadlink((_WORD) tgtsize, tgt, linkname));

	if (err == 0 && !x_netob(tgt))
	{
		while ((slash = strchr(tgt, '/')) != NULL)
			*slash = '\\';
	}

	return err;
}


/*
 * Prepend path of the link 'linkname' to a link target definition 'tgtname', 
 * if it is not given. Return a path + name string (memory allocated here).
 * Perform similar tasks for the ".\" and "..\" paths.
 * Do not add anything for a network object or id a path is explicitely 
 * given.
 */

char *x_pathlink(char *tgtname, const char *linkname)
{
	char *target = NULL;
	char *lpath;							/* path of the link */
	char *lppath;						/* parent path of the link */
	char *b;								/* position of the first backslash, or after it */
	char *p = tgtname;					/* pointer to the backslash */
	_WORD error;

	/* beware: comparison below depends on the order of execution */

	if (!x_netob(tgtname) && (((b = strchr(tgtname, '\\')) == NULL) ||	/* no '\' in the path */
							  ((*p++ == '.') &&	/* first is '.' */
							   ((*p == '\\') ||	/* second is '\' or... */
								(*p++ == '.' && *p == '\\')	/* second is '.' and third is '\\' */
							   ))))
	{
		if ((lpath = fn_get_path(linkname)) != NULL)
		{
			if (b == NULL)				/* object path does not contain '\' */
			{
				b = tgtname;
			} else						/* first character after the '\' */
			{
				b = p + 1;
			}

			if (p == tgtname + 2)		/*  it is a "..\"  */
			{
				lppath = fn_get_path(lpath);
				free(lpath);
				lpath = lppath;
			}

			/* 
			 * referenced name does not contain an explicit path, 
			 * use that of the link, or link parent path 
			 */

			target = x_makepath(lpath, b, &error);
			free(lpath);
		}
	} else
	{
		target = strdup(tgtname);
	}

	return target;
}

#endif


/*
 * Obtain the name of the object referenced by a link (link target); 
 * if "linkname" is not the name of a link, or, if some other error happens, 
 * just copy the name. This routine allocates space for the output of real 
 * object name. If the name of the target does not contain a path, prepend
 * the path of the link.
 */

char *x_fllink(const char *linkname)
{
	char *tmp = NULL;
	char *target = NULL;

	if (linkname)
	{
#if _MINT_
		_WORD error = EACCES;

		if (mint)
		{
			if ((tmp = malloc_chk(sizeof(VLNAME))) != NULL)
			{
				error = x_rdlink(sizeof(VLNAME), tmp, linkname);

				/* If the name of the referenced item has been obtained... */

				if (error)
				{
					/* this is not a link, just copy the name */
					target = strdup(linkname);
				} else
				{
					/* this is a link */
					target = x_pathlink(tmp, linkname);
				}
			}
		}

		if (tmp == NULL)
#endif
			target = strdup(linkname);

		free(tmp);
	}

	return target;
}


/* 
 * Get information about free space on a disk volume.
 * this information is returned in *diskinfo as:
 * - number of flree clusters
 * - total number of clusters
 * - sector size in bytes
 * number of sectors in a cluster 
 */

_WORD x_dfree(_DISKINFO *diskinfo, _WORD drive)
{
	return xerror(Dfree(diskinfo, drive));
}


/* 
 * Get the id of the current default drive
 * 0=A: 1=B: 2=C: 3=D: ... 
 */

_WORD x_getdrv(void)
{
	return Dgetdrv();
}


/* 
 * Set new default drive 
 * 0=A: 1=B: 2=C: ...
 */

long x_setdrv(_WORD drive)
{
	return Dsetdrv(drive);
}


/* 
 * Get information about a disk-volume label 
 * note: drive 0 = A:\, 1 = B:\, etc.
 * It seems that in other than FAT fs labels can be longer than 11 (8+3) 
 * characters.  Maximum intermediate label length is here limited to
 * 39 characters, but output label name always cramped to 12 characters.
 */

#define LBLMAX 40						/* maximum permitted intermediate label length + 1 */

_WORD x_getlabel(_WORD drive, char *label)
{
	_DTA *olddta, dta;
	_WORD error;
	char path[8];
	char lblbuf[LBLMAX];

	strcpy(path, adrive);
	strcat(path, "*.*");

	*path += (char) drive;

#if _MINT_
	if (mint)
	{
		path[3] = 0;
		error = (_WORD) x_retresult(Dreadlabel(path, lblbuf, LBLMAX));
	} else
#endif
	{
		olddta = Fgetdta();
		Fsetdta(&dta);

		if ((error = Fsfirst(path, FA_LABEL)) == 0)
			strsncpy(lblbuf, dta.dta_name, (size_t) LBLMAX);
		else
			error = ENOENT;

		Fsetdta(olddta);
	}

	if (error == 0)
		cramped_name(lblbuf, label, sizeof(INAME));
	else
		*label = 0;

	return error == ENMFILES || error == ENOENT ? 0 : error;
}


#if _EDITLABELS
/*
 * Create a volume label (for the time being, mint or magic only).
 * In single TOS, does not do anything, but does not return error.
 * Currently, this routine is not used anywhere in TeraDesk
 */

_WORD x_putlabel(_WORD drive, char *label)
{
	char path[4];

	strcpy(path, adrive);
	*path += (char) drive;

#if _MINT_
	if (mint)
		return x_retresult(Dwritelabel(path, label));
	else
#endif
		return 0;
}

#endif


/* File functions */

/* 
 * Rename a file from "oldn" to "newn"; 
 * note unusual (for C) order of arguments: (source, destination) 
 */

_WORD x_rename(const char *oldn, const char *newn)
{
	return xerror(Frename(0, oldn, newn));
}


/* 
 * "Unlink" a file (i.e. delete it) .
 * When operated on a symblic link, it deletes the link, not the file
 */

_WORD x_unlink(const char *file)
{
	return xerror(Fdelete(file));
}


/* 
 * Set GEMDOS file attributes and access rights.
 * Note: Applying Fattrib() to folders in Mint will fail
 * on -some- FAT partitions (why?).
 */

_WORD x_fattrib(const char *file,		/* file name */
				XATTR * attr			/* extended attributes */
	)
{
#if _MINT_
	_WORD mode;
	bool hasuid;						/* true if access rights  are settable */
#endif
	_WORD mask;							/* mask for changeable ms-dos attributes */
	_WORD error;						/* error code */

	/* 
	 * The following change is of V4.01. If file access rights
	 * are available, do not fail if it is impossible set MS-DOS 
	 * file attributes - user acces rights are probaly the only
	 * relevant protection.  Previous version failed when attempting 
	 * to access files on a network file system because Fattrib
	 * did not work. 
	 */

#if _MINT_
	hasuid = (x_inq_xfs(file) & FS_UID) != 0;
#endif

	mask = FA_RDONLY | FA_SYSTEM | FA_HIDDEN | FA_CHANGED;

	if ((attr->st_mode & S_IFMT) == S_IFDIR)
		mask |= FA_DIR;

	error = xerror((_WORD) Fattrib(file, 1, (attr->st_attr & mask)));

#if _MINT_

	if (hasuid)
		error = 0;

	if (mint)
	{
		mode = attr->st_mode & (DEFAULT_DIRMODE | S_ISUID | S_ISGID | S_ISVTX);

		/* Quietly fail on folders if necessary in Mint (why?) */

		if (!magx && (attr->st_mode & S_IFMT) == S_IFDIR && error == ENOENT)
			error = 0;

		/* Set access rights and owner IDs if possible */

		if (error >= 0 && hasuid)
		{
			/* Don't use Fchmod() on links; target will be modified! */

			if ((attr->st_mode & S_IFLNK) != S_IFLNK)
				error = xerror((_WORD) Fchmod(file, mode));

			/* 
			 * This (and above) may cause a problem with network file systems.
			 * on accessing -some- remote systems, Fchown and/or Fchmod
			 * may fail, depending on the settings of user-id-mapping.
			 * Therefore, such errors are quietly ignored.
			 */

			if (error >= 0)
				error = xerror((_WORD) Fchown(file, attr->st_uid, attr->st_gid));

			if (error == EACCES)
			{
				char b[8];
				const char *c = "U:\\NFS\\";

				strsncpy(b, file, 8);
				strupr(b);

				if (strcmp(b, c) == 0)
					error = 0;
			}
		}
	}
#endif

	return error;
}


/* 
 * Get or set file date & time. A handle to the file must exist first 
 */

_WORD x_datime(_DOSTIME *time, _WORD handle, _WORD wflag)
{
	return xerror(Fdatime(time, handle, wflag));
}


/* 
 * Open a file 
 */

_WORD x_open(const char *file, _WORD mode)
{
	if (!flock)
		mode &= O_ACCMODE;

	return (_WORD) x_retresult(Fopen(file, mode));
}


/* 
 * Create a new file with specified attributes and access rights
 */

_WORD x_create(const char *file, XATTR *attr)
{
	_WORD error = (_WORD) x_retresult(Fcreate(file, (attr) ? attr->st_attr : 0));

#if _MINT_
	if (mint && (error >= 0) && attr)
	{
		_WORD handle = error;

		error = x_fattrib(file, attr);

		if (error >= 0)
			error = handle;
	}
#endif

	return error;
}


/* 
 * Close an open file 
 */

_WORD x_close(_WORD handle)
{
	return xerror(Fclose(handle));
}


/* 
 * Read 'count' bytes from a file into 'buf' 
 */

long x_read(_WORD handle, long count, char *buf)
{
	return x_retresult(Fread(handle, count, buf));
}


/* 
 * Write 'count' bytes to a file from 'buf' 
 */

long x_write(_WORD handle, long count, char *buf)
{
	return x_retresult(Fwrite(handle, count, buf));
}


/*
 * Position the file pointer at some offset from file beginning
 */

long x_seek(long offset, _WORD handle, _WORD seekmode)
{
	return x_retresult(Fseek(offset, handle, seekmode));
}


/* Funkties voor het lezen van een directory */


/* 
 * Convert a DTA structure to a XATTR structure. The index, dev,
 * rdev, blksize and nblocks fields in attrib are not set. They
 * are not necessary anyway on TOS.
 * Access rights are read and write for everybody, unless an item
 * is set as readonly.
 * For directories, execute rights are added. 
 * Note: perhaps the default rights should not be rwxrwxrwx but rwxr-x--- ?
 */

static void dta_to_xattr(_DTA *dta, XATTR *attrib)
{
	attrib->st_mode = S_IRUSR | S_IRGRP | S_IROTH;	/* everything is readonly */

	if ((dta->dta_attribute & FA_RDONLY) == 0)	/* can write as well */
		attrib->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;

	if (dta->dta_attribute & FA_DIR)
		attrib->st_mode |= S_IFDIR | EXEC_MODE;
	else if (!(dta->dta_attribute & FA_LABEL))
		attrib->st_mode |= S_IFREG;

	attrib->st_size = dta->dta_size;
#if _MINT_
	attrib->st_uid = 0;
	attrib->st_gid = 0;
#endif
	dos_mtime(attrib) = dos_atime(attrib) = dos_ctime(attrib) = dta->dta_time;
	dos_mdate(attrib) = dos_adate(attrib) = dos_cdate(attrib) = dta->dta_date;
	attrib->st_attr = dta->dta_attribute & 0xFF;
}


/* 
 * Inquire about details of filesystem in which 'path' item resides.
 * HR 151102: courtesy XaAES & EXPLODE; now it also works with MagiC 
 * Dj.V. Modified here to return integer code identifying file system type.
 * Return code contains bitflags describing the filesystem: 
 * 0x0000: FS_TOS- standard TOS FAT filesystem
 * 0x0001: FS_LFN- long file names are possible
 * 0x0002: FS_LNK- symbolic links are possible
 * 0x0004: FS_UID- access rights and user/group IDs are possible.
 * 0x0008: FS_CSE- case-sensitive names are possible
 * If neither mint or magic are present, always return 0. 
 * If the inquiry is about the contents of a folder specified
 * then 'path' should be terminated by a '\'
 */

_WORD x_inq_xfs(const char *path)
{
	_WORD retcode = 0;

#if _MINT_

	if (mint)
	{
		long n;
		long t;
		long c;
		long m;
		long x;

		/* Inquire about filesystem details */

		n = Dpathconf(path, DP_NAMEMAX);	/* 3: maximum name length */
		t = Dpathconf(path, DP_TRUNC);	/* 5: name truncation */
		c = Dpathconf(path, DP_CASE);	/* 6: case-sensitive names? */
		m = Dpathconf(path, DP_MODEATTR);	/* 7: valid mode bits */
		x = Dpathconf(path, DP_XATTRFIELDS);	/* 8: valid XATTR fields */

		/* 
		 * If information can not be returned, results will be < 0, then
		 * treat as if there are no fields set.
		 */
		if (m < 0)
			m = 0;

		if (x < 0)
			x = 0;

		if (c < 0)
			c = DP_CASEINSENS;

		if (t < 0)
			t = DP_NOTRUNC;

		if (n < 0)
			n = 12;

		/* 
		 * If (m & 0x1FF00), nine access rights bits are valid mode fields
		 * If (x & 0x0030), user and group ids are valid XATTR fields
		 */

		if ((m & 0x0001FF00L) != 0 && (x & 0x00000030L) != 0)
			retcode |= FS_UID;

		/*  
		 * DP_NOSENSITIVE = 1 = not sensitive, converted to uppercase
		 * DP_DOSTRUNC = 2 = file names truncated to 8+3
		 */

		if (c != DP_CASEINSENS)
		{
			retcode |= FS_CSE;
		}
		if (t != DP_DOSTRUNC && n > 12)
			retcode |= FS_LFN;

		/* Are link itemtypes valid ?  */

		if ((m & DP_FT_LNK) != 0)
			retcode |= FS_LNK;
	}
#else
	(void) path;
#endif

	return retcode;
}


/* 
 * Open a directory
 */

XDIR *x_opendir(const char *path, _WORD *error)
{
	XDIR *dir;
	VLNAME p;

	if ((dir = malloc(sizeof(XDIR))) == NULL)
	{
		*error = ENOMEM;
	} else
	{
		dir->path = path;

		strsncpy(p, path, sizeof(VLNAME) - 1);
		if (*(p + strlen(p) - 1) != '\\')
			strcat(p, bslash);

#if _MINT_
		dir->type = x_inq_xfs(p);

		if (dir->type != 0)
		{
			/* File system with long filenames or other extensions */

			if (((dir->data.handle = Dopendir(path, 0)) & 0xFF000000L) == 0xFF000000L)
			{
				*error = xerror((_WORD) dir->data.handle);
				free(dir);
				dir = NULL;
			}
		} else
#endif
		{
			/* FAT file system */

			dir->data.gdata.first = 1;
			dir->data.gdata.old_dta = Fgetdta();
			Fsetdta(&dir->data.gdata.dta);
			*error = 0;
		}
	}

	return dir;
}


/* 
 * Read a directory entry. 
 * Note: In order to increase speed, only the pointer is passed, 
 * and the name is no more copied to output location (in all uses of this
 * routine in TeraDesk, obtained name is immediately copied elsewhere).
 * **buffer should probably be considered as readonly,
 * not to be written to or used for permanent storage
 */

long x_xreaddir(XDIR *dir, char **buffer, size_t len, XATTR *attrib)
{
	long result;

	static char fspec[sizeof(VLNAME) + 4];

	/* Prepare some pointer to return in case any error occurs later */

	*fspec = 0;
	*buffer = fspec;

#if _MINT_
	if (dir->type != 0)
	{
		/*
		 * File system with long filenames. Mint (or Magic) is surely present.
		 * Use Dxreaddir, not Dreaddir, in order to handle links correctly.
		 */
		long error, rep;
		const char *n;

		if ((error = Dxreaddir((_WORD) len, dir->data.handle, fspec, attrib, &rep)) == 0)
			*buffer = fspec + 4L;

		/* By convention, names beginning with '.' are invisible in mint */

		n = fn_get_name(*buffer);

		if (n[0] == '.' && n[1] != '.')
			attrib->st_attr |= FA_HIDDEN;

		result = x_retresult(error);
	} else
#endif
	{
		/* Mint/Magic is not present or, if it is, it is a FAT-fs volume */

		_WORD error;

		if (dir->data.gdata.first != 0)
		{
			error = make_path(fspec, dir->path, TOSDEFAULT_EXT);	/* presets[1] = "*.*"  */

			if (error == 0)
				error = xerror(Fsfirst(fspec, FA_ANY));

			dir->data.gdata.first = 0;
		} else
		{
			error = xerror(Fsnext());
		}

		if (error == 0)
		{
			if (strlen(dir->data.gdata.dta.dta_name) >= len)
			{
				error = EFNTL;
			} else
			{
				*buffer = dir->data.gdata.dta.dta_name;

				dta_to_xattr(&dir->data.gdata.dta, attrib);
			}
		}

		result = (long) error;
	}

	/* 
	 * Correct mint's arrogant tampering with filenames. 
	 * Also correct stupid assignment of execute rights in Magic.
	 */

#if _MINT_
	if (mint)
	{
		/* If access rights are not supported, don't set them */

		if ((dir->type & FS_UID) == 0)
			attrib->st_mode &= ~EXEC_MODE;
	}
#endif /* _MINT_ */

	return result;
}


/* 
 * Close an open directory
 */

long x_closedir(XDIR *dir)
{
	long error;

#if _MINT_
	if (dir->type != 0)
	{
		/* File system with long filenames */

		error = x_retresult(Dclosedir(dir->data.handle));
	} else
#endif
	{
		/* DOS file system */

		Fsetdta(dir->data.gdata.old_dta);

		error = 0L;
	}

	free(dir);
	return error;
}


/* 
 * Read file atttributes (in the extended sense) 
 * mode=0: follow link; 1: attribute of the link (or object of other type) itself
 * parameter fs_type contains bitflags ( see x_inq_xfs() ):
 *
 * FS_TOS = 0x0000 : TOS filesystem
 * FS_LFN = 0x0001 : has long filenames
 * FS_LNK = 0x0002 : has links
 * FS_UID = 0x0004 : has user rights
 * FS_CSE = 0x0008 : has case-sensitive names
 * FS_INQ = 0x0100 : inquire about filesystem
 *
 * Beware that in Mint or Magic, FS_LNK can be returned for FAT volumes.
 */

long x_attr(_WORD flag, _WORD fs_type, const char *name, XATTR *xattr)
{
	long result;

	if ((fs_type & FS_INQ) != 0)
		fs_type |= x_inq_xfs(name);		/* this change is local only */

	xattr->st_mode = 0;					/* clear any existing value, see below about Fxattr() */

#if _MINT_
	if (mint && ((fs_type & FS_ANY) != 0))
	{
		/* 
		 * This is not a FAT filesystem.
		 * Attempt to set some sensible file attributes 
		 * from access rights and filename. If noone has
		 * write permission, set READONLY attribute, but do this only
		 * on a filesystem that knows of user rights.
		 * If the name begins with a dot, set HIDDEN attribute.
		 * It seems that Fxattr() does not set all of xattr->mode (???)
		 * therefore it is above set to 0 prior to inquiry
		 */

		result = x_retresult(Fxattr(flag, name, xattr));

		if ((result >= 0) && ((fs_type & FS_UID) != 0) && ((xattr->st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0))
		{
			if ((xattr->st_mode & S_IFMT) != S_IFLNK)
				xattr->st_attr |= FA_RDONLY;
			else
				xattr->st_attr &= ~FA_RDONLY;
		}

		if (name[0] == '.' && name[1] != '.')
			xattr->st_attr |= FA_HIDDEN;
	} else
#endif
	{
		/* FAT filesystem */

		_DTA *olddta;
		_DTA dta;

		olddta = Fgetdta();
		Fsetdta(&dta);

		if ((result = (long) xerror(Fsfirst(name, FA_ANY))) == 0)
			dta_to_xattr(&dta, xattr);

		Fsetdta(olddta);
	}

#if _MINT_
	if ((fs_type & FS_UID) == 0)
	{
		/* 
		 * This is a filesystem without user rights; imagine some.
		 * Beware thet Fxattr may set some funny values in Magic;
		 * therefore, (re)set xattr->mode again below
		 */

		xattr->st_mode &= ~DEFAULT_DIRMODE;

		xattr->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);

		if ((xattr->st_attr & FA_RDONLY) == 0)
			xattr->st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);

		/* 
		 * Information about execute rights need not be always
		 * set; only for file copy. As it happens, in all such
		 * cases, fs_type parameter will always contain a FS_INQ
		 * (more-less by accident, but convenient). This saves
		 * time, because the list of program types need not be
		 * always examined.
		 */

		if (((fs_type & FS_INQ) != 0) && prg_isprogram(fn_get_name(name)))
			xattr->st_mode |= EXEC_MODE;
	}
#else
	(void) flag;
#endif

	return result;
}


/*
 * Read flags from a program file header. TPA size is not passed.
 */

long x_pflags(const char *filename)
{
	long result;
	char buf[26];
	_WORD fh = x_open(filename, O_RDONLY);

	if (fh >= 0)
	{
		if ((result = x_read(fh, 26L, buf)) == 26)
			result = (*((long *) (&buf[22]))) & 0x00001037L;
		else
			result = EIO;

		x_close(fh);
		return result;
	}
	return fh;
}


/* 
 * This function returns the maximum possible path or name length.
 * In single TOS those lengths are assigned fixed values, in Mint,
 * they are obtained through Dpathconf. 
 * Beware that negative value can be returned if sensible data
 * can not be obtained!
 */

long x_pathconf(const char *path, _WORD which)
{
#if _MINT_
	if (mint)
		return x_retresult(Dpathconf(path, which));
#else
	(void) path;
#endif

	if (which == DP_PATHMAX)
		return 128;						/* = 128 in TOS */
	else if (which == DP_NAMEMAX)
		return 12;						/* 8 + 3 in TOS */

	return 0;
}


/* 
 * Execute a program through Pexec
 */

long x_exec(_WORD mode, const char *fname, void *ptr2, void *ptr3)
{
	_WORD result = xerror((_WORD) Pexec(mode, fname, ptr2, ptr3));

	if (result != ENOENT && result != ENOTDIR && result != ENOMEM && result != ENOEXEC)
		result = 0;

	return result;
}


/* 
 * GEM funkties 
 */

char *xshel_find(const char *file, _WORD *error)
{
	char *buffer;

	if ((buffer = malloc(sizeof(VLNAME))) == NULL)
	{
		*error = ENOMEM;
	} else
	{
		strcpy(buffer, file);

		/* note: shel_find() modifies the content of 'buffer' */

		if (shel_find(buffer) == 0)
		{
			*error = ENOENT;
		} else
		{
			if ((*error = _fullname(buffer)) == 0)
				return buffer;
		}

		free(buffer);
		buffer = NULL;
	}

	return buffer;
}


/* 
 * Call a fileselector (make an extended call, if possible)
 * Memory is allocated for the returned path in "buffer"
 */

char *xfileselector(const char *path, char *name, const char *label)
{
	char *buffer;
	_WORD error, button;

	if ((buffer = malloc_chk(sizeof(VLNAME))) != NULL)
	{
		strcpy(buffer, path);

		/* Correct file specification for the more primitive selectors */

		if ((error = _fullname(buffer)) != 0)
		{
			xform_error(error);
		} else
		{
			/* A call to a file selector MUST be bracketed by wind_update() */

			xd_begupdate();

			/* 
			 * In fact there should be a check here if an alternative file selector
			 * is installed, in such case the extended file-selector call can be used
			 * although TOS is older than 1.04
			 */

			if (tos_version >= 0x104)
				error = fsel_exinput(buffer, name, &button, label);
			else
				error = fsel_input(buffer, name, &button);

			xd_endupdate();

			if ((error == 0) || (button == 0))
			{
				if (error == 0)
					alert_printf(1, MFSELERR);
			} else
			{
				if ((error = _fullname(buffer)) == 0)
					return buffer;

				xform_error(error);
			}
		}
	}

	free(buffer);
	return NULL;
}


/********************************************************************
 *																	*
 * Vervangers van fopen enz. uit de standaard bibliotheek.			*
 *																	*
 ********************************************************************/

/*
 * Read a buffer from an open file
 */

static _WORD read_buffer(XFILE *file)
{
	long n;

	if ((n = x_read(file->handle, XBUFSIZE, file->buffer)) < 0)
		return (_WORD) n;
	file->write = (_WORD) n;
	file->read = 0;
	file->eof = (n != XBUFSIZE) ? TRUE : FALSE;

	return 0;
}


/*
 * Write a buffer into an open file
 */

static _WORD write_buffer(XFILE *file)
{
	long n;

	if (file->write != 0)
	{
		if ((n = x_write(file->handle, file->write, file->buffer)) < 0)
			return (_WORD) n;
		if (n == (long) file->write)
		{
			file->write = 0;
			file->eof = TRUE;

			return 0;
		}
		return ENOSPC;
	}
	return 0;
}


/* 
 * Open a real file. Return pointer to a XFILE structure
 * which is created in this routine. 
 * Beware: check for improper 'mode' can be disabled
 */

XFILE *x_fopen(const char *file, _WORD mode, _WORD *error)
{
	XFILE *xfile;
	_WORD rwmode = mode & O_ACCMODE;

	if ((xfile = malloc(sizeof(XFILE) + XBUFSIZE)) == NULL)
	{
		*error = ENOMEM;
	} else
	{
		memclr(xfile, sizeof(*xfile));
		*error = 0;

		xfile->mode = mode;
		xfile->buffer = (char *) (xfile + 1);
		xfile->bufsize = (_WORD) XBUFSIZE;

		if (rwmode == O_WRONLY)
		{
			xfile->handle = x_create(file, NULL);
		} else
		{
#if _CHECK_RWMODE
			if (rwmode == O_RDONLY)
#endif
				xfile->handle = x_open(file, mode);
#if _CHECK_RWMODE
			else
				xfile->handle = ENOSYS;
#endif
		}

		if (xfile->handle < 0)
			*error = xfile->handle;

		if (*error != 0)
		{
			free(xfile);
			xfile = NULL;
		}
	}

	return xfile;
}


/* 
 * Open a memory area as a file (in fact, mode is always 0x01 | 0x02 )
 * If allocation is unsuccessful, return ENOMEM (-39)
 * If not read/write mode it returned ENOSYS (-32) 
 * (why bother?, this routine is used only once, so this check
 * can be disabled, but use carefully then!). If OK, return 0
 */

XFILE *x_fmemopen(_WORD mode, _WORD *error)
{
#if _CHECK_RWMODE
	_WORD rwmode = mode & O_RWMODE;		/* mode & 0x03 */
#endif
	XFILE *xfile;

	*error = 0;

	/* A memory block is allocated and some structures set */

	if ((xfile = malloc(sizeof(*xfile))) == NULL)
	{
		*error = ENOMEM;
	} else
	{
		memclr(xfile, sizeof(*xfile));

		xfile->mode = mode;
		xfile->memfile = TRUE;

#if _CHECK_RWMODE
		if (rwmode != O_RDWR)			/* 0x02 */
		{
			result = ENOSYS;			/* unknown function ? */
			free(xfile);
			xfile = NULL;
		}
#endif
	}

	return xfile;
}


/* 
 * Close a file or a memory file 
 */

_WORD x_fclose(XFILE *file)
{
	_WORD error;
	_WORD rwmode = file->mode & O_ACCMODE;

	if (file->memfile)
	{
		error = 0;
		free(file->buffer);
	} else
	{
		_WORD h;

		h = (rwmode == O_WRONLY) ? write_buffer(file) : 0;
		if ((error = x_close(file->handle)) == 0)
			error = h;
	}

	free(file);

	return error;
}


#if 0
/* This routine (a pair with x_fwrite) is never used in TeraDesk
   and not maintained anymore. Code below may be obsolete and nonworking */

/* 
 * Read file contents (not more than "length" bytes).
 * This routine can handle "memory files" too.
 */

long x_fread(XFILE *file, void *ptr, long length)
{
	long remd = length;
	long n;
	long size;
	char *dest = (char *) ptr;
	char *src;
	_WORD read, write, error;

	if (file->memfile)
	{
		/* This is a memory file */

		read = file->read;
		write = file->write;
		src = file->buffer;

		while ((remd > 0) && (file->eof == FALSE))
		{
			if (read == write)
			{
				file->eof = TRUE;
			} else
			{
				*dest++ = src[read++];
				remd--;
			}
		}

		file->read = read;

		return (length - remd);
	} else
	{
		/* This is a real file */
#if _CHECK_RWMODE
		if ((file->mode & O_RWMODE) == O_WRONLY)
			return 0;
#endif
		src = file->buffer;

		do
		{
			if ((file->read == file->write) && (file->eof == FALSE))
			{
				if ((error = read_buffer(file)) < 0)
					return (long) error;
			}

			read = file->read;
			write = file->write;

			while ((read < write) && (remd > 0))
			{
				*dest++ = src[read++];
				remd--;
			}

			file->read = read;

			if ((remd >= XBUFSIZE) && (file->eof == FALSE))
			{
				size = remd - (remd % XBUFSIZE);

				if ((n = x_read(file->handle, size, dest)) < 0)
					return n;

				if (n != size)
					file->eof = TRUE;

				remd -= n;
			}
		} while ((remd > 0) && (file->eof == FALSE));

		return (length - remd);
	}
}

#endif


/* 
 * Write 'length' bytes to a file;
 * return (negative) error code (long!) or else number of bytes written 
 * Note: open window data need about 20 bytes. For each open window about 
 * 32 bytes more is needed, plus path and mask lengths. So, one 128-byte 
 * record in a memory file would be fit for about 2-3 open windows. Better 
 * allocate slightly more, to avoid copying halfway into window saving, 
 * e.g. allocate 256 bytes.
 */

#define MRECL 256

long x_fwrite(XFILE *file, const void *ptr, long length)
{
	long remd = length;					/* number of bytes remaining to be transferred */
	long n;
	long size;
	char *dest;							/* location of the output buffer */
	const char *src = ptr;				/* position being read from */
	_WORD write;						/* position currently written to */
	_WORD error;						/* error code */

	/* Don't write into a file opened for reading */

#if _CHECK_RWMODE
	if ((file->mode & O_RWMODE) == O_RDONLY)
		return ENOSYS;
#endif

	if (file->memfile)
	{
		/* this is a "memory file" */

		write = file->write;
		dest = file->buffer;

		/* Go through all the data, until nothing remains */

		while (remd > 0)
		{
			if (write == file->bufsize)
			{
				char *new;

				/* 
				 * Existing buffer has been filled (or this is the first record)
				 * If this is the first "record" then allocate MRECL bytes;
				 * otherwise, try to increase the allocated amount;
				 */

				new = malloc(file->bufsize + MRECL);
				if (new)
				{
					if (file->buffer)
					{
						memcpy(new, file->buffer, file->bufsize);
						free(file->buffer);
					}
				} else
				{
					return ENOMEM;
				}

				dest = file->buffer = new;
				file->bufsize += MRECL;
			}

			/* Now write till the end of input */

			dest[write++] = *src++;
			remd--;
		}

		file->write = write;

		return length;
	} else
	{
		/* This is a "real" file */

		dest = file->buffer;

		do
		{
			write = file->write;

			while ((write < XBUFSIZE) && (remd > 0))
			{
				dest[write++] = *src++;
				remd--;
			}

			file->write = write;

			if (file->write == XBUFSIZE)
			{
				if ((error = write_buffer(file)) < 0)
					return (long) error;
			}

			if (remd >= XBUFSIZE)
			{
				size = remd - (remd % XBUFSIZE);

				if ((n = x_write(file->handle, size, dest)) < 0)
					return n;

				if (n != size)
					return ENOSPC;

				remd -= n;
			}
		} while (remd > 0);

		return (length - remd);
	}
}


/* 
 * Read a string from a file, but not more than 'n' characters 
 */

_WORD x_fgets(XFILE *file, char *string, _WORD n)
{
	char *dest;
	char *src;
	char ch;
	char nl = 0;
	_WORD i = 1;
	_WORD read, write, error;
	bool ready = FALSE;

	/* Why ? Just a safety precaution against careless use maybe? */

#if _CHECK_RWMODE
	if ((file->mode & O_RWMODE) != O_RDONLY)
	{
		*string = 0;
		return 0;
	}
#endif

	/* Has end-of-file been reached? */

	if (x_feof(file))
		return EEOF;

	dest = string;
	src = file->buffer;
	read = file->read;
	write = file->write;

	while (!ready)
	{
		if (read == write)
		{
			/* 
			 * end of buffer reached; read a new one, or this is end of file
			 * Note: if used carefully, read_buffer() should never happen
			 * with a memory file, because never should be more read
			 * than had been written
			 */

			if (file->eof)
			{
				ready = TRUE;
			} else
			{
				if ((error = read_buffer(file)) < 0)
					return error;
				read = file->read;
				write = file->write;
			}
		} else
		{
			/* Note: this branch also handles the memory file */

			if (nl != 0)
			{
				/* 
				 * Handle the second character (if there is any) 
				 * of <cr><lf> or <lf><cr> 
				 */

				ready = TRUE;
				ch = src[read];
				if (((ch == '\n') || (ch == '\r')) && (nl != ch))
					read++;
			} else
			{
				/* 
				 * Handle characters including the first character
				 * of a <cr><lf> or <lf><cr> pair.
				 */

				ch = src[read++];
				if (ch == '\n' || ch == '\r')
				{
					nl = ch;
				} else if (i < n)
				{
					*dest++ = ch;
					i++;
				}
			}
		}
	}

	file->read = read;

	*dest = 0;
	return 0;
}


/* 
 * Return TRUE if end-of-file has been reached
 */

bool x_feof(XFILE *file)
{
	return ((file->eof) && (file->read == file->write)) ? TRUE : FALSE;
}


/* 
 * Find whether files can be locked in this OS variant ? 
 */

void x_init(void)
{
	flock = find_cookie(C__FLK, NULL);
}
