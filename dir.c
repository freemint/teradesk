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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <boolean.h>
#include <library.h>
#include <mint.h>
#include <xdialog.h>
#include <xscncode.h>

#include "desk.h"
#include "sprintf.h"		/* get rid of stream io */
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h" 		/* moved before dir.h */
#include "dir.h"
#include "error.h"
#include "events.h"
#include "file.h"
#include "lists.h"
#include "slider.h" 
#include "filetype.h"
#include "icon.h"
#include "resource.h"
#include "screen.h"
#include "showinfo.h"
#include "copy.h"
#include "icontype.h"
#include "open.h"
#include "va.h"

/* In algol 68 its so easy.
   ref to row of ref to NDTA
   ref () ref NDTA
*/


#define TOFFSET			2
#define ICON_W			80			/* icon width (pixels) */
#define ICON_H			46			/* icon height (pixels) */

#define MAXLENGTH		128			/* Maximum length of a filename to display in a window. HR: 128 */
#define MINLENGTH		12			/* Minimum length of a filename to display in a window. */
#define ITEMLEN			44			/* Length of a line, without the filename. */


#define TOSITEMLEN			51				/* Length of a line. */

#define SIZELEN 8 /* length of field for file size, i.e. max 99999999 ~100 MB */


#define bold 1
#define light 2
#define italic 4
#define ulined 8
#define white 16


WINFO dirwindows[MAXWINDOWS]; 

RECT dmax;
static char prevdir[] = "..";
FONT dir_font;

extern SEL_INFO selection; 

extern boolean TRACE;

static void dir_closed(WINDOW *w);
extern boolean prg_isprogram(const char *name);

static WD_FUNC dir_functions =
{
	wd_type_hndlkey,
	wd_type_hndlbutton,
	wd_type_redraw,
	wd_type_topped,
	wd_type_topped,
	dir_closed,
	wd_type_fulled,
	wd_type_arrowed,
	wd_type_hslider,
	wd_type_vslider,
	wd_type_sized,
	wd_type_moved,
	0L, /* hndlmenu placeholder */
	/* wd_type_top, */ 0L,
	wd_type_iconify,
	wd_type_uniconify
};

static int		itm_find	(WINDOW *w, int x, int y);
static boolean	itm_state	(WINDOW *w, int item);
static ITMTYPE	itm_type	(WINDOW *w, int item);
static ITMTYPE	itm_tgttype	(WINDOW *w, int item);
static int		itm_icon	(WINDOW *w, int item);
static const
       char *	itm_name	(WINDOW *w, int item);
static char *	itm_fullname(WINDOW *w, int item);
static int		itm_attrib	(WINDOW *w, int item, int mode, XATTR *attrib);
static boolean	dir_islink	(WINDOW *w, int item);

static boolean	dir_open	(WINDOW *w, int item, int kstate);
static boolean	dir_copy	(WINDOW *dw, int dobject, WINDOW *sw, int n, int *list, ICND *icns, int x, int y, int kstate);
static void		itm_select	(WINDOW *w, int selected, int mode, boolean draw);
static void		itm_rselect	(WINDOW *w, int x, int y);
static boolean	itm_xlist	(WINDOW *w, int *nselected, int *nvisible, int **sel_list, ICND **icns, int mx, int my);
static boolean	itm_list	(WINDOW *w, int *n, int **list);
static void		dir_set_update	(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);
static void		dir_do_update	(WINDOW *w);
static void		dir_filemask	(WINDOW *w);
static void		dir_newfolder	(WINDOW *w);
static void		dir_disp_mode	(WINDOW *w, int mode);
static void		dir_sort		(WINDOW *w, int sort);
static void		dir_fields		(WINDOW *w, int attrib);
static void		dir_seticons	(WINDOW *w);

static ITMFUNC itm_func =
{
	itm_find,					/* itm_find */
	itm_state,					/* itm_state */
	itm_type,					/* itm_type */
	itm_tgttype,				/* target item type */
	itm_icon,					/* itm_icon */
	itm_name,					/* itm_name */
	itm_fullname,				/* itm_fullname */
	itm_attrib,					/* itm_attrib */
	dir_islink,

	dir_open,					/* itm_open */
	dir_copy,					/* itm_copy */
	item_showinfo,				/* itm_showinfo */

	itm_select,					/* itm_select */
	itm_rselect,				/* itm_rselect */
	itm_xlist,					/* itm_xlist */
	itm_list,					/* itm_list */

	dir_path,					/* wd_path */
	dir_set_update,				/* wd_set_update */
	dir_do_update,				/* wd_do_update */
	dir_filemask,				/* wd_filemask */
	dir_newfolder,				/* wd_newfolder */
	dir_disp_mode,				/* wd_disp_mode */
	dir_sort,					/* wd_sort */
/*	dir_attrib,			*/		/* wd_attrib */ 
	dir_fields,					/* wd_fields  HR 240203  */
	dir_seticons				/* wd_seticons */
};


/********************************************************************
 *																	*
 * Funkties voor het sorteren van directories.						*
 *																	*
 ********************************************************************/


typedef int SortProc(NDTA **ee1, NDTA **ee2);
#define E1E2 NDTA *e1 = *ee1, *e2 = *ee2
 
/* Sorteer op zichtbaarheid */

static SortProc _s_visible
{
	E1E2;

	if ((e1->visible == TRUE) && (e2->visible == FALSE))
		return -1;
	if ((e2->visible == TRUE) && (e1->visible == FALSE))
		return 1;
	return 0;
}


/* Sorteer op folder of file. */

static SortProc _s_folder
{
	int h;
	E1E2;

	if ((h = _s_visible(ee1, ee2)) != 0)
		return h;
	if ((e1->attrib.attrib & 0x10) && ((e2->attrib.attrib & 0x10) == 0))
		return -1;
	if ((e2->attrib.attrib & 0x10) && ((e1->attrib.attrib & 0x10) == 0))
		return 1;
	return 0;
}


/* 
 * Function for sorting directory by filename 
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
 * Function for sorting directory by filename extension
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
		if ((h = strcmp(ext1 + 1, ext2 + 1)) != 0)
			return h;
	}
	return strcmp(e1->name, e2->name);
}


/* 
 * Function for sorting directory by file length 
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
 * Function for sorting directory by file date
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
 * Function for sorting of directory by desired key
 */

static void sort_directory(RPNDTA *buffer, int n, int sort)
{
	SortProc *sortproc;

	switch (sort)
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

	qsort(buffer, n, sizeof(size_t), sortproc);
}


static void dir_sort(WINDOW *w, int sort)
{
	sort_directory(((DIR_WINDOW *) w)->buffer, ((DIR_WINDOW *) w)->nfiles, sort);
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
	char *s;

	if (n > 0)
	{
		char *msg;

		rsrc_gaddr(R_STRING, (n == 1) ? MISINGUL : MIPLURAL, &s);
		rsrc_gaddr(R_STRING, MSITEMS, &msg);
		sprintf(w->info, msg, bytes, n, s);
	}
	else
	{
		char *msg;

		rsrc_gaddr(R_STRING, (w->nvisible == 1) ? MISINGUL : MIPLURAL, &s);
		rsrc_gaddr(R_STRING, MITEMS, &msg);
		sprintf(w->info, msg, w->visbytes, w->nvisible, s);

	}

	xw_set((WINDOW *) w, WF_INFO, w->info);
}


/* 
 * Create the title of a directory window.
 * See also txt_title() in viewer.c
 */

void dir_title(DIR_WINDOW *w)
{
	int columns;

	char
		*fulltitle;

	/* 
	 * How long can the title be? 
	 * Note: "-5" takes into account window gadgets left/right of the title 
	 */

	columns = min( w->scolumns - 5, (int)sizeof(w->title) );

	/* Create path + name */

	fulltitle = fn_make_path(w->path, w->fspec);

	/* Cramp it to fit window */

	cramped_name(fulltitle, w->title, columns);
	free(fulltitle);

	/* Set window */

	xw_set((WINDOW *) w, WF_NAME, w->title);
}


