/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <boolean.h>
#include <library.h>
#include <mint.h>
#include <xdialog.h>
#include <xscncode.h>

#include "resource.h"
#include "desk.h"
#include "stringf.h"		/* get rid of stream io */
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h" 
#include "dir.h"
#include "error.h"
#include "events.h"
#include "file.h"
#include "lists.h"
#include "slider.h" 
#include "filetype.h"
#include "icon.h"
#include "screen.h"
#include "showinfo.h"
#include "copy.h"
#include "icontype.h"
#include "open.h"
#include "va.h"
#include "prgtype.h"


/* In algol 68 its so easy.
   ref to row of ref to NDTA
   ref () ref NDTA
*/


#define TOFFSET			2
#define ICON_W			80		/* icon width (pixels) */
#define ICON_H			46		/* icon height (pixels) */

#define MAXLENGTH		128		/* Maximum length of a filename to display in a window. HR: 128 */
#define MINLENGTH		12		/* Minimum length of a filename to display in a window. */
#define ITEMLEN			44		/* Length of a line, without the filename. */
#define	DRAGLENGTH		16		/* Length of a name rectangle drawn while dragging items (was 12 earlier) */

#define TOSITEMLEN		51		/* Length of a line. */
#define SIZELEN 8 /* length of field for file size, i.e. max 99999999 ~100 MB */


/* Font effects */

#define bold 1
#define light 2
#define italic 4
#define ulined 8
#define white 16


WINFO dirwindows[MAXWINDOWS]; 

RECT dmax;

const char *prevdir = "..";

FONT dir_font;

extern boolean TRACE;

static void dir_closed(WINDOW *w);
extern boolean prg_isprogram(const char *name);


static int		dir_find	(WINDOW *w, int x, int y);
static boolean	dir_state	(WINDOW *w, int item);
static ITMTYPE	dir_itmtype	(WINDOW *w, int item);
static ITMTYPE	dir_tgttype	(WINDOW *w, int item);
static int		dir_icon	(WINDOW *w, int item);
static const
       char    *dir_itmname	(WINDOW *w, int item);
static char    *dir_fullname(WINDOW *w, int item);
static int		dir_attrib	(WINDOW *w, int item, int mode, XATTR *attrib);
static boolean	dir_islink	(WINDOW *w, int item);

static boolean	dir_open	(WINDOW *w, int item, int kstate);
static boolean	dir_copy	(WINDOW *dw, int dobject, WINDOW *sw, int n, int *list, ICND *icns, int x, int y, int kstate);
static void		dir_select	(WINDOW *w, int selected, int mode, boolean draw);
static void		dir_rselect	(WINDOW *w, int x, int y);
static boolean	dir_xlist	(WINDOW *w, int *nselected, int *nvisible, int **sel_list, ICND **icns, int mx, int my);
static boolean	dir_list	(WINDOW *w, int *n, int **list);
static void		dir_set_update	(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);
static void		dir_do_update	(WINDOW *w);
static void		dir_newfolder	(WINDOW *w);
static void		dir_sort		(WINDOW *w, int sort);
static void		dir_seticons	(WINDOW *w);

static ITMFUNC itm_func =
{
	dir_find,					/* itm_find */
	dir_state,					/* itm_state */
	dir_itmtype,				/* itm_type */
	dir_tgttype,				/* target item type */
	dir_icon,					/* itm_icon */
	dir_itmname,				/* itm_name */
	dir_fullname,				/* itm_fullname */
	dir_attrib,					/* itm_attrib */
	dir_islink,					/* itm_islink */
	dir_open,					/* itm_open */
	dir_copy,					/* itm_copy */
	dir_select,					/* itm_select */
	dir_rselect,				/* itm_rselect */
	dir_xlist,					/* itm_xlist */
	dir_list,					/* itm_list */
	wd_path,					/* wd_path */
	dir_set_update,				/* wd_set_update */
	dir_do_update,				/* wd_do_update */
	dir_seticons				/* wd_seticons */
};


/********************************************************************
 *																	*
 * Funkties voor het sorteren van directories.						*
 *																	*
 ********************************************************************/


typedef int SortProc(NDTA **ee1, NDTA **ee2);
#define E1E2 NDTA *e1 = *ee1, *e2 = *ee2
 
/* 
 * Sort by visibility
 */

static SortProc _s_visible
{
	E1E2;

	if (e1->visible && !e2->visible)
		return -1;
	if (e2->visible && !e1->visible)
		return 1;
	return 0;
}


/* 
 * Sort folders before files.
 * Link target type has to be taken into account in order 
 * to sort links to folders properly among folders
 */

static SortProc _s_folder
{
	int h;
	E1E2;

	boolean e1dir, e2dir;

#if _MINT_
		e1dir = ((e1->attrib.attrib & 0x10) || (e1->tgt_type == ITM_FOLDER));
		e2dir = ((e2->attrib.attrib & 0x10) || (e2->tgt_type == ITM_FOLDER));
#else
		e1dir = (e1->attrib.attrib & 0x10);
		e2dir = (e2->attrib.attrib & 0x10);
#endif

	if ((h = _s_visible(ee1, ee2)) != 0)
		return h;
	if (e1dir && !e2dir)
		return -1;
	if (e2dir && !e1dir)
		return 1;

	return 0;
}


/* 
 * Function for sorting a directory by filename 
 */

static SortProc sortname
{
	int h;
	E1E2;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;
	return strcmp(e1->name, e2->name);
}


/* 
 * Function for sorting a directory by filename extension
 */

static SortProc sortext
{
	int h;
	char *ext1, *ext2;
	E1E2;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;
	if (strcmp(e1->name, prevdir) == 0)
		return -1;
	if (strcmp(e2->name, prevdir) == 0)
		return 1;

	ext1 = strchr(e1->name, '.');
	ext2 = strchr(e2->name, '.');

	if ((ext1 != NULL) || (ext2 != NULL))
	{
		if (ext1 == NULL)
			return -1;
		if (ext2 == NULL)
			return 1;
		if ((h = strcmp(ext1, ext2)) != 0)
			return h;
	}

	return strcmp(e1->name, e2->name);
}


/* 
 * Function for sorting a directory by file size
 */

static SortProc sortlength
{
	int h;
	E1E2;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;
	if (e1->attrib.size > e2->attrib.size)
		return 1;
	if (e2->attrib.size > e1->attrib.size)
		return -1;
	return strcmp(e1->name, e2->name);
}


/* 
 * Function for sorting a directory by object date & time
 */

static SortProc sortdate
{
	int h;
	E1E2;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;
	if (e1->attrib.mdate < e2->attrib.mdate)
		return 1;
	if (e2->attrib.mdate < e1->attrib.mdate)
		return -1;
	if (e1->attrib.mtime < e2->attrib.mtime)
		return 1;
	if (e2->attrib.mtime < e1->attrib.mtime)
		return -1;
	return strcmp(e1->name, e2->name);
}


/* 
 * Function for no-sorting :) of directory 
 */

static SortProc unsorted
{
	int h;
	E1E2;

	if ((h = _s_visible(ee1, ee2)) != 0)
		return h;
	if (e1->index > e2->index)
		return 1;
	else
		return -1;
}


/*
 * Function for reversing the order of sorted items.
 * This is to be applied -after- a sorting of any kind if reverse is wanted.
 */

void revord(NDTA **buffer, int n)
{
	int i, j = (n - 1);
	NDTA *s;

	for (i = 0; i < j / 2; i++ )
	{
		s = buffer[i];
		buffer[i] = buffer[j - i];
		buffer[j - i] = s;		
	} 
}


/*
 * Function for sorting of a directory by desired key (no drawing here)
 * Note: it would be a little shorter to have a table of addresses
 * and refer to routines by indices. But only about 16 bytes would be saved.
 */

static void sort_directory( DIR_WINDOW *w, int sort)
{
	SortProc *sortproc;

	/* Select the sorting function */

	switch ( (sort & 0x000F) )
	{
		case WD_SORT_NAME:
			sortproc = sortname;
			break;
		case WD_SORT_EXT:
			sortproc = sortext;
			break;
		case WD_SORT_DATE:
			sortproc = sortdate; 
			break;
		case WD_SORT_LENGTH:
			sortproc = sortlength;
			break;
		case WD_NOSORT:
			sortproc = unsorted;
			break;
	}

	/* Perform the actual sorting (in fact pointers only are sorted) */

	qsort(w->buffer, w->nfiles, sizeof(size_t), sortproc);

	/* Reverse order, if so selected */

	if ( sort & WD_REVSORT )
		revord( (NDTA **)(w->buffer), w->nvisible );
}


/*
 * Sort a directory by desired key, then update the window
 */

void dir_sort(WINDOW *w, int sort)
{
	sort_directory((DIR_WINDOW *)w, sort);
	wd_type_draw ((TYP_WINDOW *)w, FALSE ); 
}


/********************************************************************
 *																	*
 * Funkties voor het laden van directories in een window.			*
 *																	*
 ********************************************************************/

/* 
 * Zet de inforegel van een window. Als n = 0 is dit het aantal
 * bytes en files, als n ongelijk is aan 0 is dit de tekst
 * 'n items selected' 
 */

static void dir_info(DIR_WINDOW *w, int n, long bytes)
{
	char 
		*s,
		*msg;

	if (n > 0)
	{
		s = get_freestring( (n == 1) ? MISINGUL : MIPLURAL );
		msg = get_freestring( MSITEMS );
		sprintf(w->info, msg, bytes, n, s);
	}
	else
	{
		s = get_freestring( (w->nvisible == 1) ? MISINGUL : MIPLURAL );
		msg = get_freestring( MITEMS );
		sprintf(w->info, msg, w->visbytes, w->nvisible, s);
	}

	xw_set((WINDOW *) w, WF_INFO, w->info);
}


/*
 * Release the buffer(s) occupied by directory data
 */

static void dir_free_buffer(DIR_WINDOW *w) 
{
	long i;
	RPNDTA *b = w->buffer;

	if (b)
	{
		for (i = 0; i < w->nfiles; i++)
		{
			free((*b)[i]);		/* free NDTA + name */
		}

		free(b);				/* free pointer array */
		w->buffer = NULL;
	}
}


/*
 * Set directory items as visible (or otherwise),
 * depending on selected attributes and filemask
 */

