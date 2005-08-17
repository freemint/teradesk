/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                         2003, 2004, 2005  Dj. Vukovic
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


#include <np_aes.h>	
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <boolean.h>
#include <library.h>
#include <mint.h>
#include <vdi.h>
#include <xdialog.h>

#include "desktop.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "file.h"

#define XBUFSIZE	2048L


static boolean flock;
extern int tos_version, aes_version;
extern const char *presets[];

extern boolean prg_isprogram(const char *name);


/* 
 * Aux. function for size optimization when returning results
 * of some GEMDOS operations.
 */

static long x_retresult(long result)
{
	return (result < 0) ? (long)xerror((int)result) : result;
}


/*
 * Check if a filename or path + name is valid in this OS.
 * Return 0 if OK.
 * Beware: it is still possible that a path+name be too long for
 * some buffers in TeraDesk!
 */

int x_checkname(const char *path, const char *name)
{
	if (strlen(name) > x_pathconf(path, DP_NAMEMAX))
		return EFNTL;

	if (strlen(path) + strlen(name) + 2L > x_pathconf(path, DP_PATHMAX))
		return EPTHTL;

	return 0;
}


/* 
 * Create a path+filename string from path "path" and filename "name".
 * Return a pointer to the created string (being allocated in this routine)
 */

char *x_makepath(const char *path, const char *name, int *error)
{
	char *p;

	if ((p = malloc(strlen(path) + strlen(name) + 2L)) != NULL)
	{
		*error = 0;
		make_path(p, (char *) path, (char *) name);
	}
	else
		*error = ENSMEM;

	return p;
}


/* 
 * Check if a file or folder or link exists. Return true if it exists.
 * If EX_LINK is set in flags, check for the link itself;
 * otherwise follow the link and check for target existence. 
 */

boolean x_exist(const char *file, int flags)
{
	XATTR attr;
	int type, theflag;


#if _MINT_
	if ( x_attr( (flags & EX_LINK) ? 1 : 0, FS_LFN, file,  &attr) != 0 )
#else
	if (x_attr(0, FS_LFN, file, &attr) != 0) 
#endif
		return FALSE; /* x_attr can't find target */
	else
	{
		type = attr.mode & S_IFMT;

		switch(type)
		{
			case S_IFDIR:
				theflag = EX_DIR;
				break;
			case S_IFREG:
				theflag = EX_FILE;
				break;
#if _MINT
			case S_IFLNK:
				theflag = EX_LINK;
				break;
#endif
		}

		if ( (flags & theflag) == 0 )
			return FALSE;

		return TRUE;
	}
}


/*
 * Modify a name string so that it represents a full path + name ?
 * Note: it seems that the resulting string can be longer than the
 * original one? Use with care.
 */

static int _fullname(char *buffer)
{
	char *name, *h, *d, *s;
	int error;

	/* 
	 * Note: there will probably be two alerts 
	 * if string can not be duplicated, 
	 * as strdup already displays an alert
	 */

	if ((name = strdup(buffer)) == NULL)
		error = ENSMEM;
	else
	{
		d = buffer;

		if ((h = strchr(name, ':')) == NULL)
		{
			/* 
			 * If there is no drive name in the name, put one first.
			 * Original string will be appended to it later
			 */

			*d++ = (char) (x_getdrv() + 'A');
			*d++ = ':';
			h = name;
		}
		else
		{
			h++;
			s = name;
			while (s != h)
				*d++ = *s++;
		}

		if (*h == '\\')
		{
			strcpy(d, h);
			error = 0;
		}
		else
		{
			if ((error = xerror(Dgetpath(d, buffer[0] - 'A' + 1))) == 0)
				make_path(buffer, buffer, h);
		}
		free(name);
	}
	return error;
}


char *x_fullname(const char *file, int *error)
{
	char *buffer;

	if ((buffer = malloc(sizeof(VLNAME))) == NULL)
	{
		*error = ENSMEM;
		return NULL;
	}

	strcpy(buffer, file);

	if ((*error = _fullname(buffer)) != 0)
	{
		free(buffer);
		return NULL;
	}

	return buffer;
}


