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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <np_aes.h>
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <library.h>
#include <tos.h>
#include <mint.h>
#include <xdialog.h>
#include <xscncode.h>
#include <limits.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "events.h"
#include "copy.h"
#include "printer.h"
#include "font.h"
#include "file.h"
#include "config.h"
#include "window.h"
#include "lists.h"
#include "slider.h"
#include "filetype.h"
#include "viewer.h"
#include "applik.h"
#include "showinfo.h"
#include "dir.h"
#include "va.h"
#include "open.h"


typedef struct copydata
{
	int result;
	XDIR *dir;
	char *spath;
	char *dpath;
	char *sname;
	char *dname;
	struct copydata *prev;
	boolean chk;
} COPYDATA;

#if _SHOWFIND
extern int search_nsm;
#endif

DOSTIME
	now,
	optime;

int
#if _MINT_
	opmode,
	opuid,
	opgid,
	curruid = 0,
	currgid = 0,
#endif
	opattr;

static int
	sd = 0; /* amount of reduction of dialog height */

static XDINFO
	cfdial;

boolean
	cfdial_open,
	rename_files = FALSE;

static boolean
	set_oldpos,
	overwrite,
	updatemode = FALSE,
	restoremode = FALSE;

static int del_folder(const char *name, int function, int prev, XATTR *attr);
static int del_file(const char *name, int prev, XATTR *attr);
static int chk_access(XATTR *attr);
int trash_or_print(ITMTYPE type);
static void hideskip(int n, OBJECT *obj); /* from icon.h */


/*
 * Check for user abort of an operation by [ESCAPE]
 */

void check_opabort (int *result)
{
	if (*result != XFATAL)
	{
		if ( escape_abort(cfdial_open) )
			*result = XABORT;
	}
}


/*
 * Update displayed information on number of files/folders currently being
 * copied/moved/printed/deleted/touched.
 * These fields are updated while the dialog is open; in the resource they
 * should be set to background, opaque, white, no pattern, in order to
 * inherit gray dialog background colour, but Magic does not seem to respect
 * this. Therefore they are drawn transparent in an opaque background box.
 * This may cause some flicker on a slower machine.
 * If folders is set to -1, don't update number of folders or files
 * (used for updating only the number of bytes when copying large files)
 */

void upd_copyinfo(long folders, long files, LSUM *bytes)
{
	long
		total;


	if (folders >= 0)
	{
		rsc_ltoftext(copyinfo, NFOLDERS, folders);
		rsc_ltoftext(copyinfo, NFILES, files);
	}

	size_sum(&total, bytes);

	rsc_ltoftext(copyinfo, NBYTES, total);

	if (cfdial_open)
		xd_draw(&cfdial, CINFBOX, 1);
}


/*
 * A small routine that helps to change the size of the copyinfo dialog;
 * "d" is the amount of increase of dialog height.
 */

static void ci_resize (int d)
{
		copyinfo[COKBOX].r.y += d; 		/* make the dialog as small as practical */
		copyinfo[COPYBOX].r.h += d;		/* same */
}


/*
 * Open the dialog showing information during copying, moving, printing...
 */

int open_cfdialog(long folders, long files, LSUM *bytes, int function)
{
	int
		sd1,
		mask,
		button,
		title;

	static const int
		items[] = {PSETBOX, CSETBOX, CPT3, CPDEST, CFOLLNK, CPRFILE, 0};


	sd = 0;
	sd1 = copyinfo[COKBOX].r.y - copyinfo[CPT3].r.y - xd_fnt_h;

	/* Set default visibility and state of some objects */

	rsc_hidemany(copyinfo, items);
	xd_set_rbutton(copyinfo, PSETBOX, PRTXT + printmode);

	/*
	 * (Almost) always display the dialog.
	 * Some dirty code here: using several gotos to reduce program size
	 */

	switch (function)
	{
		case CMD_COPY:
		{
			title = (rename_files) ? DTCOPYRN : DTCOPY;
			goto unhide1;
		}
		case CMD_MOVE:
		{
			title = (rename_files) ? DTMOVERN : DTMOVE;

			unhide1:;

			mask = CF_COPY;
			obj_unhide(copyinfo[CPT3]);
			obj_unhide(copyinfo[CPDEST]);
#if _MINT_
			obj_unhide(copyinfo[CFOLLNK]);
#endif
			goto unhide2;
		}
		case CMD_TOUCH:
		{
			mask = CF_TOUCH;
			title = DTTOUCH;

			unhide2:;

			obj_unhide(copyinfo[CSETBOX]);
#if _MINT_
			copyinfo[CFOLLNK].r.y = copyinfo[CSETBOX].r.y;
#endif
			break;
		}
		case CMD_DELETE:
		{
			mask = CF_DEL;
			title = DTDELETE;
#if _MINT_
			copyinfo[CFOLLNK].r.y = copyinfo[CPT3].r.y;
			obj_unhide(copyinfo[CFOLLNK]);
#endif
			sd = sd1;
			break;
		}
		case CMD_PRINT:
		{
			title = DTPRINT;
			obj_unhide(copyinfo[PSETBOX]);
			goto unhide3;
		}
		case CMD_PRINTDIR:
		{
			title = DTPRINTD;
			sd = sd1 - xd_fnt_h;

			unhide3:;

			mask = CF_PRINT;
			obj_unhide(copyinfo[CPRFILE]);
		}
	}

#if _MINT_
	if(!mint)
		obj_hide(copyinfo[CFOLLNK]);
#endif

	/* In update or restore mode, override dialog title assignment */

	if ( updatemode )
		title = DTUPDAT;

	if ( restoremode )
		title = DTRESTO;

	/*
	 * Write numbers of folders, files and bytes to dialog fields;
	 * note: as the open flag is still not set, nothing will be drawn
	 */

	upd_copyinfo(folders, files, bytes);

	/* Set dialog title */

	rsc_title(copyinfo, CPTITLE, title);

	/* Open (or redraw) the dialog */

	if ( !cfdial_open && (options.cprefs & ( mask | CF_SHOWD )) )
	{
		ci_resize(-sd);

		if(chk_xd_open(copyinfo, &cfdial) >= 0)
			cfdial_open = TRUE;				/* dialog has just been opened */
		else
			ci_resize(sd);
	}

	/*
	 * If option is set to confirm action, wait for the correct button;
	 * If confirm is not needed, just redraw the dialog if it is open.
	 */

	button = COPYOK;

	if ( cfdial_open )
	{
		if ((options.cprefs & mask) != 0)	/* If confirm, call xd_form_do */
			button = xd_form_do(&cfdial, ROOT);
		else
			xd_drawdeep(&cfdial, ROOT);
	}

	set_oldpos = FALSE;

	return button;
}


/*
 * Close the informational dialog for copying/moving/printing
 */

void close_cfdialog(int button)
{
	if (cfdial_open)
	{
		xd_buttnorm(&cfdial, button);
		xd_close(&cfdial);
		ci_resize(sd);

		sd = 0;

		if (printfile)
			x_fclose(printfile); /* this does a free() as well */

		printfile = NULL;
	}

	cfdial_open = FALSE;
}


/*
 * Display a more complete information for file copy.
 * Note1: see upd_copyinfo for proper setting of these fields in the resource.
 * Note2: because of background colour problem in Magic, all fields are
 * always drawn in a background box.
 */

void upd_copyname( const char *dest, const char *folder, const char *file )
{
	if ( cfdial_open )
	{
		/* Note: this can't be done selectively, because of transparency problems */

		if ( folder )
			cv_fntoform(copyinfo, CPFOLDER, folder);

		if ( file )
			cv_fntoform(copyinfo, CPFILE, fn_get_name(file) );

		if ( dest )
			cv_fntoform(copyinfo, CPDEST, dest);

		xd_draw(&cfdial, CNAMBOX, 1);
	}
}


/*
 * Push an item onto the stack
 */

static int push(COPYDATA **stack, const char *spath, const char *dpath, boolean chk)
{
	COPYDATA
		*new;

	int
		error = 0;


	if ((new = malloc(sizeof(COPYDATA))) == NULL)
		error = ENSMEM;
	else
	{
		new->spath = (char *)spath;
		new->dpath = (char *)dpath;
		new->chk = chk;
		new->result = 0;

		if ((new->dir = x_opendir(spath, &error)) != NULL)
		{
			new->prev = *stack;
			*stack = new;
		}
		else
			free(new);
	}

	return error;
}


/*
 * Pull an item from the stack
 */

static boolean pull(COPYDATA **stack, int *result)
{
	COPYDATA
		*top = *stack;


	x_closedir(top->dir);
	*result = top->result;
	*stack = top->prev;

	free(top);

	return (*stack == NULL) ? TRUE : FALSE;
}


/*
 * Read directory entry on the stack.
 * Beware: "*name" buffer should be at least sizeof(VLNAME) characters long
 */