static void dir_free_buffer(DIR_WINDOW *w) 
{
	long i;
	RPNDTA *b = w->buffer;

	if (b != NULL)
	{
		for (i = 0; i < w->nfiles; i++)
		{
			free((*b)[i]);			/* free NDTA + name */
		}
		free(b);				/* free pointer array */
		w->buffer = NULL;
	}
}


/*
 * Set directory items as visible (or otherwise)
 */

static void set_visible(DIR_WINDOW *w)
{
	long i, v = 0L;
	int n = 0;


	if (w->buffer == NULL)
		return;

	for (i = 0; i < w->nfiles; i++)
	{
		NDTA *b = (*w->buffer)[i];

		b->selected = FALSE;
		b->newstate = FALSE;

		if (   (   (options.attribs  & FA_HIDDEN) != 0 /* permit hidden */
		        || (b->attrib.attrib & FA_HIDDEN) == 0 /* or item is not hidden */
		       )
		    && (   (options.attribs  & FA_SYSTEM) != 0 /* permit system */
		        || (b->attrib.attrib & FA_SYSTEM) == 0 /* or item is not system */
		       )
		    && (   (options.attribs  & FA_SUBDIR) != 0 /* permit subdirectory */
		        || (b->attrib.attrib & FA_SUBDIR) == 0 /* or item is not subdirectory */
		       )
		    && (   (options.attribs  & FA_PARDIR) != 0 /* permit parrent dir */  
				|| strcmp(b->name, prevdir) != 0       /* or item is not parent dir */ 
		       )
		    && (   (b->attrib.mode & S_IFMT)       == S_IFDIR /* permit directory */
			    || cmp_wildcard(b->name, w->fspec) == TRUE /* or mask match */
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
}


/* 
 * Funktie voor het copieren van een dta naar de buffer van een window. 
 */

static int copy_DTA(NDTA **dest, char *name, XATTR *src, XATTR *tgt, int index, bool link)	/* link */
{
	long l = 0;
	NDTA *new;

	l = strlen(name);

/*	
	alert_msg("DTA: l = %ld | name = %s", l, name);
*/	
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
				new->tgt_type = (strcmp(name, prevdir) == 0) ? ITM_PREVDIR : ITM_FOLDER;
			else
				new->tgt_type = ITM_FILE;
		}
		else
#endif
		{
			if ((src->mode & S_IFMT) == S_IFDIR)
				new->item_type = (strcmp(name, prevdir) == 0) ? ITM_PREVDIR : ITM_FOLDER;
			else
				new->item_type = ITM_FILE;
		
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
			new->icon = icnt_geticon(name, new->tgt_type, new->link);

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
	long memused = 0;
	int error = 0, maxl = 0;
	long length, bufsiz, n;
	XATTR 
		tgtattr = {0},
		attr = {0};

	XDIR *dir;
   
	n = 0;
	length = 0;

	dir_free_buffer(w); /* clear and release old buffer, if it exists */

	/* HR we only prepare a pointer array :-) */

	bufsiz = (long)v3_options.max_dir;		/* thats how many pointers we allocate */

	w->buffer = malloc(bufsiz*sizeof(size_t));

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

					if (n == bufsiz)		/* don't violate allocated amount */
					{
						/* Allocate a 50% bigger buffer, then copy contents */

						void *newb;

						bufsiz = 3 * bufsiz / 2;	/* HR 120803 expand pointer array mildly exponentially */
						newb = malloc(bufsiz * sizeof(size_t));
						if (newb)
						{
							/* Copy to new buffer, free old, point to new */
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
						 * Why get target? So that items get sorted as they should:
						 * links to folders will be sorted among folders, etc.
 						 */

						char *fulln = fn_make_path(dir->path, name);
						x_attr(0, fulln, &tgtattr); /* follow the link to show attributes */
						free(fulln);
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
					    error = copy_DTA(*w->buffer + n, name, &attr, &tgtattr, n, link);

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
				} /* err = 0 ? */
			} /* while ... */

			x_closedir(dir);
		}

		if ((error == ENMFIL) || (error == EFILNF))
			error = 0;
	}
	else
		/* Can't allocate poiners for the new directory */
		error = ENSMEM;

	/* Number of files, total size, maximum name length */

	w->nfiles = (int) n;
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
		sort_directory(w->buffer, (int) n, options.sort);
	}

	return error;
}


/* 
 * Set information to be displayed in window title and info line
 */

static void dir_setinfo(DIR_WINDOW *w)
{
	w->nselected = 0;
	w->refresh = FALSE;

	calc_nlines(w);

	dir_title(w);
	dir_info(w, 0, 0L);
	set_sliders((TYP_WINDOW *)w); 
}


/* 
 * Funktie voor het inlezen van een directory in de buffer van een window. 
 */

static int dir_read(DIR_WINDOW *w)
{
	int error;
	error = read_dir(w);
	dir_setinfo(w);

	return error;
}


void dir_refresh_wd(DIR_WINDOW *w)
{
	int error;

	graf_mouse(HOURGLASS, NULL);

	if ((error = dir_read(w)) != 0)
		xform_error(error);
	graf_mouse(ARROW, NULL);
	wd_type_draw ((TYP_WINDOW *)w, TRUE );
	wd_reset((WINDOW *) w);
}


/*
 * Remove a trailing backslash from a path,
 * except if the path is that of a root directory
 */

