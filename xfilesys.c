/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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

#include "desk.h"
#include "error.h"
#include "desktop.h"
#include "xfilesys.h"
#include "file.h"

#define XBUFSIZE	2048L

static boolean flock;
extern int tos_version, aes_version;


/*
 * Check if a filename or path + name is valid in this OS/
 * Return 0 if OK.
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
 * Return pointer to created string (being allocated in this routine)
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
 * otherwise follow the link. 
 */

boolean x_exist(const char *file, int flags)
{
	XATTR attr;
	int type, theflag;


#if _MINT_
	if ( x_attr( (flags & EX_LINK) ? 1 : 0, file,  &attr) != 0 )
#else
	if (x_attr(0, file, &attr) != 0) 
#endif
		return FALSE; /* x_attr can't find trget */
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



static int _fullname(char *buffer)
{
	char *name, *h, *d, *s;
	int error;

	if ((name = strdup(buffer)) == NULL)
		error = ENSMEM;
	else
	{
		d = buffer;

		if ((h = strchr(name, ':')) == NULL)
		{
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

	if ((buffer = malloc(sizeof(LNAME))) == NULL)
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


/* Funkties voor directories */

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

	if ((buffer = malloc(sizeof(LNAME))) == NULL)
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
 * Read target name of a symbolic link 
 */

int x_rdlink( int tgtsize, char *tgt, const char *linkname )
{
	return xerror( (int)Freadlink( tgtsize, tgt, (char *)linkname ) );
}


/*
 * Append a path to a link target definition, if it is not given.
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
 * just copy the name.This routine allocates space for the output "real" 
 * object name. If te name of the target does not contain a path, append
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
			if ( (tmp = malloc( sizeof(VLNAME) )) != NULL )
			{
				error = x_rdlink( (int)sizeof(VLNAME), tmp, linkname ); 
	
				/* If the name of the referenced item has been obtained... */

				if ( !error )
				{
/* improved below
					if ( strchr(tmp,'\\') == NULL )
					{
						/* referenced name does not contain a path, use that of the link */

						char 
							*lpath;

						if ( (lpath = fn_get_path(linkname)) != NULL )
						{
							target = x_makepath(lpath, tmp, &error);
							free(lpath);
						}
					}
					else
						target = strdup(tmp);
*/
					target = x_pathlink(tmp, linkname);
				}
				else
					/* this is not a link, just copy the name */
					target = strdup(linkname);
			}
			else /* tmp = NULL */
				xform_error(ENSMEM);
		}

		if (tmp == NULL)

#endif
			target = strdup(linkname);

		if ( target == NULL )
			xform_error(ENSMEM);

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
 */

int x_getlabel(int drive, char *label)
{
	DTA *olddta, dta;
	int error;
	char path[10];

	olddta = Fgetdta();
	Fsetdta(&dta);

	strcpy(path, "A:\\*.*");
	path[0] += (char) drive;

	error = Fsfirst(path, 0x3F);

	if ((error == 0) && (dta.d_attrib & FA_VOLUME)) 
		strcpy(label, dta.d_fname);
	else
		*label = 0;

	Fsetdta(olddta);

	return ((error == -49) || (error == -33)) ? 0 : error;
}


/* File funkties */

/* 
 * Rename a file from "oldname" to "newname"; 
 * note unusual (for C) order of arguments: (source, destination) 
 */

int x_rename(const char *oldname, const char *newname)
{
	return xerror(Frename(0, oldname, newname));
}


/* 
 * "Unlink" a file (i.e. delete) .
 * When operated on a symblic link, it deletes the link not the file
 */

int x_unlink(const char *file)
{
	return xerror(Fdelete(file));
}


/* 
 * Get/set GEMDOS file attributes 
 */

int x_fattrib(const char *file, int wflag, int attrib)
{
	return xerror(Fattrib(file, wflag, attrib));
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
	long result;

	if (!flock)
		mode &= O_RWMODE;

	result = Fopen(file, mode);

	return (result < 0) ? xerror((int)result) : (int)result;
}


/* 
 * Create a new file with specified attributes
 */

int x_create(const char *file, int attr)
{
	long result;

	result = Fcreate(file, attr);

	return (result < 0) ? xerror((int)result) : (int)result;
}


/* 
 * Close a file 
 */

int x_close(int handle)
{
	return xerror(Fclose(handle));
}


/* 
 * Read from a file 
 */

long x_read(int handle, long count, char *buf)
{
	long result;

	result = Fread(handle, count, buf);

	return (result < 0) ? (long) xerror((int) result) : result;
}


/* 
 * Write to a file 
 */

long x_write(int handle, long count, char *buf)
{
	long result;

	result = Fwrite(handle, count, buf);

	return (result < 0) ? (long) xerror((int) result) : result;
}


long x_seek(long offset, int handle, int seekmode)
{
	long result;

	result = Fseek(offset, handle, seekmode);

	return (result < 0) ? (long) xerror((int) result) : result;
}


/* Funkties voor het lezen van een directory */


/* 
 * Convert a DTA structure to a XATTR structure. The index, dev,
 * rdev, blksize and nblocks fields in attrib are not set. They
 * are not necessary anyway on TOS. 
 */

static void dta_to_xattr(DTA *dta, XATTR *attrib)
{

	attrib->mode = 0777;
	if (dta->d_attrib & FA_SUBDIR)
		attrib->mode |= S_IFDIR;
	else
		attrib->mode |= S_IFREG;
	if (dta->d_attrib & FA_READONLY)
		attrib->mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	attrib->size = dta->d_length;
	attrib->uid = 0;
	attrib->gid = 0;
	attrib->mtime = attrib->atime = attrib->ctime = dta->d_time;
	attrib->mdate = attrib->adate = attrib->cdate = dta->d_date;
	attrib->attr = (int) dta->d_attrib & 0xFF;
}


/* 
 * Inquire about details of filesystem
 * HR 151102: courtesy XaAES & EXPLODE; now it also works with MagiC 
 */

boolean x_inq_xfs(const char *path, boolean *casesens)
{
	long c, t, csens; char p[256];

	strcpy(p, path);
	if (*(p + strlen(p) - 1) != '\\')
		strcat(p, "\\");

	c = Dpathconf(p, DP_CASE);
	t = Dpathconf(p, DP_TRUNC);

	csens = !(c == DP_NOSENSITIVE && t == DP_DOSTRUNC);

	if (casesens)
		*casesens = csens;

	return (c >= 0 && csens);
}


/* 
 * Open a directory
 */

XDIR *x_opendir(const char *path, int *error)
{
	XDIR *dir;

	if ((dir = malloc(sizeof(XDIR))) == NULL)
		*error = ENSMEM;
	else
	{
		dir->path = (char *) path;
#if _MINT_
		dir->type = mint ? (x_inq_xfs(path, nil) ? 0 : 1) : 1;

		if (dir->type)
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
 * Read a directory entry 
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
	if (dir->type)
#endif
	{
		/* Mint is not present or, if it is, it is a DOS-filesystem volume */	

		int error;

		if (dir->data.gdata.first != 0)
		{
			/* 0x37 = READONLY | HIDDEN | SYSTEM | SUBDIR | ARCHIVE */
			make_path(fspec, dir->path, "*.*");	
			error = xerror(Fsfirst(fspec, 0x37));
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
		 * File system with long filenames.
		 * Use Dxreaddir in stead of Dreaddir, handle links correctly.
		 */
		long error, rep;

		if ((error = Dxreaddir(len, dir->data.handle, fspec, (long)attrib, &rep)) == 0)
			*buffer = fspec + 4L;

		if (error == 0)
			return error;
		
		return (long) xerror((int) error);
	}
#endif /* _MINT_ */
}


long x_rewinddir(XDIR *dir)
{

#if _MINT_
	if (dir->type)
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
	
		return (long)xerror((int)Drewinddir(dir->data.handle));
	}
#endif /* _MINT_ */
}


/* 
 * Close an open directory
 */

long x_closedir(XDIR *dir)
{
	long error;

#if _MINT_
	if (dir->type)
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
	
		error = (long)xerror((int)Dclosedir(dir->data.handle));
	}
#endif /* _MINT_ */

	free(dir); 
	return error; 
}


/* 
 * Read file atttributes (in the extended sense) 
 */

long x_attr(int flag, const char *name, XATTR *xattr)
{
#if _MINT_
	if (mint)
		return xerror((int)Fxattr(flag, name, xattr));
	else
#endif
	{
		DTA *olddta, dta;
		int result;
	
		olddta = Fgetdta();
		Fsetdta(&dta);
	
		if ((result = xerror(Fsfirst(name, 0x37))) == 0) /* 0x37=anything but volume */
			dta_to_xattr(&dta, xattr);

		Fsetdta(olddta);
		return result;
	}
}


/* 
 * Configuratie funkties 
 */

long x_pathconf(const char *path, int which)
{
#if _MINT_
	if (mint)
	{
		long result;
	
		result = Dpathconf(path, which);

		return (result < 0) ? xerror((int) result) : result;
	}
	else
#endif
	{
		if (which == DP_PATHMAX)
			 return PATH_MAX;			/* = 128 in TOS */
		else if (which == DP_NAMEMAX)
			return 12;
		return 0;
	}
}


/* 
 * Execute a program through Pexec
 */

long x_exec(int mode, void *ptr1, void *ptr2, void *ptr3)
{
	int result;

	result = xerror((int) Pexec(mode, ptr1, ptr2, ptr3));

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

	if ((buffer = calloc(sizeof(LNAME), 1)) == NULL)
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
 */

char *xfileselector(const char *path, char *name, const char *label)
{
	char *buffer;
	int error, button;

	if ((buffer = malloc(sizeof(LNAME))) == NULL)
	{
		xform_error(ENSMEM);
		return NULL;
	}
	strcpy(buffer, path);

	wind_update(BEG_UPDATE);
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
		return (int) n;

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
 * Open a real file 
 */

XFILE *x_fopen(const char *file, int mode, int *error)
{
	int result = 0, rwmode = mode & O_RWMODE;
	XFILE *xfile;

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
			if ((xfile->handle = x_create(file, 0)) < 0)
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
 * Open a memory area as a file 
 * (in fact, mode is always 0x01 | 0x02 )
 * If allocation is unsuccessful, return ENSMEM (-39)
 * If not read/write mode it returned EINVFN (-32) 
 * (why bother?, this routine is used only once, so this check
 * is disabled, but use carefully then!)
 * If OK, return 0
 */

XFILE *x_fmemopen(int mode, int *error)
{
	int 
		result = 0, 
		rwmode = mode & O_RWMODE; /* mode & 0x03 */

	XFILE 
		*xfile;

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

/* no need to bother, routine is used only once in a controlled way 
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
/*	not needed, already tested in free()
		if (file->buffer)
*/
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


/* 
 * Read file contents (not more than "lentgh" bytes)
 */

long x_fread(XFILE *file, void *ptr, long length)
{
	long rem = length, n, size;
	char *dest = (char *) ptr, *src;
	int read, write, error;

	if (file->memfile)
	{
		read = file->read;
		write = file->write;
		src = file->buffer;

		while ((rem > 0) && (file->eof == FALSE))
		{
			if (read == write)
				file->eof = TRUE;
			else
			{
				*dest++ = src[read++];
				rem--;
			}
		}

		file->read = read;

		return (length - rem);
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

			while ((read < write) && (rem > 0))
			{
				*dest++ = src[read++];
				rem--;
			}

			file->read = read;

			if ((rem >= XBUFSIZE) && (file->eof == FALSE))
			{
				size = rem - (rem % XBUFSIZE);

				if ((n = x_read(file->handle, size, dest)) < 0)
					return n;

				if (n != size)
					file->eof = TRUE;

				rem -= n;
			}
		}
		while ((rem > 0) && (file->eof == FALSE));

		return (length - rem);
	}
}


/* 
 * Write "length" bytes to a file;
 * return (negative) error code (long!) or else number of bytes written 
 */

long x_fwrite(XFILE *file, void *ptr, long length)
{
	long 
		rem = length, /* number of bytes remaining to be transferred */ 
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

		while (rem > 0)
		{
			if (write == file->bufsize)
			{
				char *new;

				/* 
				 * Existing buffer has been filled (or this is the first record)
				 * If this is the first "record" then allocate 128 bytes;
				 * otherwise, try to increase the allocated amount;
				 */

				new  = malloc(file->bufsize + 128L);
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
				file->bufsize += 128;
			}

			/* Now write till the end of input */

			dest[write++] = *src++;
			rem--;
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

			while ((write < XBUFSIZE) && (rem > 0))
			{
				dest[write++] = *src++;
				rem--;
			}

			file->write = write;

			if (file->write == XBUFSIZE)
			{
				if ((error = write_buffer(file)) < 0)
					return (long) error;
			}

			if (rem >= XBUFSIZE)
			{
				size = rem - (rem % XBUFSIZE);

				if ((n = x_write(file->handle, size, dest)) < 0)
					return n;

				if (n != size)
					return EDSKFULL;

				rem -= n;
			}
		} while (rem > 0);

		return (length - rem);
	}
}


/*
 * Position file pointer to location "offset" from the start of file
 */

long x_fseek(XFILE *file, long offset, int mode)
{
	if (!file->memfile || (mode != 0) || (offset != 0))
		return EINVFN;
	else
	{
		file->read = (int) offset;
		return 0;
	}
}


/* 
 * Read a string from a file, but not more than "n" characters 
 */

int x_fgets(XFILE *file, char *string, int n)
{
	boolean ready = FALSE;
	int i = 1, read, write, error;
	char *dest, *src, ch, nl = 0;

/* DjV 076: why? Disabled to use these routines for the memory file

	if (file->memfile)
		return EINVFN;
*/

/* DjV 076: why ? Just a safety precaution against careless use maybe?

	if ((file->mode & O_RWMODE) != O_RDONLY)
	{
		*string = 0;
		return 0;
	}
*/

	/* Has end-of-file been reached? */

	if (x_feof(file) == TRUE)
		return EEOF;

	dest = string;
	src = file->buffer;
	read = file->read;
	write = file->write;

	while (ready == FALSE)
	{
		if (read == write)
		{
			/* 
			 * end of buffer reached; read a new one, or this is end of file
			 * Note: if used carefully, read_buffer() should never happen
			 * with the memory file, because never should be more read
			 * than had been written
			 */

			if (file->eof == TRUE)
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
				ready = TRUE;
				if ((((ch = src[read]) == '\n') || (ch == '\r')) && (nl != ch))
					read++;
			}
			else
			{
				if (((ch = src[read++]) == '\n') || (ch == '\r'))
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
	return ((file->eof == TRUE) && (file->read == file->write)) ? TRUE : FALSE;
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