static int stk_readdir(COPYDATA *stack, char *name, XATTR *attr, boolean *eod)
{
	int
		error;

	char
		*fname;

	size_t
		ms = sizeof(VLNAME);


	/*
	 * Changed name to &fname. Actually, for a DOS/TOS FAT fs, fname now points
	 * to a string which is also pointed to from stack->dir;
	 * for other FSes it points to a static space defined in x_xreaddir.
	 */

	while (((error = (int)x_xreaddir(stack->dir, &fname, ms, attr)) == 0)
		   && ((strcmp(prevdir, fname) == 0) || (strcmp(".", fname) == 0)));

	strsncpy ( name, fname, ms );

	if ((error == ENMFIL) || (error == EFILNF))
	{
		error = 0;
		*eod = TRUE;
	}
	else
		*eod = FALSE;

	return error;
}


/*
 * Two routines for summing or subtracting file sizes,
 * sum possibly exceeding 32-bit limit. Kilobytes are summed
 * separately from the remainders
 */

void add_size(LSUM *nbytes, long fsize)
{
	long
		k, b;


	fsize &= 0x7FFFFFFFL;	/* guard against 31bit overflow ? */

#if KBMB == 1024
	b = fsize & 0x000003FFL;
	k = (fsize ^ 0x000003FFL) >> 10;
#else
	b = fsize & 0x000FFFFFL;
	k = (fsize ^ 0x000FFFFFL) >> 20;
#endif

	nbytes->kbytes += k;
	nbytes->bytes += b;

	if(nbytes->bytes > (KBMB - 1))
	{
		nbytes->kbytes += 1;
		nbytes->bytes -= KBMB;
	}
}


void sub_size(LSUM *nbytes, long fsize)
{
	long k, b;

	fsize &= 0x7FFFFFFFL;	/* guard against 31bit overflow ? */

#if KBMB == 1024
	b = fsize & 0x000003FFL;
	k = (fsize ^ 0x000003FFL) >> 10;
#else
	b = fsize & 0x000FFFFFL;
	k = (fsize ^ 0x000FFFFFL) >> 20;
#endif

	nbytes->kbytes -= k;
	nbytes->bytes -= b;

	if(nbytes->bytes < 0)
	{
		nbytes->kbytes -= 1;
		nbytes->bytes += KBMB;
	}
}


/*
 * This function prepares data for rsc_ltoftext() which
 * displays object size in either bytes, kilobytes or megabytes.
 * If output value is negative, it represents KB.
 * (depending on the definition of KBMB it may arepresent MB)
 */

void size_sum(long *total, LSUM *bytes)
{
	if(bytes->kbytes > (DISP_KBMB / KBMB) )
	{
		*total = -(bytes->kbytes);

		if(bytes->bytes)
			*total -= 1;
	}
	else
	{
		*total = bytes->kbytes * KBMB + bytes->bytes;
	}
}


/*
 * Routine for counting the files and directories and summing thir sizes.						                                *
 * Also used to recursively search for a file/folder.
 * Parmeter attribs specifes a filter for counting items.
 * Beware: currently there is no protection agains overrunning 32-bit sums!
 */

