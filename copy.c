/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
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
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>
#include <tos.h>
#include <mint.h>
#include <xscncode.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "events.h"
#include "printer.h"
#include "resource.h"
#include "xfilesys.h"
#include "copy.h"
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

typedef struct copydata
{
	int result;
	XDIR *dir;
	char *spath;
	char *dpath;
	char *sname;
	char *dname;
	boolean chk;
	struct copydata *prev;
} COPYDATA;

boolean
	rename_files = FALSE; 

static boolean 
	set_oldpos, 
	overwrite, 
	updatemode = FALSE,
	restoremode = FALSE;

boolean cfdial_open;

#if _SHOWFIND
extern int search_nsm;
#endif	

DOSTIME 
	now,
	optime;
	
int opattr;
unsigned int Tgettime(void);
unsigned int Tgetdate(void);

static XDINFO cfdial;
extern int tos_version;

static int del_folder(const char *name, int function, int prev);


/********************************************************************
 *																	*
 * Routines voor de dialoogbox.										*
 *																	*
 ********************************************************************/

/*
 * Open the dialog showing information during copying, moving, printing... 
 */
 
int open_cfdialog(long folders, long files, long bytes, int function) 
{
	int 
		mask,
		button, 
		title;

	cfdial_open = FALSE;

	/* (Almost) always display the dialog */

	switch (function)
	{
		case CMD_COPY:
			mask = CF_COPY;		
			title = (rename_files == TRUE) ? DTCOPYRN : DTCOPY;
			break;
		case CMD_MOVE:
			mask = CF_COPY;	
			title = (rename_files == TRUE) ? DTMOVERN : DTMOVE;
			break;
		case CMD_DELETE:
			mask = CF_DEL;
			title = DTDELETE;
			break;
		case CMD_PRINT:
			mask = CF_PRINT;
			title = DTPRINT;
			break;
		case CMD_PRINTDIR: 
			mask = CF_PRINT;
			title = DTPRINTD;
			break;		
		case CMD_TOUCH:	
			mask = CF_TOUCH;
			title = DTTOUCH;
			break;
	}

	/* In update or restore mode, override dialog title assignment */

	if ( updatemode )
		title = DTUPDAT;
	if ( restoremode )
		title = DTRESTO;

	/*
	 *  Write numbers of folders, files and bytes to dialog fields;
	 * note: as the open flag is still not set, nothing will be drawn
	 */

	upd_copyinfo(folders, files, bytes);

	/* Set dialog title */

	rsc_title(copyinfo, CPTITLE, title);

	/* Open (or redraw) the dialog */

	if ( !cfdial_open && (options.cprefs & ( mask | CF_SHOWD )) ) 
	{
		xd_open(copyinfo, &cfdial);
		cfdial_open = TRUE;
	}

	/* 
	 * If option is set to confirm action, wait for the correct button;
	 * If confirm is not needed, just redraw the dialog
	 */

	button = COPYOK;

	if ( cfdial_open )
	{
		if ((options.cprefs & mask) != 0)	/* If confirm, call xd_form_do */
			button = xd_form_do(&cfdial, 0);
		else
		{
			/* This must exist, or the dialog is not drawn at the beginning */
			xd_draw ( &cfdial, ROOT, MAX_DEPTH );
		}
	}

	set_oldpos = FALSE;

	return button;
}


/*
 * Close the informational dialog for copying/moving/printing
 */

void close_cfdialog(int button)
{
	if (cfdial_open == TRUE)
	{
		xd_change(&cfdial, button, NORMAL, 0);

		xd_close(&cfdial);
		cfdial_open = FALSE;
	}
}


/*
 * Update displayed information on number of files/folders 
 * currently being copied/moved/printed/deleted/touched.
 * These fields are updated while the dialog is open;
 * in the resource they should be set to background, opaque, 
 * white, no pattern, in order to inherit gray dialog background
 * colour, but Magic does not seem to respect this. Therefore they 
 * are drawn transparent in an opaque background box. This may cause
 * some flicker on a slower machine.
 */

void upd_copyinfo(long folders, long files, long bytes)	
{
	rsc_ltoftext(copyinfo, NFOLDERS, folders);
	rsc_ltoftext(copyinfo, NFILES, files);
	rsc_ltoftext(copyinfo, NBYTES, bytes);

	if (cfdial_open == TRUE)
		xd_draw(&cfdial, CINFBOX, 1);
}


/*
 * Display a more complete information for file copy.
 * Note1: see upd_copyinfo for proper setting of these fields in the resource. 
 * Note2: because of background colour problm in Magic, all fields are
 * always drawn in a background box.
 */

void upd_copyname( const char *dest, const char *folder, const char *file )
{
	if ( cfdial_open == TRUE )
	{

		/* Note: this can't be done selectively, because of transparency problems */

		if ( folder != NULL )
			cv_fntoform(copyinfo + CPFOLDER, folder);
		if ( file != NULL )
			cv_fntoform(copyinfo + CPFILE, fn_get_name(file) ); 
		if ( dest != NULL )
			cv_fntoform(copyinfo + CPDEST, dest);
		
		xd_draw(&cfdial, CNAMBOX, 1); 
	}
}


/********************************************************************
 *																	*
 * Routines die de stack vervangen.									*
 *																	*
 ********************************************************************/

/*
 * Push an item onto the stack
 */

