/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren,
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

#define XBUFSIZE	2048L

static boolean flock;
extern int tos_version, aes_version;


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
 * Check if a file or folder exists 
 */

boolean x_exist(const char *file, int flags)
{
	XATTR attr;

	if (x_attr(0, file, &attr) != 0)
		return FALSE;
	else
	{
		if (((attr.mode & S_IFMT) == S_IFDIR) && ((flags & EX_DIR) == 0))
			return FALSE;
		else
		{
			if (((attr.mode & S_IFMT) != S_IFDIR) && ((flags & EX_FILE) == 0))
				return FALSE;
		}
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
	char *buffer, *h;

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
/*		HR 120803 not really a point anymore
	h = realloc(buffer, strlen(buffer) + 1);

	return (h == NULL) ? buffer : h;
*/
	return buffer;
}

/* Funkties voor directories */

/* Set a directory path */
int x_setpath(const char *path)
{
	return xerror(Dsetpath(path));
}

/* 
 * Get current default path, return pointer to this new-allocated string 
 */

char *x_getpath(int drive, int *error)
{
	char *buffer, *h;

	if ((buffer = malloc(sizeof(LNAME))) == NULL)
	{
		*error = ENSMEM;
		return NULL;
	}

	buffer[0] = (char) (((drive == 0) ? x_getdrv(): (drive - 1)) + 'A');
	buffer[1] = ':';

	if ((*error = xerror(Dgetpath(buffer + 2, drive))) < 0)
	{
		free(buffer);
		return NULL;
	}
	
/*		HR 120803 not really a point anymore
	h = realloc(buffer, strlen(buffer) + 1);

	return (h == NULL) ? buffer : h;
*/
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


/* 
 * Get information about free space on a disk volume 
 */

int x_dfree(DISKINFO *diskinfo, int drive)
{
	return xerror(Dfree(diskinfo, drive));
}

/* Get information about current default drive */
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
 * note unusual (for C) order of arguments: (src, dest) 
 */

int x_rename(const char *oldname, const char *newname)
{
	return xerror(Frename(0, oldname, newname));
}


/* 
 * "Unlink" a file (i.e. delete) 
 */

int x_unlink(const char *file)
{
	return xerror(Fdelete(file));
}


/* 
 * Get GEMDOS file attributes 
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

/* handle this differently now in set_visible()
	/* 
	 * DjV 004 300103 Add pseudo attribute "parent dir" 
	 * note: can this be a problem? A document on TOS functions
	 * states that bits 6 and 7 (i.e. 0x40 and 0x80, and FA_PARDIR = 0x40 !)
	 * of attribute field are "reserved"?
	 */

	if ( (dta->d_fname[0] == '.') && (dta->d_fname[1] == '.') )
		dta->d_attrib |= FA_PARDIR;
*/

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
 * DjV 071 270703 code size reduction 
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
 * Note: DjV 070: In order to increase speed, only the pointer is passed, 
 * and the name is no more copied to output location (in all uses of this
 * routine in TeraDesk, obtained name is immediately copied elsewhere).
 * **buffer should probably be considered as readonly,
 * not to be written to or used for permanent storage
 * DjV 070 270703 code size reduction
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
		/* File system with long filenames */
	
		return (long)xerror(Drewinddir(dir->data.handle));
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
	
		error = (long)xerror(Dclosedir(dir->data.handle));
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
		return xerror(Fxattr(flag, name, xattr));
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
			 return 128;
		else if (which == DP_NAMEMAX)
			return 12;
		return 0;
	}
}


/* 
 * Funkties voor het uitvoeren van programma's 
 */

long x_exec(int mode, void *ptr1, void *ptr2, void *ptr3)
{
	int result;

	result = xerror((int) Pexec(mode, ptr1, ptr2, ptr3));

	if ((result != EFILNF) && (result != EPTHNF) && (result != ENSMEM) && (result != EPLFMT))
		result = 0;

	return result;
}


/* Geheugen funkties */