int cnt_items
(
	const char *path,	/* path to look into */
	long *folders,		/* folders count */
	long *files, 		/* files count */
	LSUM *bytes,		/* bytes count */
	int attribs, 		/* attributes mask for item selection */
	boolean search		/* true if this is done while searching */
)
{
	COPYDATA
		*stack;

	int
		error,
		dummy,
		result;

	unsigned int
		type;				/* item type */

	VLNAME
		name;   			/* Can this be LNAME ? */

	XATTR
		attr;

	ITMTYPE
		inftype;

	char
		*fpath = NULL;		/* item path */

	boolean
		found = FALSE,		/* match not found yet */
		gosub = ( !search || ((options.xprefs & S_SKIPSUB) == 0) ),
		ready = FALSE,
		eod = FALSE,
		nodir = FALSE;


	type = 0;
	*folders = 0;			/* folder count */
	*files = 0;				/* files count  */
	bytes->bytes = 0;		/* bytes count  */
	bytes->kbytes = 0;		/* kbytes count */
	stack = NULL;
	result = XSKIP;

	hourglass_mouse();

	if ((error = push(&stack, path, NULL, FALSE)) != 0)
	{
		if(search && error == EPTHNF )
		{
			nodir = TRUE;
			error = 0;		/* a fix needed to search in selected items */
		}
		else
			return error;
	}

	do
	{
		if (/* nodir || */ error == 0) /* nodir commented - see the fix above */
		{
			inftype = 0;
			found = FALSE;

			if (!nodir && ((error = stk_readdir(stack, name, &attr, &eod)) == 0) && (eod == FALSE))
			{
				type = attr.mode & S_IFMT;
				nodir = FALSE;

				if ( search )
				{
					/* Has a search match been found ? */


					if ( (found = searched_found( stack->spath, name, &attr)) == TRUE )
						fpath = x_makepath(stack->spath, name, &error);
				}

				if (type == S_IFDIR)
				{
					/* This item is a directory */

					if ( (attribs & FA_SUBDIR) != 0 )
						*folders += 1;

					if ( gosub && (stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						if ((error = push(&stack, stack->sname, NULL, FALSE)) != 0)
							free(stack->sname);
						else
							if( (attribs & FA_SUBDIR) != 0 )
								inftype = ITM_FOLDER;
					}
				}
				else if (type == S_IFREG || type == S_IFLNK)
				{
					/* This item is a file or a link */

					*files += 1;
					add_size(bytes, attr.size);
					inftype = ITM_FILE;
				}
			} /* error == 0 ? */
			else if(search && !eod && (nodir || error == EPTHNF))
			{
				/* this branch is only for searching in a list of files */

				char *pathonly = NULL;

				if (x_attr(0, FS_INQ, path, &attr) >= 0)
				{
					if((attr.mode & S_IFMT) == S_IFREG)
					{
						pathonly = fn_get_path(path);
						strsncpy(name, fn_get_name(path), sizeof(VLNAME));

						if ((found = searched_found(pathonly, name, &attr)) != 0 )
							fpath= strdup(path);

						ready = TRUE;
					}

					result = error = XSKIP;
				}

				free(pathonly);

				inftype = ITM_FILE;
			}

			if ( search )
			{
				if ( found && inftype && (result = object_info(inftype, fpath, name, &attr ) ) != 0 )
					free(fpath);

				/*
				 * Note: escape_abort() also processes AV messages and AP_TERM.
				 * If escape_abort() were permitted outside search,
				 * more frequent operations would be slowed down dramatically.
				 */

				if ( escape_abort(TRUE) )
					result = XABORT;
				else if (found)
					hourglass_mouse();
			}
		} /* error == 0 ? */

		if (!nodir && (eod || (error != 0) || (result == XABORT)) )
		{
			if ((ready = pull(&stack, &dummy)) == FALSE)
				free(stack->sname);
		}

		/* Result will be 0 only if selected OK in dialogs */

		if ( search && (result != XSKIP) )
		{
			closeinfo(); /* close the info and search dialogs */

			if ( fpath != NULL && result == 0 )
			{
				path_to_disp ( fpath );
				wd_menu_ienable(MSEARCH, 0);
				wd_deselect_all();
				dir_add_window ( fpath, NULL, name  );
#if _SHOWFIND
				if ( search_nsm > 0 )		/* Open a text window to show searched-for string */
					txt_add_window (xw_top(), selection.selected, 0, NULL);
#endif
			}

			return result;
		}
	}
	while (!ready);

	if ( search && (result != XSKIP) )
		return XSKIP;
	else
		return error;
}


#if __USE_MACROS

#define dir_error(x,y)	xhndl_error(MESHOWIF, x, y)

#else

static int dir_error(int error, const char *file)
{
	return xhndl_error(MESHOWIF, error, file);
}

#endif


/*
 * Count items specified for a copy/move operation, and find total
 * numbers of folders, files, and bytes.
 * This routine recurses into subdirecories.
 * Beware: there is no protection against overflowing 32-bit sums
 * in large directories.
 */

static boolean count_items
(
	WINDOW *w,
	int n,			/* number of selected items */
	int *list,		/* list of indices of selected items */
	long *folders,	/* returned count of folders to act upon */
	long *files,	/* returned count of files to act upon */
	LSUM *bytes,	/* returned sum of bytes to act upon */
	int function	/* function to perform */
)
{
	const char
		*path;

	long
		dfolders,
		dfiles;

	LSUM
		dbytes;

	XATTR
		attr;

	int
		*listi = list,
		i,
		error,
		item;

	ITMTYPE
		type;

	boolean
		link,
		ok = TRUE;


	/* Zero all sums, error, etc */

	*folders = 0;
	*files = 0;
	bytes->bytes = 0;
	bytes->kbytes = 0;
	error = 0;
	i = 0;

	hourglass_mouse();

	while ((i < n) && ok)
	{
		/* Restore all indices to positive values */

		if(*listi < 0)
		{
			if(*listi == INT_MIN)
				*listi = 0;
			else
				*listi = - *listi;
		}

		item = *listi;
		error = 0;

		/*
		 * If deleting, it may happen that both the link
		 * and the referenced object have to be counted.
		 * The following block will be executed only if the
		 * object is a link, and is to be followed in order
		 * to delete both the link and the referenced object.
		 */

		if
		(
			itm_follow(w, item, &link, (char **)(&path), &type) &&
			(function == CMD_DELETE)
		)
		{
#if _MINT_
			if ((error = itm_attrib(w, item, 1, &attr)) == 0)
			{
				*files += 1;
				add_size(bytes, attr.size);
			}
			else
			{
				*listi = -1; /* will  later become -list[i] */
				goto next; /* dirty, dirty :) */
			}
#endif
		}

		if (isfileprog(type) || link)
		{
			if ((error = itm_attrib(w, item, (link || (type == ITM_NETOB)) ? 1 : 0, &attr)) == 0)
			{
				*files += 1;
				add_size(bytes, attr.size);
			}
			else
				*listi = -1; /* will later become -list[i] */
		}
		else
		{
			if(path != NULL)
			{
				if ( function != CMD_PRINT && function != CMD_PRINTDIR )
				{
					if ((error = cnt_items(path, &dfolders, &dfiles, &dbytes, FA_ANY, FALSE)) == 0)
					{
						*folders += dfolders + ((type == ITM_DRIVE) ? 0 : 1);
						*files += dfiles;
						bytes->kbytes += dbytes.kbytes;
						add_size(bytes, dbytes.bytes);
					}
					else
						*listi = -1; /* will later becme -list[i] */
				}
				else
					if ( function == CMD_PRINTDIR )
						*folders += ((type == ITM_FOLDER) ? 1 : 0);
				free(path);
			}
			else
				ok = FALSE;
		}

#if _MINT_
		next:;
#endif
		if (error != 0)
		{
			if( item == 0 && (*listi == -1) )
				*listi = INT_MIN;

			if (dir_error(error, itm_name(w, item)) != XERROR)
				ok = FALSE;
		}

		i++;
		listi++;
	}

	arrow_mouse();

	if ((*files == 0) && (*folders == 0))
		ok = FALSE;

	return ok;
}


/*
 * Copy one file and set date/time and attributes.
 * Note: rbytes is changed locally as "bytes"
 */

static int filecopy
(
	const char *sname,	/* source name */
	const char *dname,	/* destination name */
	XATTR *src_attrib,	/* source attributes */
	DOSTIME *time,		/* time/date stamp */
	LSUM *rbytes		/* file size */
)
{
	LSUM
		bytes = *rbytes;

	long
		slength,
		dlength,
		size,
		mbsize;

	void
		*buffer;

	int
		fh1,
		fh2 = -1,
		error = 0;


	/*
	 * Create a buffer for copying: If it is not possible to create the
	 * buffer as specified, find the largest free block and leave 8KB.
	 * Buffer should be at least 1KB large
	 */

	mbsize = (long)options.bufsize * 1024L;

	if ((size = (long) Malloc(-1L) - 8192L) > mbsize)
		size = mbsize;

	if ((size >= 1024L) && ((buffer = malloc(size)) != NULL) )
	{
		fh1 = x_open(sname, O_DENYW | O_RDONLY);

		if (fh1 < 0)
			error = fh1;
		else
		{
			fh2 = x_create(dname, src_attrib);

			if(fh2 < 0 )
				error = fh2;
			else
			{
				do
				{
					slength = 0; /* Probably not needed */

					if ((slength = x_read(fh1, size, buffer)) > 0)
					{
						check_opabort(&error);

						dlength = x_write(fh2, slength, buffer);

						if ((dlength < 0) || (slength != dlength))
							error = (dlength < 0) ? (int)dlength : EDSKFULL;

						/* If full buffer read, probably not end of file */

						if(slength == size && dlength >= 0)
						{
							sub_size(&bytes, dlength);
							upd_copyinfo(-1L, 0, &bytes);
							check_opabort(&error);
						}

					}
					else
						error = (int)slength; /* a small negative number */
				}
				while ((slength == size) && (error == 0));

				/* Set file date and time */

				if (error == 0)
					x_datime(time, fh2, 1);

				x_close(fh2);
			}

			if (error != 0)
				x_unlink(dname);

			x_close(fh1);

		} /* fh1 ? */

		free(buffer);
	}
	else
		error = ENSMEM;

	return error;
}


#if _MINT_
/*
 * Copy a symbolic link.
 * Currently this just creates a new link with the same contents as the old one
 * Attributes and date/time are ignored (for the time being?).
 */

static int linkcopy
(
	const char *sname,	/* source name */
	const char *dname	/* destination name */
/* currently unused
	,XATTR *src_attrib,	/* source attributes */
	DOSTIME *time		/* time stamp */
*/
)
{
	char
		*tgtname;	/* link target name */

	int
		error = 0;	/* error code */


	/* Determine link target name */

	if ( ( tgtname = x_fllink( (char *)sname) ) == NULL )
		return ENSMEM;

	/* If an identically named link already exists at destination, try to delete it */

	if (x_exist( dname, EX_LINK ))
		error = x_unlink( dname );

	/* If previous operation succeeded, make a new link */

	if (!error)
		error = x_mklink( dname, tgtname );

	free(tgtname);
	return error;
}
#endif


/*
 * Routine voor het afhandelen van fouten.
 */

int copy_error
(
	int error, 			/* error code */
	const char *name,	/* object name */
	int function		/* operation in which the error appears: copy, move... */
)
{
	int
		msg,	/* message identifier */
		irc;	/* return code */


	switch(function)
	{
		case CMD_DELETE:
		{
			msg = MEDELETE;
			break;
		}
		case CMD_MOVE:
		{
			msg = MEMOVE;
			break;
		}
		case CMD_COPY:
		{
			msg = MECOPY;
			break;
		}
		case CMD_TOUCH:
		{
			msg = MESHOWIF;
			break;
		}
		default:			/* CMD_PRINT, CMD_PRINTDIR */
		{
			msg = MEPRINT;
		}
	}

	/* a table would have been even better above ! */

	arrow_mouse();
	irc = xhndl_error(msg, error, name);
	hourglass_mouse();

	return irc;
}


/*
 * Routine voor het controleren van het kopieren.
 * Check if all items can be copied (or deleted)
 * Note: parameter 'list' is locally modified.
 */

static boolean check_copy
(
	WINDOW *w,			/* source directory window */
	int n,				/* number of items selected */
	int *list,			/* list of item indices */
	const char *dest	/* name of destination */
)
{
	const char
		*path;

	long l;

	int
		i = 0,
		mes;

	ITMTYPE
		type;

	boolean
		result = TRUE;


	/* Check if all specified items can be copied */

	while ((i < n) && result)
	{
		/* Note: nothing will be checked if dest is NULL */

		if ((((type = itm_type(w, *list)) == ITM_FOLDER) || (type == ITM_DRIVE)) && (dest != NULL))
		{
			/* Note: space for path allocated here */

			if ((path = itm_fullname(w, *list)) != NULL)
			{
				l = (long)strlen(path);

				if ((strncmp(path, dest, l) == 0) &&
					(((type != ITM_DRIVE) && ((dest[l] == '\\') || (dest[l] == 0))) ||
					 ((type == ITM_DRIVE) && (dest[l] != 0))))
				{
					alert_printf(1, AILLCOPY);
					result = FALSE;
				}

				free(path);
			}
			else
				result = FALSE;
		}
		else
		{
			/* Can't copy the trashcan or the printer (or unknown or network object) */

			if ((mes = trash_or_print(type)) != 0)
			{
				alert_cantdo(mes, MNOCOPY);
				result = FALSE;
			}
		}

		i++;
		list++;
	}

	return result;
}


/*
 * Just rename a single file and update windows if needed
 */

int frename
(
	const char *oldfname, 	/* old filename */
	const char *newfname, 	/* new filename */
	XATTR *attr				/* attributes */
)
{
	int
		error = chk_access(attr);


	if(!error)
		error = x_rename(oldfname, newfname);

	if (!error)
		wd_set_update(WD_UPD_MOVED, oldfname, newfname);

	return error;
}


/*
 * Take a new name from the name conflict dialog and rename "old" file
 */

static int _rename(char *old, XATTR *attr)
{
	char
		*new,
		*name;

	VLNAME
		newfname;

	int
		error,
		result = 0;


	/* Get new name from the dialog */

	cv_formtofn(newfname, nameconflict, OLDNAME);

	if(x_checkname(empty, newfname)) /* too long? */
		return XFATAL;

	/* Extract old name only without path */

	name = fn_get_name(old);

	/* Create new full name */

	if ((new = fn_make_newname(old, newfname)) == NULL)
		return XFATAL;
	else
	{
		/* If a name has been successfully created, try to rename item */

		if ( (error = frename(old, new, attr)) !=  0 ) /* this updates windows as well */
		{
			/*
			 * Earlier versions of this routine had a parameter 'function'
			 * that propagated from the actual file operation, and it could be
			 * "move", "copy", etc. but this dialog always handles renames. Therefore,
			 * force error to move/rename error
			 */

			result = copy_error(error, name, CMD_MOVE);
		}
		free(new);
	}

	return result;
}


static int exist
(
	const char *sname,		/* source item name */
	unsigned int smode,		/* source item type (S_IFREG, S_IFDIR, etc.) */
	const char *dname,		/* destination name */
	unsigned int *dmode,	/* destination item type */
	XATTR *dxattr, 			/* attributes of item at destination */
	int function			/* what operation is being attempted */
)
{
	XATTR
		attr;

	int
		error,
		attmode;


	attmode = ( options.cprefs & CF_FOLL ) ? 0 : 1;

	if ((error = (int)x_attr(attmode, FS_INQ, dname, &attr)) == 0)
	{
		*dmode = attr.mode & S_IFMT;

		*dxattr = attr; /* return existing destination attributes */

		/*
		 * If silent overwrite flag is set,
		 * and names differ and item is of the same type,
		 * then return XOVERWRITE
		 * note: smode and *dmode have been masked with S_IFMT
		 */

		if (overwrite && strcmp(sname, dname) && (*dmode == smode))
			return XOVERWRITE;
		else
			return XEXIST;
	}
	else
		return (error == EFILNF) ? 0 : copy_error(error, fn_get_name(dname), function);
}


/*
 * Aux. routine which serves to reduce program size somewhat
 */

int set_posmode(int mode)
{
	return (set_oldpos) ? xd_setposmode(mode) : mode;
}


/*
 * Tidying-up after the name-conflict dialog
 */

static void redraw_after(void)
{
	if (cfdial_open)
		xd_drawdeep(&cfdial, ROOT);
}


#if _MINT_

/*
 * Truncate a long filename to 8+3 pattern if permitted.
 * 'path' should contan only the destination path, 'name' only the name
 * this function modifies 'name' so pointer 'name' must not be NULL,
 * and must be accessible for writing
 */

int truncate (const char *path, char *name)
{
	int
		result = x_checkname(path, name);


	if(result == EFNTL  && ((options.cprefs & CF_TRUNN) != 0) )
	{
		char
			*p1,		/* position of the firs '.' */
			*p2,		/* position of the last '.' */
			buf[14];	/* temporary storage for name */

		/* name is too long, truncate it */

		p1 = strchr(name, '.');
		p2 = strrchr(name, '.');

		if(p1)
			*p1 = 0;

		strsncpy(buf, name,  9);	/* at most 8 characters + \0 */

		if(p2)
		{
			*p2 = '.';	/* in case p2 == p1 and was set to 0 above */
			strncat(buf, p2, 4);
		}

		/* test if the new name would be ok */

		if((result = x_checkname(path, buf)) == 0)
			strcpy(name, buf);	/* copy back to 'name' */
	}

	return result;
}

#endif


/*
 * Handle the name conflict dialog: Open, edit (in a loop), close.
 * This dialog also handles whatever is needed for the "update" and "restore"
 * copying (i.e. copying newer or older files only).
 */

static int hndl_nameconflict
(
	char **dname,		/* name of the destination */
	XATTR *attr, 		/* Attributes of the source */
	const char *sname, 	/* name of the source */
	int function		/* function to perform */
)
{
	XDINFO
		xdinfo;			/* dialog parameters */

	VLNAME
		dupl;			/* name of the duplicate item */

	char
		*dnameonly; 	/* name of the destination item */

	XATTR
		dxattr;			/* attributes of the item at destination */

	unsigned int
		sd, st,			/* source date and time */
		dd, dt;			/* destination date and time */

	int
		button, 		/* id of the button clicked in the dialog */
		result, 		/* result return code */
		oldmode;		/* dialog position code */

	unsigned int
		smode,			/* source item attributes and access modes */
		dmode;			/* same for destination */

	boolean
		again,			/* exit the outermost loop if false */
		stop, 			/* exit the inner loop if true */
		first = TRUE;	/* first time in the loop */


	smode = (attr->mode) & S_IFMT;
	result = 0;

	/* Does destination already exist ? */

	result = exist(sname, smode, *dname, &dmode, &dxattr, function);

	/*
	 * If copying is in update mode (newer files only) check dates/times
	 * and skip the file if needed.
	 * If copying in restore mode, treate dates/times the opposite way.
	 *
	 * Currently this does not apply to folders, because they are
	 * not in fact copied, but created anew, and most often their
	 * date/time can not be controlled.
	 */

	if (updatemode)
	{
		sd = attr->mdate;
		st = attr->mtime;
		dd = dxattr.mdate;
		dt = dxattr.mtime;
	}

	if (restoremode)
	{
		sd = dxattr.mdate;
		st = dxattr.mtime;
		dd = attr->mdate;
		dt = attr->mtime;
	}

	if ( (smode != S_IFDIR) && (updatemode || restoremode) )
	{
		if ( (sd < dd) || (( sd == dd ) && ( st <= dt )) )
			result = XSKIP;
	}

	if ( result != XEXIST )
		return result;

	/* Set dialog title */

	rsc_title(nameconflict, RNMTITLE, DTNMECNF);

	do
	{
		again = FALSE;

		/* Obtain the pointer to name-only part of destination */

		dnameonly = fn_get_name(*dname);

		/*
		 * In TOS < 1.4 it is not possible to rename a folder,
		 * so then set the newname field to noneditable.
		 * Note: it is assumed that if mint is present, folders
		 * CAN always be renamed. Is this true?
		 * All this applies to existing (old) file or folder;
		 * new one can always be renamed, since it doesn't exist yet.
		 */

		if ((strcmp(sname, *dname) == 0) || ((dmode == S_IFDIR) &&
#if _MINT_
			!((tos_version >= 0x104) || mint)
#else
			tos_version < 0x104
#endif
		))
			nameconflict[OLDNAME].ob_flags &= ~EDITABLE;
		else
			nameconflict[OLDNAME].ob_flags |= EDITABLE;

		/* Put old name into dialog field */

		cv_fntoform(nameconflict, OLDNAME, fn_get_name(*dname));

		/* Put old name into dialog field as the new name also */

		cv_fntoform(nameconflict, NEWNAME, dnameonly);

		dir_briefline(nameconflict[NCCINFO].ob_spec.tedinfo->te_ptext, attr);

		strcpy(dupl, oldname); /* copy from the dialog field */

		stop = FALSE;

		do
		{
			result = 0;

			arrow_mouse();

			dir_briefline(nameconflict[NCNINFO].ob_spec.tedinfo->te_ptext, &dxattr);

			/* Open the dialog only the first time in the loop */

			if (first)
			{
				oldmode = set_posmode(XD_CURRPOS);
				first = FALSE;

				xd_open(nameconflict, &xdinfo);
				set_posmode(oldmode);
			}
			else
				xd_drawdeep(&xdinfo, ROOT);

			/* Wait for a button, then immediately set it back to normal */

			button = xd_form_do_draw(&xdinfo);
			hourglass_mouse();

			if (button == NCOK)
			{
				if ((*newname == 0) || (*oldname == 0))
				{
					/* Some name(s) must be entered! */
					alert_iprint(MFNEMPTY);
				}
				else
				{
					if (strcmp(dupl, oldname))
					{
						/*
						 * old name has been changed; try to rename existing file;
						 * if a file with the name same as the changed name exists,,
						 * an error will be generated.
						 */

						if ((result = _rename(*dname, attr)) != XERROR)
						{
							stop = TRUE;


							if (result == 0)
								again = TRUE;
						}
					}
					else
						stop = TRUE;
				}
			}
			else
				stop = TRUE;
		}
		while (!stop);

		if (result == 0)
		{
			if ((button == NCOK) || (button == NCALL))
			{
				if ((button == NCOK) && strcmp(dupl, newname))
				{
					char *new;
					VLNAME name; /* Can this be a LNAME ? */

					cv_formtofn(name, nameconflict, NEWNAME);

					if ((new = fn_make_newname(*dname, name)) == NULL)
					{
						result = copy_error(ENSMEM, fn_get_name(*dname), function);
						again = FALSE;
					}
					else
					{
						free(*dname);
						*dname = new;
						again = TRUE;
					}
				}

				if (button == NCALL)
					overwrite = TRUE;

				if (result == 0)
				{
					/*
					 * beware: the earlier versions allowed only copying of
					 * files over files, or links over links, ec. links could
					 * not be overwritten by files.
					 * overwritting of files with links and vv. has now
					 * been permitted
					 */

					if
					(
						(smode != dmode)
#if _MINT_
						&& !(smode == S_IFREG && dmode == S_IFLNK)
						&& !(smode == S_IFLNK && dmode == S_IFREG)
#endif
					)
						again = TRUE;


					if (strcmp(sname, *dname) == 0)
					{
						alert_iprint(MCOPYSLF);
						again = TRUE;
					}

					result = XOVERWRITE;
				}
			}
			else
				result = (button == NCABORT) ? XABORT : XSKIP;
		}
	}
	while
	(
		again &&
		((result = exist(sname, smode, *dname, &dmode, &dxattr, function)) == XEXIST)
	);

	/* Close the dialog and tidy-up */

	xd_close(&xdinfo);
	set_oldpos = TRUE;
	redraw_after();

	return result;
}


/*
 * Handle the dialog for name conflict in case of file rename
 */

static int hndl_rename(char *name, XATTR *attr)
{
	int
		oldmode,
		button;


	/* Write filenames to dialog fields */

	dir_briefline(nameconflict[NCNINFO].ob_spec.tedinfo->te_ptext, attr);
	dir_briefline(nameconflict[NCCINFO].ob_spec.tedinfo->te_ptext, attr);

	cv_fntoform(nameconflict, OLDNAME, name);
	cv_fntoform(nameconflict, NEWNAME, name);

	/* Set dialog title */

	rsc_title(nameconflict, RNMTITLE, DTRENAME);

	/* Set editable and enabled fields. Only one field is editable */

	nameconflict[OLDNAME].ob_flags &= ~EDITABLE;
	obj_disable(nameconflict[NCALL]);

	arrow_mouse();

	/*
	 * This is needed in order to set multiple openings
	 * of the dialog (when there is more than one file to rename)
	 * in the same place on the screen
	 */

	oldmode = set_posmode(XD_CURRPOS);

	/* Now do the dialog stuff */

	button = chk_xd_dialog(nameconflict, NEWNAME);

	set_posmode(oldmode);

	set_oldpos = TRUE;
	redraw_after();

	hourglass_mouse();

	obj_enable(nameconflict[NCALL]);

	if (button == NCOK)
	{
		cv_formtofn(name, nameconflict, NEWNAME);
		return 0;
	}
	else
	{
		if (button == NCABORT || button < 0)
			return XABORT;
		else
			return XSKIP;
	}
}


/*
 * Check attributes for permission to access an item.
 * Report an error if access is not permited.
 * Currently, only the FA_READONLY attribute is checked. Hopefully,
 * in other filesystems this attribute will be correctly set by
 * TeraDesk by analyzing access rights.
 */

static int chk_access(XATTR *attr)
{
	if((attr->attr & FA_READONLY) != 0)
		return EACCDN;

	return 0;
}


/*
 * To be used to "touch" files (and maybe folders?):
 * Note: currently can not be used on folders or links
 * EXCEPT if a link is to be "touched" with the current date/time
 * in which case a new link is effectively made.
 * Note2: Attribute FA_READONLY is artificially set in TeraDesk's
 * structures for readonly files even on not-FAT filesystems
 * If attr is NULL, do not set attributes
 */

int touch_file
(
	const char *fullname,	/* name of the object (i.e. file) */
	DOSTIME *time, 			/* time & date to set */
	XATTR *attr,			/* attributes to set */
	boolean link			/* true if this is a link */
)
{
	int
		wp = 0,
		error = 0;

	/*
	 * If file is to be 'unprotected' it has to be done before
	 * changing the timestamp
	 */

	if( attr && ((wp = attr->attr & FA_READONLY) == 0) )
		error = x_fattrib(fullname, attr);

	if( (error >= 0) && (time != NULL) )
	{
#if _MINT_

		if ( link )
		{
			/* Timestamps of links can only be set to current time */

			if (time->time == now.time && time->date == now.date)
			{
				char *linktgt;

				/*
				 * Make a new link identical to the old
				 * note: if there is insufficient memory, it may
				 * happen that the old link be deleted and the new one
				 * can not be created
				 */

				if ( (linktgt = x_fllink((char *)fullname)) == NULL )
					return ENSMEM;

				error = x_unlink(fullname);

				if ( error >= 0 )
					x_mklink(fullname, linktgt);

				free(linktgt);
			}
		}
		else
#endif
		{
			if ((attr->mode & S_IFMT) == S_IFREG)
			{
				/*
				 * Date/time can be set for files only.
				 * Note: this may set the 'archive' attribute
				 * which must then be fixed. 'error' is a
				 * positive filehandle here
				 */
				if ( (error = x_open(fullname, 0)) >= 0 )
				{
					x_datime( time, error, 1 );
					x_close( error );
				}
			}
		}
	}

	/*
	 * If all is well so far, change attribues if needed. If timestamp
	 * has been changed, 'archived' attribute may have to be reset.
	 */

	if(error >= 0)
	{
		error = 0;

		if( attr && ( wp || ((attr->attr & FA_ARCHIVE) == 0) ))
			error = x_fattrib(fullname, attr);
	}

	return error;
}


/*
 * Touch, copy or move a file or a link
 */

static int copy_file
(
	const char *sname,	/* name of the source item */
	const char *dpath, 	/* destination path */
	XATTR *attr, 		/* source (extended) attributes */
	int function,		/* function to perform */
	int prev,
	boolean chk,		/* check for nameconflict */
#if _MINT_
	boolean link,		/* true if item is a link */
#endif
	LSUM *rbytes
)
{
	char
		*dname; 		/* name of the file at destination */

	VLNAME
		name;

	int
		oldattr,
		error,
		result;

	XATTR
		*theattr;

	DOSTIME
		*thetime,
		time;


	strcpy(name, fn_get_name(sname));

	/* Obtain object's attributes */

	x_attr(1, FS_INQ, sname, attr);

	/* Check for name change (open dialog) */

	if (rename_files && ((result = hndl_rename(name, attr)) != 0))
		return (result == XSKIP) ? 0 : result;

	/*
	 * Is the new name perhaps too long ?
	 * Note: this routine inquires the filesystem on destination
	 */

#if _MINT_
	if ((error = truncate(dpath, name)) == 0 )
#else
	if ((error = x_checkname(dpath, name)) == 0 )
#endif
	{
		/* Form full path+name for the destination (allocate dname here) */

		if ((dname = x_makepath(dpath, name, &error)) != NULL)
		{
			/*
			 * Keep, or set new, file date/time, attributes and rights;
			 * if CF_CTIME option is enabled, or this is a "touch file",
			 * each file gets the same new date/time- the one which was current
			 * when operation was started, or set in the show-info dialog
			 */

			oldattr = attr->attr;	/* keep original */

			if ( (options.cprefs & CF_CATTR) != 0 ) /* reset atributes ? */
			{
				attr->attr = opattr;	/* as set in the fileinfo dialog */
				theattr = attr;
#if _MINT_
				attr->mode &= ~(DEFAULT_DIRMODE |  S_ISUID | S_ISGID | S_ISVTX); /* remove permissions and sticky bit */

				if(function == CMD_TOUCH)
				{
					attr->mode |= (opmode & (DEFAULT_DIRMODE |  S_ISUID | S_ISGID | S_ISVTX));
					attr->uid = opuid;
					attr->gid = opgid;
				}
				else
					attr->mode |= DEFAULT_DIRMODE;
#endif
			}
			else
				theattr = NULL;	/* do not reset attributes */

			if ((options.cprefs & CF_CTIME) != 0)	/* reset date/time ? */
			{
				time.time = optime.time;
				time.date = optime.date;
				thetime = &time;
				theattr = attr; /* attributes must be recreated after setting file time */
			}
			else	/* do not reset date/time */
			{
				time.time = attr->mtime;
				time.date = attr->mdate;
				thetime = NULL;
			}

			if ( function == CMD_TOUCH )
				result = 0; /* is this really needed */
			else
				result = (chk) ? hndl_nameconflict(&dname, attr, sname, function) : 0;

			if ((result == 0) || (result == XOVERWRITE))
			{
				if ((function == CMD_MOVE) && (sname[0] == dname[0]) /* same drive */ )
				{
					/*
					 * Move to the same drive is actually a rename.
					 * If the file is write-protected, there will be an error,
					 * and so touch_file() will not be executed
					 * (this relies on the left-to-right evaluation of the expression below)
					 */
					if((error = chk_access(attr)) == 0)
					{
						if ((error = (result == XOVERWRITE) ? x_unlink(dname) : 0) == 0)
						{
							if
							(
								((error = x_rename(sname, dname)) == 0) &&
								((options.cprefs & (CF_CATTR | CF_CTIME) ) != 0)
							)
								error = touch_file( dname, thetime, theattr, FALSE);
						}
					}
				}
				else
				{
					/* File touch, or copy, or move to another drive */

					if ( function == CMD_TOUCH )
					{
						/*
						 * Touch file. If it is write protected, allow
						 * changes only if "reset attributes" is set.
						 */

						if
						(
							((oldattr & FA_READONLY) != 0) && /* this was a readonly file, and... */
							((options.cprefs & CF_CATTR) == 0) && /* reset of attributes not specified, and */
							(
								( ( FA_ANY & (oldattr ^ attr->attr) ) != FA_READONLY) || /* either old and new attributes differ, or */
								((options.cprefs & CF_CTIME) != 0) /* or timestamp is to be changed */
							 )
						)
							error = EACCDN;
						else
							error = touch_file( sname, thetime, theattr, FALSE);
					}
					else
					{
						/* Check for a write-protected object at destination */

						XATTR dattr, oattr;
						error = 0;
						dattr.mode = 0;

						if (x_attr(1, FS_INQ,  dname, &dattr) >= 0) /* object itself, don't follow links */
							error = chk_access(&dattr);

						if (error == 0)
						{
							/* In case of a copy, attributes can be changed at will */


							/*
							 * Note: files can not be simply overwritten with links,
							 * or v.v.; the existing item has to be explicitely
							 * deleted first
							 */
#if _MINT_
							if(mint && ((dattr.mode & S_IFMT) != 0) && ((dattr.mode & S_IFMT) != (attr->mode & S_IFMT)))
								error = x_unlink(dname);

							if(error == 0)
#endif
							{
								oattr = *attr;
#if _MINT_
								if(function == CMD_COPY)
								{
									oattr.gid = currgid;
									oattr.uid = curruid;
								}

								if ( link )
									error = linkcopy(sname, dname /* , &oattr, &time */); /* two paameters currently unused in linkcopy and disabled */
								else
#endif
									error = filecopy(sname, dname, &oattr, &time, rbytes);
							}

							if ((function == CMD_MOVE) && (error == 0))
							{
								/*
						 	 	 * Move to another drive is in fact a copy;
						 	 	 * the original file has to be deleted.
							 	 * Note 1: in this way, if there is an error,
							 	 * (for example, a write-prottected file)
							 	 * the file will be copied, not moved.
							 	 * Note 2: if source object is a link
							 	 * its target will not be moved, just the link,
								 * but the destination will be a real file
							 	 * with the contents of the link target
 						 	 	 */

								error = x_unlink(sname);
							}

							/*
							 * If there is an error, destination window must
							 * be updated because, if the copied file already
							 * exists at destination, it will be deleted
							 */

							if(error != 0)
								wd_set_update(WD_UPD_COPIED, dname, NULL);
						}
					}
				}

				/*
				 * Update windows. In case of a move, both the source
				 * and the target have to be updated. In case of a copy,
				 * (or a touch) only the target is updated.
				 */

				if (error == 0)
				{
					if (function == CMD_MOVE)
						wd_set_update(WD_UPD_MOVED, sname, dname);
					else
						wd_set_update(WD_UPD_COPIED, dname, NULL);

					result = 0;
				}
				else
					result = copy_error(error, name, function);
			}
			else if (result == XSKIP)
				result = 0;

			free(dname);
		}
		else
			result = copy_error(error, name, function);
	}
	else
		result = copy_error(error, name, function);

	return ((result != 0) ? result : prev);
}


/*
 * Create a folder during copying/moving
 */

static int create_folder
(
	const char *sname,	/* source path+name */
	const char *dpath,	/* destination path */
	char **dname, 		/* name at estination */
	XATTR *attr,		/* source attributes */
	long *folders,		/* (pointer to) folders count */
	long *files,		/* (pointer to) files count */
	LSUM *bytes,		/* (pointer to) bytes count*/
	int function,		/* operation code (CMD_COPY...etc.) */
	boolean *chk		/* if true, check for nameconflict */
)
{
	int
		error = 0,
		result;

	LSUM
		nbytes;

	long
		nfiles,
		nfolders;

	VLNAME
		name;


	strcpy(name, fn_get_name(sname));

	if (rename_files && ((result = hndl_rename(name, attr)) != 0))
		return result;

	/* Is the new name perhaps too long ?  */

	if ((error = x_checkname(dpath, name)) == 0)
	{
		if ((*dname = x_makepath(dpath, name, &error)) != NULL)
		{
			/* *chk is false for CMD_TOUCH */

			result = (*chk) ? hndl_nameconflict(dname, attr, sname, function) : 0;

			if (result == 0)
			{
				if ( function != CMD_TOUCH )
				{
					if ((error = x_mkdir(*dname)) != 0)
						free(*dname);
					else
						*chk = FALSE;
				}
			}
			else
			{
				if (result == XOVERWRITE)
					result = 0;
				else
				{
					if (result == XSKIP)
					{
						if ((error = cnt_items(sname, &nfolders, &nfiles, &nbytes, FA_ANY, FALSE)) == 0)
						{
							*files -= nfiles;
							*folders -= nfolders;
							bytes->kbytes -= nbytes.kbytes;
							sub_size(bytes, nbytes.bytes);
						}
					}

					free(*dname);
				}
			}
		}
	}

	return (error < 0) ? copy_error(error, name, function) : result;
}


static int copy_path
(
	const char *spath,	/* source path */
	const char *dpath,	/* destination path */
	const char *fname,	/* current name */
	long *folders,		/* number of folders acted upon */
	long *files,		/* number of files acted upon */
	LSUM *bytes,		/* number of bytes acted upon */
	int function,		/* what to do */
	boolean chk			/* if true, check for nameconflict */
)
{
	COPYDATA
		*stack = NULL;

	int
		error,
		result;

	unsigned int
		type;

	VLNAME
		name;

	XATTR
		attr;

	boolean
#if _MINT_
		link,
#endif
		ready = FALSE,
		eod = FALSE;


	if ((error = push(&stack, spath, dpath, chk)) != 0)
		return copy_error(error, fname, function);
	do
	{
		if(!mustabort(stack->result))
		{
			if (((error = stk_readdir(stack, name, &attr, &eod)) == 0) && (eod == FALSE))
			{
				type = attr.mode & S_IFMT;
#if _MINT_
				link = (type == S_IFLNK );
#endif
				if (type == S_IFDIR)
				{
					/* This is a directory (or a link to one) */

					int tmpres;
					boolean tmpchk = stack->chk;

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						upd_copyname(stack->dpath, stack->sname, empty);

						if ((tmpres = create_folder(stack->sname, stack->dpath, &stack->dname, &attr, folders, files, bytes, function, &tmpchk)) == 0)
						{
							if ((error = push(&stack, stack->sname, stack->dname, tmpchk)) != 0)
							{
								wd_set_update(WD_UPD_COPIED, stack->dname, NULL);
								free(stack->sname);
								free(stack->dname);
								tmpres = copy_error(error, name, function);
							}
						}
						else
							free(stack->sname);
					}
					else
						tmpres = copy_error(error, name, function);

					if (tmpres != 0)
					{
						*folders -= 1;
						stack->result = (tmpres == XSKIP) ? stack->result : tmpres;
					}
				}

				if(type == S_IFREG || type == S_IFLNK )
				{
					/* This is a regular file or a link to it */

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						upd_copyname( stack->dpath, stack->spath, name);
#if _MINT_
						stack->result = copy_file(stack->sname, stack->dpath, &attr, function, stack->result, stack->chk, link, bytes);
#else
						stack->result = copy_file(stack->sname, stack->dpath, &attr, function, stack->result, stack->chk, bytes);
#endif
						free(stack->sname);
					}
					else
						stack->result = copy_error(error, name, function);
					*files -= 1;
					sub_size(bytes, attr.size);
				}
			}
			else
			{
				if (error < 0)
					stack->result = copy_error(error, fn_get_name(stack->spath), function);
			}
		}

		check_opabort(&(stack->result));

		if (mustabort(stack->result) || eod)
		{
			if ((ready = pull(&stack, &result)) == FALSE)
			{
				if ((result == 0) && (function == CMD_MOVE) && ((result = del_folder(stack->sname, CMD_MOVE, 0, &attr)) == 0))
					wd_set_update(WD_UPD_MOVED, stack->sname, stack->dname);
				else
					wd_set_update(WD_UPD_COPIED, stack->dname, NULL);

				if (result != 0)
					stack->result = result;

				free(stack->sname);
				free(stack->dname);
				*folders -= 1;

				if(!mustabort(stack->result))
					upd_copyinfo(*folders, *files, bytes);
			}
		}
		else
			upd_copyinfo(*folders, *files, bytes);
	}
	while (!ready);

	return result;
}