void dir_trim_slash ( char *path )
{
	char 
		*b = strrchr(path,'\\');

	if ( strcmp(b, "\\" ) == 0 && !isroot(path) )
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
 * Refresh a directory  window and return slider to 0
 */

void dir_readnew(DIR_WINDOW *w)
{
	w->py = 0;
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
	if (isroot(path) == TRUE)
		l++;
	if (strlen(path) != l)
		return FALSE;
	return (strncmp(path, fname, l) != 0) ? FALSE : TRUE;
}


/*
 * Mark a directory window for an update.
 */

static void dir_set_update(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2)
{
	DIR_WINDOW *dw;

	dw = (DIR_WINDOW *) w;

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
 * Also inform signed-on AV-clients that this directory 
 * should be updated.
 * Note: notification of AV-clients may slow down file copying.
 * If there are no AV-protocol clients, skip this copmpletely
 * to speed things up
 */

static void dir_do_update(WINDOW *w)
{
	if (((DIR_WINDOW *)w)->refresh == TRUE)
	{
		dir_refresh_wd((DIR_WINDOW *) w);

#if _MORE_AV
		if ( avclients )
			va_pathupdate( ((DIR_WINDOW *)w)->path);
#endif
	}
}


/* 
 * Unconditional refresh of a directory window 
 */

void dir_always_update(WINDOW *w)
{
	dir_refresh_wd( (DIR_WINDOW *)w );
}


/* 
 * Funktie voor het zetten van de mode van een window.
 * Display a directory window in text mode or icon mode 
 */

static void dir_disp_mode(WINDOW *w, int mode)
{
	RECT work;

	xw_get(w, WF_WORKXYWH, &work);
	calc_rc((TYP_WINDOW *)w, &work);
	calc_nlines((DIR_WINDOW *) w);
	set_sliders((TYP_WINDOW *)w);
	do_redraw(w, &work, TRUE); 		
}


/********************************************************************
 *																	*
 * Funkties voor het zetten van het file mask van een window.		*
 *																	*
 ********************************************************************/

static void dir_newdir(DIR_WINDOW *w)
{
	set_visible(w);
	wd_reset((WINDOW *) w);
	dir_setinfo(w);
	sort_directory(w->buffer, w->nfiles, options.sort);
	wd_type_draw ((TYP_WINDOW *)w, TRUE); 
}


static void dir_filemask(WINDOW *w)
{
	char *newmask;

	if ((newmask = wd_filemask(((DIR_WINDOW *) w)->fspec)) != NULL)
	{
		free(((DIR_WINDOW *) w)->fspec);
		((DIR_WINDOW *) w)->fspec = newmask;
		dir_newdir((DIR_WINDOW *) w);
	}
}


/*
 * Funkties voor het zetten van de file attributen van een window.	
 */

static void dir_fields(WINDOW *w, int attrib)
{
	set_visible((DIR_WINDOW *) w);
	dir_newdir((DIR_WINDOW *) w);
}


/*
 * Funktie voor het zetten van de iconen van objecten in een 
 * window.
 */

static void dir_seticons(WINDOW *w)
{
	RPNDTA *pb;
	long i, n;

	pb = ((DIR_WINDOW *) w)->buffer;
	n = ((DIR_WINDOW *) w)->nfiles;

	for (i = 0; i < n; i++)
	{
		NDTA *b = (*pb)[i];
		b->icon = icnt_geticon(b->name, b->tgt_type, b->link);
	}

	if (options.mode != TEXTMODE)
		wd_type_draw ((TYP_WINDOW *)w, FALSE );
}


/*
 * Funktie voor het maken van een nieuwe directory.
 */

static void dir_newfolder(WINDOW *w)
{
	int button, error;
	char *nsubdir;
	LNAME name;	
	DIR_WINDOW *dw;

	dw = (DIR_WINDOW *) w;

	cv_fntoform(&newfolder[DIRNAME], "\0");
	rsc_title(newfolder, NDTITLE, DTNEWDIR);
	newfolder[DIRNAME].ob_flags &= ~HIDETREE;
	newfolder[OPENNAME].ob_flags |= HIDETREE;

	button = xd_dialog(newfolder, DIRNAME);

	if ((button == NEWDIROK) && (*dirname))
	{
		cv_formtofn(name, &newfolder[DIRNAME]);

		/* Note: the following segment can be a separate routine */

		if ((error = x_checkname(dw->path, name)) == 0)
		{
			if ((nsubdir = fn_make_path(dw->path, name)) != NULL)
			{
				graf_mouse(HOURGLASS, NULL);
				error = x_mkdir(nsubdir);
				graf_mouse(ARROW, NULL);

				if (error == EACCDN)
					alert_iprint(MDEXISTS);
				else
					xform_error(error);

				if (error == 0)
				{
					wd_set_update(WD_UPD_COPIED, nsubdir, NULL);
					wd_do_update();
				}
				free(nsubdir);
			}
		}
		else
			xform_error(error);
	}
}


#if _MINT_
/*
 * Create a symbolic link
 * Note: appropriate file system is checked in wd_hndlmenu.
 * It is assumed that if fs is not tos, links can always
 * be created- which in fact may not always be true
 * and should better be checked
 */

void dir_newlink(WINDOW *w, char *target)
{
	int 
		button, 
		error;

	char
		*linkto,
		*targetname, 
		*linkname;

	LNAME 
		name;

	DIR_WINDOW 
		*dw;

	dw = (DIR_WINDOW *) w;

	rsrc_gaddr(R_STRING, TLINKTO, &linkto);
	targetname = fn_get_name(target);

	/* The string pointed to by dirname is a LNAME, therefore sizeof() below */

	strcpy(dirname, linkto);
	strncat(dirname, targetname, sizeof(LNAME) - strlen(linkto) - 1L ); 

	/* This will always be a scrolled editable text, so xd_init_shift can be simply applied */

	rsc_title(newfolder, NDTITLE, DTNEWLNK);
	newfolder[DIRNAME].ob_flags &= ~HIDETREE;
	newfolder[OPENNAME].ob_flags |= HIDETREE;
	xd_init_shift(&newfolder[DIRNAME], dirname);

	button = xd_dialog(newfolder, DIRNAME);

	if ((button == NEWDIROK) && (*dirname))
	{
		cv_formtofn(name, &newfolder[DIRNAME]);

		if ((error = x_checkname(dw->path, name)) == 0)
		{
			if ((linkname = fn_make_path(dw->path, name)) != NULL)
			{
				graf_mouse(HOURGLASS, NULL);
				error = x_mklink(linkname, target);
				graf_mouse(ARROW, NULL);
				xform_error(error);

				if (error == 0)
				{
					wd_set_update(WD_UPD_COPIED, linkname, NULL);
					wd_do_update();
				}
				free(linkname);
			}
		}
		else
			xform_error(error);
	}
}

#endif


/* 
 * Funktie voor het berekenen van het totale aantal regels in een window. 
 */

void calc_nlines(DIR_WINDOW *w)
{
	if (options.mode == TEXTMODE)
		w->nlines = w->nvisible;
	else
	{
		int columns = w->columns;

		w->nlines = (w->nvisible + columns - 1) / columns;
	}
}


/********************************************************************
 *																	*
 * Functions for setting the sliders.								*
 *																	*
 ********************************************************************/

/* 
 * Calculate the length of a line in a window. In the TOS version it
 * returns a constant, in the MiNT version it returns a value
 * depending on the length of the longest filename. 
 * Length of the line depends on which fields are shown
 */


int linelength(DIR_WINDOW *w)
{

	int l;						/* length of filename             */
	int f;						/* length of other visible fields */
  
	if (w->fs_type)				/* for TOS (8+3) filesystem */
		l = 12;					/* length of filename */
	else
	{	
		if (w->namelength < MINLENGTH)
			l = MINLENGTH;
		else if (w->namelength > MAXLENGTH)
			l = MAXLENGTH;
		else
			l = w->namelength;
	}
	
	/*
	 * Beside the filename there will always be:
	 * - 3 spaces for the character used to mark directories
	 * - 2 spaces separating filename from the rest
	 * - 2 additional spaces aftr each field as separators
	 * below should be: f = 5 = 3+2, but for some unclear reason
	 * hor.slider works better if one char less is specified here;
	 * left and right border are -not- included in calculation
	 */
	  
	f = 4;
	
	if ( options.V2_2.fields & WD_SHSIZ ) f = f + SIZELEN + 2;	/* size: 8 char + 2 sep */
	if ( options.V2_2.fields & WD_SHDAT ) f = f + 10; 			/* date: 8 char + 2 sep. */
	if ( options.V2_2.fields & WD_SHTIM ) f = f + 10; 			/* time: 8 char + 2 sep. */
	if ( options.V2_2.fields & WD_SHATT )             			/* attributes: depending on fs */
	{
#if _MINT_
		if ( mint  && ( w->fs_type == 0 )  )
			f = f + 12;	/* other (i.e.mint) filesystem: attributes use 10 char + 2 sep. */
		else
#endif
			f = f + 7;	/* TOS/FAT filesystem: attributes use 5 char + 2 sep. */
	} 
	
	return f + l;	
}


/* 
 * Funktie voor het zetten van de grootte van de verticale slider. 
 */


/********************************************************************
 *																	*
 * Functies voor het tekenen van de inhoud van een directorywindow.	*
 *																	*
 ********************************************************************/

/*
 * Bereken de rechthoek die de te tekenen tekst omsluit.
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
	int offset, delta;

	if (dw->px > TOFFSET)
	{
		offset = dw->px - TOFFSET;
		r->x = work->x;
	}
	else
	{
		offset = 0;
		r->x = work->x + (TOFFSET - dw->px) * dir_font.cw;
	}
	r->y = DELTA + work->y + (int) (item - dw->py) * (dir_font.ch + DELTA);
	r->w = dir_font.cw * (linelength(dw) - offset);
	if ((delta = r->x + r->w - work->x - work->w) > 0)
		r->w -= delta;
	r->h = dir_font.ch;
}


/*
 * Convert a time to a string.
 *
 * Parameters:
 *
 * tstr		- destination string
 * time		- current time in GEMDOS format
 *
 * this string is 8 characters long 
 *
 * Result: Pointer to the end of tstr.
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
 * Convert a date to a string.
 *
 * Parameters:
 *
 * tstr		- destination string
 * date		- current date in GEMDOS format
 *
 * this string is 8 characters long 
 *
 * Result: Pointer to the -end- of tstr.
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


/*
 * Macros used in dir_line.
 * FILL_UNTIL fills a string until a counter reaches a certain value.
 */

#define FILL_UNTIL(dest, i, value)			while (i < (value))	\
											{					\
												*dest++ = ' ';	\
												i++;			\
											}

/*
 * Make a string with the contents of a line in a directory window.
 */

static void dir_line(DIR_WINDOW *dw, char *s, int item, boolean clip)
{
	NDTA *h;
	char *d, t[16];
	const char *p;
	int i, lwidth;
	int parent;		/* flag that this is a .. (parent) dir */

	lwidth = linelength(dw);
	parent= FALSE;

	if (item < dw->nvisible)
	{
		h = (*dw->buffer)[(long)item];
		d = s; /* beginning of a directory line */

		*d++ = ' ';
		*d++ = (h->tgt_type == ITM_FOLDER || h->tgt_type == ITM_PREVDIR) ? '\007' : ' '; /* dir mark */
		*d++ = ' ';

		p = h->name;
		i = 0;


		/* This will modify display of ".." in all filesystems now */

		if (strcmp(h->name, prevdir) == 0)
			parent = TRUE;

#if _MINT_
		if (mint && dw->fs_type == 0)
		{
			while ((*p) && (i < MAXLENGTH))
			{
				*d++ = *p++;
				i++;
			}

	        FILL_UNTIL(d, i, dw->namelength) /* use correct value. */
		}
		else
#endif
		{	/* not mint fs */
			/* Copy the filename. Handle '..' as a special case. */
	
			if (strcmp(h->name, prevdir) != 0)
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
				/* '..'. Just copy. */
	
				/* parent= TRUE; */

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

		if ( options.V2_2.fields & WD_SHSIZ )	/* show file size? */
		{
			if ((h->attrib.mode & S_IFMT) != S_IFDIR) /* this is not a subdirectory */
			{
				int l;

				/* 
				 * Note: SIZELEN, if set to "8", permits maximum
				 * size of about 100 MB (99999999 bytes) to be displayed
				 */

				ltoa(h->attrib.size, t, 10); /* File size to string      */

				/* Explicitely limit displayed size to 8 characters
				 * display larger files as "12345678*" 
				 */

				t[SIZELEN] = 0;      /* so that it doesn't become too long */
				if ( h->attrib.size >= 99999999L )
					t[SIZELEN-1] = '*';

				l = (int) strlen(t);         /* how long is this string? */
				p = t;
				i = 0;
				FILL_UNTIL(d, i, SIZELEN - l);
				while (*p)
					*d++ = *p++;
			}
			else
			{
				i = 0;
				FILL_UNTIL(d, i, SIZELEN);

			}

			*d++ = ' '; /* separate size from the next fields */
			*d++ = ' ';
		  
		}

		if ( options.V2_2.fields & WD_SHDAT )
		{
			if ( !parent ) 	/* Don't show meaningless date for parent folder */
			{
				d = datestr(d, h->attrib.mdate);
				*d++ = ' ';
				*d++ = ' ';
			}
			else
			{                         
				i=0;                       
				FILL_UNTIL ( d, i, 10 );     
			}
      
		}

		if ( options.V2_2.fields & WD_SHTIM )
		{
			if ( !parent ) 	/* don't show meaningless time for parent folder */
			{
				d = timestr(d, h->attrib.mtime);
				*d++ = ' ';
				*d++ = ' ';
			}
			else
			{                         
				i=0;                        
				FILL_UNTIL ( d, i, 10 );     
			}								
		}

		if ( options.V2_2.fields & WD_SHATT )
		{
#if _MINT_
			if (mint && dw->fs_type == 0)
			{
				/* This is not a FAT filesystem */
				switch(h->attrib.mode & S_IFMT)
				{
					/* these attributes handle type of item  */
					case S_IFDIR :	/* directory (folder) */
						*d++ = 'd';
						break;
					case S_IFREG :	/* regular file */
						*d++ = '-';
						break;
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
					default : 
						*d++ = '?';
						break;
				}

				/* These handle access rights for owner, group and others */
	
				{
					int j, bits[] = {S_IRUSR,S_IWUSR,S_IXUSR,S_IRGRP,S_IWGRP,S_IXGRP,S_IROTH,S_IWOTH,S_IXOTH};
					char fl[] = {'r','w','x','r','w','x','r','w','x'};

					for ( j = 0; j < 9; j++ )
						*d++ = (h->attrib.mode & bits[j]) ? fl[j] : '-';
				
				}

			} /* mint ? */
			else
#endif
			{
				/* This is a FAT filesystem */
				*d++ = ((h->attrib.mode & S_IFMT) == S_IFDIR) ? 'd' : '-';
				*d++ = (h->attrib.attrib & 0x04) ? 's' : '-';
				*d++ = (h->attrib.attrib & 0x02) ? 'h' : '-';
				*d++ = (h->attrib.attrib & 0x01) ? '-' : 'w';
				*d++ = (h->attrib.attrib & 0x20) ? 'a' : '-';
			}

			*d++ = ' ';
			*d++ = ' ';
		
		}
		
		*d = 0;
	}
	else
	{
		/* Line is not visible. Return a string filled with spaces. */

		for (i = 0; i < lwidth; i++)
			s[i] = ' ';
		s[lwidth] = 0;
	}

	/* Clip the string on the right border of the window. */

	if ( clip )	
		s[min(dw->columns + dw->px - 2, lwidth)] = 0;
}


/*
 * routine get_dir_line conveniently obtains some information 
 * needed for hardcopy directory printout. Needed because
 * DIR_WINDOW is not known outside DIR.C, and so that it does not
 * have to be defined in header file
 * (310703: well, the above is not true anymore, but let it be...)
 *
 * item >= 0 : get directory line #item;
 * item = -1 : get window title
 * item = -2 : get window info line
 * note: routine actually copies the string to destination
 */
 
void get_dir_line
(
	WINDOW *dw,	/* pointer to window structure */ 
	char *str,	/* output string */ 
	int item	/* item number */
)
{
	if ( item >= 0 )				/* get directory line */
		dir_line ( (DIR_WINDOW *)dw, str, item, FALSE );
	else if ( item == -1 )			/* get window title */
		strcpy(str, ((DIR_WINDOW *)dw)->title);
	else if ( item == -2 )			/* get window info line */
		strcpy(str, ((DIR_WINDOW *)dw)->info);
}


static void dir_prtchar(DIR_WINDOW *dw, int column, int item, RECT *area, RECT *work)
{
	RECT 
		r, 
		in;

	char 
		s[256];

	int 
		c,
		c2;	/* column - 2 */

	c = column - dw->px;
	c2 = column - 2;

	dir_line(dw, s, item, TRUE);

	r.x = work->x + c * dir_font.cw;
	r.y = work->y + DELTA + (int) (item - dw->py) * (dir_font.ch + DELTA);
	r.w = dir_font.cw;
	r.h = dir_font.ch;

	if (xd_rcintersect(area, &r, &in) == TRUE)
	{
		if ( column >= 2 && c2 < linelength(dw) )
		{
			NDTA *d;
			bool link = FALSE;

			if ( item < dw->nvisible )
			{
				d = (*dw->buffer)[(long)item];
				link = d->link;
			}

			s[column - 1] = 0;
			pclear(&r); 

			vswr_mode( vdi_handle, MD_TRANS);

			/* Consider only lines containing text */

			if ( s[3] != ' ' && s[3] != 0 )
			{
				if (link)
					vst_effects(vdi_handle, italic);	

				v_gtext(vdi_handle, r.x, r.y, &s[c2]);

				if (link)
					vst_effects(vdi_handle, 0);	
			}

			if ((item < dw->nlines) && (d->selected == TRUE))
				invert(&in);
		}
		else
			pclear(&in);
	}
}


/********************************************************************
 *																	*
 * Funkties voor het afdrukken in iconmode.							*
 *																	*
 ********************************************************************/

/*
 * Count how many icons have to be drawn in a window
 */

long count_ic( DIR_WINDOW *dw, int sl, int lines )
{
	int columns;
	long n, start;

	columns = dw->columns;

	n = columns * lines;
	start = sl * columns;

	if ((start + n) > dw->nvisible)
		n = lmax((long)(dw->nvisible) - start, 0L);

	return n;
}


/*
 * Create object tree for drawing icons in a window. If parameter smode
 * is false, all icons will be drawn, and background object as well
 */

OBJECT *make_tree(DIR_WINDOW *dw, int sl, int lines, boolean smode, RECT *work)
{
	int columns, yoffset, row;
	long n, start, i;
	OBJECT *obj;

	columns = dw->columns;
	start = sl * columns;

	n = count_ic( dw, sl, lines );

	if ((obj = malloc((n + 1) * sizeof(OBJECT) + n * sizeof(CICONBLK))) == NULL) /* ciconblk (the largest) */
	{
		xform_error(ENSMEM);
		return NULL;
	}

	if ((row = (sl - dw->py) * ICON_H) == 0)
		yoffset = YOFFSET;
	else
		yoffset = 0;

	wd_set_obj0( obj, smode, row, lines, yoffset, work );

	for (i = 0; i < n; i++)
	{
		int icon_no;
		NDTA *h = (*dw->buffer)[start + i];
		boolean selected;

		icon_no = h->icon;

		if (smode == TRUE)
		{
			if (h->selected == h->newstate)
				continue;
			selected = h->newstate;
		}
		else
			selected = h->selected;

		set_obji
		( 
			obj, 
			i, 
			n,
			selected, 
			icon_no, 
			( (int)i % columns ) * ICON_W + XOFFSET,
			( (int)i / columns ) * ICON_H + yoffset,
			(char *)h->name 
		);
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

static void draw_icons(DIR_WINDOW *dw, int sl, int lines, RECT *area,
					  boolean smode, RECT *work)
{
	OBJECT *obj;

	if ((obj = make_tree(dw, sl, lines, smode, work)) == NULL)
		return;
	draw_tree(obj, area);

	free(obj);
}


static void icn_comparea(DIR_WINDOW *dw, int item, RECT *r1, RECT *r2, RECT *work)
{
	int s, columns, x, y;
	CICONBLK *h;			/* ciconblk (the largest) */

	columns = dw->columns;

	s = item - columns * dw->py;
	x = s % columns;
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
 * dir_prtline calls VDI routines to actually print a directory line on screen
 */
 
void dir_prtline(DIR_WINDOW *dw, int line, RECT *area, RECT *work)
{

	/*
	 * Don't do anything in iconified window
	 * (and perhaps this is not even needed ?) 
	 */

	if ( (dw->winfo)->flags.iconified != 0 ) 	
		return; 

	if (options.mode == TEXTMODE)
	{
		RECT r, in, r2;
		char s[256];
		int i;
		
		/* Compose a directory line; test if it can be seen */

		dir_line(dw, s, line, TRUE);
		dir_comparea(dw, line, &r, work);

		if (xd_rcintersect(area, &r, &in) == TRUE)
		{
			NDTA *d;
			bool link = false;

			if (line < dw->nvisible)
			{
				d = (*dw->buffer)[(long)line];
				link = d->link;

			}

			i = (dw->px < 2) ? 0 : (dw->px - 2);

			pclear( &r );

			/* Consider only lines with some text */

			if ( s[3] != 0 && s[3] != ' ' )
			{
				vswr_mode( vdi_handle, MD_TRANS );

				/* Show links in italic type */

				if (link)
					vst_effects(vdi_handle, italic);	

				v_gtext(vdi_handle, r.x, r.y, &s[i]);
			
				if (link)
					vst_effects(vdi_handle, 0);	

				if ((line < dw->nvisible) && (d->selected == TRUE))
					invert(&in);
			}
		}
		r2.y = r.y;
		r2.h = dir_font.ch;
		if (dw->px < 2)
		{
			r2.x = work->x;
			r2.w = (2 - dw->px) * dir_font.cw;
			if (xd_rcintersect(area, &r2, &in) == TRUE)
				pclear(&r2);  
		}
		r2.x = r.x + r.w;
		r2.w = work->x + work->w - r2.x;
		if (xd_rcintersect(area, &r2, &in) == TRUE)
			pclear(&r2); 
	}
	else
		draw_icons(dw, line, 1, area, FALSE, work);

}


/* 
 * Funktie voor het opnieuw tekenen van alle regels in een window.
 * Alleen voor tekst windows. 
 */

static void dir_prtlines(DIR_WINDOW *dw, RECT *area, RECT *work)
{
	int i;

	set_txt_default(dir_font.id, dir_font.size);

	for (i = 0; i < dw->rows; i++)
		dir_prtline(dw, (int)dw->py + i, area, work);
}


void do_draw(DIR_WINDOW *dw, RECT *r, OBJECT *tree, boolean clr,
					boolean text, RECT *work)
{
	if (text)
	{
		if (clr == TRUE)
			pclear(r);  
		dir_prtlines(dw, r, work);
	}
	else
		draw_tree(tree, r);
}


/* 
 * Funktie om een pagina naar boven te scrollen. 
 */

void dir_prtcolumn(DIR_WINDOW *dw, int column, RECT *area, RECT *work)
{
	int i;

	for (i = 0; i < dw->rows; i++)
		dir_prtchar(dw, column, (int)dw->py + i, area, work);
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
 * Either close a dir window completely, or go one level up
 */

void dir_close(WINDOW *w, int mode)
{
	char *p;

	if ( w != NULL )
	{
		((TYP_WINDOW *)w)->winfo->flags.iconified = 0;

		if ((isroot(((DIR_WINDOW *) w)->path) == TRUE) || mode)
		{
			/* 
			 * Either this is a root window, or it should be closed entirely;
			 * In any case, close it.
			 */

			xw_close(w);
			dir_rem((DIR_WINDOW *) w);
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

			if ((p = fn_get_path(((DIR_WINDOW *) w)->path)) != NULL)
			{

				/* Successful; free previous path */

				free(((DIR_WINDOW *) w)->path);

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

static WINDOW *dir_do_open(WINFO *info, const char *path,
						   const char *fspec, int px, int py,
						   int *error) 
{
	DIR_WINDOW *w;
	RECT size;
	int errcode;

	wd_in_screen( info ); /* modify position to come into screen */

	if ((w = (DIR_WINDOW *) xw_create(DIR_WIND, &dir_functions, DFLAGS, &dmax,
									  sizeof(DIR_WINDOW), NULL, &errcode)) == NULL)
	{
		if (errcode == XDNMWINDOWS)
		{
			alert_iprint(MTMWIND); 
			*error = ERROR;
		}
		else if (errcode == XDNSMEM)
			*error = ENSMEM;
		else
			*error = ERROR;

		free(path);
		free(fspec);

		return NULL;
	}

	info->typ_window = (TYP_WINDOW *)w;
	w->itm_func = &itm_func;
	w->px = px;
	w->py = py;
	w->path = path;
	w->fspec = fspec;
	w->buffer = NULL;
	w->nfiles = 0;
	w->nvisible = 0;
	w->winfo = info;

	wd_calcsize(info, &size); 

	/* Now read the contents of the directory */

	graf_mouse(HOURGLASS, NULL);
	*error = dir_read(w);
	graf_mouse(ARROW, NULL);

	if (*error != 0)
	{
		dir_rem(w);
		xw_delete((WINDOW *) w);		/* after dir_rem (MP) */
		return NULL;
	}
	else
	{
		if ( avsetw.flag )
		{
			size = avsetw.size;
			avsetw.flag = FALSE;
		}
		wd_iopen( (WINDOW *)w, &size); 
		info->used = TRUE;
		return (WINDOW *) w;
	}
}


boolean dir_add_window
(
	const char *path,	/* path of the window to open */ 
	const char *thespec,/* file mask specification */
	const char *name	/* name to be selected in the open window, NULL if none */
) 
{
	int 
		j = 0, 
		error;		/* error code */

	char 
		*fspec;		/* current (default) file specification */
	
	WINDOW
		*dw;		/* pointer to the window being opened */

	/* Find an unused directory window slot */

	while ((j < MAXWINDOWS - 1) && (dirwindows[j].used != FALSE))
		j++;

	if (dirwindows[j].used == TRUE)
	{
		/* All windows are used, can't open any more */

		alert_iprint(MTMWIND); 
		free(path);
		return FALSE;
	}
	else
	{
		/* OK; allocate space for file specification */

		if ( thespec )
			fspec = (char *)thespec;
		else
		{
			fspec = strdup(
#if _MINT_
					(mint) ? DEFAULT_EXT :
#endif
					         TOSDEFAULT_EXT ); 
		}

		if (fspec == NULL)
		{
			/* Can't allocate a string for default file specification */

			free(path);
			return FALSE;
		}
		else
		{
			/* Ok, now really we can open the window, in the slot "j" */

			if ( (dw = dir_do_open(&dirwindows[j], path, fspec, 0, 0, &error)) == NULL)
			{
				xform_error(error);
				return FALSE;
			}
			else

				/* 
				 * Code below used to show search result, if a name is
				 * specified, this means that a search has been performed
				 */

				if ( name != NULL )
				{ 
					int
						nv,    /* number of visible items in the directory */
						i,     /* item counter */
						hp,vp; /* calculated slider positions */

					NDTA
						*h;

					nv = ((DIR_WINDOW *)(dirwindows[j].typ_window))->nvisible; 

					/* Find the item which matches the name */

					for ( i = 0; i < nv; i++ )
					{
						h = ( *( (DIR_WINDOW *)(dirwindows[j].typ_window) )->buffer)[i]; 

						if ( strcmp( h->name, name ) == 0 )
						{
							/* 
							 * Calculate positions of sliders;
							 * horizontal slider is here in case
							 * of future development of multicolumn
							 * display, when it might be needed.
							 * Therefore, if-then-else below is 
							 * more complicated than it need currently be 
							 */

/* currently not needed - but may be in multicolumn mode
							if ( options.mode == TEXTMODE )
							{
								hp = 0;			/* until multicolumn */
								vp = min( 1000, (int)( (long)(i + 1) * 1000 / nv ) );
							}
							else
*/


							{
								hp = 0;			/* always ? */ 
								vp = min( 1000, (int)( (long)(i + 1) * 1000 / nv ) );
							}

							if ( hp || vp )
							{
								/* Don't set sliders if not needed */
								wd_type_hslider ( dw, hp ); /* in fact, currently not needed */
								wd_type_vslider ( dw, vp );
							}
							selection.w = dw;
							selection.selected = i;
							selection.n = 1; 					
							itm_select( dw, i, 3, TRUE );
							break; /* don't search anymore */
						}
					}
				}
				return TRUE;
		}
	}
}


/********************************************************************
 *																	*
 * Funkties voor het initialisatie, laden en opslaan.				*
 *																	*
 ********************************************************************/

void dir_init(void)
{
	RECT work;

	xw_calc(WC_WORK, DFLAGS, &screen_info.dsk, &work, NULL);
	dmax.x = screen_info.dsk.x;
	dmax.y = screen_info.dsk.y;
	dmax.w = work.w - (work.w % screen_info.fnt_w);
	dmax.h = work.h + DELTA - (work.h % (screen_info.fnt_h + DELTA));
}


/*
 * Calculate default positions and sizes of directory windows.
 * Windows get stacked rightwards and downwards, starting 
 * from (100,30); they are all of the same size
 */
 
void dir_default(void)
{
	int i;

	dir_font = def_font;

	for (i = 0; i < MAXWINDOWS; i++)
	{
		dirwindows[i].x = 100 + i * screen_info.fnt_w * 2 - screen_info.dsk.x;
		dirwindows[i].y = 30 + i * screen_info.fnt_h - screen_info.dsk.y;
		dirwindows[i].w = screen_info.dsk.w / (2 * screen_info.fnt_w);
		dirwindows[i].h = (screen_info.dsk.h * 11) / (20 * screen_info.fnt_h + 20 * DELTA);
		dirwindows[i].flags.fulled = 0;
		dirwindows[i].flags.iconified = 0;
		dirwindows[i].flags.resvd = 0;
		dirwindows[i].used = FALSE;
	}
}


typedef struct
{
	int 
		px, 
		py, 
		index;
	LNAME 
		path, 
		spec;
	WINDOW 
		*w;
} SINFO2;

static SINFO2 
	that;


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
	{CFG_D,   0, "yrel", &that.py	 },
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save one open directory window
 */

CfgNest dir_one
{
	if (io == CFG_SAVE)
	{
		int i = 0;
		DIR_WINDOW *dw;

		/* Identify window's index by pointer to that window */

		while (dirwindows[i].typ_window != (TYP_WINDOW *) that.w)
			i++;

		dw = (DIR_WINDOW *)(dirwindows[i].typ_window);
		that.index = i;
		that.px = dw->px;
		that.py = (int)dw->py;
		strcpy(that.path, dw->path);
		strcpy(that.spec, dw->fspec);
		
		*error = CfgSave(file, dirw_table, lvl + 1, CFGEMP);
	}
	else
	{
		memset(&that, 0, sizeof(that));

		*error = CfgLoad(file, dirw_table, (int)sizeof(LNAME), lvl + 1); 

		if (*error == 0 )
		{
			if (   that.path[0] == 0 
				|| that.spec[0] == 0
				|| that.index >= MAXWINDOWS
				|| that.px > 1000
				|| that.py > 1000
				)
					*error = EFRVAL;
		}

		if (*error == 0)
		{
			char *path = malloc(strlen(that.path) + 1);
			char *spec = malloc(strlen(that.spec) + 1);

			if (path && spec)
			{
				strcpy(path, that.path);
				strcpy(spec, that.spec);

				/* Note: in case of error path & spec are deallocated in dir_do_open */

				dir_do_open(   &dirwindows[that.index],
								path,
								spec,
								that.px,
								that.py,
								error
							);
				that.index++;
			}
			else
				*error = ENSMEM;
		}
	}
}


int dir_save(XFILE *file, WINDOW *w, int lvl)
{
	int error = 0;

	that.w = w;
	dir_one(file, "dir", lvl + 1, true, &error); 

	return error;
}


CfgNest dir_config
{
	if ( io == CFG_LOAD )
	{
		memset(&thisw, 0, sizeof(thisw));
		memset(&that, 0, sizeof(that));
	}

	cfg_font = &dir_font;
	wtype_table[0].s = "directories";
	thisw.windows = &dirwindows[0];

	*error = handle_cfg(file, wtype_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, NULL, NULL);
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

static int itm_find(WINDOW *w, int x, int y)
{
	int hy, item;
	RECT r1, r2, work;
	DIR_WINDOW *dw;

	/*
	 * Don't do anything in iconified window
	 * (and perhaps this is not even needed ?) 
	 */

	if ( (((TYP_WINDOW *)w)->winfo)->flags.iconified != 0 ) 	
		return -1; 

	dw = (DIR_WINDOW *) w;

	xw_get(w, WF_WORKXYWH, &work);

	if (options.mode == TEXTMODE)
	{
		hy = (y - work.y - DELTA);
		if (hy < 0)
			return -1;
		hy /= (dir_font.ch + DELTA);
		item = hy + (int) dw->py;
		if (item >= dw->nvisible)
			return -1;

		dir_comparea(dw, item, &r1, &work);

		if (inrect(x, y, &r1) == TRUE)
			return item;
		else
			return -1;
	}
	else
	{
		int columns, hx;

		columns = dw->columns;

		hx = (x - work.x) / ICON_W;
		hy = (y - work.y) / ICON_H;

		if ((hx < 0) || (hy < 0) || (hx >= columns))
			return -1;

		item = hx + (dw->py + hy) * columns;
		if (item >= dw->nvisible)
			return -1;

		icn_comparea(dw, item, &r1, &r2, &work);

		if (inrect(x, y, &r1))
			return item;
		else if (inrect(x, y, &r2))
			return item;
		else
			return -1;
	}
}


/* 
 * Funktie die aangeeft of een directory item geselecteerd is. 
 */

static boolean itm_state(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->selected;
}


/* 
 * Funktie die het type van een directory item teruggeeft. 
 */

static ITMTYPE itm_type(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->item_type;
}



static ITMTYPE itm_tgttype(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->tgt_type;
}


static int itm_icon(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->icon;
}


/* 
 * Funktie die de directory van een window teruggeeft. 
 */

const char *dir_path(WINDOW *w)	
{
	return ((DIR_WINDOW *) w)->path;
}


/* 
 * Funktie die de naam van een directory item teruggeeft. 
 */

static const char *itm_name(WINDOW *w, int item)
{
	return (*((DIR_WINDOW *) w)->buffer)[(long) item]->name;
}


/*
 * Return pointer to a string with full path + name of an item in a window.
 * Note: this routine allocates space for the fullname string
 */

static char *itm_fullname(WINDOW *w, int item)
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

static int itm_attrib(WINDOW *w, int item, int mode, XATTR *attr)
{
	NDTA *itm = (*((DIR_WINDOW *) w)->buffer)[(long) item];
	char *name;
	int error;

	if ((name = x_makepath(((DIR_WINDOW *) w)->path, itm->name, &error)) == NULL)
		return error;
	else
	{
		error = (int) x_attr(mode, name, attr);
		free(name);
		return error;
	}
}


/*
 * Is the item perhaps a link
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
		if (h->selected == TRUE)
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
 * redrawn as well.
 */

#define MSEL 8 /* if so many icons change state, draw complete window */

static void dir_drawsel(DIR_WINDOW *w)
{
	long i, h;
	RPNDTA *pb = w->buffer;
	RECT r1, r2, d, work;

	xw_get((WINDOW *) w, WF_WORKXYWH, &work);

	if (options.mode == TEXTMODE)
	{
		/* Display as text */

		h = lmin(w->py + (long)w->rows, w->nlines);

		wind_update(BEG_UPDATE);

		graf_mouse(M_OFF, NULL);
		xw_get((WINDOW *) w, WF_FIRSTXYWH, &r2);
		while ((r2.w != 0) && (r2.h != 0))
		{
			xd_clip_on(&r2);
			for (i = w->py; i < h; i++)
			{
				NDTA *b =(*pb)[i];
				if (b->selected != b->newstate)
				{
					dir_comparea(w, (int) i, &r1, &work);
					if (xd_rcintersect(&r1, &r2, &d) == TRUE)
						invert(&d);
				}
			}

			xd_clip_off();
			xw_get((WINDOW *) w, WF_NEXTXYWH, &r2);
		}
		graf_mouse(M_ON, NULL);

		wind_update(END_UPDATE);
	}
	else
	{
		/* Display as icons */ 

		OBJECT *tree;
		int j, n = 0, ncre;
		NDTA *b;

		boolean
			all = TRUE,		/* draw all icons at once */ 
			sm = TRUE;		/* don't draw background and unchanged icons */

		if ( colour_icons )
		{
			/* Count how many icons have to be redrawn */

			ncre = (int)count_ic(w, (int)w->py, w->rows);

			for ( j = 0; j < ncre; j++ )
			{
				b =(*pb)[j + w->py * w->columns];
				if ( b->selected != b->newstate )
					n++;
			}

			/* In which way to draw ? */

			if ( n > MSEL )
				sm = FALSE;		/* this will make all icons in the tree */
			else
				all = FALSE;	/* this will draw icons one by one */				
		}

		/* Create icon objects tree */

		if ((tree = make_tree(w, w->py, w->rows, sm, &work)) == NULL)
			return;

		/* Now draw it */

		wind_update(BEG_UPDATE);

		if ( colour_icons )
		{
			/* Background object will always be visible for colour icons */

			tree[0].ob_type = G_BOX;

			/* 
			 * Attempt to draw as quickly as possible, depending on 
			 * the number of icons selected
			 */

			for ( j = 0; j < ncre; j++ )
			{
				b =(*pb)[j + w->py * w->columns];
				if ( n > MSEL )
					tree[j + 1].ob_state = (b->newstate) ? SELECTED : NORMAL;
				else
				{
					if (b->selected != b->newstate)
					{
						xd_objrect(tree, j + 1, &r2);
						draw_icrects( (WINDOW *)w, tree, &r2 );
					}
				}
			}
		}

		if ( all )
			draw_icrects( (WINDOW *)w, tree, &work );

		wind_update(END_UPDATE);

		free(tree);
	}
}


/*
 * Funktie voor het selecteren van een item in een window.
 * mode = 0: selecteer selected en deselecteer de rest.
 * mode = 1: inverteer de status van selected.
 * mode = 2: deselecteer selected.
 * mode = 3: selecteer selected.
 * mode = 4: selecteer alles. 
 */

static void itm_select(WINDOW *w, int selected, int mode, boolean draw)
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

		switch (mode)
		{
			case 0:
					b->newstate = (i == selected) ? TRUE : FALSE;
				break;
			case 1:
					b->newstate = b->selected;
				break;
			case 2:
					b->newstate = (i == selected) ? FALSE : b->selected;
				break;
			case 3:
					b->newstate = (i == selected) ? TRUE : b->selected;
				break;
			case 4:
					b->newstate = TRUE;
				break;
		}
	}

	if (mode == 1)
		(*pb)[selected]->newstate = ((*pb)[selected]->selected) ? FALSE : TRUE;

	if (draw == TRUE)
		dir_drawsel((DIR_WINDOW *) w);
	dir_setnws((DIR_WINDOW *) w, draw);
}


/* 
 * Funktie, die de index van de eerste file die zichtbaar is,
 * teruggeeft. Ook wordt het aantal teruggegeven.
 */

static void calc_vitems(DIR_WINDOW *dw, int *s, int *n)
{
	int h;

	if (options.mode == TEXTMODE)
	{
		*s = (int)dw->py;
		h = dw->rows;
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
 * Rubberbox funktie met scrolling 
 */

static void rubber_box(DIR_WINDOW *w, RECT *work, int x, int y, RECT *r)
{
	int x1 = x, y1 = y, x2 = x, y2 = y, ox = x, oy = y, hy = y;
	int kstate, h, scroll, delta, absdelta;
	boolean released, redraw;

	absdelta = (options.mode == TEXTMODE) ? dir_font.ch + DELTA : ICON_H;

	wind_update(BEG_MCTRL);
	graf_mouse(POINT_HAND, NULL);

	set_rect_default();
	draw_rect(x1, y1, x2, y2);

	do
	{
		redraw = FALSE;
		scroll = -1;

		released = xe_mouse_event(0, &x2, &y2, &kstate);

		if (y2 < screen_info.dsk.y)
			y2 = screen_info.dsk.y;

		if ((y2 < work->y) && (w->py > 0) && (y1 < INT_MAX - absdelta))
		{
			redraw = TRUE;
			scroll = WA_UPLINE;
			delta = absdelta;
		}

		if ((y2 >= work->y + work->h) && ((w->py + w->nrows) < w->nlines) && (y1 > -INT_MAX + absdelta))
		{
			redraw = TRUE;
			scroll = WA_DNLINE;
			delta = -absdelta;
		}

		if ((released != FALSE) || (x2 != ox) || (y2 != oy))
			redraw = TRUE;

		if (redraw != FALSE)
		{
			set_rect_default();
			draw_rect(x1, hy, ox, oy);

			if (scroll >= 0)
			{
				y1 += delta;
				if (y1 < (h = work->y))
					hy = h - 1;
				else if (y1 > (h = work->y + work->h))
					hy = h;
				else
					hy = y1;
				w_scroll((TYP_WINDOW *)w, scroll); 
			}

			if (released == FALSE)
			{
				set_rect_default();
				draw_rect(x1, hy, x2, y2);
				ox = x2;
				oy = y2;
			}
		}
	}
	while (released == FALSE);

	graf_mouse(ARROW, NULL);
	wind_update(END_MCTRL);

	if ((h = x2 - x1) < 0)
	{
		r->x = x2;
		r->w = -h + 1;
	}
	else
	{
		r->x = x1;
		r->w = h + 1;
	}

	if ((h = y2 - y1) < 0)
	{
		r->y = y2;
		r->h = -h + 1;
	}
	else
	{
		r->y = y1;
		r->h = h + 1;
	}
}


/* 
 * Funktie voor het selecteren van items die binnen een bepaalde
 * rechthoek liggen. 
 */

static void itm_rselect(WINDOW *w, int x, int y)
{
	long i, h;
	int s, n;
	RECT r1, r2, r3, work;
	RPNDTA *pb;

	/* Don't do anything in an iconified window */

	if ((((TYP_WINDOW *)w)->winfo)->flags.iconified != 0  ) 
		return;

	xw_get(w, WF_WORKXYWH, &work);

	rubber_box((DIR_WINDOW *) w, &work, x, y, &r1);

	s = 0;
	n = ((DIR_WINDOW *) w)->nfiles;
	h = (long) (s + n);
	pb = ((DIR_WINDOW *) w)->buffer;

	for (i = s; i < h; i++)
	{
		NDTA *b = (*pb)[i];

		if (options.mode == TEXTMODE)
		{
			dir_comparea((DIR_WINDOW *) w, (int) i, &r2, &work);
			if (rc_intersect2(&r1, &r2))
				b->newstate = (b->selected) ? FALSE : TRUE;
		}
		else
		{
			icn_comparea((DIR_WINDOW *) w, (int) i, &r2, &r3, &work);
	
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
	if (options.mode == TEXTMODE)
	{
		int x, y, w, h;

		icnd->item = obj;
		icnd->np = 5;
		x = work->x + (2 - wd->px) * dir_font.cw - mx;
		y = work->y + DELTA + (obj - (int) wd->py) * (dir_font.ch + DELTA) - my;
		w = 12 * dir_font.cw - 1;
		h = dir_font.ch - 1;
		icnd->coords[0] = x;
		icnd->coords[1] = y;
		icnd->coords[2] = x;
		icnd->coords[3] = y + h;
		icnd->coords[4] = x + w;
		icnd->coords[5] = y + h;
		icnd->coords[6] = x + w;
		icnd->coords[7] = y;
		icnd->coords[8] = x + 1;
		icnd->coords[9] = y;
	}
	else
	{
		int columns, s;
		RECT ir, tr;

		columns = wd->columns;
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
		icnd->m_x = work->x + (s % columns) * ICON_W + ICON_W / 2 - mx;
		icnd->m_y = work->y + (s / columns) * ICON_H + ICON_H / 2 - my;

		icnd->coords[0] = tr.x;
		icnd->coords[1] = tr.y;
		icnd->coords[2] = ir.x;
		icnd->coords[3] = tr.y;
		icnd->coords[4] = ir.x;
		icnd->coords[5] = ir.y;
		icnd->coords[6] = ir.x + ir.w;
		icnd->coords[7] = ir.y;
		icnd->coords[8] = ir.x + ir.w;
		icnd->coords[9] = tr.y;
		icnd->coords[10] = tr.x + tr.w;
		icnd->coords[11] = tr.y;
		icnd->coords[12] = tr.x + tr.w;
		icnd->coords[13] = tr.y + tr.h;
		icnd->coords[14] = tr.x;
		icnd->coords[15] = tr.y + tr.h;
		icnd->coords[16] = tr.x;
		icnd->coords[17] = tr.y + 1;
	}
}


static boolean get_list(DIR_WINDOW *w, int *nselected, int *nvisible, int **sel_list)
{
	int j = 0, n = 0, nv = 0, *list, s, h;
	long i, m;
	RPNDTA *d;

	d = w->buffer;
	m = w->nvisible;

	calc_vitems(w, &s, &h);
	h += s;

	for (i = 0; i < m; i++)
	{
		if ((*d)[i]->selected == TRUE)
		{
			n++;
			if (((int) i >= s) && ((int) i < h))
				nv++;
		}
	}

	*nvisible = nv;

	if ((*nselected = n) == 0)
	{
		*sel_list = NULL;
		return TRUE;
	}

	list = malloc(n * sizeof(int));

	*sel_list = list;

	if (list == NULL)
	{
		xform_error(ENSMEM);
		return FALSE;
	}
	else
	{
		for (i = 0; i < m; i++)
			if ((*d)[i]->selected == TRUE)
				list[j++] = (int) i;

		return TRUE;
	}
}


/* 
 * Routine voor het maken van een lijst met geselekteerde items. 
 */

static boolean itm_xlist(WINDOW *w, int *nselected, int *nvisible,
						 int **sel_list, ICND **icns, int mx, int my)
{
	int j = 0;
	long i;
	ICND *icnlist;
	RPNDTA *d;
	RECT work;

	xw_get(w, WF_WORKXYWH, &work);

	if (get_list((DIR_WINDOW *) w, nselected, nvisible, sel_list) == FALSE)
		return FALSE;

	if (nselected == 0)
	{
		*icns = NULL;
		return TRUE;
	}

	icnlist = malloc(*nvisible * sizeof(ICND));
	*icns = icnlist;

	if (icnlist == NULL)
	{
		xform_error(ENSMEM);
		free(*sel_list);
		return FALSE;
	}
	else
	{
		int s, n, h;

		calc_vitems((DIR_WINDOW *) w, &s, &n);
		h = s + n;
		d = ((DIR_WINDOW *) w)->buffer;
		for (i = s; (int) i < h; i++)
			if ((*d)[i]->selected == TRUE)
				get_itmd((DIR_WINDOW *) w, (int) i, &icnlist[j++], mx, my, &work);

		return TRUE;
	}
}


/* 
 * Routine voor het maken van een lijst met geseleketeerde items. 
 */

static boolean itm_list(WINDOW *w, int *n, int **list)
{
	int dummy;

	return get_list((DIR_WINDOW *)w, n, &dummy, list);
}


static boolean dir_open(WINDOW *w, int item, int kstate)
{
	char *newpath;

	if ((itm_type(w, item) == ITM_FOLDER) && ((kstate & 8) == 0))
	{
		if ((newpath = itm_fullname(w, item)) != NULL)
		{
			free(((DIR_WINDOW *) w)->path);
			((DIR_WINDOW *) w)->path = newpath;
			dir_readnew((DIR_WINDOW *) w);
		}

		return FALSE;
	}
	else if ((itm_type(w, item) == ITM_PREVDIR) && ((kstate & 8) == 0))
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
 * Simulate enough of an open directory window containing just one item 
 * so that some operations can be executed using existing routines
 * which expect an item selected in a directory window
 */

void dir_simw(DIR_WINDOW **dwa, char *path, char *name, ITMTYPE type, size_t size, int attrib)
{
	static DIR_WINDOW 
		dw;

	static NDTA
		buffer,
		*pbuffer[1]; 

	memset(&dw, 0, sizeof(DIR_WINDOW));
	memset(&buffer, 0, sizeof(NDTA));

	((WINDOW *)&dw)->xw_type = DIR_WIND;	/* this is a directory window */
	dw.path = path;							/* path of this dir */
	dw.itm_func = &itm_func;				/* pointers to specific functions */
	dw.nfiles = 1;							/* number of files in window */
	dw.nvisible = 1;						/* one visible item */
	dw.buffer = &pbuffer;					/* location of NDTA pointer array */
	*dwa = &dw;								/* DIR_WINDOW address */

	pbuffer[0] = &buffer;

	buffer.name = name;						/* filename */
	buffer.item_type = type;				/* type of item in the dir */
	buffer.attrib.size = size;				/* file size */
	buffer.attrib.attrib = attrib;			/* file attributes */

	/* Now "select" the object */

	itm_select( (WINDOW *)&dw, 0, 3, FALSE);
}


/*
 * Try to find what item type should be assigned to a name,
 * so that this item can be opened in the appropriate way.
 * This routine is used only in open.c, and only after
 * the target of a possible link has been determined.
 */

ITMTYPE diritem_type( char *fullname )
{
	XATTR attr;

	attr.attr = 0;

	/* Is this name that of a volume root directory ? */

	if ( isroot(fullname) )
		return ITM_DRIVE;

	if ( strcmp(fn_get_name(fullname), prevdir) == 0 )
		return ITM_PREVDIR;

	/* Get info on the object itself, don't follw links */

	if ( x_attr(  1, fullname, &attr )  >= 0 )
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