static void set_visible(DIR_WINDOW *w)
{
	long 
		i, 
		v = 0L;

	int 
		oa = options.attribs,
		n = 0;

	NDTA
		*b;

	if (w->buffer == NULL)
		return;

	for (i = 0; i < w->nfiles; i++)
	{
		b = (*w->buffer)[i];

		b->selected = FALSE;
		b->newstate = FALSE;

		if (   (   (oa & FA_HIDDEN) != 0 				/* permit hidden */
		        || (b->attrib.attrib & FA_HIDDEN) == 0 	/* or item is not hidden */
		       )
		    && (   (oa & FA_SYSTEM) != 0 				/* permit system */
		        || (b->attrib.attrib & FA_SYSTEM) == 0 	/* or item is not system */
		       )
		    && (   (oa & FA_SUBDIR) != 0 				/* permit subdirectory */
		        || (b->attrib.attrib & FA_SUBDIR) == 0 	/* or item is not subdirectory */
		       )
		    && (   (oa & FA_PARDIR) != 0 				/* permit parent dir */  
				|| strcmp(b->name, prevdir) != 0       	/* or item is not parent dir */ 
		       )
		    && (   (b->attrib.mode & S_IFMT) == S_IFDIR /* permit directory */
			    || cmp_wildcard(b->name, w->fspec)	   	/* or mask match */
			   )
		   )
		{
			v += b->attrib.size; /* do this always, or on files only ? */			
			b->visible = TRUE;
			n++;
		}
		else
			b->visible = FALSE;
	}
	w->nvisible = n;
	w->visbytes = v;
}


/*
 * Transform data from extended attributes
 */

void xattr_to_fattr(XATTR *xattr, FATTR *fattr)
{
	fattr->mode = xattr->mode;
	fattr->size = xattr->size;
	fattr->mtime = xattr->mtime;
	fattr->mdate = xattr->mdate;
	fattr->attrib = xattr->attr;
#if _MINT_
	fattr->gid = xattr->gid;
	fattr->uid = xattr->uid;
#endif
}


/* 
 * Funktie voor het copieren van een dta naar de buffer van een window.
 * Parameters fulln and tgt are used only with links 
 */

static int copy_DTA(NDTA **dest, char *fulln, char *name, XATTR *src, XATTR *tgt, int index, bool link)	/* link */
{
	long l = strlen(name);
	NDTA *new;


	new = malloc(sizeof(NDTA) + l + 1);		/* Allocate space for NDTA and name together */
	if (new)
	{
		new->index = index;
		new->link  = link;				/* handle links */
	
#if _MINT_
		if ( link )
		{
			new->item_type = ITM_FILE;
			if ((tgt->mode & S_IFMT) == S_IFDIR)
				new->tgt_type = (strcmp(fn_get_name(fulln), prevdir) == 0) ? ITM_PREVDIR : ITM_FOLDER;
			else
			{
				/* Note: this slows down opening of windows */

				if (prg_isproglink(fulln))
					new->tgt_type = ITM_PROGRAM;
				else
					new->tgt_type = ITM_FILE;
			}
		}
		else
#endif
		{
			if ((src->mode & S_IFMT) == S_IFDIR)
				new->item_type = (strcmp(name, prevdir) == 0) ? ITM_PREVDIR : ITM_FOLDER;
			else
			{
				/* Note: This slows down opening of windows !*/

				if(prg_isprogram(name))
					new->item_type = ITM_PROGRAM;
				else
					new->item_type = ITM_FILE;
			}		
			new->tgt_type = new->item_type;
		}

		/* Convert file attributes */
	
		xattr_to_fattr(src, &new->attrib);

		/* 
		 * Determine which icon is to be used for files in this window. 
		 * Note: if there are many window icons assigned, this routine
		 * slows down window opening dramatically (because of the wildcard
		 * matching), therefore a modification below to do this only 
		 * in icon mode (for a fast machine the "if" could be disabled...)
		 */

		if ( options.mode )	
			new->icon = icnt_geticon(name, new->item_type, new->tgt_type, new->link);

		new->name = new->alname;		/* the pointer makes it possible to use NDTA in local name space (auto) */
		strcpy(new->alname, name);
		*dest = new;

		return l;
	}

	*dest = NULL;
	return ENSMEM;
}


/* 
 * Funktie voor het inlezen van een directory. Het resultaat is
 * ongelijk nul als er een fout is opgetreden. De directory wordt
 * gesorteerd. 
 */

static int read_dir(DIR_WINDOW *w)
{
	long 
		bufsiz, 
		memused = 0,
		length = 0, 
		n = 0;

	int 
		error = 0, 
		maxl = 0;

	XATTR 
		tgtattr = {0},
		attr = {0};

	XDIR 
		*dir;
   

	dir_free_buffer(w); /* clear and release old buffer, if it exists */

	/* HR we only prepare a pointer array :-) */

	bufsiz = (long)options.max_dir;		/* thats how many pointers we allocate */

	w->buffer = malloc(bufsiz * sizeof(size_t));

	if (w->buffer != NULL)
	{
		/* Open the directory. This allocates space for "dir" */

		dir = x_opendir(w->path, &error);

		/* If successful, read directory entries in a loop */

		if (dir)
		{

#if _MINT_
			w->fs_type = dir->type;		/* Need to know the filesystem type elsewhere. */
#endif
			while (error == 0)
			{ 
				char *name;   

				error = (int)x_xreaddir(dir, &name, 256, &attr); 

				if (error == 0)
				{
					bool link = false;
					char *fulln = NULL;

					/* 
					 * Note: number of items in a directory must currently
					 * be limited to int16 values, because routines for
					 * selection of items return numbers of items as
					 * short integers!
					 */
		
					if (n > 32766)
					{
						alert_iprint(MDIRTBIG); 
						break;
					}					
					
					/* Bufer space may have to be increased... */

					if (n == bufsiz)		/* don't exceed allocated amount */
					{
						/* Allocate a 50% bigger buffer, then copy contents */

						void *newb;

						/* HR 120803 expand pointer array mildly exponentially */

						bufsiz = 3 * bufsiz / 2;

						newb = malloc(bufsiz * sizeof(size_t));
						if (newb)
						{
							/* 
							 * Copy to new buffer, free old, point to new;
							 * this code is smaller than using realloc()
							 */
							memcpy(newb, w->buffer, n * sizeof(size_t));
							free(w->buffer);
							w->buffer = newb;
						}
						else
						{
							/* Can't allocate so much memory */
							alert_iprint(MDIRTBIG); 
							break;
						}

					} /* n = bufsiz ? */

#if _MINT_ 		
					/* Handle links */

					if ((attr.mode & S_IFMT) == (unsigned int)S_IFLNK)
					{
						/* 
						 * Why get target? So that items get sorted as they should be:
						 * links to folders will be sorted among folders, etc.
 						 */

						fulln = fn_make_path(dir->path, name);
						x_attr(0, w->fs_type, fulln, &tgtattr); /* follow the link to show attributes */
						link = true;
					}
					else
#endif
						tgtattr = attr;

					if (strcmp(".", name) != 0)
					{
						/* 
						 * An area is allocated here for a NDTA + a name
						 * then the area is filled with data
						 */
					    error = copy_DTA(*w->buffer + n, fulln, name, &attr, &tgtattr, n, link);

					    /* DO NOT put () around w->buffer + n !!! */

					    if (error >= 0)
						{
							length += attr.size;
							memused += error;
							if (error > maxl)
								maxl = error;
							error = 0;
							n++;
						}
					}

					free(fulln);

				} /* err = 0 ? */
			} /* while ... */

			x_closedir(dir);
		}

		if ((error == ENMFIL) || (error == EFILNF))
			error = 0;
	}
	else
		error = ENSMEM;	/* Can't allocate poiners for the new directory */

	/* Number of files, total size, maximum name length */

	w->nfiles = (int)n;
	w->usedbytes = length;
	w->namelength = maxl;

	/* Is everything OK ? */

	if (error != 0)
	{
		/* An error has occured, free memory used */

		dir_free_buffer(w);
		w->nfiles = 0;
		w->nvisible = 0;
		w->usedbytes = 0;
		w->visbytes = 0;
	}
	else
	{
		/* Sort directory */

		set_visible(w);
		sort_directory(w, options.sort);
	}

	return error;
}


/* 
 * Set information to be displayed in window title and info line.
 * Also set window sliders
 */

static void dir_setinfo(DIR_WINDOW *w)
{
	w->nselected = 0;
	w->refresh = FALSE;
	calc_rc((TYP_WINDOW *)w, &(w->xw_work));
	dir_info(w, 0, 0L);
	wd_type_title((TYP_WINDOW *)w);
}


/* 
 * Read a directory into a window buffer 
 */

static int dir_read(DIR_WINDOW *w)
{
	int error;

	error = read_dir(w);
	dir_setinfo(w);			/* line length is calculated here */
	return error;
}


/*
 * Aux. function for the two routines further below. This routine
 * also resets the fulled title if the numbe of items or the length
 * of the longest name has changed.
 */

void dir_reread(DIR_WINDOW *w)
{
	int error, 
	oldn = w->nvisible, 
	oldl = w->llength;

	hourglass_mouse();
	error = dir_read(w);
	arrow_mouse();
	xform_error(error);

	if(w->nvisible != oldn || w->llength != oldl)
		wd_type_nofull((WINDOW *)w);

	wd_type_draw ((TYP_WINDOW *)w, TRUE );
}


/*
 * Unconditional update of a directory window
 */

void dir_refresh_wd(DIR_WINDOW *w)
{
	dir_reread(w);
	wd_reset((WINDOW *)w);
}


/*
 * Unconditionally update all directory windows.
 * Don't reset flags
 */

void dir_refresh_all(void)
{
	WINDOW *w = xw_first();

	while(w)
	{
		if(w->xw_type == DIR_WIND)
			dir_reread((DIR_WINDOW *)w);
		w = w->xw_next;
	}
}


/*
 * Remove a trailing backslash from a path,
 * except if the path is that of a root directory
 */

void dir_trim_slash( char *path )
{
	char 
		*b = strrchr(path, '\\');

	if ( b && strcmp(b, bslash) == 0 && !isroot(path) )
		*b = '\0';
}


/* 
 * Refresh a directory window given by path, or else top that window
 * If required directory is found, return TRUE
 * action: DO_PATH_TOP = top, DO_PATH_UPDATE = update
 */