/* 
 * Allocate a memory block, return address of the block
 * HR 120803 only used now for interrogation 
 */

void *x_alloc(long amount)
{
	return Malloc(amount);
}


/* 
 * Deallocate a memory block 
 */

int x_free(void *block)
{
	return xerror(Mfree(block));
}


/* 
 * Shrink a block of allocated memory 
 */

int x_shrink(void *block, long newsize)
{
	return xerror(Mshrink(0, block, newsize));
}


/* 
 * GEM funkties 
 */

char *xshel_find(const char *file, int *error)
{
	char *buffer, *h;

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
			{
			/*		HR 120803 not really a point anymore
				if ((h = realloc(buffer, strlen(buffer) + 1)) == NULL)
					h = buffer;
				return h;
			*/
				return buffer;
			}
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
	char *buffer, *h;
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
		{
			/*		HR 120803 not really a point anymore
			if ((h = realloc(buffer, strlen(buffer) + 1)) == NULL)
				h = buffer;
			return h;
			*/

			return buffer;
		}
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
 * Open a file 
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
 * Open a memory area sa a file 
 */

XFILE *x_fmemopen(int mode, int *error)
{
	int result = 0, rwmode = mode & O_RWMODE;
	XFILE *xfile;

	if ((xfile = malloc(sizeof(XFILE))) == NULL)
		result = ENSMEM;
	else
	{
		xfile->mode = mode;
		xfile->buffer = NULL;
		xfile->bufsize = 0;
		xfile->read = 0;
		xfile->write = 0;
		xfile->eof = FALSE;
		xfile->memfile = TRUE;

		if (rwmode != O_RDWR)
		{
			result = EINVFN;
			free(xfile);
			xfile = NULL;
		}
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
		if (file->buffer)
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
	long rem = length, n, size;
	char *dest, *src = (char *) ptr;
	int write, error;

	if ((file->mode & O_RWMODE) == O_RDONLY)
		return EINVFN;

	if (file->memfile)
	{
		write = file->write;
		dest = file->buffer;

		while (rem > 0)
		{
			if (write == file->bufsize)
			{
				char *new;

				if (file->buffer)
				{
					new = realloc(file->buffer, file->bufsize + 128);
/* HR 120803: the official realloc doesnt initialize the new buffer */
					if (new != file->buffer)
						memcpy(new, file->buffer, file->bufsize);
				}
				else
					new = malloc(128);

				if (new == NULL)
					return ENSMEM;
				dest = file->buffer = new;
				file->bufsize += 128;
			}

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
 * It is imparative that this function has a maximum
 * be it only to protect the stack in certain calling functions. 
 */

char *x_freadstr(XFILE *file, char *string, size_t max, int *error)
{
	int l;
	size_t ll;	/* a long is expected in some calls later */

	long n;
	char *s;

	if ((n = x_fread(file, &l, sizeof(int))) == sizeof(int))
	{
		ll = (size_t)l;
		if (ll > max - 1)		/* max is basically the size of an array that leaves place for a null character. */
			ll = max - 1;

		if (string == NULL)
		{
			if ((s = malloc(ll + 1L)) == NULL)
			{
				*error = ENSMEM;
				return NULL;
			}
		}
		else
			s = string;

		if ((n = x_fread(file, s, ll + 1L)) != ll + 1L) 
		{
			if (string == NULL)
				free(s);
			*error = (n < 0) ? (int) n : EEOF;
			return NULL;
		}
	}
	else
	{
		*error = (n < 0) ? (int) n : EEOF;
		return NULL;
	}

	*error = 0;

	return s;
}


/* 
 * Write a string to a file 
 */

int x_fwritestr(XFILE *file, const char *string)
{
	int l;

	long n;

	l = (int) strlen(string);

	if ((n = x_fwrite(file, &l, sizeof(int))) < 0)
		 return (int) n;

	if ((n = x_fwrite(file, string, (size_t)(l + 1) )) < 0)
		return (int) n;

	return 0;
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

/* DjV 076: why ? Just a safety precaution maybe?

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