/*
 * Copy, move or touch items specified in the list
 * Note: parameter 'list' is locally modified
 */

static boolean copy_list
(
	WINDOW *w,			/* pointer to source window */
	int n,				/* number of selected items */
	int *list,			/* list of item ordinals */
	const char *dest,	/* destination path */
	long *folders,		/* folder count */
	long *files,		/* file count */
	LSUM *bytes,		/* total size of selected objects */
	int function		/* what to do */
)
{
	int
		i,
		error,
		result,
		tmpres;

	XATTR
		attr;

	const char
		*path,
		*name;

	char
		*cpath,
		*dpath;

	ITMTYPE
		type;

	boolean
		link,
		chk;


	result = 0;

	/* Initial destination name */

	upd_copyname(dest, NULL, NULL);

	/* For each item in the list... */

	for (i = 0; i < n; i++)
	{
		int fa = 0; /* 0=target atributes, 1=link/object attributes */

		if (*list < 0)
		{
			list++;
			continue;
		}

		if(!itm_follow(w, *list, &link, (char **)(&path), &type))
		{
#if _MINT_
			if(link)
				type = ITM_LINK;

			if(type != ITM_NETOB)
#endif
				fa = 1;
		}

		if (path == NULL)
			result = copy_error(ENSMEM, itm_name(w, *list), function);
		else
		{
			cpath = fn_get_path(path); /* allocate */
			name = fn_get_name(path);

			switch (type)
			{
#if _MINT_
				case ITM_LINK:
#endif
				case ITM_FILE:
				case ITM_PROGRAM:
				{
					if ((error = itm_attrib(w, *list, fa, &attr)) == 0)
					{
						upd_copyname( dest, cpath, name );
#if _MINT_
						result = copy_file(path, dest, &attr, function, result, TRUE, link, bytes);
#else
						result = copy_file(path, dest, &attr, function, result, TRUE, bytes);
#endif
					}
					else
						result = copy_error(error, name, function);

					*files -= 1;
					sub_size(bytes, attr.size);
					break;
				}
				case ITM_FOLDER:
				{
					if ( function == CMD_TOUCH )
						chk = FALSE;
					else
						chk = TRUE;

					if ((error = itm_attrib(w, *list, fa, &attr)) == 0)
					{
						upd_copyname( dest, path, empty);

						if ((tmpres = create_folder(path, dest, &dpath, &attr, folders, files, bytes, function, &chk)) == 0)
						{
							if (((tmpres = copy_path(path, dpath, name, folders, files, bytes, function, chk)) == 0) &&
							     (function == CMD_MOVE) &&
							     ((tmpres = del_folder(path, CMD_MOVE, 0, &attr)) == 0))
								wd_set_update(WD_UPD_MOVED, path, dpath);
							else
								wd_set_update(WD_UPD_COPIED, dpath, NULL);
							free(dpath);
						}
						else if (tmpres == XSKIP)
							tmpres = 0;
					}
					else
						tmpres = copy_error(error, name, function);

					if (tmpres != 0)
						result = tmpres;

					*folders -= 1;
					break;
				}
				case ITM_DRIVE:
				{
					upd_copyname(dest, cpath, name );
					tmpres = copy_path(path, dest, name, folders, files, bytes, function, TRUE);

					if (tmpres != 0)
						result = tmpres;
				}
			} /* switch ? */

			free(cpath);
			free(path);
		}

		upd_copyinfo(*folders, *files, bytes);
		check_opabort(&result);

		if(mustabort(result))
			break;

		list++;
	}

	return ((result == XFATAL) ? FALSE : TRUE);
}