boolean dir_do_path( char *path, int action )
{
	WINDOW *w;
	const char *wpath;

	boolean result = FALSE;

 	w = xw_first();
	while( w )
	{
		wpath = wd_path(w);

		if ( xw_type(w) == DIR_WIND && wpath && path && (strcmp( path, wpath ) == 0) )
		{
			if ( action == DO_PATH_TOP )
				wd_type_topped( w );
			else			
				dir_refresh_wd( (DIR_WINDOW *)w );
			result = TRUE;
		}
		w = w->xw_next;
	}

	return result;
}


/* 
 * Refresh a directory  window and return sliders to 0
 */

void dir_readnew(DIR_WINDOW *w)
{
	w->py = 0;
	w->px = 0;
	dir_refresh_wd(w);
}


/*
 * Funktie om te controleren of het pad van file fname gelijk is
 * aan path.
 */

static boolean cmp_path(const char *path, const char *fname)
{
	const char *h;
	long l;

	if ((h = strrchr(fname, '\\')) == NULL)
		return FALSE;
	l = h - fname;
	if (isroot(path))
		l++;
	if (strlen(path) != l)
		return FALSE;
	return (strncmp(path, fname, l) != 0) ? FALSE : TRUE;
}


/*
 * Mark a directory window for an update.
 * Two fullnames are given- a source and a destinaton.
 * Update is performed only where needed.
 * This routine does not do any update, it just sets the update flag.
 */

static void dir_set_update(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2)
{
	DIR_WINDOW *dw = (DIR_WINDOW *)w;

	if (type == WD_UPD_ALLWAYS)
		dw->refresh = TRUE;
	else
	{
		if (cmp_path(dw->path, fname1) != FALSE)
			dw->refresh = TRUE;
		if ((type == WD_UPD_MOVED) && (cmp_path(dw->path, fname2) != FALSE))
			dw->refresh = TRUE;
	}
}


/*
 * Update a directory window if it is marked for update
 * Also inform signed-on AV-clients that this directory should be updated.
 * Note: notification of AV-clients may slow down file copying.
 * If there are no AV-protocol clients, skip this copmletely for speed
 */

static void dir_do_update(WINDOW *w)
{
	if (((DIR_WINDOW *)w)->refresh)
	{
		dir_refresh_wd((DIR_WINDOW *)w);

#if _MORE_AV
		if ( avclients )
			va_pathupdate( ((DIR_WINDOW *)w)->path);
#endif
	}
}


/* 
 * Funktie voor het zetten van de mode van een window.
 * Display a directory window in text mode or icon mode.
 * Mode is taken directly from options.mode 
 */

void dir_disp_mode(WINDOW *w)
{
	RECT work;

	xw_getwork(w, &work);
	calc_rc((TYP_WINDOW *)w, &work);
	set_sliders((TYP_WINDOW *)w);
	do_redraw(w, &work); 		
}


/********************************************************************
 *																	*
 * Funkties voor het zetten van het file mask van een window.		*
 *																	*
 ********************************************************************/


/*
 * Funkties voor het zetten van deusile attributen van een window.	
 * Routine set_visible used options.attribs, etc. 
 */

void dir_newdir(DIR_WINDOW *w)
{
	set_visible(w);
	wd_reset((WINDOW *)w);
	dir_setinfo(w);
	sort_directory(w, options.sort);
	wd_type_draw ((TYP_WINDOW *)w, TRUE); 
}


void dir_filemask(DIR_WINDOW *w)
{
	char *newmask;

	if ((newmask = wd_filemask(w->fspec)) != NULL)
	{
		free(w->fspec);
		w->fspec = newmask;
		dir_newdir(w);
	}
}


/*
 * Funktie voor het zetten van de iconen van objecten in een window.
 */

static void dir_seticons(WINDOW *w)
{
	RPNDTA *pb;
	NDTA *b;
	long i, n;

	pb = ((DIR_WINDOW *) w)->buffer;
	n = ((DIR_WINDOW *) w)->nfiles;

	for (i = 0; i < n; i++)
	{
		b = (*pb)[i];
		b->icon = icnt_geticon(b->name, b->item_type, b->tgt_type, b->link);
	}

	if (options.mode != TEXTMODE) 
		wd_type_draw ((TYP_WINDOW *)w, FALSE );
}


/*
 * Create a new folder or a link.
 * if target==NULL a folder is created; otherwise a link.
 */

static int dir_makenew(DIR_WINDOW *dw, char *name, char *target)
{
	char *thename;
	int button, error = 0;

	obj_unhide(newfolder[DIRNAME]);
	obj_hide(newfolder[OPENNAME]);

	button = chk_xd_dialog(newfolder, DIRNAME);

	if ((button == NEWDIROK) && (*dirname))
	{
		cv_formtofn(name, newfolder, DIRNAME);

		/* Is the new name perhaps too long ? */

		if ((error = x_checkname(dw->path, name)) == 0)
		{
			if ((thename = fn_make_path(dw->path, name)) != NULL)
			{
				hourglass_mouse();
#if _MINT_			
				if (target)
					error = x_mklink(thename, target);
				else
#endif
					error = x_mkdir(thename);

				if (error == 0)
				{
					wd_set_update(WD_UPD_COPIED, thename, NULL);
					wd_do_update();
				}

				free(thename);
				arrow_mouse();
			}
		}

		if (error == EACCDN && target == NULL)
			alert_iprint(MDEXISTS);
		else
			xform_error(error);
	}

	return error;
}


/*
 * Funktie voor het maken van een nieuwe directory.
 * Handle the dialog for creating new folders.
 */

void dir_newfolder(WINDOW *w)
{
	LNAME name;	
	DIR_WINDOW *dw = (DIR_WINDOW *)w;

	cv_fntoform(newfolder, DIRNAME, empty);
	rsc_title(newfolder, NDTITLE, DTNEWDIR);
	dir_makenew(dw, name, NULL);
}


#if _MINT_
/*
 * Create a symbolic link (open the dialog to enter data).
 * Note: appropriate file system is checked for in wd_hndlmenu.
 * It is assumed there that if filesystem is not tos, links can 
 * always be created- which in fact may not always be true
 * and should better be checked
 */

void dir_newlink(WINDOW *w, char *target)
{
	char
		*linkto,
		*targetname; 

	LNAME 
		name;

	DIR_WINDOW 
		*dw;


	dw = (DIR_WINDOW *)w;
	linkto = get_freestring(TLINKTO);
	targetname = fn_get_name(target);

	/* The string pointed to by dirname is a LNAME, therefore sizeof() below */

	strcpy(dirname, linkto);
	strncat(dirname, targetname, sizeof(LNAME) - strlen(linkto) - 1L ); 

	/* This will always be a scrolled editable text, so xd_init_shift can be simply applied */

	rsc_title(newfolder, NDTITLE, DTNEWLNK);
	xd_init_shift(&newfolder[DIRNAME], dirname);

	dir_makenew(dw, name, target);
}

#endif


/* 
 * Funktie voor het berekenen van het totale aantal regels in een window.
 * This routine calculates number of lines, columns and text columns.
 * It does -not- calculate the number of visible columns. 
 */

void calc_nlines(DIR_WINDOW *dw)
{
	int nvisible = dw->nvisible;

	if (options.mode == TEXTMODE)
	{
		int ll, mcol, dc;

		dw->llength = linelength(dw);			/* length of an item line */

		if ( options.aarr )
		{
			ll = dw->llength + CSKIP;
			mcol = (max_w / dir_font.cw - BORDERS) / ll; /* max. columns to fit on screen */

			dc = min( (dw->ncolumns + dw->llength / 2) / ll, mcol);
			dw->dcolumns = minmax( 1, dc, nvisible );
 			dw->nlines = (nvisible - 1) / dw->dcolumns + 1; 					/* lines */
		}
		else
		{
			dw->nlines = nvisible;
			dw->dcolumns = 1;
		}

		dw->columns = (dw->llength + CSKIP) * dw->dcolumns + BORDERS - CSKIP;
	}
	else
	{
		if (options.aarr)
		{
 			dw->columns = ( dw->xw_work.w + ICON_W / 2 ) / ICON_W;		
		}
		else
		{
 			dw->columns = (max_w - XOFFSET)/ ICON_W;		
		}
		dw->columns = max(1, min(dw->columns, nvisible)); 
		dw->nlines = (nvisible - 1) / dw->columns + 1; 					/* lines */
	}
}


/* 
 * Calculate the length of a line in a window. Returned value
 * depends on filename length and visible directory fields.
 * In the TOS version of TeraDesk name length is a constant, 
 * in the MiNT version it is the length of the longest filename
 * in the directory. 
 */

int linelength(DIR_WINDOW *w)
{

	int l;						/* length of filename             */
	int f;						/* length of other visible fields */
	char of = options.fields;	/* which fields are shown */	
  
	/* Determine filename length */

	if ( ((w->fs_type) & FS_LFN) == 0 )	/* for TOS (8+3) filesystem */
		l = 12;							/* length of filename */
	else
		l = minmax(MINLENGTH, w->namelength, MAXLENGTH);
	
	/*
	 * Beside the filename there will always be:
	 * + 3 spaces for the character used to mark directories 
	 *     including a leading space
	 * + 2 spaces separating filename from the rest
	 * + 2 additional spaces after each field as separators
	 *     (there will be only one space after the last field)
	 * below should be: f = 5 = 3+2, but for some unclear reason
	 * hor.slider works better if one char less is specified here;
	 * left and right border are -not- included in calculation
	 */
	  
	f = 4;
	
	if ( of & WD_SHSIZ ) 
		f += SIZELEN + 2;	/* size: 8 char + 2 sep */
	if ( of & WD_SHDAT ) 
		f += 10; 			/* date: 8 char + 2 sep. */
	if ( of & WD_SHTIM ) 
		f += 10; 			/* time: 8 char + 2 sep. */

	if ( of & WD_SHATT )             		/* attributes: depending on fs */
	{
#if _MINT_
		if ( (w->fs_type & FS_UID) != 0 )
		{
			f += 12;	/* other (i.e.mint) filesystem: attributes use 10 char + 2 sep. */
		}
		else
#endif
			f += 7;	/* FAT/VFAT filesystem: attributes use 5 char + 2 sep. */
	} 

#if _MINT_
	if ((of & WD_SHOWN) && (w->fs_type & FS_UID))
		f += 11;/* owner and group id: 4 + 1 + 4 + 2 sep. */
#endif

	return f + l;	
}