static int push(COPYDATA **stack, const char *spath, const char *dpath,
				boolean chk)
{
	COPYDATA *new;
	int error = 0;

	if ((new = malloc(sizeof(COPYDATA))) == NULL)
		error = ENSMEM;
	else
	{
		new->spath = (char *) spath;
		new->dpath = (char *) dpath;
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
	COPYDATA *top = *stack;

	x_closedir(top->dir);
	*result = top->result;
	*stack = top->prev;
	free(top);

	return (*stack == NULL) ? TRUE : FALSE;
}


/* 
 * Read directory entry on the stack. 
 * Beware "*name" should be at least 256 characters long
 */

static int stk_readdir(COPYDATA *stack, char *name, XATTR *attr, boolean *eod)
{
	int error;
	char *fname;

	/*
	 * DjV 070 270703 changed name to &fname. Actually, for a DOSfs,
	 * fname now points to a string which is also pointed from stack->dir;
	 * for other FSes it points to a static space defined in x_xreaddir.
	 */

	while (((error = (int) x_xreaddir(stack->dir, &fname, 256, attr)) == 0) 
		   && ((strcmp("..", fname) == 0) || (strcmp(".", fname) == 0)));

	strsncpy ( name, fname, 256 );

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
 * Routine voor het tellen van het aantal files en folders in een directory.						                                *
 * Also used to recursively search for a file/folder
 */

int cnt_items(const char *path, long *folders, long *files, long *bytes, int attribs, boolean search)
{
	COPYDATA *stack = NULL;
	boolean ready = FALSE, eod = FALSE;
	int error, dummy;
	VLNAME name;   					/* Can this be LNAME ? */
	XATTR attr;

	int result = XSKIP;
	unsigned int type = 0;	/* item type */
	char *fpath;			/* item path */
	bool found;				/* match found */

	*folders = 0;			/* folder count */
	*files = 0;				/* files count  */
	*bytes = 0;				/* bytes count  */
	fpath = NULL;
	found = FALSE;

	if ((error = push(&stack, path, NULL, FALSE)) != 0)
		return error;

	do
	{
		if ( search )
			graf_mouse(HOURGLASS, NULL);

		if (error == 0)
		{
			if (((error = stk_readdir(stack, name, &attr, &eod)) == 0) && (eod == FALSE))
			{
				type = attr.mode & S_IFMT;

				if ( search )
				{
					if ( (found = searched_found( stack->spath, name, &attr)) == TRUE )
						fpath = x_makepath(stack->spath, name, &error);

				}
				else
					found = FALSE;

				if (type == S_IFDIR)
				{
					/* This item is a directory */

					if ( (attribs & FA_SUBDIR) != 0 )
						*folders += 1;
					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						if ((error = push(&stack, stack->sname, NULL, FALSE)) != 0)
							free(stack->sname);
						else /* BEWARE of recursion below; folder_info contains cnt_items  */
							if ( search && found && ( (attribs & FA_SUBDIR) != 0 ) )

								/* Beware of recursion below; folder_info contains cnt_items  */

								if ( (result = object_info( ITM_FOLDER, fpath, name, &attr )) != 0)
									free(fpath);
					}
				}

				if (type == S_IFREG || type == S_IFLNK)
				{
					/* This item is a file */

					/*
					 * below: show hidden or file is not hidden, or
					 * show system or file is not system
					 */

					if (   (   (attribs   & FA_HIDDEN) != 0
					        || (attr.attr & FA_HIDDEN) == 0
					       )
					    && (   (attribs   & FA_SYSTEM) != 0
						    || (attr.attr & FA_SYSTEM) == 0
						   )
					   )
					{
						*files += 1;
						*bytes += attr.size;

						if ( search && found )	
							if ( (result = object_info(ITM_FILE, fpath, name, &attr ) ) != 0 )
/* this was most probably an error; should be fpath!
								free(path);
*/
								free(fpath);
					}
				}
			}
		}

		if ((eod == TRUE) || (error != 0))
		{
			if ((ready = pull(&stack, &dummy)) == FALSE)
				free(stack->sname);
		}

		/* 
		 * Result will be 0 only if selected OK in dialogs 
		 */ 

		if ( search && (result != XSKIP) )
		{
			closeinfo();
			if ( fpath != NULL && result == 0 )
			{
				/* Trim window path */

				path_to_disp ( fpath );

				menu_ienable(menu, MSEARCH, 0);  
				wd_deselect_all(); 				 
				dir_add_window ( fpath, NULL, name  ); 

#if _SHOWFIND
				if ( search_nsm > 0 )		/* Open a text window to show searched-for string */
					txt_add_window (xw_top(), wd_selected(), 0, NULL);
#endif
			}

			return result; 
		}
	}
	while (ready == FALSE);

		if ( search && (result != XSKIP) )
		return XSKIP;
	else
		return error;
}


static int dir_error(int error, const char *file)
{
	return xhndl_error(MEREADDR, error, file);
}


/*
 * Count items specified for a copy/move operation, and find total
 * numbers of folders, files, and bytes.
 * This routine recurses into subdirecories.
 */

static boolean count_items	
(
	WINDOW *w,
	int n,
	int *list,
	long *folders,
	long *files,
	long *bytes,
	int function 
)
{
	int 
		i = 0, 
		error = 0, 
		item;

	ITMTYPE 
		type;

	const char 
		*path;

	long 
		dfolders, 
		dfiles, 
		length;

	boolean 
		ok = TRUE;

	XATTR 
		attr;

	boolean
		link;

	*folders = 0;
	*files = 0;
	*bytes = 0;

	graf_mouse(HOURGLASS, NULL);

	while ((i < n) && (ok == TRUE))
	{
		item = list[i];
		type = itm_type(w, item);
		link = itm_islink(w, item, TRUE);

		error = 0;

		if (link || (type == ITM_FILE) || (type == ITM_PROGRAM && function != CMD_PRINT))
		{
			if ((error = itm_attrib(w, item, (link) ? 1 : 0, &attr)) == 0)
			{
				*files += 1;
				*bytes += attr.size;
			}
			else
				list[i] = -1;
		}
		else
		{
			if ((path = itm_fullname(w, item)) != NULL)
			{
				if ( function != CMD_PRINT && function != CMD_PRINTDIR )
				{
					if ((error = cnt_items(path, &dfolders, &dfiles, &length, 0x37, FALSE)) == 0)
					{
						*folders += dfolders + ((type == ITM_DRIVE) ? 0 : 1);
						*files += dfiles;
						*bytes += length;
					}
					else
						list[i] = -1;
				}
				else
					if ( function == CMD_PRINTDIR )
						*folders += ((type == ITM_FOLDER) ? 1 : 0);
				free(path);
			}
			else
				ok = FALSE;
		}

		if (error != 0)
		{
			if (dir_error(error, itm_name(w, item)) != XERROR)
				ok = FALSE;
		}
		i++;
	}

	graf_mouse(ARROW, NULL);

	if ((*files == 0) && (*folders == 0))
		ok = FALSE;

	return ok;
}



/********************************************************************
 *																	*
 * Kopieer routine voor een file.									*
 *																	*
 ********************************************************************/


/*
 * Copy one file and set date/time and attributes
 */

static int filecopy(const char *sname, const char *dname, int src_attrib, DOSTIME *time)
{
	int fh1, fh2, error = 0;
	long slength, dlength, size, mbsize;
	void *buffer;

	/* Create a buffer for copying: find the largest free block and leave 8KB */

	mbsize = (long) options.bufsize * 1024L;

	if ((size = (long) Malloc(-1L) - 8192L) > mbsize)
		size = mbsize;

	/* 
	 * Note: If size < 1024L and comparison continues, 
	 * buffer will be left allocated even though the opertion failed.
	 * (In Pure-C this is not supposed to happen)
	 */

	if ((size >= 1024L) && (buffer = malloc(size)) != NULL)	
	{
		fh1 = x_open(sname, O_DENYW | O_RDONLY);
		if (fh1 < 0)
			error = fh1;
		else
		{
			slength = x_read(fh1, size, buffer);
			if (slength < 0)
				error = (int) slength;
			else
			{
				fh2 = x_create(dname, src_attrib);
				if (fh2 < 0)
					error = fh2;
				else
				{
					dlength = x_write(fh2, slength, buffer);

					if ((dlength < 0) || (slength != dlength))
						error = (dlength < 0) ? (int) dlength : EDSKFULL;

					while ((slength == size) && (error == 0))
					{
						if ((slength = x_read(fh1, size, buffer)) > 0)
						{
							dlength = x_write(fh2, slength, buffer);
							if ((dlength < 0) || (slength != dlength))
								error = (dlength < 0) ? (int) dlength : EDSKFULL;
						}
						else if (slength < 0)
							error = (int) slength;
					}

                    /* Set file date and time */

					if (error == 0)
						x_datime(time, fh2, 1);

					x_close(fh2);

					if (error != 0)
						x_unlink(dname);
				}
			}
			x_close(fh1);
		}
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

static int linkcopy(const char *sname, const char *dname, int src_attrib, DOSTIME *time)
{
	int error = 0;
	char *tgtname;

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
 
int copy_error(int error, const char *name, int function)
{
	int msg;
	SNAME shortname;


	cramped_name( name, shortname, (int)sizeof(SNAME) );

	switch(function)	
	{
		case CMD_DELETE:
			msg = MEDELETE;
			break;
		case CMD_MOVE:
			msg = MEMOVE;
			break;
		case CMD_COPY:
			msg = MECOPY;
			break;
		case CMD_TOUCH:
			msg = METOUCH;
			break;
		default:
			msg = 0;
	}

	/* a table would have been even better ! */

	return xhndl_error(msg, error, shortname);
}


/*
 * Routine voor het controleren van het kopieren.
 * Check if all items can be copied (or deleted)
 */

static boolean check_copy(WINDOW *w, int n, int *list, const char *dest)
{
	int 
		i = 0, 
		item;

	const char 
		*path, 
		*mes;

	long l;

	boolean 
		result = TRUE;

	ITMTYPE 
		type;

	/* Check if all specified items can be copied */

	while ((i < n) && (result == TRUE))
	{
		item = list[i];

		if ((((type = itm_type(w, item)) == ITM_FOLDER) || (type == ITM_DRIVE)) && (dest != NULL))
		{
			/* Note: space for path allocated here */
			if ((path = itm_fullname(w, item)) == NULL)
				result = FALSE;
			else
			{
				l = strlen(path);
				if ((strncmp(path, dest, l) == 0) &&
					(((type != ITM_DRIVE) && ((dest[l] == '\\') || (dest[l] == 0))) ||
					 ((type == ITM_DRIVE) && (dest[l] != 0))))
				{
					alert_printf(1, AILLCOPY);
					result = FALSE;
				}

				free(path);
			}
		}
		else
		{
			/* Can't copy the trashcan or the printer */

			switch (type)
			{
				case ITM_TRASH:
					rsrc_gaddr(R_STRING, MTRASHCN, &mes);
					break;
				case ITM_PRINTER:
					rsrc_gaddr(R_STRING, MPRINTER, &mes);
					break;
				default:
					mes = NULL;
					break;
			}

			if (mes != NULL)
			{
				alert_printf(1, ANOCOPY, mes);
				result = FALSE;
			}
		}
		i++;
	}
	return result;
}


/********************************************************************
 *																	*
 * Routine voor het afhandelen van een naamconflict.				*
 *																	*
 ********************************************************************/

/*
 * Just rename a file and update windows if needed
 */  

int frename(const char *oldfname, const char *newfname)
{
	int error;

	error = x_rename(oldfname, newfname);
	if (!error)
		wd_set_update(WD_UPD_MOVED, oldfname, newfname);

	return error;
}


/*
 * Take a new name from the name conflict dialog and rename "old" file
 */

static int _rename(char *old, int function)
{
	int 
		error, 
		result = 0;

	char 
		*new, 
		*name;
 
	VLNAME
		newfname; 

	/* Get new name from the dialog */

	cv_formtofn(newfname, &nameconflict[OLDNAME]);

	/* Extract old name only without path */

	name = fn_get_name(old); 

	/* Create new full name */

	if ((new = fn_make_newname(old, newfname)) == NULL)
		return XFATAL;
	else
	{
		/* If name has been successfully created, try to rename item */

		if ( (error = frename(old, new)) !=  0 ) /* this updates windows as well */
		{
/*
 "function" propagates from the actual file operation, and it can be
 "move", "copy", etc. but this dialog always handles renames. Therefore,
 force error to move/rename error
				result = copy_error(error, name, function);
*/
				result = copy_error(error, name, CMD_MOVE);

		}
		free(new);
	}
	return result;
}


static int exist(const char *sname, int smode, const char *dname,
				 int *dmode, XATTR *dxattr, int function)
{
	int error, attmode;
	XATTR attr;

	attmode = ( options.cprefs & CF_FOLL ) ? 0 : 1;

	if ((error = (int)x_attr(attmode, dname, &attr)) == 0) 
	{
		*dmode = attr.mode & S_IFMT;
	
		*dxattr = attr; /* return existing destination attributes */

		/* 
		 * If silent overwrite flag is set, 
		 * and names differ and item is of the same type,
		 * then return XOVERWRITE
		 */

		if ((overwrite == TRUE) && strcmp(sname, dname) && (*dmode == smode))
			return XOVERWRITE;
		else
			return XEXIST;
	}
	else
		return (error == EFILNF) ? 0 : copy_error(error, fn_get_name(dname), function);
}


/*
 * Handle the name conflict dialog: Open, edit (in a loop), close.
 * This dialog also handles whatever is neded for the "update" and "restore"
 * copying (i.e. copying newer or older files only).
 */ 

static int hndl_nameconflict
(
	char **dname, 
	XATTR *attr, 
	const char *sname, 
	int function
)
{
	boolean 
		again, 
		stop, 
		first = TRUE;

	int 
		sd, st,					/* source date and time */
		dd, dt,					/* destination date and time */
		smode = attr->mode,
		button, 
		result = 0, 
		oldmode, 
		dmode;

	XDINFO 
		xdinfo;

	char
		*dnameonly, 
		dupl[256];

	XATTR
		dxattr;

	smode &= S_IFMT;

	/* Does destination already exist ? */

	result = exist(sname, smode, *dname, &dmode, &dxattr, function);

	/* 
	 * If copying is in update mode (newer files only) check dates/times
	 * and skip file if needed.
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
		dd = attr->mdate;
		dt = attr->mtime;
		sd = dxattr.mdate;
		st = dxattr.mtime;
	}

	if ( (smode != S_IFDIR) && (updatemode || restoremode) )
	{
		if ( sd < dd )
			result = XSKIP;
		else
			if ( sd == dd )
				if ( st <= dt )
					result = XSKIP;
	} 

	if ( result != XEXIST )
		return result;

	/* Set dialog title */

	rsc_title(nameconflict, RNMTITLE, DTNMECNF);

	do
	{
		again = FALSE;

		/* Obtain the pointer to name only part of destination */

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

		cv_fntoform(nameconflict + OLDNAME, fn_get_name(*dname));

		/* Put old name into dialog field as well as new */

		cv_fntoform(nameconflict + NEWNAME, dnameonly);

		strcpy(dupl, oldname); /* copy from the dialog field */

		stop = FALSE;

		do
		{
			result = 0;

			graf_mouse(ARROW, NULL);

			/* Open the dialog only the first time in the loop */

			if (first)
			{
				if (set_oldpos == TRUE)
					oldmode = xd_setposmode(XD_CURRPOS); 

				xd_open(nameconflict, &xdinfo);

				if (set_oldpos == TRUE)
					xd_setposmode(oldmode);

				first = FALSE;
			}
			else
				xd_draw(&xdinfo, ROOT, MAX_DEPTH);

			button = xd_form_do(&xdinfo, NEWNAME);
			xd_change(&xdinfo, button, NORMAL, 0);

			graf_mouse(HOURGLASS, NULL);

			if (button == NCOK)
			{
				if ((*newname == 0) || (*oldname == 0))
					/* Some name(s) must be entered! */
					alert_iprint(MFNEMPTY);
				else
				{
					if (strcmp(dupl, oldname))
					{
						/* old name has been changed; try to rename existing */

						if ((result = _rename(*dname, function)) != XERROR)
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
		while (stop == FALSE);

		if (result == 0)
		{
			if ((button == NCOK) || (button == NCALL))
			{
				if ((button == NCOK) && strcmp(dupl, newname))
				{
					char *new; 
					VLNAME name; /* Can this be a LNAME ? */

					cv_formtofn(name, &nameconflict[NEWNAME]);

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
					if (smode != dmode)
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
	while ((again == TRUE) && ((result = exist(sname, smode, *dname, &dmode, &dxattr, function)) == XEXIST));

	xd_close(&xdinfo);
	set_oldpos = TRUE;

	return result;
}


/* 
 * Handle the dialog for name conflict in case of file rename 
 */

static int hndl_rename(char *name)
{
	int button,oldmode;

	/* Write filenames to dialog fields */

	cv_fntoform(nameconflict + OLDNAME, name);
	cv_fntoform(nameconflict + NEWNAME, name);

	/* Set dialog title */

	rsc_title(nameconflict, RNMTITLE, DTRENAME);

	/* Set editable and enabled fields */

	nameconflict[OLDNAME].ob_flags &= ~EDITABLE;
	nameconflict[NCALL].ob_state |= DISABLED;

	graf_mouse(ARROW, NULL);

	if (set_oldpos == TRUE)
		oldmode = xd_setposmode(XD_CURRPOS);

	button = xd_dialog(nameconflict, NEWNAME);

	if (set_oldpos == TRUE)
		xd_setposmode(oldmode);

	set_oldpos = TRUE;

	graf_mouse(HOURGLASS, NULL);

	nameconflict[NCALL].ob_state &= ~DISABLED;

	if (button == NCOK)
	{
		cv_formtofn(name, &nameconflict[NEWNAME]);
		return 0;
	}
	else
	{
		if (button == NCABORT)
			return XABORT;
		else
			return XSKIP;
	}
}


/*
 * To be used to "touch" files:
 * open to get handle, set time, close. Set attributes.
 * Use nonnegative values of error for file handle.
 * Note: currently can not be used on folders or links
 * EXCEPT if a link is to be "touched" with the current date/time
 * in which case a new link is effectively made.
 */

int touch_file
( 
	const char *fullname,	/* name of the object (i.e. file) */ 
	DOSTIME *time, 			/* time & date to set */
	int attr,				/* attributes to set */
	boolean link			/* true if this is a link */
)
{
	int error = 0;

#if _MINT_
	char *linktgt;
#endif

	if ( (error  = x_fattrib( fullname, 1, (attr & ~FA_SUBDIR) )) >= 0 )
	{

#if _MINT_
		if ( link )
		{
			if (time->time == now.time && time->date == now.date)
			{
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
			/* Date/time can be set for files only */

			if ( (error = x_open(fullname, 0)) >= 0 )
			{
				x_datime( time, error, 1 ); 
				x_close( error );
			}
		}
	}

	return (error >=0) ? 0 : error;
}


/********************************************************************
 *																	*
 * Routines voor het kopieren van files.							*
 *																	*
 ********************************************************************/

static int copy_file(const char *sname, const char *dpath, XATTR *attr, 
					 int function, int prev, boolean chk, boolean link)
{
	char 
		*dname; 
	
	VLNAME 
		name; 

	int 
		oldattr,
		error, 
		result;

	DOSTIME 
		time;

	/* 
	 * Get name only of the source. If the source s a link, and is
	 * to be followed, get link target name.
	 */

#if _MINT_
	if ( link && (options.cprefs & CF_FOLL) != 0 )
	{
		char *tgtname = x_fllink((char *)sname);
		strcpy(name, fn_get_name((const char *)tgtname));
		free(tgtname);
	}
	else
#endif
		strcpy(name, fn_get_name(sname));

	/* Check for name change (open dialog) */

	if ((rename_files == TRUE) && ((result = hndl_rename(name)) != 0))
		return (result == XSKIP) ? 0 : result;

	if ((error = x_checkname(dpath, name)) == 0) 
	{
		/* Form full path+name for the destination (allocate dname here) */

		if ((dname = x_makepath(dpath, name, &error)) != NULL)
		{
			/* 
			 * Keep, or set new, file date/time; 
			 * if CF_CTIME option is enabled, or this is a "touch file",
			 * each file gets the same new date/time- the one which was current
			 * when operation was started, or set in the show-info dialog
			 */

			oldattr = attr->attr;

			if ( (options.cprefs & CF_CATTR) != 0 ) 
				attr->attr = opattr;

			if ((options.cprefs & CF_CTIME) != 0) 
			{
				time.time = optime.time;
				time.date = optime.date;
			}
			else
			{
				time.time = attr->mtime;
				time.date = attr->mdate;
			}

			if ( function == CMD_TOUCH )
				result = 0; /* is this really needed */
			else
				result = (chk) ? hndl_nameconflict(&dname, attr, sname, function) : 0;
	
			if ((result == 0) || (result == XOVERWRITE))
			{
				if ((function == CMD_MOVE) && (sname[0] == dname[0]))
				{
					/* 
					 * Move to the same drive is actually a rename.
					 * If the file is write-prottected, there will be an error,
					 * and so touch_file() will not be executed
					 * (this relies on the left-to-right evaluation of the expression below)
					 */

					if ((error = (result == XOVERWRITE) ? x_unlink(dname) : 0) == 0)
						if (   ((error = x_rename(sname, dname)) == 0)
						    && ((options.cprefs & (CF_CATTR | CF_CTIME) ) != 0) 
						   )
							error = touch_file( dname, &time, attr->attr, FALSE ); 
				}
				else
				{
					/* File touch, or copy, or move to another drive */

					if ( function == CMD_TOUCH )
					{
						/* 
						 * Touch file. If it is write protected, allow only
						 * resetting of readonly attribute. Disalow change
						 * of date/time, or change of other attributes.
						 */

						if ( (oldattr & FA_READONLY) && ( ( ((oldattr ^ attr->attr) & 0x37) != FA_READONLY) || options.cprefs & CF_CTIME) )
							error = EACCDN;
						else
							error = touch_file( sname, &time, (attr->attr & ~FA_SUBDIR), FALSE );
					}
					else
					{
						/* In case of a copy, attributes can be changed at will */

#if _MINT_
						if ( link )
							error = linkcopy(sname, dname, attr->attr, &time);
						else
#endif
							error = filecopy(sname, dname, attr->attr, &time);
			
						if ((function == CMD_MOVE) && (error == 0))
						{
							/* 
						 	 * Move to another drive is in fact a copy;
						 	 * the original file has to be deleted.
							 * if there is an error, 
							 * update destination window (why?).
							 * Note: in this way, if there is an error,
							 * (for example, a write-prottected file)
							 * the file will be copied, not moved.
							 * Note: if source object is a link
							 * its target will not be moved, just the link,
							 * but the destination will be a real file
							 * with the contents of the link target
 						 	 */

							if ((error = x_unlink(sname)) != 0)
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

/* why?
	if ( function == CMD_TOUCH && dpath != NULL )
		free(dpath); /* DjV 068 230703 */
*/

	return ((result != 0) ? result : prev);
}


/* 
 * Create a folder during copying/moving 
 */

static int create_folder
(
	const char *sname,	/* source path+name */
	const char *dpath,	/* destination path */
	char **dname, 
	XATTR *attr, 
	long *folders,
	long *files, 
	long *bytes, 
	int function,		/* operation code (CMD_COPY...etc.) */
	boolean *chk
)
{
	int 
		error, 
		result;

	long 
		nfiles, 
		nfolders, 
		nbytes;

	VLNAME 
		name;


	strcpy(name, fn_get_name(sname)); 

	if ((rename_files == TRUE) && ((result = hndl_rename(name)) != 0))
		return result;

	if ((error = x_checkname(dpath, name)) == 0)
	{
		if ((*dname = x_makepath(dpath, name, &error)) != NULL)
		{
			result = (*chk) ? hndl_nameconflict(dname, attr, sname, function) : 0;

			if (result == 0)
			{
				if ( function != CMD_TOUCH )
				{
					if ((error = x_mkdir(*dname)) != 0)
					{
						free(*dname);
						result = copy_error(error, name, function);
					}
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
						if ((error = cnt_items(sname, &nfolders, &nfiles, &nbytes, 0x37, FALSE)) == 0)
						{
							*files -= nfiles;
							*folders -= nfolders;
							*bytes -= nbytes;
						}
						else
							result = copy_error(error, name, function);
					}
					free(*dname);
				}
			}
		}
		else
			result = copy_error(error, name, function);
	}
	else
		result = copy_error(error, name, function);

	return result;
}


static int copy_path(const char *spath, const char *dpath,
					 const char *fname, long *folders, long *files,
					 long *bytes, int function, boolean chk)
{
	COPYDATA *stack = NULL;
	boolean ready = FALSE, eod = FALSE;
	int error, result;
	VLNAME name;
	XATTR attr;
	unsigned int type;
	boolean link;

	if ((error = push(&stack, spath, dpath, chk)) != 0)
		return copy_error(error, fname, function);

	do
	{
		if ((stack->result != XFATAL) && (stack->result != XABORT))
		{
			if (((error = stk_readdir(stack, name, &attr, &eod)) == 0) && (eod == FALSE))
			{

				type = attr.mode & S_IFMT;
				link = (type == S_IFLNK ) ? TRUE : FALSE;

				if (type == S_IFDIR)
				{
					/* This is a directory (or a link to one) */

					int tmpres;
					boolean tmpchk = stack->chk;

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						upd_copyname(stack->dpath, stack->sname, "");

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

						stack->result = copy_file(stack->sname, stack->dpath, &attr, function, stack->result, stack->chk, link);
						free(stack->sname);
					}
					else
						stack->result = copy_error(error, name, function);
					*files -= 1;
					*bytes -= attr.size;
				}
			}
			else
			{
				if (error < 0)
					stack->result = copy_error(error, fn_get_name(stack->spath), function);
			}
		}

		if (stack->result != XFATAL)
		{
			if ( escape_abort( cfdial_open ) )
				stack->result = XABORT;
		}

		if ((stack->result == XFATAL) || (stack->result == XABORT) || (eod == TRUE))
		{
			if ((ready = pull(&stack, &result)) == FALSE)
			{
				if ((result == 0) && (function == CMD_MOVE) && ((result = del_folder(stack->sname, function, 0)) == 0))
					wd_set_update(WD_UPD_MOVED, stack->sname, stack->dname);
				else
					wd_set_update(WD_UPD_COPIED, stack->dname, NULL);

				if (result != 0)
					stack->result = result;

				free(stack->sname);
				free(stack->dname);
				*folders -= 1;

				if ((stack->result != XFATAL) && (stack->result != XABORT))
				{
					upd_copyinfo(*folders, *files, *bytes);
				}
			}
		}
		else
			upd_copyinfo(*folders, *files, *bytes);
	}
	while (ready == FALSE);

	return result;
}


static boolean copy_list
(
	WINDOW *w,
	int n,
	int *list,
	const char *dest,
	long *folders,
	long *files,
	long *bytes,
	int function
)
{
	int i, error, item, result, tmpres;	
	XATTR attr;
	const char *path, *name;
	char *cpath, *dpath;
	boolean chk;

	/* Initial destination name */

	upd_copyname(dest, NULL, NULL);

	/* For each item in the list... */

	for (i = 0; i < n; i++)
	{
		if ((item = list[i]) == -1)
			continue;

		name = itm_name(w, item);

		if ((path = itm_fullname(w, item)) == NULL) /* allocate */
			result = copy_error(ENSMEM, name, function);
		else
		{
			boolean link = itm_islink(w, item, TRUE);
			ITMTYPE type = itm_type(w, item);

			if ( link )
				type = ITM_LINK;

			cpath = fn_get_path( path ); /* allocate */

			switch (type)
			{
			case ITM_LINK:
			case ITM_FILE:
			case ITM_PROGRAM:

				if ((error = itm_attrib(w, item, 0, &attr)) == 0)
				{
					upd_copyname( dest, cpath, name );

					if ( link )
						error = itm_attrib(w, item, 1, &attr);

					result = copy_file(path, dest, &attr, function, result, TRUE, link);
					*bytes -= attr.size;
				}
				else
					result = copy_error(error, name, function);
				*files -= 1;
				break;
			case ITM_FOLDER:
				if ( function == CMD_TOUCH )	
					chk = FALSE;
				else
					chk = TRUE;

				if ((error = itm_attrib(w, item, 0, &attr)) == 0)
				{
					upd_copyname( dest, path, "" );

					if ((tmpres = create_folder(path, dest, &dpath, &attr, folders, files, bytes, function, &chk)) == 0)
					{
						if (((tmpres = copy_path(path, dpath, name, folders, files, bytes, function, chk)) == 0) && 
						     (function == CMD_MOVE) && 
						     ((tmpres = del_folder(path, function, 0)) == 0))
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
			case ITM_DRIVE:
				upd_copyname(dest, cpath, name );
				tmpres = copy_path(path, dest, name, folders, files, bytes, function, TRUE);
				if (tmpres != 0)
					result = tmpres;
				break;
			}
			free(cpath);
			free(path);
		}

		upd_copyinfo(*folders, *files, *bytes);

		if (result != XFATAL)
		{
			if ( escape_abort(cfdial_open) )
				result = XABORT;
		}

		if ((result == XABORT) || (result == XFATAL))
			break;
	}
	return ((result == XFATAL) ? FALSE : TRUE);
}


/* 
 * Copy a list of file/folder/disk items 
 */

static boolean itm_copy
(
	WINDOW *dw,		/* Destination window */ 
	int dobject,	/* Destination object */ 
	WINDOW *sw,		/* Source window */ 
	int n,			/* Number of items to work upon */
	int *list,		/* List of selected items' ordinals */ 
	int kstate		/* state of control keys */
)
{	
	boolean 
		result = FALSE;	/* TRUE if an operation is successful */

	int  
		function;		/* operation code (copy/move/delete... ) */

	const char 
		*dest;			/* destination path */


	/* 
	 * Check if operation makes sense, depending on the type
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

		if ((itm_type(dw, dobject) == ITM_DRIVE) && (check_drive((int) ( (dest[0] & 0xDF) - 'A') ) == FALSE))
		{
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

	/* Now handle the actual operation */

    itmlist_op(sw, n, list, dest, function);

	free(dest);

	return result;
}


/********************************************************************
 *																	*
 * Routines voor het wissen van files.								*
 *																	*
 ********************************************************************/


static int del_one( const char *name )
{
	int error;
	if ((error = x_unlink(name)) == 0)
		wd_set_update(WD_UPD_DELETED, name, NULL);
	return error;
}


/* 
 * Delete a single file or a link.
 * Il "follow" is true and the object is a link, both the link and the 
 * target object are deleted (link is deleted first). 
 */

static int del_file(const char *name, int prev, boolean follow)
{
	int error;
	char *tgtname;

	/* Delete the specified object. It can be a file or a link */

	error = del_one(name); /* note: there is wd_set_update() in del_one() */

	/* maybe it was a link; follow it just in case, to delete the target as well */

	if ( follow && !error )
	{
		tgtname = x_fllink((char *)name);
		error = del_one(tgtname);
		free(tgtname);
	}		

	return ((error != 0) ? copy_error(error, fn_get_name(name), CMD_DELETE) : prev);
}


/*
 * Delete a single folder 
 */

static int del_folder(const char *name, int function, int prev)
{
	int error;

	if ((error = x_rmdir(name)) == 0)
	{
		if (function == CMD_DELETE)
			wd_set_update(WD_UPD_DELETED, name, NULL);
	}

	return ((error != 0) ? copy_error(error, fn_get_name(name), function) : prev);
}


/* 
 * Delete everything in the specified path 
 */

static int del_path(const char *path, const char *fname, long *folders,
					long *files, long *bytes)
{
	COPYDATA *stack = NULL;
	boolean ready = FALSE, eod = FALSE;
	int error, result;
	char name[256];
	XATTR attr;
	unsigned int type;

	if ((error = push(&stack, path, NULL, FALSE)) != 0)
		return copy_error(error, fname, CMD_DELETE);

	do
	{
		if ((stack->result != XFATAL) && (stack->result != XABORT))
		{
			if (((error = stk_readdir(stack, name, &attr, &eod)) == 0) && (eod == FALSE))
			{
				type = attr.mode & S_IFMT;

				if (type == S_IFDIR)
				{

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
						if ((error = push(&stack, stack->sname, NULL, FALSE)) != 0)
							free(stack->sname);
					if (error != 0)
					{
						*folders -= 1;
						stack->result = copy_error(error, name, CMD_DELETE);
						upd_copyname( NULL, stack->spath, "" );
					}
				}
				if (type == S_IFREG || type == S_IFLNK)
				{

					if ((stack->sname = x_makepath(stack->spath, name, &error)) != NULL)
					{
						upd_copyname(NULL, stack->spath, name);

						stack->result = del_file(stack->sname, stack->result, (type == S_IFLNK && ( (options.cprefs & CF_FOLL) != 0 )));
						free(stack->sname);
					}
					else
						stack->result = copy_error(error, name, CMD_DELETE);
					*files -= 1;
					*bytes -= attr.size;
				}
			}
			else
			{
				if (error < 0)
					stack->result = copy_error(error, fn_get_name(stack->spath), CMD_DELETE);
			}
		}

		if (stack->result != XFATAL)
		{
			if ( escape_abort(cfdial_open) )
				stack->result = XABORT;
		}

		if ((stack->result == XFATAL) || (stack->result == XABORT) || (eod == TRUE))
		{
			if ((ready = pull(&stack, &result)) == FALSE)
			{
				upd_copyname(NULL, stack->sname, "");

				stack->result = (result == 0) ? del_folder(stack->sname, CMD_DELETE, stack->result) : result;
				*folders -= 1;
				free(stack->sname);

				if ((stack->result != XFATAL) && (stack->result != XABORT))
				{
					upd_copyname(NULL, stack->spath, "");
					upd_copyinfo(*folders, *files, *bytes);
				}
			}
		}
		else
			upd_copyinfo(*folders, *files, *bytes);
	}
	while (ready == FALSE);

	return result;
}


/* 
 * Delete everything in a list of selected items 
 */

static boolean del_list(WINDOW *w, int n, int *list, long *folders, long *files, long *bytes)
{
	int i, item, error, result;	
	ITMTYPE type;
	const char *cpath, *path, *name;
	XATTR attr;
	boolean ulink, link;

	for (i = 0; i < n; i++)
	{
		if ((item = list[i]) == -1)
			continue;

		name = itm_name(w, item);
		link = itm_islink(w, item, TRUE);
		ulink = itm_islink(w, item, FALSE);

		if ((path = itm_fullname(w, item)) == NULL)
			result = copy_error(ENSMEM, name, CMD_DELETE);
		else
		{
			type = itm_type(w, item);

			if ((type == ITM_FILE) || (type == ITM_PROGRAM) || link )
			{
				cpath = fn_get_path(path);

				upd_copyname(NULL, cpath, name);

				if ((error = itm_attrib(w, item, (link) ? 1 : 0, &attr)) == 0)
				{
					/* Delete a file or a link */

					result = del_file(path, result, (ulink && !link) );

					*bytes -= attr.size;
				}
				else
					result = copy_error(error, name, CMD_DELETE);
				*files -= 1;

				free(cpath);
			}
			else
			{
				int tmpres;
				upd_copyname(NULL, path, "");

				tmpres = del_path(path, name, folders, files, bytes);
				if (type == ITM_FOLDER)
				{
					result = (tmpres == 0) ? del_folder(path, CMD_DELETE, result) : tmpres;
					*folders -= 1;
				}
			}
			free(path);
		}

		upd_copyinfo(*folders, *files, *bytes);

		if (result != XFATAL)
		{
			if ( escape_abort(cfdial_open) )
				result = XABORT;
		}

		if ((result == XABORT) || (result == XFATAL))
			break;
	}
	return ((result == XFATAL) ? FALSE : TRUE);
}


/* 
 * Routine itm_delete has been removed and replaced by a similar but more general 
 * routine itmlist_op which is used in copying/moving/touching/deleting/printing
 * files. This routine is now the only one which opens/closes the copy-info dialog.
 * Routine returns TRUE if successful.
 */

boolean itmlist_op
(
	WINDOW *w,				/* pointer to source window */ 
	int n,					/* number of items to work upon */ 
	int *list,				/* pointer to a list of item ordinals */ 
	const char *destpath,	/* destination path (NULL if destination doesn't make sense) */ 
	int function			/* function identifier: CMD_COPY, CMD_DELETE etc. */
)
{
	long 
		folders,	/* number of folders to do */ 
		files,		/* number of files to do   */ 
		bytes;		/* number of bytes to do   */

	int 
		button;		/* button code */

	boolean 
		result = FALSE, 
		cont;		/* true if there is some action to perform */

	char
		*spath;		/* initial source path */

	const char
		*dest;		/* destination path (local) */


	/* If destination path is not given, assume something (doesn't matter) */

	if ( destpath == NULL )
	{
		/* 
		 * Destination sometimes doesn't matter anyway, so set it to anything
		 * (the program crashed when touching or deleting items selected
		 * as icons on the desktop- i.e. when there was no dir window path)
		 */
			dest = "A:\\";
	}
	else
		dest = destpath;

	/* Adjust some activities to the function before starting */

	switch (function )
	{
		case CMD_COPY:
		case CMD_MOVE:
			result = check_copy(w, n, list, dest);
			break;
		case CMD_TOUCH:
		case CMD_DELETE:
			result = check_copy(w, n, list, NULL);
			break;
		case CMD_PRINT:
			result = check_print(w, n, list);
			break;
		case CMD_PRINTDIR:
			result = TRUE;
			break;
		default:
			break;
	}

	if ( result == FALSE ) 
		return FALSE;

	/* Count the items to work upon. Are there any at all? */

	cont = count_items(w, n, list, &folders, &files, &bytes, function);

	/* Yes, something has to be done */

	if (cont == TRUE)
	{
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

		spath = itm_fullname(w, list[0]);
		if ( spath == NULL )
			return FALSE;

		/* Show first source file name. It is blank if starting with a folder */

		if ( itm_type(w, list[0]) == ITM_FILE || itm_type(w, list[0]) == ITM_PROGRAM )
		{
			cv_fntoform( copyinfo + CPFILE, itm_name( w,list[0] ) );
			path_to_disp(spath);
		}
		else
			*cpfile = 0;
		
		/* Show initial source and destination paths */
		
		cv_fntoform ( copyinfo + CPFOLDER, spath );	
		cv_fntoform ( copyinfo + CPDEST, dest );
		free(spath);			

		/* 
		 * Currently, "change time" and "change attributes" options are active 
		 * for a single operation only; so, always reset them. 
 		 */

		options.cprefs &= ~(CF_CTIME | CF_CATTR);

		/* Set buttons for these copy options, when appropriate */

		set_opt(copyinfo, options.cprefs, CF_CTIME, CCHTIME); 
		set_opt(copyinfo, options.cprefs, CF_CATTR, CCHATTR); 

		/* Otherwise, hide these two buttons */

		if ( function == CMD_DELETE || function == CMD_PRINT || function == CMD_PRINTDIR )
			copyinfo[CSETBOX].ob_flags |= HIDETREE;
		else
			copyinfo[CSETBOX].ob_flags &= ~HIDETREE;

		/* Also, destination path is hidden if it doesn't make sense */

		if ( function != CMD_COPY  && function != CMD_MOVE )
		{
			copyinfo[CPT3].ob_flags |= HIDETREE;
			copyinfo[CPDEST].ob_flags |= HIDETREE;
		}
		else
		{
			copyinfo[CPT3].ob_flags &= ~HIDETREE;
			copyinfo[CPDEST].ob_flags &= ~HIDETREE;
		}

		/* Open the dialog. Wait for the button */

		button = open_cfdialog(folders, files, bytes, function);

		/* Act depending on the button code */

		if (button == COPYOK)
		{
			/* Check for copy options. Change date or attributes? */

			get_opt( copyinfo, &options.cprefs, CF_CTIME, CCHTIME ); 
			get_opt( copyinfo, &options.cprefs, CF_CATTR, CCHATTR ); 

			/* Copy/move/touch/delete a list of files */

			graf_mouse(HOURGLASS, NULL);

			switch(function)
			{
				case CMD_COPY:
				case CMD_MOVE:
				case CMD_TOUCH:
					result = copy_list(w, n, list, dest, &folders, &files, &bytes, function);
					break;
				case CMD_DELETE:
					result = del_list(w, n, list, &folders, &files, &bytes);
					break;
				case CMD_PRINT:
				case CMD_PRINTDIR:
					result = print_list(w, n, list, &folders, &files, &bytes, function);
				default:
					result = FALSE;
					break;
			}

			graf_mouse(ARROW, NULL);
		}

		/* 
		 * Currently "change time" and "change attributes" are active 
		 * for a single operation only; so, reset again 
 		 */

		options.cprefs &= ~(CF_CTIME | CF_CATTR);

		/* 
		 * Close the information/confirmation dialog if it was open;
		 * then update windows
		 */

		close_cfdialog(button);
		wd_do_update();
	}

	return result;
}


/*
 * Hoofdprogramma kopieer gedeelte.
 */

boolean item_copy(WINDOW *dw, int dobject, WINDOW *sw, int n, int *list, int kstate)
{
	const char *program;
	ITMTYPE type;
	int wtype;
	boolean result = FALSE;

	wtype = xw_type(dw);

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
				return itmlist_op(sw, n, list, NULL, CMD_DELETE);
			case ITM_PRINTER:
				return itmlist_op(sw, n, list, NULL, CMD_PRINT);
			case ITM_PROGRAM:
				if ((program = itm_fullname(dw, dobject)) != NULL)
				{
					result = app_exec(program, NULL, sw, list, n, kstate);
					free(program);
				}
				return result;
			default:
				alert_printf(1, AILLCOPY);
				return FALSE;
			}
		}
		else
			return itm_copy(dw, dobject, sw, n, list, kstate);
	}
	else
	{
		alert_printf(1, AILLCOPY);
		return FALSE;
	}
}