/*
 * Copy a list of file/folder/disk items
 */

static boolean itm_copyit
(
	WINDOW *dw,		/* Destination window */
	int dobject,	/* Destination object */
	WINDOW *sw,		/* Source window */
	int n,			/* Number of items to work upon */
	int *list,		/* List of selected items' ordinals */
	int kstate		/* state of control keys */
)
{
	int
		function;		/* operation code (copy/move/delete... ) */

	const char
		*dest;			/* destination path */

	boolean
		result = FALSE;	/* TRUE if an operation is successful */


	/*
	 * Check if the operation makes sense, depending on the type
	 * of destination window (disallow copy to text window)
	 * or object (disallow copy to nonexistent drives)
	 */

	if (dobject < 0)
	{
		/* Copy to a window */

		if (xw_type(dw) == DIR_WIND)	/* copy to dir window is possible */
		{
			if ((dest = strdup(wd_path(dw))) == NULL)
				return FALSE;
		}
		else	/* but copy is not possible to other window types */
		{
			alert_iprint(AILLCOPY);
			return FALSE;
		}
	}
	else
	{
		/* Copy to an object */

		if ((dest = itm_fullname(dw, dobject)) == NULL)
			return FALSE;

		if ((itm_type(dw, dobject) == ITM_DRIVE) && (check_drive((int)( (dest[0] & 0x5F) - 'A') ) == FALSE))
		{
			/* drive does not exist */
			free(dest);
			return FALSE;
		}
	}

	/*
	 * Detect which function and options are required,
	 * depending on the state of the control keys pressed:
	 * Control: move instead of copy
	 * Alternate: rename
	 * Left Shift: update mode (newer or nonexistent files only)
	 * Right Shift: restore mode (older or nonexistent files only)
	 */

	function = (kstate & K_CTRL) ? CMD_MOVE : CMD_COPY;
	rename_files = (kstate & K_ALT) ? TRUE : FALSE;
	overwrite = (options.cprefs & CF_OVERW) ? FALSE : TRUE;
	updatemode = ( (kstate & K_LSHIFT) != 0 ) ? TRUE : FALSE;
	restoremode = ( !updatemode && ((kstate & K_RSHIFT) != 0) ) ? TRUE : FALSE;

	/* Now handle the actual operation; return status */

    result = itmlist_op(sw, n, list, dest, function);
	free(dest);

	return result;
}