/********************************************************************
 *																	*
 * Functies voor het tekenen van de inhoud van een directorywindow.	*
 *																	*
 ********************************************************************/

/*
 * Bereken de rechthoek die de te tekenen tekst omsluit.
 * Compute position of one text line (index="item") in a directory window.
 *
 * Parameters:
 *
 * tw		- pointer naar window
 * item		- regelnummer
 * r		- resultaat
 * work		- RECT structuur met de grootte van het werkgebied van het
 *			  window.
 */

static void dir_comparea(DIR_WINDOW *dw, int item, RECT *r, RECT *work)
{
	int 
		ll = dw->llength + CSKIP,
		col; 

	/* Find item's column and its x-position */

	col = item / dw->nlines;

	r->x = work->x  + (col * ll + TOFFSET - dw->px) * dir_font.cw; 
	r->y = DELTA + work->y + (int) ( (item - dw->nlines * col ) - dw->py) * (dir_font.ch + DELTA);
	r->w = dir_font.cw * dw->llength;
	r->h = dir_font.ch;
}


/*
 * Convert a time to a string (format HH:MM:SS)
 * This string is always 8 characters long. 
 * Attention: no termination 0 byte!
 * NOte: a single temporary variable could have been used 
 * instead of sec, min, hour below, but that produces 
 * slightly longer code. Same for datestr().
 *
 * Parameters:
 *
 * tstr		- destination string
 * time		- current time in GEMDOS format
 * Result   - Pointer to the -end- of tstr.
 */

static char *timestr(char *tstr, unsigned int time)
{
	unsigned int sec, min, hour, h;

	sec = (time & 0x1F) * 2;
	h = time >> 5;
	min = h & 0x3F;
	hour = (h >> 6) & 0x1F;
	digit(tstr, hour);
	tstr[2] = ':';
	digit(tstr + 3, min);
	tstr[5] = ':';
	digit(tstr + 6, sec);
	return tstr + 8;
}


/*
 * Convert a date to a string (format DD-MM-YY).
 * This string is always 8 characters long.
 * Attention: no termination 0 byte!
 *
 * Parameters:
 *
 * tstr		- destination string
 * date		- current date in GEMDOS format
 * Result:  - Pointer to the -end- of tstr.
 */

static char *datestr(char *tstr, unsigned int date)
{
	unsigned int day, mon, year, h;

	day = date & 0x1F;
	h = date >> 5;
	mon = h & 0xF;
	year = (((h >> 4) & 0x7F) + 80) % 100;
	digit(tstr, day);
	tstr[2] = '-';
	digit(tstr + 3, mon);
	tstr[5] = '-';
	digit(tstr + 6, year);
	return tstr + 8;
}


#if _MINT_

/* 
 * Create a string for displaying user/group id (range 0:9999).
 * Leading zeros are shown; no termination zero byte.
 * Return pointer to -end- of string.
 * Hopefully this will be faster than sprintf( ... %i ...)
 */

static char *uidstr(char *idstr, int id)
{
	int i, k = 1000;

	for( i = 0; i < 4; i++ )
	{
		idstr[i] = id / k;
		id -= k * idstr[i];
		k /= 10;
		idstr[i] += '0';
	}

	return idstr + 4;
}
#endif


/*
 * Macro used in dir_line.
 * FILL_UNTIL fills a string with ' ' until a counter reaches a certain value.
 * Note: there is no point in creating a routine to do this;
 * the resulting code would have exactly the same length, but
 * would be slower.
 */


#define FILL_UNTIL(dest, i, value)			while (i < (value))	\
											{					\
												*dest++ = ' ';	\
												i++;			\
											}

/*
 * Make a string with the contents of a line in a directory window.
 * If there is no content, produce a string of spaces.
 */

void dir_line(DIR_WINDOW *dw, char *s, int item)
{
	NDTA 
		*h;						/* pointer to directory item data */

	char 
		of = options.fields,	/* which fields are shown */
		*d; 					/* current position in the string */

	const 
		char *p;				/* current position in the source */

	int 
		i,						/* length counter */ 
		parent;					/* flag that this is a .. (parent) dir */


#if _MINT_
	static const int 
		rbits[9] = {S_IRUSR,S_IWUSR,S_IXUSR,S_IRGRP,S_IWGRP,S_IXGRP,S_IROTH,S_IWOTH,S_IXOTH};
	static const char 
		fl[9] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};
#endif

	unsigned int 
		hmode;

	parent = FALSE;

	if (item < dw->nvisible)
	{

		/* Beware! must be:
		 * mark[ITM_FOLDER] = '\007';
		 * mark[ITM_PREVDIR] = '\007';
		 * mark[ITM_PROGRAM] = '-';
		 */
		static const char mark[9] = {' ',' ',' ',' ','\007','-',' ','\007',' '};
 
		h = (*dw->buffer)[(long)item]; /* file data */
		hmode = h->attrib.mode & S_IFMT; 
		d = s; /* beginning of the destination string for the line */

		/* Mark a folder or parent directory; also mark programs */

		*d++ = ' ';
		*d++ = mark[h->tgt_type];
		*d++ = ' ';

		p = h->name;
		i = 0;

		/* This will modify display of ".." in all filesystems now */

		if (strcmp(h->name, prevdir) == 0)
			parent = TRUE;

		/* Compose the filename */

#if _MINT_
		if (dw->fs_type != 0)
		{
			/* 
			 * This is not TOS fs. (No need to check for mint)
			 * Copy the filename. '..' is handled like everything else 
			 */

			while ((*p) && (i < MAXLENGTH))
			{
				*d++ = *p++;
				i++;
			}

	        FILL_UNTIL(d, i, minmax(MINLENGTH, dw->namelength, MAXLENGTH))
		}
		else
#endif
		{	
			/* 
			 * This is TOS fs
			 * Copy the filename. Handle '..' as a special case. 
			 */
	
			if (!parent)
			{
				/* Copy the name, until the start of the extension found. */
				
				while ((*p) && (*p != '.'))
				{
					*d++ = *p++;
					i++;
				}
	
				FILL_UNTIL(d, i, 9);

				/* Copy the extension. */
	
				if (*p++ == '.')
				{
					while (*p)
					{
						*d++ = *p++;
						i++;
					}
				}
			}
			else
			{
				/* This is the parent directory (..). Just copy. */
	
				while (*p)
				{
					*d++ = *p++;
					i++;
				}
			}
	
			FILL_UNTIL(d, i, 12);

		}  /* mint fs? */

		*d++ = ' ';		/* always separate filename from other fields */
		*d++ = ' ';		

		/* Compose size string */

		if ( of & WD_SHSIZ )	/* show file size? */
		{
			if (hmode != S_IFDIR) /* this is not a subdirectory */
			{
				int l;
				long s = h->attrib.size & 0x7FFFFFFFL;
				char t[SIZELEN + 4];		/* temporary string */

				/* 
				 * Note: SIZELEN, if set to "8", permits maximum
				 * size of about 99 MB (99999999 bytes) to be displayed.
				 * Size of larger files will be displayed in kilobytes
				 * e.g. as 1234K, which permits up to about 9 GB,
				 * which is above the 32bit long integer limit anyway.
				 * Larger than -that- would produce incorrect display.
				 */

				if ( s > 99999999L )
				{
					s = s / 1024;
					ltoa(s, t, 10);
					strcat(t, "K");
				}
				else
					ltoa(s, t, 10);
				
				t[SIZELEN] = 0;      	/* so that it doesn't become too long */

				l = (int)strlen(t);   	/* how long is this string? */
				p = t;
				i = 0;

				FILL_UNTIL(d, i, SIZELEN - l);

				while (*p)
				{
					*d++ = *p++;
				}
			}
			else
			{
				i = 0;
				FILL_UNTIL(d, i, SIZELEN);
			}

			*d++ = ' '; /* separate size from the next fields */
			*d++ = ' ';		  
		}

		/* Compose date string */

		if ( of & WD_SHDAT )
		{
			if ( parent ) 	/* Don't show meaningless date for parent folder */
			{ 
				i = 0;                       
				FILL_UNTIL ( d, i, 10 );     
			}
 			else
			{
				d = datestr(d, h->attrib.mdate);
				*d++ = ' ';
				*d++ = ' ';
			}
		}

		/* Compose time string */

		if ( of & WD_SHTIM )
		{
			if ( parent ) 	/* don't show meaningless time for parent folder */
			{  
				i = 0;                        
				FILL_UNTIL ( d, i, 10 );     
			}								
			else
			{
				d = timestr(d, h->attrib.mtime);
				*d++ = ' ';
				*d++ = ' ';
			}
		}

		/* Compose attributes */

		if ( of & WD_SHATT )
		{
			switch(hmode)
			{
				/* these attributes handle diverse item types  */
				case S_IFDIR :	/* directory (folder) */
					*d++ = 'd';
					break;
				case S_IFREG :	/* regular file */
					*d++ = '-';
					break;
#if _MINT_
				case S_IFCHR :	/* BIOS special character file */
					*d++ = 'c';
					break;
				case S_IFIFO :	/* pipe */
					*d++ = 'p';
					break;
				case S_IFLNK :	/* symbolic link */
					*d++ = 'l';
					break;
				case S_IMEM :	/* shared memory or process data */
					*d++ = 'm';
					break;
#endif
				default : 
					*d++ = '?';
					break;
				}
#if _MINT_
			if ((dw->fs_type & FS_UID) != 0)
			{
				/* This is not a FAT/VFAT filesystem */

				char *r = d;

				/* These handle access rights for owner, group and others */
	
				for ( i = 0; i < 9; i++ )
					*d++ = (h->attrib.mode & rbits[i]) ? fl[i] : '-';				

				if(h->attrib.mode & S_ISUID)
					r[2] = (r[2] == 'x') ? 's' : 'S';
				if(h->attrib.mode & S_ISGID)
					r[5] = (r[5] == 'x') ? 's' : 'S';
				if(h->attrib.mode & S_ISVTX)
					r[8] = (r[8] == 'x') ? 't' : 'T';

			} /* non-FAT filesystem */
			else
#endif
			{
				unsigned int aa = h->attrib.attrib;				

				/* This is a FAT/VFAT filesystem */

				*d++ = (aa & FA_SYSTEM)   ? 's' : '-';
				*d++ = (aa & FA_HIDDEN)   ? 'h' : '-';
				*d++ = (aa & FA_READONLY) ? '-' : 'w';
				*d++ = (aa & FA_ARCHIVE)  ? 'a' : '-';
			}

			*d++ = ' ';
			*d++ = ' ';		
		}

#if _MINT_
		if (((dw->fs_type & FS_UID) != 0) && (of & WD_SHOWN) )
		{
			d = uidstr(d, h->attrib.gid); /* first group id */
			*d++ = '/';
			d = uidstr(d, h->attrib.uid); /* then user id */

			*d++ = ' ';
			*d++ = ' ';
		}
#endif
		
		/* One of the last two blanks is too much */

		*--d = 0;
	}
	else
	{
		/* Line is not visible. Return a string filled with spaces. */

		memset(s, ' ', (size_t)(dw->llength));
		s[dw->llength] = 0;
	}
}