/* Directory functions */

/* 
 * Set a directory path 
 */

int x_setpath(const char *path)
{
	return xerror(Dsetpath(path));
}


/* 
 * Get current default path, return pointer to this new-allocated string 
 */

char *x_getpath(int drive, int *error)
{
	char *buffer;

	/* Allocate a buffer */

	if ((buffer = malloc(sizeof(VLNAME))) == NULL)
	{
		*error = ENSMEM;
		return NULL;
	}

	/* Put drive id at the beginning of the buffer */

	buffer[0] = (char) (((drive == 0) ? x_getdrv(): (drive - 1)) + 'A');
	buffer[1] = ':';

	/* Append the rest */

	if ((*error = xerror(Dgetpath(buffer + 2, drive))) < 0)
	{
		free(buffer);
		return NULL;
	}
	
	return buffer;
}


/* 
 * Create a directory 
 */

int x_mkdir(const char *path)
{
	return xerror(Dcreate(path));
}


/* 
 * Remove a directory 
 */

int x_rmdir(const char *path)
{
	return xerror(Ddelete(path));
}


#if _MINT_

/*
 * Create a symbolic link. "newname" will point to a real object "oldname"
 */

int x_mklink(const char *newname, const char *oldname)
{
	return xerror( (int)Fsymlink( (char *)oldname, (char *)newname ) );
}


/*
 * Read target name of a symbolic link.
 * Any '/' characters are converted to '\', otherwise other routines
 * may not find this target
 */

int x_rdlink( int tgtsize, char *tgt, const char *linkname )
{
	char *slash;
	int err;

	err =  xerror( (int)Freadlink( tgtsize, tgt, (char *)linkname ) );

	if (err == 0)
	{
		while( ( slash = strchr(tgt, '/') ) != NULL )
			*slash = '\\';
	}

	return err;
}


/*
 * Prepend a path to a link target definition, if it is not given.
 * Return a path + name string (memory allocated here)
 */

char *x_pathlink( char *tgtname, char *linkname )
{
	char
		*target,
		*lpath;

	int
		error;

	if ( strchr(tgtname,'\\') == NULL )
	{
		/* referenced name does not contain a path, use that of the link */

		if ( (lpath = fn_get_path(linkname)) != NULL )
		{
			target = x_makepath(lpath, tgtname, &error);
			free(lpath);
		}
	}
	else
		target = strdup(tgtname);

	return target;
}

#endif


/*
 * Obtain the name of the object referenced by a link (link target); 
 * if "linkname" is not the name of a link, or, if some other error happens, 
 * just copy the name.This routine allocates space for the output of real 
 * object name. If the name of the target does not contain a path, prepend
 * the path of the link.
 */