/*
 * Delete a single file or a link.
 * This routine is used in del_path() and del_list().
 * Note: attributes of 'name' are first checked for write-protection.
 * Object specified by 'name' is deleted; in case of a link, it is NOT
 * followed to the referenced object.
 */

static int del_file(const char *name, int prev_result, XATTR *attr)
{
	int
		error = chk_access(attr); /* Is it write-protected? */


	/* Attempt to delete the specified object. It can be a file or a link */

	if (!error && (error = x_unlink(name)) == 0)
		wd_set_update(WD_UPD_DELETED, name, NULL);

	return ((error != 0) ? copy_error(error, fn_get_name(name), CMD_DELETE) : prev_result);
}


/*
 * Delete a single folder
 */

static int del_folder(const char *name, int function, int prev_result, XATTR *attr)
{
	int error = chk_access(attr);

	if (!error && (error = x_rmdir(name)) == 0)
	{
		if (function == CMD_DELETE)
			wd_set_update(WD_UPD_DELETED, name, NULL);
	}

	return ((error != 0) ? copy_error(error, fn_get_name(name), function) : prev_result);
}


/*
 * Delete everything in the specified path
 */

static int del_path
(
	const char *path,
	const char *fname,
	long *folders,
	long *files,
	LSUM *bytes
)
{
	COPYDATA
		*stack = NULL;

	VLNAME
		name;

	XATTR
		attr;

	int
		error,
		result;

	unsigned int
		type;

	boolean
		ready = FALSE,
		eod = FALSE;


	if ((error = push(&stack, path, NULL, FALSE)) != 0)
		return copy_error(error, fname, CMD_DELETE);

	do
	{
		if(!mustabort(stack->result))
		{
			if (((error = stk_readdir(stack, name, &attr, &eod)) == 0) && (eod == FALSE))
			{
				type = attr.mode & S_IFMT;

				if (type == S_IFDIR)
				{
					/* Item is a directory */

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
						if ((error = push(&stack, stack->sname, NULL, FALSE)) != 0)
							free(stack->sname);
					if (error != 0)
					{
						*folders -= 1;
						stack->result = copy_error(error, name, CMD_DELETE);
						upd_copyname( NULL, stack->spath, empty);
					}
				}
#if _MINT_
				else if (type == S_IFREG || type == S_IFLNK)
#else
				else if (type == S_IFREG)
#endif
				{
					/* Item is a file or a link */

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						upd_copyname(NULL, stack->spath, name);
						stack->result = del_file(stack->sname, stack->result, &attr);
						free(stack->sname);
					}
					else
						stack->result = copy_error(error, name, CMD_DELETE);

					*files -= 1;
					sub_size(bytes, attr.size);
				}
			}
			else
			{
				if (error < 0)
					stack->result = copy_error(error, fn_get_name(stack->spath), CMD_DELETE);
			}
		}

		check_opabort(&(stack->result));

		if(mustabort(stack->result) || eod)
		{
			if ((ready = pull(&stack, &result)) == FALSE)
			{
				upd_copyname(NULL, stack->sname, empty);

				stack->result = (result == 0) ? del_folder(stack->sname, CMD_DELETE, stack->result, &attr) : result;
				*folders -= 1;
				free(stack->sname);

				if(!mustabort(stack->result))
				{
					upd_copyname(NULL, stack->spath, empty);
					upd_copyinfo(*folders, *files, bytes);
				}
			}
		}
		else
			upd_copyinfo(*folders, *files, bytes);
	}
	while (!ready);

	return result;
}