/********************************************************************
 *																	*
 * Funkties voor het afdrukken in iconmode.							*
 *																	*
 ********************************************************************/

/*
 * Tell how many icons have to be drawn in a window.
 * Note: the routine does not actuallu -count- the icons, 
 * but calculates their number. This may sometimes give a larger 
 * number than needed.
 */

long count_ic( DIR_WINDOW *dw, int sl, int lines )
{
	int columns;
	long n, start;

	columns = dw->columns;
	n = columns * lines;
	start = sl * columns + dw->px;

	if ((start + n) > dw->nvisible)
		n = lmax((long)(dw->nvisible) - start, 0L);

	return n;
}


/*
 * Create an object tree for drawing icons in a window. 
 * If parameter 'smode' is false, all icons will be drawn, 
 * and background object as well
 */

OBJECT *make_tree
(
	DIR_WINDOW *dw, 
	int sc,				/* first column to display */ 
	int ncolumns, 		/* number of columns to display */
	int sl, 			/* first line to display */
	int lines, 			/* number of lines to display */
	boolean smode,		/* draw all icons + background if false */
	RECT *work			/* work area of the window */
)
{
	int 
		icon_no,
		j,
		ci,			/* column in which an icon is drawn */
		columns, 
		yoffset, 
		row;

	boolean
		hidden;		/* flag for a hidden object */

	long 
		oi = 0,		/* index of an object in the tree */
		lo,			/* offset of labels area from root object */
		n,			/* approximate number of objects to draw */ 
		start, 
		i;			/* counter */

	OBJECT 
		*obj;		/* pointer to the root object */

	NDTA 
		*h;			/* pointer to directory items */

	INAME
		*labels;	/* pointers to icons labels */


	columns = dw->columns;
	start = sl * columns + dw->px;

	/* 
	 * How many icons have to be drawn? 
	 * This is only an approximate calculation, actual number may be lower
	 */

	n = count_ic( dw, sl, lines );

	/* 
	 * Allocate memory for background and icon objects.
	 * This may be more than actually needed because maybe
	 * not all objects will be drawn. Fortunately, it will
	 * soon be released.
	 */

	lo = (n + 1) * sizeof(OBJECT) + n * sizeof(CICONBLK);

	if((obj = malloc_chk(lo + n * sizeof(INAME))) != NULL)
	{
		labels = (INAME *)((char *)(obj) + lo);

		if ((row = (sl - dw->py) * ICON_H) == 0)
			yoffset = YOFFSET;
		else
			yoffset = 0;

		/* Set background object */

		wd_set_obj0( obj, smode, row, lines, yoffset, work );

		/* Note: this below must agree whith dir_drawsel() */

		for (i = 0; i < n; i++)
		{
			boolean selected;

			j = start + i;
			ci = j % columns;

			if ( colour_icons || ((ci >= sc) && (ci <= (sc + ncolumns))) )
			{
				h = (*dw->buffer)[j];

				hidden = ((h->attrib.attrib & FA_HIDDEN) != 0);
				icon_no = h->icon;

				if (smode)
				{
					if (h->selected == h->newstate)
						continue; /* next loop !!! */
					selected = h->newstate;
				}
				else
					selected = h->selected;
#if _MINT_
				cramped_name((char *)h->name, labels[i], sizeof(INAME));
#else
				strcpy(labels[i], (char *)h->name); /* shorter, and safe in single-TOS */
#endif
				set_obji
				( 
					obj, 
					oi++,
					n,
					selected, 
					hidden,
					h->link,
					icon_no, 	
					(ci - dw->px) * ICON_W + XOFFSET,
					(j / columns - sl) * ICON_H + yoffset,
					labels[i]
				);
			}
		}
	}

	return obj;
}


/*
 * Draw an entire object tree within a rectangle
 */

static void draw_tree(OBJECT *tree, RECT *clip)
{
	objc_draw(tree, ROOT, MAX_DEPTH, clip->x, clip->y, clip->w, clip->h);
}


/*
 * Draw icons in a directory window in icon mode
 */

static void draw_icons(DIR_WINDOW *dw, int sc, int columns, int sl, int lines, RECT *area,
					  boolean smode, RECT *work)
{
	OBJECT *tree;

	if ((tree = make_tree(dw, sc, columns, sl, lines, smode, work)) == NULL)
		return;
	draw_tree(tree, area);

	free(tree);
}


/*
 * Compute screen position of an object in icon mode
 */

static void icn_comparea
(
	DIR_WINDOW *dw, /* pointer to a window */
	int item, 		/* item index in a directory */
	RECT *r1,		/* icon (image?) rectangle */ 
	RECT *r2, 		/* icon text rectangle */
	RECT *work		/* inside area of the window */
)
{
	int 
		s, 
		columns = dw->columns, 
		x, 
		y;

	CICONBLK 
		*h;			/* ciconblk (the largest) */

	s = item - columns * dw->py; /* item index starting from 1st visible */
	x = s % columns - dw->px; 
	y = s / columns;

	if ((s < 0) && (x != 0))
	{
		y -= 1;
		x = columns + x;
	}

	x = work->x + x * ICON_W + XOFFSET;
	y = work->y + y * ICON_H + YOFFSET;

	h = icons[(*dw->buffer)[(long) item]->icon].ob_spec.ciconblk;

	*r1 = h->monoblk.ic;
	r1->x += x;
	r1->y += y;

	*r2 = h->monoblk.tx;
	r2->x += x;
	r2->y += y;
}


/* 
 * dir_prtline calls VDI routines to actually display a directory line 
 * on screen, in text or in icon mode.
 * This routine is called once for each directory entry.
 * "line" is in fact item index (0 to w->nvisible)
 */
 
void dir_prtline(DIR_WINDOW *dw, int line, RECT *area, RECT *work)
{
	/*
	 * Don't do anything in an iconified window
	 * (and perhaps this is not even needed ?) 
	 */

	if ( (dw->winfo)->flags.iconified != 0 ) 	
		return; 

	if (options.mode == TEXTMODE)
	{
		RECT r, in; 
		VLNAME s;
		int i;
		
		/* Compute position of a line */

		dir_comparea(dw, line, &r, work);

		if (xd_rcintersect(area, &r, &in))
		{
			NDTA *d;
			boolean link = FALSE;

			/* Compose a directory line */

			dir_line(dw, s, line);

			/* First column of text to be displayed */

			i = 0;
			xd_clip_on(&in);
			pclear(&r);

			/* Consider only lines with some text */

			if ( line < dw->nvisible )
			{
				int effects = 0;

				d = (*dw->buffer)[(long)line];
				link = d->link;	

				/* Show links in italic type */

				if (link)
					effects |= italic;
				if ((d->attrib.attrib & FA_HIDDEN) != 0)
					effects |= light;
				if (effects)
					vst_effects(vdi_handle, effects);	

				w_transptext(r.x, r.y, &s[i]);
				
				if (effects)
					vst_effects(vdi_handle, 0);	

				if (d->selected)
					invert(&r);
			}
		}

		xd_clip_off();
	}
	else
		draw_icons(dw, dw->px, dw->ncolumns, line, 1, area, FALSE, work);
}



/*
 * Calculate index of the last visible item in a directory window 
 * in text mode
 */

long dir_pymax(DIR_WINDOW *w)
{

	long py = w->py + (long)w->nrows + w->nlines * (w->dcolumns - 1);

	return lmin(py, w->nvisible);
}



/* 
 * Funktie voor het opnieuw tekenen van alle regels in een window.
 * Alleen voor tekst windows. 
 * Display all items in a directory window
 */

static void dir_prtlines(DIR_WINDOW *dw, RECT *area, RECT *work)
{
	int i;

	set_txt_default(&dir_font);

	for(i = 0; i < dw->rows; i++ )
		dir_prtcolumns(dw, dw->py + i, area, work);
}


void do_draw(DIR_WINDOW *dw, RECT *r, OBJECT *tree, boolean text, RECT *work)
{
	if (text)
	{
		pclear(r);
		dir_prtlines(dw, r, work);
	}
	else
		draw_tree(tree, r);
}


/* 
 * Funktie om een pagina naar boven te scrollen. 
 */

void dir_prtcolumn(DIR_WINDOW *dw, int column, int nc, RECT *area, RECT *work)
{
	int 
		i;

	RECT 
		r, 
		in;

	r.y = work->y;
	r.h = work->h;

	if ( options.mode == TEXTMODE )
	{
		r.w = nc * dir_font.cw;
		r.x = work->x + (column - dw->px) * dir_font.cw;

		if (xd_rcintersect(area, &r, &in))
		{
			int i0, col;

			col = (column - TOFFSET) / (dw->llength + CSKIP);	
			i0 = col * dw->nlines + dw->py;

			for (i = 0; i < dw->nrows; i++)
				dir_prtline(dw, i0 + i, &in, work);
		}
	}
	else
	{
		r.w = nc * ICON_W;
		r.x = work->x + (column - dw->px) * ICON_W + XOFFSET;
		if (xd_rcintersect(area, &r, &in))
			draw_icons(dw, column, nc, (int)(dw->py), dw->rows, &in, FALSE, work);
	}
}


/*
 * Display a line containing several directory columns
 */

void dir_prtcolumns(DIR_WINDOW *w, long line, RECT *in, RECT *work)
{
	int j, k = 1;

	if ( options.mode == TEXTMODE )
		k = w->dcolumns;

	for ( j = 0; j < k; j++ )
		dir_prtline((DIR_WINDOW *)w, (int)line + j * w->nlines, in, work);
}


/*
 * Remove a directory 
 */