char *x_fllink( char *linkname )
{	
	char 
		*tmp = NULL,
		*target = NULL;

	if ( linkname )
	{
#if _MINT_
		int 
			error = EACCDN;

		if (mint)
		{
			if ( (tmp = malloc_chk( sizeof(VLNAME) )) != NULL )
			{
				error = x_rdlink( (int)sizeof(VLNAME), tmp, linkname ); 
	
				/* If the name of the referenced item has been obtained... */

				if ( error )
					/* this is not a link, just copy the name */
					target = strdup(linkname);
				else
					/* this is a link */
					target = x_pathlink(tmp, linkname);
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
 * Get information about free space on a disk volume 
 */

int x_dfree(DISKINFO *diskinfo, int drive)
{
	return xerror(Dfree(diskinfo, drive));
}


/* 
 * Get the id of the current default drive 
 */

int x_getdrv(void)
{
	return Dgetdrv();
}


/* 
 * Set new default drive 
 */

long x_setdrv(int drive)
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

#define LBLMAX 40 /* maximum permitted intermediate label length + 1 */

int x_getlabel(int drive, char *label)
{
	DTA *olddta, dta;
	int error;
	char path[8], lblbuf[LBLMAX];

	strcpy(path, "A:\\*.*");
	path[0] += (char)drive;

#if _MINT_
	if(mint)
	{
		path[3] = 0;
		error = x_retresult(Dreadlabel(path, lblbuf, LBLMAX));
	}
	else
#endif
	{
		olddta = Fgetdta();
		Fsetdta(&dta);

		if (((error = Fsfirst(path, 0x3F)) == 0) && (dta.d_attrib & FA_VOLUME)) 
			strsncpy(lblbuf, dta.d_fname, (size_t)LBLMAX);
		else
			error = EFILNF;

		Fsetdta(olddta);
	}

	if(error == 0)
		cramped_name(lblbuf, label, sizeof(INAME));
	else
		*label = 0;

	return ((error == ENMFIL) || (error == EFILNF)) ? 0 : error;
}


#if _EDITLABELS

/*
 * Create a volume label (for the time being, mint or magic only).
 * In single TOS, does not do anything, but does not return error.
 * Currently, this routine is not used anywhere in TeraDesk
 */

int x_putlabel(int drive, char *label)
{
	char path[4];

	strcpy(path, "A:\\");
	path[0] += (char)drive;

#if _MINT_
	if(mint)
		return x_retresult(Dwritelabel(path, label));
	else
#endif
		return 0;
}

#endif


/* File functions */

/* 
 * Rename a file from "oldname" to "newname"; 
 * note unusual (for C) order of arguments: (source, destination) 
 */

int x_rename(const char *oldname, const char *newname)
{
	return xerror(Frename(0, oldname, newname));
}


/* 
 * "Unlink" a file (i.e. delete it) .
 * When operated on a symblic link, it deletes the link, not the file
 */

int x_unlink(const char *file)
{
	return xerror(Fdelete(file));
}


/* 
 * Set GEMDOS file attributes and access rights.
 * Note: Applying Fattrib() to folders in Mint will fail
 * on -some- FAT partitions (why?) 
 */

int x_fattrib(const char *file, XATTR *attr)
{
	int 
		error = xerror((int)Fattrib(file, 1, attr->attr));

#if _MINT_
	int
		mode = attr->mode & (DEFAULT_DIRMODE | S_ISUID | S_ISGID | S_ISVTX);

	if(mint)
	{
		/* Quietly fail if necessary in Mint (why?) */

		if(!magx && (attr->mode & S_IFMT) == S_IFDIR && error == EFILNF)
			error = 0;

		/* Set access rights and owner IDs if possible */

		if ( error >= 0 && ((x_inq_xfs(file) & FS_UID) != 0) )
		{
			/* Don't use Fchmod() on links; target will be modified! */

			if ( (attr->mode & S_IFLNK) != S_IFLNK ) 
				error = xerror( (int)Fchmod((char *)file, mode) );
			if ( error >= 0 )
				error = xerror( (int)Fchown((char *)file, attr->uid, attr->gid) );
		}
	}
#endif

	return error;
}


/* 
 * Get or set file date & time. A handle to the file must exist first 
 */

int x_datime(DOSTIME *time, int handle, int wflag)
{
	return xerror(Fdatime(time, handle, wflag));
}


/* 
 * Open a file 
 */

int x_open(const char *file, int mode)
{
	if (!flock)
		mode &= O_RWMODE;

	return (int)x_retresult(Fopen(file, mode));
}


/* 
 * Create a new file with specified attributes and access rights
 */

int x_create(const char *file, XATTR *attr)
{
	int error = (int)x_retresult(Fcreate(file, (attr) ? attr->attr : 0));

#if _MINT_
	if (mint && (error >= 0) && attr)
	{
		int handle = error;

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

int x_close(int handle)
{
	return xerror(Fclose(handle));
}


/* 
 * Read 'count' bytes from a file into 'buf' 
 */

long x_read(int handle, long count, char *buf)
{
	return x_retresult(Fread(handle, count, buf));
}


/* 
 * Write 'count' bytes to a file from 'buf' 
 */

long x_write(int handle, long count, char *buf)
{
	return x_retresult(Fwrite(handle, count, buf));
}


/*
 * Position the file pointer at some offset from file beginning
 */

long x_seek(long offset, int handle, int seekmode)
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

static void dta_to_xattr(DTA *dta, XATTR *attrib)
{
	attrib->mode = 	(S_IRUSR | S_IRGRP | S_IROTH); 	/* everything is readonly */

	if ((dta->d_attrib & FA_READONLY) == 0)			/* can write as well */
		attrib->mode |= (S_IWUSR | S_IWGRP | S_IWOTH);

	if (dta->d_attrib & FA_SUBDIR)
		attrib->mode |= (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH );
	else if (!(dta->d_attrib & FA_VOLUME))
		attrib->mode |= S_IFREG;

	attrib->size = dta->d_length;
#if _MINT_
	attrib->uid = 0;
	attrib->gid = 0;
#endif
	attrib->mtime = attrib->atime = attrib->ctime = dta->d_time;
	attrib->mdate = attrib->adate = attrib->cdate = dta->d_date;
	attrib->attr = (int)dta->d_attrib & 0xFF;
}


/* 
 * Inquire about details of filesystem in which 'path' item resides.
 * HR 151102: courtesy XaAES & EXPLODE; now it also works with MagiC 
 * Dj.V. Modified here to return integer code identifying file system type.
 * Return code contains bitflags describing the filesystem: 
 * 0x0000: standard TOS FAT filesystem
 * 0x0001: long file names are possible
 * 0x0002: symbolic links are possible
 * 0x0004: access rights and user/group IDs are possible.
 * If neither mint or magic are present, always return 0. 
 * If the inquiry is about the contents of a folder specified
 * then 'path' should be terminated by a '\'
 */

int x_inq_xfs(const char *path)
{
	int
		retcode = 0;

#if _MINT_

	if (mint)
	{
		long c, t, m, x;

		/* Inquire about filesystem details */

		c = Dpathconf(path, DP_CASE); 	/* case-sensitive names? */
		t = Dpathconf(path, DP_TRUNC);	/* name truncation */
		m = Dpathconf(path, DP_MODE);	/* valid mode bits */
		x = Dpathconf(path, DP_XATT);	/* valid XATTR fields */

		/* 
		 * If information can not be returned, results will be < 0, then
		 * treat as if there are no valid fields
		 */

		if ( m < 0 )
			m = 0;
		if ( x < 0 )
			x = 0;

		/* 
		 * If (m & 0x1FF00), nine access rights bits are valid mode fields
		 * If (x & 0x0030), user and group ids are valid XATTR fields
		 */

		if ((m & 0x0001FF00L) != 0 && (x & 0x00000030L) != 0 )
			retcode |= FS_UID;

		/* Are link itemtypes valid ? */

		if ( (m & 0x01000000L) != 0 )
			retcode |= FS_LNK;

		/*  
		 * DP_NOSENSITIVE = 1 = not sensitive, converted to uppercase
		 * DP_DOSTRUNC = 2 = file names truncated to 8+3
		 */

		if (c >= 0 && t >= 0 && !(c == DP_NOSENSITIVE && t == DP_DOSTRUNC))
			retcode |= FS_LFN;

		/* 
		 * Generally, retcode will contain FS_LFN | FS_LNK (0x0003) for LFN+links, 
		 * and FS_UID (0x0004) for user access rights 
		 */
	}
#endif

	return retcode;
}


/* 
 * Open a directory
 */

XDIR *x_opendir(const char *path, int *error)
{
	XDIR *dir;
	VLNAME p;

	if ((dir = malloc(sizeof(XDIR))) == NULL)
		*error = ENSMEM;
	else
	{
		dir->path = (char *)path;

		strsncpy(p, path, sizeof(VLNAME)-1L);
		if (*(p + strlen(p) - 1) != '\\')
			strcat(p, bslash);

#if _MINT_

		dir->type = x_inq_xfs(p);

		if (dir->type == 0)
		{
#endif
			/* Dos file system */

			dir->data.gdata.first = 1;
			dir->data.gdata.old_dta = Fgetdta();
			Fsetdta(&dir->data.gdata.dta);
			*error = 0;

#if _MINT_
		}
		else
		{
			/* File system with long filenames */
	
			if (((dir->data.handle = Dopendir(path, 0)) & 0xFF000000L) == 0xFF000000L)
			{
				*error = xerror((int) dir->data.handle);
				free(dir);
				dir = NULL;
			}
		}
#endif
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

long x_xreaddir(XDIR *dir, char **buffer, int len, XATTR *attrib) 
{
	static char fspec[260];

	/* Prepare some pointer to return in case any error occurs later */

	fspec[0] = 0;
	*buffer = fspec;

#if _MINT_
	if (dir->type == 0)
#endif
	{
		/* Mint/Magic is not present or, if it is, it is a FAT-fs volume */	

		int error;

		if (dir->data.gdata.first != 0)
		{
			make_path(fspec, dir->path, TOSDEFAULT_EXT); /* presets[1] = "*.*"  */	
			error = xerror(Fsfirst(fspec, FA_ANY));
			dir->data.gdata.first = 0;
		}
		else
			error = xerror(Fsnext());

		if (error == 0)
		{
			if (strlen(dir->data.gdata.dta.d_fname) + 1 > len)
				error = EFNTL;
			else
			{
#if _MINT_
				if ( mint ) 				/* no need to waste time otherwise */
					strupr(dir->data.gdata.dta.d_fname); /* arrogant MiNT tampering with filenames. */
#endif
				*buffer = dir->data.gdata.dta.d_fname; 
				dta_to_xattr(&dir->data.gdata.dta, attrib);
			}
		}
		return (long) error;
	}
#if _MINT_
	else
	{
		/*
		 * File system with long filenames. Mint (or Magic) is surely present.
		 * Use Dxreaddir, not Dreaddir, in order to handle links correctly.
		 */

		long error, rep;
		char *n;

		if ((error = Dxreaddir(len, dir->data.handle, fspec, (long)attrib, &rep)) == 0)
			*buffer = fspec + 4L;

		/* By convention, names beginning with '.' are invisible in mint */

		n = fn_get_name(*buffer);
		if ( n[0] == '.' && n[1] != '.' )
			attrib->attr |= FA_HIDDEN;

		return x_retresult(error);
	}
#endif /* _MINT_ */
}


/* This routine is currently not used in TeraDesk

/*
 * Rewind an open directory, so that the next reading is that of the first
 * item in it.
 */

long x_rewinddir(XDIR *dir)
{

#if _MINT_
	if (dir->type == 0)
	{
#endif
		/* DOS file system */
	
		dir->data.gdata.first = 1;
		return 0L;
#if _MINT_  
	}
	else
	{
		/* File system with long filenames (why this long/int nonsense?) */
	
		return x_retresult(Drewinddir(dir->data.handle));
	}
#endif /* _MINT_ */
}

*/


/* 
 * Close an open directory
 */

long x_closedir(XDIR *dir)
{
	long error;

#if _MINT_
	if (dir->type == 0)
	{
#endif
		/* DOS file system */
	
		Fsetdta(dir->data.gdata.old_dta);

		error = 0L;
#if _MINT_
	}
	else
	{
		/* File system with long filenames */

		error = x_retresult(Dclosedir(dir->data.handle));
	}
#endif /* _MINT_ */

	free(dir); 
	return error; 
}


/* 
 * Read file atttributes (in the extended sense) 
 * mode=0: follow link; 1: attribute of the link itself
 * fs_type contains bitflags ( see x_inq_xfs() ):
 * FS_TOS = 0x0000 : TOS filesystem
 * FS_LFN = 0x0001 : has long filenames
 * FS_LNK = 0x0002 : has links
 * FS_UID = 0x0004 : has user rights
 * FS_INQ = 0x0100 : inquire about filesystem
 */

long x_attr(int flag, int fs_type, const char *name, XATTR *xattr)
{
	long result;

	if ( (fs_type & FS_INQ) != 0 )
		fs_type |= x_inq_xfs(name); /* this change is local only */ 

#if _MINT_
	if (mint && ((fs_type & FS_ANY) != 0) )
	{
		/* 
		 * Not FAT filesystem.
		 * Attempt to set some sensible file attributes 
		 * from access rights and filename. If noone has
		 * write permission, set READONLY attribute;
		 * If the name begins with a dot, set HIDDEN attribute.
		 */

		result = x_retresult(Fxattr(flag, name, xattr));

		if ( (result >= 0) && ((xattr->mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0))
			if((xattr->mode & S_IFMT)  != S_IFLNK)
				xattr->attr |= FA_READONLY;

		if( name[0] == '.' && name[1] != '.' )
			xattr->attr |= FA_HIDDEN;			
	}
	else
#endif
	{
		/* FAT filesystem */

		DTA *olddta, dta;
	
		olddta = Fgetdta();
		Fsetdta(&dta);
	
		if ((result = (long)xerror(Fsfirst(name, FA_ANY))) == 0) 
			dta_to_xattr(&dta, xattr);

		Fsetdta(olddta);
	}

#if _MINT_
	if ((fs_type & FS_UID) == 0 )
	{
		/* This is a filesystem without user rights; imagine some */

		xattr->mode |= (S_IRUSR | S_IRGRP | S_IROTH);

		if ((xattr->attr & FA_READONLY) == 0 )
			xattr->mode |= (S_IWUSR | S_IWGRP | S_IWOTH);

		/* 
		 * Information about execute rights need not be always
		 * set; only for file copy. As it happens, in all such
		 * cases, fs_type parameter will always contain a FS_INQ
		 * (more-less by accident, but convenient). 
		 */

		if ( ((fs_type & FS_INQ) != 0) && prg_isprogram(name))
			xattr->mode |= ( S_IXUSR | S_IXGRP | S_IXOTH ); 
	}
#endif

	return result;
}


/*
 * Read flags from a program file header.
 * TPA size is not passed.
 */

long x_pflags(char *filename)
{
	char buf[26];
	long result;
	int fh = x_open(filename, O_RDONLY);

	if (fh >= 0)
	{
		if ((result = x_read(fh, 26L, buf)) == 26 )
			result =  (*((long *)(&buf[22]))) & 0x00001037L; 
		else
			result = EREAD;
	
		x_close(fh);
		return result;
	}
	else
		return fh;
}


/* 
 * This function returns the maximum possible path or name length.
 * In single TOS those lengths are assigned fixed values, in Mint,
 * they are obtained through Dpathconf. 
 */

long x_pathconf(const char *path, int which)
{
#if _MINT_
	if (mint)
	{
		return x_retresult(Dpathconf(path, which));
	}
	else
#endif
	{
		if (which == DP_PATHMAX)
			 return PATH_MAX;			/* = 128 in TOS */
		else if (which == DP_NAMEMAX)
			return 12;					/* 8 + 3 in TOS */
		return 0;
	}
}


/* 
 * Execute a program through Pexec
 */

long x_exec(int mode, void *ptr1, void *ptr2, void *ptr3)
{
	int result = xerror((int) Pexec(mode, ptr1, ptr2, ptr3));


	if ((result != EFILNF) && (result != EPTHNF) && (result != ENSMEM) && (result != EPLFMT))
		result = 0;

	return result;
}


/* 
 * GEM funkties 
 */

char *xshel_find(const char *file, int *error)
{
	char *buffer;

	if ((buffer = calloc(sizeof(VLNAME), 1)) == NULL)
	{
		*error = ENSMEM;
		return NULL;
	}
	else
	{
		strcpy(buffer, file);

		if (shel_find(buffer) == 0)
			*error = EFILNF;
		else
		{
			if ((*error = _fullname(buffer)) == 0)
				return buffer;
		}
		free(buffer);
		return NULL;
	}
}


/* 
 * Call a fileselector (make an extended call, if possible)
 * Memory is allocated for the returned path in "buffer"
 */

char *xfileselector(const char *path, char *name, const char *label)
{
	char *buffer;
	int error, button;

	if ((buffer = malloc_chk(sizeof(VLNAME))) == NULL)
		return NULL;

	strcpy(buffer, path);

	/* A call to a file selector MUST be surrounded by wind_update() */

	wind_update(BEG_UPDATE);

	/* 
	 * In fact there should be a check here if an alternative file selector
	 * is installed, in such case the extended file-selector call can be used
	 * although TOS is older than 1.04
	 */

	if ( tos_version >= 0x104 )
		error = fsel_exinput(buffer, name, &button, (char *) label);
	else
		error = fsel_input(buffer, name, &button);

	wind_update(END_UPDATE);

	if ((error == 0) || (button == 0))
	{
		if (error == 0)
			alert_printf(1, MFSELERR);
	}
	else
	{
		if ((error = _fullname(buffer)) == 0)
			return buffer;

		xform_error(error);
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

static int read_buffer(XFILE *file)
{
	long n;

	if ((n = x_read(file->handle, XBUFSIZE, file->buffer)) < 0)
		return (int)n;
	else
	{
		file->write = (int) n;
		file->read = 0;
		file->eof = (n != XBUFSIZE) ? TRUE : FALSE;

		return 0;
	}
}


/*
 * Write a buffer into an open file
 */

static int write_buffer(XFILE *file)
{
	long n;

	if (file->write != 0)
	{
		if ((n = x_write(file->handle, file->write, file->buffer)) < 0)
			return (int) n;

		else
		{
			if (n == (long) file->write)
			{
				file->write = 0;
				file->eof = TRUE;

				return 0;
			}
			else
				return EDSKFULL;
		}
	}
	else
		return 0;
}


/* 
 * Open a real file. Return pointer to a XFILE structure
 * which is created in this routine. 
 */

XFILE *x_fopen(const char *file, int mode, int *error)
{
	int 
		result = 0, 
		rwmode = mode & O_RWMODE;

	XFILE 
		*xfile;


	if ((xfile = malloc(sizeof(XFILE) + XBUFSIZE)) == NULL)
		result = ENSMEM;
	else
	{
		xfile->mode = mode;
		xfile->buffer = (char *) (xfile + 1);
		xfile->bufsize = (int) XBUFSIZE;
		xfile->read = 0;
		xfile->write = 0;
		xfile->eof = FALSE;
		xfile->memfile = FALSE;

		if (rwmode == O_RDONLY)
		{
			if ((xfile->handle = x_open(file, mode)) < 0)
				result = xfile->handle;
		}
		else if (rwmode == O_WRONLY)
		{
			if ((xfile->handle = x_create(file, NULL)) < 0)
				result = xfile->handle;
		}
		else
			result = EINVFN;

		if (result != 0)
		{
			free(xfile);
			xfile = NULL;
		}
	}

	*error = result;

	return xfile;
}


/* 
 * Open a memory area as a file (in fact, mode is always 0x01 | 0x02 )
 * If allocation is unsuccessful, return ENSMEM (-39)
 * If not read/write mode it returned EINVFN (-32) 
 * (why bother?, this routine is used only once, so this check
 * is disabled, but use carefully then!). If OK, return 0
 */

XFILE *x_fmemopen(int mode, int *error)
{
	int 
		result = 0; 

/* not needed
		rwmode = mode & O_RWMODE; /* mode & 0x03 */
*/

	XFILE 
		*xfile;

	/* A memory block is allocated and some structures set */

	if ((xfile = malloc(sizeof(XFILE))) == NULL)
		result = ENSMEM;
	else
	{
		xfile->mode = mode;
		xfile->buffer = NULL;	/* buffer location         */
		xfile->bufsize = 0;		/* buffer size             */
		xfile->read = 0;		/* read position (offset)  */
		xfile->write = 0;		/* write position (offset) */
		xfile->eof = FALSE;		/* end of file not reached */
		xfile->memfile = TRUE;	/* this is a memory file   */

/* no need to bother, routine is used only once and in a controlled way 
		if (rwmode != O_RDWR) /* 0x02 */
		{
			result = EINVFN; /* unknown function ? */
			free(xfile);
			xfile = NULL;
		}
*/
	}

	*error = result;
	return xfile;
}


/* 
 * Close a file 
 */

int x_fclose(XFILE *file)
{
	int error, rwmode = file->mode & O_RWMODE;

	if (file->memfile)
	{
		error = 0;
		free(file->buffer);
	}
	else
	{
		int h;

		h = (rwmode == O_RDONLY) ? 0 : write_buffer(file);
		if ((error = x_close(file->handle)) == 0)
			error = h;
	}

	free(file);

	return error;
}


/* This routine (a pair to x_fwrite) is never used in TeraDesk

/* 
 * Read file contents (not more than "length" bytes)
 */

long x_fread(XFILE *file, void *ptr, long length)
{
	long remd = length, n, size;
	char *dest = (char *) ptr, *src;
	int read, write, error;

	if (file->memfile)
	{
		read = file->read;
		write = file->write;
		src = file->buffer;

		while ((remd > 0) && (file->eof == FALSE))
		{
			if (read == write)
				file->eof = TRUE;
			else
			{
				*dest++ = src[read++];
				remd--;
			}
		}

		file->read = read;

		return (length - remd);
	}
	else
	{
		if ((file->mode & O_RWMODE) == O_WRONLY)
			return 0;

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
		}
		while ((remd > 0) && (file->eof == FALSE));

		return (length - remd);
	}
}

*/


/* 
 * Write 'length' bytes to a file;
 * return (negative) error code (long!) or else number of bytes written 
 * Note: open window data need about 20 bytes. For each open window about 
 * 32 bytes more is needed, plus path and mask lengths. So, one 128-byte 
 * record in a memory file would be fit for about 2-3 open windows. Better 
 * allocate slightly more, to avoid copying halfway into window saving, 
 * e.g. allocate 256 bytes.
 */

#define MRECL 256L

long x_fwrite(XFILE *file, void *ptr, long length)
{
	long 
		remd = length, /* number of bytes remaining to be transferred */ 
		n, 
		size;

	char 
		*dest, 					/* location of the output buffer */
		*src = (char *) ptr; 	/* position being read from */

	int 
		write,			 		/* position currently written to */
		error;					/* error code */

	if ((file->mode & O_RWMODE) == O_RDONLY)
		return EINVFN;

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

				new  = malloc(file->bufsize + MRECL);
				if ( new )
				{
					if ( file->buffer )
					{
						memcpy(new, file->buffer, file->bufsize);
						free(file->buffer);
					}
				}
				else
					return ENSMEM;	

				dest = file->buffer = new;
				file->bufsize += MRECL;
			}

			/* Now write till the end of input */

			dest[write++] = *src++;
			remd--;
		}

		file->write = write;

		return length;
	}
	else
	{
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
					return EDSKFULL;

				remd -= n;
			}
		} while (remd > 0);

		return (length - remd);
	}
}


/* Not used anymore

/*
 * Position file pointer to location 'offset' from the start of 
 * a 'memory file'
 */

long x_fseek(XFILE *file, long offset, int mode)
{
	if (!file->memfile || (mode != 0) || (offset != 0))
		return EINVFN;
	else
	{
		file->read = (int)offset;
		return 0;
	}
}

*/


/* 
 * Read a string from a file, but not more than 'n' characters 
 */

int x_fgets(XFILE *file, char *string, int n)
{
	boolean ready = FALSE;
	int i = 1, read, write, error;
	char *dest, *src, ch, nl = 0;

/* Why ? Just a safety precaution against careless use maybe?

	if ((file->mode & O_RWMODE) != O_RDONLY)
	{
		*string = 0;
		return 0;
	}
*/
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
				ready = TRUE;
			else
			{
				if ((error = read_buffer(file)) < 0)
					return error;
				read = file->read;
				write = file->write;
			}
		}
		else
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
			}
			else
			{
				/* 
				 * Handle characters including the first character
				 * of a <cr><lf> or <lf><cr> pair.
				 */

				ch = src[read++];
				if ((ch == '\n') || (ch == '\r'))
					nl = ch;
				else if (i < n)
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

boolean x_feof(XFILE *file)
{
	return ((file->eof) && (file->read == file->write)) ? TRUE : FALSE;
}


/* 
 * Find whether files can be locked in this OS variant ? 
 */

void x_init(void)
{
	if (find_cookie('_FLK') != -1)
		flock = TRUE;
	else
		flock = FALSE;
}