/*
 * Delete everything in a list of selected items.
 * Note: parameter 'list' is locally modified
 */

static boolean del_list
(
	WINDOW *w,
	int n,
	int *list,
	long *folders,
	long *files,
	LSUM *bytes
)
{
	const char
		*cpath,
		*path,
#if _MINT_
		*lpath = NULL,
#endif
		*name;

	XATTR
		attr;

	int
		i,
		error = 0,
		result = 0,
		tmpres,
		fa;

	ITMTYPE
		type;

	boolean
#if _MINT_
		llink,
#endif
		link;


	/* Loop for each item in the list... */

	for (i = 0; i < n; i++)
	{
		if (*list < 0)
		{
			list++;
			continue;
		}

#if _MINT_
		llink = itm_islink(w, *list);
#endif
		if(itm_follow(w, *list, &link, (char **)(&path), &type))
			fa = 0;
		else
		{
			fa = 1;
			if(link)
				type = ITM_LINK;
		}

		if (path == NULL)
			result = copy_error(ENSMEM, itm_name(w, *list), CMD_DELETE);
		else
		{
			name = fn_get_name(path);
			cpath = NULL;

#if _MINT_

			/*
			 * If the object is not a link or if a link has been followed
			 * to the referenced object, delete (referenced) object...
			 */

			if(!link && type != ITM_NETOB)
#endif
			{
				if(isfileprog(type) || type == ITM_LINK)
				{
					/* delete a file or a link (link referenced by link) */

					cpath = fn_get_path(path);
					name = fn_get_name(path);

					upd_copyname(NULL, cpath, name);

					if ((error = itm_attrib(w, *list, fa, &attr)) == 0)
					{
						/* Delete a single file or a link */

						result = del_file(path, result, &attr );
					}
#if _MINT_
					if(!llink)
#endif
					{
						*files -= 1;
						sub_size(bytes, attr.size);
					}

					free(cpath);
				}
				else
				{
					/* delete complete path */

					name = fn_get_name(path);
					upd_copyname(NULL, path, empty);

					tmpres = del_path(path, name, folders, files, bytes);
					if (type == ITM_FOLDER)
					{
						if ((error = itm_attrib(w, *list, fa, &attr)) == 0)
						{
							result = (tmpres == 0) ? del_folder(path, CMD_DELETE, result, &attr) : tmpres;
						}

						*folders -= 1;
					}
				}

				if(error)
					result = copy_error(error, name, CMD_DELETE);
			}

			free(path);

#if _MINT_
			/*
			 * If this is a link, always delete it.It can not be
			 * done before deleting the item itself, because
			 * object attributes would not be valid
			 */

			if(llink)
			{
				lpath = itm_fullname(w, *list);
				cpath = fn_get_path(lpath);
				name = fn_get_name(lpath);

				upd_copyname(NULL, cpath, name);

				if ((error = itm_attrib(w, *list, 1, &attr)) == 0)
				{
					/* Delete a single link */

					result = del_file(lpath, result, &attr );
				}

				if(link)
				{
					*files -= 1;
					sub_size(bytes, attr.size);
				}

				free(cpath);
				free (lpath);
			}
#endif
		}

		upd_copyinfo(*folders, *files, bytes);
		check_opabort(&result);

		if(mustabort(result))
			break;

		list++;
	}

	return ((result == XFATAL) ? FALSE : TRUE);
}