static void dir_rem(DIR_WINDOW *w)
{
	free(w->path);
	free(w->fspec);
	dir_free_buffer(w);
	w->winfo->used = FALSE;
}


/*
 * Either close a directory window completely, or go one level up.
 * If mode > 0 window is closed completely.
 */

void dir_close(WINDOW *w, int mode)
{
	char 
		*thepath = (char *)((DIR_WINDOW *)w)->path,
		*p;

	if ( w )
	{
		((TYP_WINDOW *)w)->winfo->flags.iconified = 0;

		if (isroot(thepath) || mode)
		{
			/* 
			 * Either this is a root window, or it should be closed entirely;
			 * In any case, close it.
			 */

			xw_close(w);
			dir_rem((DIR_WINDOW *)w);
			wd_reset(NULL);
			xw_delete(w);			/* after dir_rem (MP) */
		}
		else
		{
			/* 
			 * Move one level up the diectory tree;
			 * First, extract parent path from the window's path specification.
			 * New memory is allocated for that; 
			 */

			if ((p = fn_get_path(thepath)) != NULL)
			{
				/* Successful; free previous path */

				free(thepath);
				((DIR_WINDOW *) w)->path = p;
				dir_readnew((DIR_WINDOW *) w);
			}
		}
	}
}


/*
 * Funkties voor het afhandelen van window events.
 */

static void dir_closed(WINDOW *w)
{
	dir_close(w, 0);
}


/********************************************************************
 *																	*
 * Funkties voor het openen van een tekst window.					*
 *																	*
 ********************************************************************/

/* 
 * Open een window. path en fspec moet een gemallocde string zijn.
 * Deze mogen niet vrijgegeven worden door de aanroepende funktie.
 * Bij een fout worden deze strings vrijgegeven. 
 */

static WINDOW *dir_do_open
(WINFO *info, const char *path, const char *fspec, int px, int py, int *error) 
{
	DIR_WINDOW *w;
	RECT size;
	WDFLAGS oldflags = info->flags;

	wd_in_screen( info ); /* modify position to come into screen */

	/* Create a window. Note: window structure is completely zeroed here */

	if ((w = (DIR_WINDOW *)xw_create(DIR_WIND, &wd_type_functions, DFLAGS, &dmax,
									  sizeof(DIR_WINDOW), NULL, error)) == NULL)
	{
		free(path);
		free(fspec);
		return NULL;
	}

	hourglass_mouse();

	/* Fields not explicitely specified will remain zeroed */

	info->typ_window = (TYP_WINDOW *)w;
	w->itm_func = &itm_func;
	w->px = px;
	w->py = py;
	w->path = path;
	w->fspec = fspec;
	w->winfo = info;

	/* In order to calculate columns and rows properly... */

	info->flags.fulled = 0;
	info->flags.fullfull = 0;

	/* Now read the contents of the directory */

	noicons = FALSE; /* so that missing icons be reported */
	*error = dir_read(w);
	wd_calcsize(info, &size);

	arrow_mouse();

	if (*error != 0)
	{
		dir_rem(w);
		xw_delete((WINDOW *)w);		/* after dir_rem (MP) */
		return NULL;
	}
	else
	{
		info->flags = oldflags;
		wd_calcsize(info, &size); 
		wd_type_title((TYP_WINDOW *)w);

		if ( avsetw.flag )
		{
			size = avsetw.size;
			avsetw.flag = FALSE;
		}

		wd_iopen( (WINDOW *)w, &size); 
		return (WINDOW *)w;
	}
}


/*
 * Create a directory window
 * Note: all elements will be deallocated at window closing,
 * must be kept until then.
 * In case of an error, path is freed.
 */

boolean dir_add_window
(
	const char *path,	/* path of the window to open */ 
	const char *thespec,/* file mask specification */
	const char *name	/* name to be selected in the open window, NULL if none */
) 
{
	int 
		j = 0,		/* counter */ 
		error;		/* error code */

	char 
		*fspec;		/* current (default) file specification */
	
	WINDOW
		*w;			/* pointer to the window being opened */

	/* Find an unused directory window slot */

	while ((j < MAXWINDOWS - 1) && dirwindows[j].used )
		j++;

	if (dirwindows[j].used)
	{
		/* All windows are used, can't open any more */

		alert_iprint(MTMWIND); 
		free(path);
	}
	else
	{
		/* OK; allocate space for file specification */

		if ( thespec )
			fspec = (char *)thespec;
		else
			fspec = strdup( fsdefext );

		if (fspec == NULL)
		{
			/* Can't allocate a string for default file specification */

			free(path);
		}
		else
		{
			/* Ok, now really we can open the window, in WINFO slot "j" */

			if ( (w = dir_do_open(&dirwindows[j], path, fspec, 0, 0, &error)) == NULL)
			{
				xform_error(error);
			}
			else
			{
				/* 
				 * Code below shows search result; if a name is specified,
				 * this means that a search has been performed
				 */

				if ( name )
				{ 
					int
						nv,    /* number of visible items in the directory */
						i;     /* item counter */

					NDTA
						*h;

					DIR_WINDOW 
						*dw = (DIR_WINDOW *)(dirwindows[j].typ_window);

					nv = dw->nvisible; 

					/* Find the item which matches the name */

					for ( i = 0; i < nv; i++ )
					{
						h = ( *( (DIR_WINDOW *)(dirwindows[j].typ_window) )->buffer)[i]; 

						if ( strcmp( h->name, name ) == 0 )
						{
							int dwpx;
							long dwpy;

							if ( options.mode == TEXTMODE )
							{
								dwpx = (i / dw->nlines) * (dw->llength + CSKIP);
								dwpy = (long)(i % dw->nlines);
							}
							else
							{
								dwpx = i % dw->columns;
								dwpy = (long)(i / dw->columns);
							}

							w_page((TYP_WINDOW *)dw, dwpx, dwpy);

							selection.w = w;
							selection.selected = i;
							selection.n = 1; 					
							dir_select( w, i, 3, TRUE );
							break; /* don't search anymore */
						}
					}
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}


/*
 * For the sake of size optimization, a call with fewer parameters
 */

boolean dir_add_dwindow(const char *path)
{
	return dir_add_window(path, NULL, NULL);
}


/********************************************************************
 *																	*
 * Funkties voor het initialisatie, laden en opslaan.				*
 *																	*
 ********************************************************************/


/*
 * Configuration table for one open directory window
 */

static CfgEntry dirw_table[] =
{
	{CFG_HDR, 0, "dir" },
	{CFG_BEG},
	{CFG_D,   0, "indx", &that.index },
	{CFG_S,   0, "path",  that.path	 },
	{CFG_S,   0, "mask",  that.spec	 },
	{CFG_D,   0, "xrel", &that.px	 },
	{CFG_L,   0, "yrel", &that.py	 }, 
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save one open directory window
 */

TYP_WINDOW *wd_that(WINFO *wi, SINFO2 *that);

CfgNest dir_one
{
	if (io == CFG_SAVE)
	{
		int i = 0;
		DIR_WINDOW *dw;

		/* Identify window's index in WINFO by pointer to that window */

		while (dirwindows[i].typ_window != (TYP_WINDOW *)that.w)
			i++;

		dw = (DIR_WINDOW *)(dirwindows[i].typ_window);
		that.index = i;
		that.px = dw->px;
		that.py = dw->py;
		strsncpy(that.path, dw->path, sizeof(that.path));
		strsncpy(that.spec, dw->fspec, sizeof(that.spec));
		
		*error = CfgSave(file, dirw_table, lvl, CFGEMP);
	}
	else
	{
		memclr(&that, sizeof(that));
		*error = CfgLoad(file, dirw_table, (int)sizeof(LNAME), lvl); 

		if ( (*error == 0 ) && (that.path[0] == 0 || that.spec[0] == 0 || that.index >= MAXWINDOWS) )
			*error = EFRVAL;

		if (*error == 0)
		{
			char *path = malloc(strlen(that.path) + 1);
			char *spec = malloc(strlen(that.spec) + 1);

			if (path && spec)
			{
				strcpy(path, that.path);
				strcpy(spec, that.spec);

				/* 
				 * Window's index in WINFO is always taken from the file;
				 * Otherwise, position and size would not be preserved
				 * Note: in case of error path & spec are deallocated in dir_do_open 
				 */

				dir_do_open
				(
					&dirwindows[that.index],
					path,
					spec,
					that.px,
					that.py,
					error
				);

				/* If there is an error, display an alert */

				wd_checkopen(error);
			}
			else
			{
				free(path);
				free(spec);
				*error = ENSMEM;
			}
		}
	}
}


CfgNest dir_config
{
	if ( io == CFG_LOAD )
	{
		memclr(&thisw, sizeof(thisw));
		memclr(&that, sizeof(that));
	}

	cfg_font = &dir_font;
	wtype_table[0].s = "directories";
	thisw.windows = &dirwindows[0];

	*error = handle_cfg(file, wtype_table, lvl, CFGEMP, io, NULL, NULL);
}


/********************************************************************
 *																	*
 * Funkties voor items in directory windows.						*
 *																	*
 ********************************************************************/

/* 
 * Routine voor het vinden van een element. x en y zijn de
 * coordinaten van de muis. Het resultaat is -1 als er geen
 * item is aangeklikt. 
 */

static int dir_find(WINDOW *w, int x, int y)
{
	int 
		hx,	
		hy, 
		cw,
		ch,
		item;

	RECT 
		r1, 
		r2, 
		work;

	DIR_WINDOW 
		*dw;

	/*
	 * Don't do anything in iconified window
	 * (and perhaps this is not even needed ?) 
	 */

	if ( (((TYP_WINDOW *)w)->winfo)->flags.iconified != 0 ) 	
		return -1; 

	dw = (DIR_WINDOW *)w;

	xw_getwork(w, &work);
	wd_cellsize((TYP_WINDOW *)w, &cw, &ch);

	hx = (x - work.x) / cw;
	hy = (y - work.y) / ch;

	if ( hx < 0 || hy < 0 ) 
		return -1;

	if (options.mode == TEXTMODE)
	{
		hx = max(0, (hx - TOFFSET + dw->px)/(dw->llength + CSKIP));
		item = hx * dw->nlines + hy + (int) dw->py;

		if (item >= dw->nvisible)
			return -1;

		dir_comparea(dw, item, &r1, &work);
		r2 = r1;
	}
	else
	{
		int 
			columns = dw->columns; 

		if ( hx >= columns )
			return -1;

		item = hx + dw->px + (dw->py + hy) * columns;

		if (item >= dw->nvisible)
			return -1;

		icn_comparea(dw, item, &r1, &r2, &work);
	}

	if (inrect(x, y, &r1))
		return item;
	else if (inrect(x, y, &r2))
		return item;
	else
		return -1;
}


/* 
 * Funktie die aangeeft of een directory item geselecteerd is. 
 */

static boolean dir_state(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long)item]->selected;
}


/* 
 * Funktie die het type van een directory item teruggeeft. 
 */

static ITMTYPE dir_itmtype(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long)item]->item_type;
}



static ITMTYPE dir_tgttype(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long)item]->tgt_type;
}


static int dir_icon(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long)item]->icon;
}


/* 
 * Funktie die de naam van een directory item teruggeeft. 
 */

static const char *dir_itmname(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->name;
}


/*
 * Return pointer to a string with full path + name of an item in a window.
 * Note: this routine allocates space for the fullname string
 */

static char *dir_fullname(WINDOW *w, int item)
{
	NDTA *itm = (*((DIR_WINDOW *) w)->buffer)[(long) item];

	if (itm->item_type == ITM_PREVDIR)
		return fn_get_path(((DIR_WINDOW *) w)->path);
	else
		return fn_make_path(((DIR_WINDOW *) w)->path, itm->name);
}


/* 
 * Funktie die de dta van een directory item teruggeeft. 
 */

static int dir_attrib(WINDOW *w, int item, int mode, XATTR *attr)
{
	NDTA *itm = (*((DIR_WINDOW *)w)->buffer)[(long) item];
	char *name;
	int error;

	if ((name = x_makepath(((DIR_WINDOW *) w)->path, itm->name, &error)) == NULL)
		return error;
	else
	{
		error = (int)x_attr(mode, ((DIR_WINDOW *)w)->fs_type, name, attr);
		free(name);
		return error;
	}
}


/*
 * Is the item perhaps a link?
 */

static boolean dir_islink(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->link;
}


/* 
 * Funktie voor het zetten van de nieuwe status van de items in een
 * window 
 */

static void dir_setnws(DIR_WINDOW *w, boolean draw)
{
	long i, hh, bytes = 0;
	int n = 0;
	RPNDTA *pb;

	hh = w->nvisible;
	pb = w->buffer;

	for (i = 0; i < hh; i++)
	{
		NDTA *h = (*pb)[i];

		h->selected = h->newstate;
		if (h->selected)
		{
			n++;
			bytes += h->attrib.size;
		}
	}

	if ( draw )
		dir_info(w, n, bytes);

	w->nselected = n;
}


/* 
 * Funktie voor het selecteren en deselecteren van items waarvan
 * de status veranderd is. 
 * See also dsk_drawsel() ! Functionality is somewhat duplicated
 * for the sake of colour icons when the background object has to be
 * redrawn as well. An attempt is made to optimize the drawing for speed.
 */

#define MSEL 8 /* if so many icons change state, draw complete window */

static void dir_drawsel(DIR_WINDOW *w)
{
	long i, h;
	RPNDTA *pb = w->buffer;
	RECT r1, r2, d, work;


	xw_getwork((WINDOW *)w, &work);
	wind_update(BEG_UPDATE);
	moff_mouse();

	if (options.mode == TEXTMODE)
	{
		/* Display as text */

		h = dir_pymax(w);
		xw_getfirst((WINDOW *) w, &r2);

		while (r2.w != 0 && r2.h != 0)
		{
			xd_clip_on(&r2);
			for (i = w->py; i < h; i++)
			{
				NDTA *b =(*pb)[i];
				if (b->selected != b->newstate)
				{
					dir_comparea(w, (int)i, &r1, &work);
					if (xd_rcintersect(&r1, &r2, &d))
						invert(&d);
				}
			}

			xd_clip_off();
			xw_getnext((WINDOW *)w, &r2);
		}
	}
	else
	{
		/* Display as icons */ 

		OBJECT 
			*tree;
		int 
			j,
			j0 = w->py * w->columns + w->px, 
			n = 0, 
			ncre;

		NDTA 
			*b;

		boolean
			all = TRUE,		/* draw all icons at once */ 
			sm = TRUE;		/* don't draw background and unchanged icons */

		/* Attempt to optimize (slow) redraw of animated colour icons */

		if ( colour_icons )
		{
			/* Count how many icons have to be redrawn */

			ncre = (int)count_ic(w, (int)w->py, w->rows);

			for ( j = 0; j < ncre; j++ )
			{
				b =(*pb)[j + j0];
				if ( b->selected != b->newstate )
					n++;
			}

			/* In which way to draw ? */

			if ( n > MSEL )
				sm = FALSE;		/* this will make all icons in the tree */
			else
				all = FALSE;	/* this will draw icons one by one */				
		}

		/* Create icon objects tree  */

		if ((tree = make_tree(w, w->px, w->ncolumns, w->py, w->rows, sm, &work)) == NULL)
			return;

		/* Now draw it */

		if ( colour_icons )
		{
			int oi = 1; 

			/* Background object will always be visible for colour icons */

			tree[0].ob_type = G_BOX;

			/* 
			 * Attempt to draw as quickly as possible, depending on 
			 * the number of icons selected.
			 * Note: this routine must agree with what was
			 * created in make_tree().
			 */

			for ( j = 0; j < ncre; j++ )
			{
				b =(*pb)[j + j0];

				if ( n > MSEL )
					tree[j + 1].ob_state = (b->newstate) ? SELECTED : NORMAL;
				else
				{
					/* draw all that changed state */

					if (b->selected != b->newstate)
					{
						xd_objrect(tree, oi++, &r2); /* icon x & y */
						draw_icrects( (WINDOW *)w, tree, &r2 );
					}
				}
			}
		}

		if ( all )
			draw_icrects( (WINDOW *)w, tree, &work );

		free(tree);
	}

	mon_mouse();
	wind_update(END_UPDATE);
}


/*
 * Funktie voor het selecteren van een item in een window.
 * mode = 0: selecteer selected en deselecteer de rest.
 * mode = 1: inverteer de status van selected.
 * mode = 2: deselecteer selected.
 * mode = 3: selecteer selected.
 * mode = 4: select all (except parent directory). 
 */

static void dir_select(WINDOW *w, int selected, int mode, boolean draw)
{
	long i, h;
	RPNDTA *pb;

	/* Don't do anything in iconified window */

	if ( xw_exist(w) && (((TYP_WINDOW *)w)->winfo)->flags.iconified != 0 ) 	
		return; 

	h = ((DIR_WINDOW *) w)->nvisible;
	pb = ((DIR_WINDOW *) w)->buffer;

	if (pb == NULL)
		return;

	for (i = 0; i < h; i++)
	{
		NDTA *b = (*pb)[i];
		changestate(mode, &(b->newstate), i, selected, b->selected, (b->item_type == ITM_PREVDIR) ? b->selected : TRUE);
	}

	if (mode == 1)
		(*pb)[selected]->newstate = ((*pb)[selected]->selected) ? FALSE : TRUE;

	if (draw)
		dir_drawsel((DIR_WINDOW *) w);
	dir_setnws((DIR_WINDOW *) w, draw);
}


/* 
 * Funktie, die de index van de eerste file die zichtbaar is,
 * teruggeeft. Ook wordt het aantal teruggegeven.
 * Get the index of the first visible items, and the number of
 * items visible
 */

static void calc_vitems
(
	DIR_WINDOW *dw,	/* pointer to window */ 
	int *s,			/* index of first item */ 
	int *n			/* number of visible items */
)
{
	int h;

	if (options.mode == TEXTMODE)
	{
		*s = (int)dw->py;
		h = dir_pymax(dw) - *s;
	}
	else
	{
		int columns = dw->columns;

		*s = columns * dw->py;
		h = columns * dw->rows;
	}

	if ((*s + h) > dw->nvisible)
		h = dw->nvisible - *s;

	*n = h;
}


/* 
 * Rubberbox funktie met scrolling.
 * This is for directory windows only. See also icon.c 
 * Note1: window can be scrolled in x and y simultaneously
 * Note2: if set_rect_default() is removed, rubberbox will not be correctly
 * redrawn when a window is scrolled.
 */

static void rubber_box(DIR_WINDOW *w, RECT *work, int x, int y, RECT *r)
{
	int 
		x1 = x,
		x2 = x, 
 		wx = x,
		ox = x, 
		y1 = y,  
		y2 = y, 
		oy = y, 
		hy = y,
		kstate, 
		deltax, 
		deltay, 
		absdeltax, 
		absdeltay,
		scrollx, 
		scrolly, 
		rx, 
		ry;

	boolean 
		released, 
		redraw;


	wd_cellsize((TYP_WINDOW *)w, &absdeltax, &absdeltay);

	start_rubberbox();
	draw_rect(x1, y1, x2, y2);

	rx = work->x + work->w;
	ry = work->y + work->h;

	do
	{
		redraw = FALSE;
		scrollx = 0;
		scrolly = 0;

		released = xe_mouse_event(0, &x2, &y2, &kstate);

		/* Box should not exceed window border */

		x2 = minmax( work->x, x2, rx );
		y2 = minmax( work->y, y2, ry );

		if ((x2 <= work->x) && (w->px > 0) && (x1 < INT_MAX - absdeltax) )
		{
			redraw = TRUE;
			scrollx = WA_LFLINE;
			deltax = absdeltax;
		}

		if ((y2 <= work->y) && (w->py > 0) && (y1 < INT_MAX - absdeltay) )
		{
			redraw = TRUE;
			scrolly = WA_UPLINE;
			deltay = absdeltay;
		}

		if ((x2 >= rx) && ((w->px + w->ncolumns) < w->columns) && (x1 > -INT_MAX + absdeltax) )
		{
			redraw = TRUE;
			scrollx = WA_RTLINE;
			deltax = -absdeltax;
		}

		if ((y2 >= ry) && ((w->py + w->nrows) < w->nlines) && (y1 > -INT_MAX + absdeltay) )
		{
			redraw = TRUE;
			scrolly = WA_DNLINE;
			deltay = -absdeltay;
		}

		if ((released) || (x2 != ox) || (y2 != oy))
			redraw = TRUE;

		if (redraw)
		{
/* not needed ?
			set_rect_default();
*/
			draw_rect(wx, hy, ox, oy);

			if (scrollx)
			{
				x1 += deltax; 
				wx = minmax( work->x, x1, rx );
				w_scroll((TYP_WINDOW *)w, scrollx);
			}

			if (scrolly)
			{
				y1 += deltay;
				hy = minmax( work->y, y1, ry );
				w_scroll((TYP_WINDOW *)w, scrolly);
			}

			if (!released)
			{
				set_rect_default();			/* this call is needed! */
				draw_rect(wx, hy, x2, y2); 
				ox = x2;
				oy = y2;
			}
		}
	}
	while (!released);

	rubber_rect(x1, x2, y1, y2, r);
}