/*
 * Routine itmlist_op is used in copying/moving/touching/deleting/printing
 * files. This routine is now the only one which opens/closes the copy-info dialog.
 * Returns TRUE if successful.
 */

boolean itmlist_op
(
	WINDOW *w,				/* pointer to source window */
	int n,					/* number of items to work upon */
	int *list,				/* pointer to a list of item ordinals */
	const char *dest,		/* destination path (NULL if destination doesn't make sense) */
	int function			/* function identifier: CMD_COPY, CMD_DELETE etc. */
)
{
	long
		folders,			/* number of folders to do */
		files;				/* number of files to do   */

	LSUM
		bytes;				/* number of bytes to do   */

	int
		itm0,				/* first item in the list */
		button = COPYCAN;	/* button code */

	char
		*spath,				/* initial source path */
		anypath[6];			/* Dummy destination path when it does not matter */

	boolean
		cont,				/* true if there is some action to perform */
		result = FALSE; 	/* returned value */


	/* Save the index of the first item in the list (may become -1 later) */

	if(n)
		itm0 = list[0];

	/* The confirmation dialog is currently closed */

	close_cfdialog(-1);

	/*
	 * Currently, "Change date/time" and "Change attributes" options are
	 * active for a single operation only; so, always reset them.
	 * Same with "Follow links" and "Truncate names"
 	 */

	options.cprefs &= ~(CF_CTIME | CF_CATTR | CF_FOLL );

	/*
	 * Set checkbox buttons for these copy options, when appropriate.
	 * As these options are forced to OFF above, it is sufficient just to
	 * deslect those objects.
	 */

	obj_deselect(copyinfo[CCHTIME]);
	obj_deselect(copyinfo[CCHATTR]);

#if _MINT_
	if(mint)
	{
		curruid = Pgetuid();
		currgid = Pgetgid();
	}

	/* If state of the 'follow link' is changed, one must return here */

	recalc:;

	set_opt(copyinfo, options.cprefs, CF_FOLL,  CFOLLNK);
#endif

	/*
	 * Adjust some activities to the function before starting.
	 * Note: in a future development, free space on destination may be
	 * determined here before commencing the copying/moving.
	 */

	switch (function )
	{
		case CMD_COPY:
		case CMD_MOVE:
		{
			result = check_copy(w, n, list, dest);
			break;
		}
		case CMD_TOUCH:
		case CMD_DELETE:
		{
			result = check_copy(w, n, list, NULL);
			break;
		}
		case CMD_PRINT:
		{
			result = check_print(w, n, list);
			break;
		}
		case CMD_PRINTDIR:
		{
			result = TRUE;
			break;
		}
		default:
		{
			break;
		}
	}

	if ( result )
	{
		/* Count the items to work upon. Are there any at all? */

		cont = count_items(w, n, list, &folders, &files, &bytes, function );

		/* Is there anything to do? */

		if (cont)
		{
			ITMTYPE itype0 = itm_type(w, itm0);

			/*
			 * Remember operation date and time, in case it is needed to
			 * reset file data during copying. Also reset file attributes.
			 * In case of a "touch", these values are set manually from
			 * the dialog, so then do not set them
			 */

			if ( function != CMD_TOUCH )
			{
				now.date = Tgetdate();
				now.time = Tgettime();
				optime = now;

				opattr = FA_ARCHIVE; /* set all to just the "file changed" bit */
			}

			/* Find path of the first source (or its full name if it is a folder) */

			spath = itm_fullname(w, itm0);

			if ( spath != NULL )
			{
				/* Always provide some destination path */

				if(dest == NULL)
				{
					strsncpy(anypath, spath, 4); /* Disk drive of the source will do */
					dest = anypath;
				}

				/* Show first source file name. It is blank if starting with a folder */

				if ( isfileprog(itype0) )
				{
					cv_fntoform( copyinfo, CPFILE, itm_name(w, itm0) );
					path_to_disp(spath);
				}
				else
					*(copyinfo[CPFILE].ob_spec.tedinfo->te_ptext) = 0;

				/* Show initial source and destination paths */

				cv_fntoform ( copyinfo, CPFOLDER, spath );
				cv_fntoform ( copyinfo, CPDEST, dest );
				free(spath);

				/* Open the dialog. Wait for a button */

				button = open_cfdialog(folders, files, &bytes, function);

				/* Act depending on the button code */

#if _MINT_
				if(button == CFOLLNK)
				{
					/*
					 * If state of the  'follow link' has changed, return to the
				 	 * beginning to recalculate total number and size of items
					 */

					options.cprefs ^= CF_FOLL;
					cfdial_open = TRUE;
					goto recalc;
				}
#endif

				/* If action is acceptable, proceed... */

				if (button == COPYOK)
				{
					/* Copy/move/touch/delete a list of files */

					hourglass_mouse();

					if(n == 1 && folders < 1)
					{
						hideskip(n, &nameconflict[NCSKIP]);
						hideskip(n, &nameconflict[NCALL]);
					}

					switch(function)
					{
						case CMD_COPY:
						case CMD_MOVE:
						case CMD_TOUCH:
						{
							get_opt( copyinfo, &options.cprefs, CF_CTIME, CCHTIME );
							get_opt( copyinfo, &options.cprefs, CF_CATTR, CCHATTR );
							result = copy_list(w, n, list, dest, &folders, &files, &bytes, function);
							break;
						}
						case CMD_DELETE:
						{
							result = del_list(w, n, list, &folders, &files, &bytes);
							break;
						}
						case CMD_PRINT:
						{
							printmode = xd_get_rbutton(copyinfo, PSETBOX) - PRTXT;
						}
						case CMD_PRINTDIR:
						{
							int tofile = 0;

							printfile = NULL;
							get_opt( copyinfo, &tofile, 1, CPRFILE );

							if (tofile)
							{
								int error = 0;
								LNAME thename = {0};
								char *printname = locate(thename, L_PRINTF);

								if (printname)
									printfile = x_fopen(printname, O_DENYRW | O_WRONLY, &error);

								free(printname);
								xform_error(error);
							}

							if(printfile || !tofile)
								result = print_list(w, n, list, &folders, &files, &bytes, function);

							break;
						}
						default:
						{
							result = FALSE;
						}
					}

					arrow_mouse();
				}				/* button ? */
			}					/* spath ? */
		}						/* cont ? */
	} 							/* result ? */

	/*
	 * Close the information/confirmation dialog if it was open;
	 * then update windows if necessary.
	 */

	close_cfdialog(button);

	if(cont)
		wd_do_update();

	restoremode = FALSE;
	updatemode = FALSE;

	return result;
}


/*
 * A shorter form of the above, to be used when there is no destination.
 * Some bytes of program size will be saved (about 30) with a small penalty
 * in speed.
 */

boolean itmlist_wop(WINDOW *w, int n, int *list, int function)
{
	return itmlist_op(w, n, list, NULL, function);
}


/*
 * Hoofdprogramma kopieer gedeelte.
 */

boolean item_copy(WINDOW *dw, int dobject, WINDOW *sw, int n, int *list, int kstate)
{
	const char
		*program;

	ITMTYPE
		type;

	int
		wtype;

	boolean
		result = FALSE;


	wtype = xw_type(dw);	/* Destination window type */

	if ((wtype == DIR_WIND) || ((wtype == DESK_WIND) && (dobject >= 0)))
	{
		/* Determine destination object type */

		if (dobject >= 0)
			type = itm_tgttype(dw, dobject);

		/* Now, what to do ? */

		if ((dobject >= 0) && (type != ITM_FOLDER) && (type != ITM_DRIVE) && (type != ITM_PREVDIR))
		{
			switch (type)
			{
				case ITM_TRASH:
				case ITM_PRINTER:
				{
					return itmlist_wop(sw, n, list, (type == ITM_TRASH) ? CMD_DELETE : CMD_PRINT);
				}
				case ITM_PROGRAM:
				{
					if ((program = itm_fullname(dw, dobject)) != NULL)
					{
						if((kstate & K_ALT) == 0)
							onfile = TRUE;

						result = app_exec(program, NULL, sw, list, n, kstate);
						onfile = FALSE;
						free(program);
					}

					return result;
				}
				default:
				{
					break;
				}
			} /* switch */
		}
		else
			return itm_copyit(dw, dobject, sw, n, list, kstate);
	}

	alert_printf(1, AILLCOPY);
	return FALSE;
}