/* 
 * Funktie voor het selecteren van items die binnen een bepaalde
 * rechthoek liggen. 
 * Select items catched in a rubberbox.
 */

static void dir_rselect(WINDOW *w, int x, int y)
{
	long 
		i, h;

	RECT 
		r1, r2, r3, work;

	RPNDTA 
		*pb;


	/* Don't do anything in an iconified window */

	if ((((TYP_WINDOW *)w)->winfo)->flags.iconified != 0  ) 
		return;

	xw_getwork(w, &work);
	rubber_box((DIR_WINDOW *) w, &work, x, y, &r1);

	h = (long)(((DIR_WINDOW *) w)->nvisible);
	pb = ((DIR_WINDOW *) w)->buffer;

	for (i = 0; i < h; i++ )
	{
		NDTA *b = (*pb)[i];

		if (options.mode == TEXTMODE)
		{
			dir_comparea((DIR_WINDOW *) w, (int)i, &r2, &work);
			if (rc_intersect2(&r1, &r2))
				b->newstate = (b->selected) ? FALSE : TRUE;
		}
		else
		{
			icn_comparea((DIR_WINDOW *) w, (int)i, &r2, &r3, &work);
	
			if (rc_intersect2(&r1, &r2))
				b->newstate = (b->selected) ? FALSE : TRUE;
			else if (rc_intersect2(&r1, &r3))
				b->newstate = (b->selected) ? FALSE : TRUE;
			else
				b->newstate = b->selected;
		}
	}

	dir_drawsel((DIR_WINDOW *) w);
	dir_setnws((DIR_WINDOW *) w, TRUE);
}


static void get_itmd(DIR_WINDOW *wd, int obj, ICND *icnd, int mx, int my, RECT *work)
{
	RECT ir;

int *icndcoords = &icnd->coords[0];

	if (options.mode == TEXTMODE)
	{
		icnd->item = obj;
		icnd->np = 5;
		dir_comparea(wd, obj, &ir, work);

		ir.x -= mx;
		ir.y -= my;
		ir.w = DRAGLENGTH * dir_font.cw - 1;
		ir.h -= 1;
		*icndcoords++ = ir.x;
		*icndcoords++ = ir.y;
		*icndcoords++ = ir.x;
		*icndcoords++ = ir.y + ir.h;
		*icndcoords++ = ir.x + ir.w;
		*icndcoords++ = icnd->coords[3] /* ir.y + ir.h */ ;
		*icndcoords++ = icnd->coords[4] /* ir.x + ir.w */ ;
		*icndcoords++ = ir.y;
		*icndcoords++ = ir.x + 1;
		*icndcoords = ir.y;
	}
	else
	{
		int 
			columns = wd->columns, 
			s;

		RECT 
			tr;

		s = obj - (int)(wd->py * columns);

		icn_comparea(wd, obj, &ir, &tr, work);

		ir.x -= mx;
		ir.y -= my;
		ir.w -= 1;
		tr.x -= mx;
		tr.y -= my;
		tr.w -= 1;
		tr.h -= 1;

		icnd->item = obj;
		icnd->np = 9;

		icnd->m_x = work->x + (s % columns - wd->px) * ICON_W + ICON_W / 2 - mx;
		icnd->m_y = work->y + (s / columns) * ICON_H + ICON_H / 2 - my;

		icn_coords(icnd->coords, &tr, &ir); 
	}
}


/*
 * Create a list of (visible ?) selected items
 * This routine will return FALSE and a NULL list pointer
 * if no list has been created.
 * Note: at most 32767 items can be returned because 'nselected'  and
 * 'nvisible' are 16 bits long
 */

static boolean get_list(DIR_WINDOW *w, int *nselected, int *nvisible, int **sel_list)
{
	int 
		j = 0, 
		n = 0, 
		nv = 0, 
		*list, 
		s,		/* first visible item */ 
		h;		/* number of visible items */

	long 
		i, 
		m;

	RPNDTA 
		*d;

	d = w->buffer;
	m = w->nvisible;

	calc_vitems(w, &s, &h);
	h += s;

	for (i = 0; i < m; i++)
	{
		if ((*d)[i]->selected)
		{
			n++;
			if (((int)i >= s) && ((int)i < h))
				nv++;
		}
	}

	*nvisible = nv;
	*nselected = n;

	if (n == 0)
	{
		*sel_list = NULL;
		return FALSE;
	}

	list = malloc_chk(n * sizeof(int));

	*sel_list = list;

	if (list != NULL)
	{
		for (i = 0; i < m; i++)
		{
			if ((*d)[i]->selected)
				list[j++] = (int)i;
		}
		return TRUE;
	}

	return FALSE;
}


/* 
 * Routine voor het maken van een lijst met geselekteerde items. 
 */

static boolean dir_xlist(WINDOW *w, int *nselected, int *nvisible,
						 int **sel_list, ICND **icns, int mx, int my)
{
	int j = 0;
	long i;
	ICND *icnlist;
	RPNDTA *d;
	RECT work;

	xw_getwork(w, &work);

	if (get_list((DIR_WINDOW *) w, nselected, nvisible, sel_list) == FALSE)
		return FALSE;

	if (nselected == 0)
	{
		*icns = NULL;
		return TRUE;
	}

	icnlist = malloc_chk(*nvisible * sizeof(ICND));
	*icns = icnlist;

	if (icnlist != NULL)
	{
		int s, n, h;

		calc_vitems((DIR_WINDOW *) w, &s, &n);
		h = s + n;
		d = ((DIR_WINDOW *) w)->buffer;
		for (i = s; (int) i < h; i++)
			if ((*d)[i]->selected)
				get_itmd((DIR_WINDOW *) w, (int) i, &icnlist[j++], mx, my, &work);

		return TRUE;
	}

	free(*sel_list);
	return FALSE;
}


/* 
 * Routine voor het maken van een lijst met geseleketeerde items. 
 * Note: at most 32767 items can be returned because 'n' is 16 bits long
 */

static boolean dir_list(WINDOW *w, int *n, int **list)
{
	int dummy;
	return get_list((DIR_WINDOW *)w, n, &dummy, list);
}


static boolean dir_open(WINDOW *w, int item, int kstate)
{
	char *newpath;

	if ((dir_itmtype(w, item) == ITM_FOLDER) && ((kstate & 8) == 0))
	{
		if ((newpath = dir_fullname(w, item)) != NULL)
		{
			free(((DIR_WINDOW *) w)->path);
			((DIR_WINDOW *) w)->path = newpath;
			dir_readnew((DIR_WINDOW *) w);
		}

		return FALSE;
	}
	else if ((dir_itmtype(w, item) == ITM_PREVDIR) && ((kstate & 8) == 0))
	{
		dir_close(w, 0);
		return FALSE;
	}
	else
		return item_open(w, item, kstate, NULL, NULL);
}


static boolean dir_copy(WINDOW *dw, int dobject, WINDOW *sw, int n,
						int *list, ICND *icns, int x, int y, int kstate)
{
	return item_copy(dw, dobject, sw, n, list, kstate);
}


/* 
 * Simulate (partially) an open directory window containing just one item 
 * so that some operations can be executed using existing routines
 * which expect an item selected in a directory window.
 * Note: this window will have a NULL pointer to WINFO structure, and so
 * it can be distinguished from a real window.
 * Also, 'sim window' flag is set.
 * This routine does -not- allocate or deallocate any memory which
 * has to be freed later.
 */

void dir_simw(DIR_WINDOW **dwa, char *path, char *name, ITMTYPE type, size_t size, int attrib)
{
	static DIR_WINDOW 
		dw;

	static NDTA
		buffer,
		*pbuffer[1]; 

	memclr(&dw, sizeof(DIR_WINDOW));
	memclr(&buffer, sizeof(NDTA));

	((WINDOW *)&dw)->xw_type = DIR_WIND;	/* this is a directory window */
	((WINDOW *)&dw)->xw_xflags |= XWF_SIM;	/* a simulated one */
	dw.path = path;							/* path of this dir */
	dw.itm_func = &itm_func;				/* pointers to specific functions */
	dw.nfiles = 1;							/* number of files in window */
	dw.nvisible = 1;						/* one visible item */
	dw.buffer = &pbuffer;					/* location of NDTA pointer array */
	*dwa = &dw;								/* DIR_WINDOW pointer address */

	pbuffer[0] = &buffer;					/* point to buffer */

	buffer.name = name;						/* filename */
	buffer.item_type = type;				/* type of item in the dir */
	buffer.attrib.size = size;				/* file size */
	buffer.attrib.attrib = attrib;			/* file attributes */

	/* Now "select" the one and only object in this "window" */

	dir_select( (WINDOW *)&dw, 0, 3, FALSE);
}


/*
 * Try to find what item type should be assigned to a name,
 * so that this item can be opened in the appropriate way.
 * This routine is used in open.c, and only after
 * the target of a possible link has been determined.
 * It is also used in icon.c
 */

ITMTYPE diritem_type(char *fullname )
{
	XATTR attr;

	attr.attr = 0;

	/* Is this name that of a volume root directory ? */

	if ( isroot(fullname) )
		return ITM_DRIVE;

	if ( strcmp(fn_get_name(fullname), prevdir) == 0 )
		return ITM_PREVDIR;

	/* Get info on the object itself, don't follow links */

	if ( x_attr(  1, FS_INQ, fullname, &attr )  >= 0 )
	{
		if ( prg_isprogram(fn_get_name(fullname)) )
			return ITM_PROGRAM;
		if ( attr.attr & FA_SUBDIR ) 
			return ITM_FOLDER;
		else if ( !( attr.attr & FA_VOLUME) )
			return ITM_FILE;
	}
	else
		alert_iprint(MNOITEM); /* item not found or item type unknown */

	return ITM_NOTUSED;
}