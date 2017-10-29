/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2017  Dj. Vukovic
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
#include <xscncode.h>
#include <sys/stat.h>

/* #include <stdio.h>  for TEST only */

#include "resource.h"
#include "desk.h"
#include "stringf.h"		/* get rid of stream io */
#include "xfilesys.h"
#include "font.h"
#include "config.h"
#include "window.h" 
#include "copy.h"
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
#include "icontype.h"
#include "open.h"
#include "va.h"
#include "prgtype.h"


#define TOFFSET			2

#define MAXLENGTH		128		/* Maximum length of a filename to display in a window. HR: 128 */
#define MINLENGTH		12		/* Minimum length of a filename to display in a window. */
#define	DRAGLENGTH		16		/* Length of a name rectangle drawn while dragging items (was 12 earlier) */

/* Beware that SIZELEN must be in accordance with DISP_KBMB in RESOURCE.H */

#define SIZELEN 8 /* 1 + length of field for file size, i.e. for max. 9999999 ~10 MB, otherwise add sufix K or M */



XDFONT dir_font;

WINFO dirwindows[MAXWINDOWS]; 

RECT dmax;

bool clearline = TRUE;

static _WORD		dir_find	(WINDOW *w, _WORD x, _WORD y);
static bool	dir_state	(WINDOW *w, _WORD item);
static ITMTYPE	dir_itmtype	(WINDOW *w, _WORD item);
static ITMTYPE	dir_tgttype	(WINDOW *w, _WORD item);
static const char *dir_itmname(WINDOW *w, _WORD item);
static char    *dir_fullname(WINDOW *w, _WORD item);
static _WORD		dir_attrib	(WINDOW *w, _WORD item, _WORD mode, XATTR *attrib);
static bool	dir_islink	(WINDOW *w, _WORD item);
static bool	dir_open	(WINDOW *w, _WORD item, _WORD kstate);
static bool	dir_copy	(WINDOW *dw, _WORD dobject, WINDOW *sw, _WORD n, _WORD *list, ICND *icns, _WORD x, _WORD y, _WORD kstate);
static void		dir_select	(WINDOW *w, _WORD selected, _WORD mode, bool draw);
static void		dir_rselect	(WINDOW *w, _WORD x, _WORD y);
static ICND *dir_xlist	(WINDOW *w, _WORD *nselected, _WORD *nvisible, _WORD **sel_list, _WORD mx, _WORD my);
static _WORD *dir_list	(WINDOW *w, _WORD *n);
static void		dir_set_update	(WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);
static void		dir_do_update	(WINDOW *w);
static void 	dir_showfirst(DIR_WINDOW *dw, _WORD i);
static char *sizestr(char *tstr, long size);


static ITMFUNC itm_func =
{
	dir_find,					/* itm_find */
	dir_state,					/* itm_state */
	dir_itmtype,				/* itm_type */
	dir_tgttype,				/* target item type */
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


ITMFUNC *dir_func = &itm_func; /* needed for wd_do_dirs() */

/********************************************************************
 *																	*
 * Funkties voor het sorteren van directories.						*
 *																	*
 ********************************************************************/


typedef int SortProc(NDTA **ee1, NDTA **ee2);
 
/* 
 * Sort by visibility
 */

static int _s_visible(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;

	if (e1->visible && !e2->visible)
		return -1;

	if (e2->visible && !e1->visible)
		return 1;

	return 0;
}


/*
 * Sort filename strings case sensitive or insensitive,
 * depending on the flag in options.sort
 */

static int strfncmp(const char *s1, const char *s2)
{
#if _MINT_
	if(options.sort & WD_NOCASE)
    {
      return stricmp(s1,s2);
    }
    else
#endif
    {
      return strcmp(s1, s2);
    }
}


/* 
 * Sort folders before files.
 * Link target type has to be taken into account in order 
 * to sort links to folders properly among folders
 */

static int _s_folder(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;
	int h;

	bool e1dir, e2dir;

#if _MINT_
		e1dir = ((e1->attrib.attrib & FA_DIR) || (e1->tgt_type == ITM_FOLDER));
		e2dir = ((e2->attrib.attrib & FA_DIR) || (e2->tgt_type == ITM_FOLDER));
#else
		e1dir = (e1->attrib.attrib & FA_DIR);
		e2dir = (e2->attrib.attrib & FA_DIR);
#endif

	if ((h = _s_visible(ee1, ee2)) != 0)
		return h;

	if (e1dir && (!e2dir || (strfncmp(e1->name, prevdir) == 0)))
		return -1;

	if (e2dir && (!e1dir || (strfncmp(e2->name, prevdir) == 0)))
		return 1;

	return 0;
}


/* 
 * Function for sorting a directory by filename. 
 * This also sorts folders before files
 */

static int sortname(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;
	int h;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;

	return strfncmp(e1->name, e2->name);
}


/* 
 * Function for sorting a directory by filename extension.
 * This also sorts folders before files
 */

static int sortext(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;
	int h;

	char 
		*ext1, 
		*ext2;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;

	ext1 = strchr(e1->name, '.');
	ext2 = strchr(e2->name, '.');

	if ((ext1 != NULL) || (ext2 != NULL))
	{
		if (ext1 == NULL)
			return -1;

		if (ext2 == NULL)
			return 1;

		if ((h = strfncmp(ext1, ext2)) != 0)
			return h;
	}

	return strfncmp(e1->name, e2->name);
}


/* 
 * Function for sorting a directory by file size.
 * This also sorts folders before files
 */

static int sortlength(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;
	int h;

	if ((h = _s_folder(ee1, ee2)) != 0)
		return h;

	if (e1->attrib.size > e2->attrib.size)
		return 1;

	if (e2->attrib.size > e1->attrib.size)
		return -1;

	return strfncmp(e1->name, e2->name);
}


/* 
 * Function for sorting a directory by object date & time.
 * This also sorts folders before files
 */

static int sortdate(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;
	int h;

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

	return strfncmp(e1->name, e2->name);
}


/* 
 * Function for no-sorting :) of directory 
 */

static int unsorted(const void *ee1, const void *ee2)
{
	const NDTA *e1 = *(const NDTA *const *)ee1;
	const NDTA *e2 = *(const NDTA *const *)ee2;
	int h;

	if ((h = _s_visible(ee1, ee2)) != 0)
		return h;

	if (e1->index > e2->index)
		return 1;

	return -1;
}


/*
 * Function for reversing the order of sorted items.
 * This is to be applied -after- a sorting of any kind if reverse is wanted.
 */

static void revord(NDTA **buffer, _WORD n)
{
	NDTA 
		**b1 = &buffer[0],
		**b2 = &buffer[n - 1],
		*s;

	_WORD 
		i, 
		k = n / 2;


	for (i = 0; i < k; i++ )
	{
		s = *b1;
		*b1++ = *b2;
		*b2-- = s;		
	} 
}


/*
 * Function for sorting of a directory by desired key (no drawing done here)
 * Function indices in the array of pointers are WD_SORT_NAME, WD_SORT_EXT,
 * WD_SORT_DATE, WD_SORT_LENGTH, WD_NOSORT. This is shorter than the earlier
 * version that used a switch.
 */

static void sort_directory( DIR_WINDOW *w )
{
	static int (*const sortproc[5])(const void *, const void *) = { sortname, sortext, sortdate, sortlength, unsorted };

	/* Perform the actual sorting (in fact pointers only are sorted) */

	qsort(w->buffer, w->nfiles, sizeof(size_t), sortproc[options.sort & 0x000F]);

	/* Reverse the order, if so selected */

	if ( options.sort & WD_REVSORT )
		revord( (NDTA **)(w->buffer), w->nvisible );	
}


/*
 * Sort a directory by desired key, then update the window
 */

void dir_sort(WINDOW *w)
{
	sort_directory((DIR_WINDOW *)w);
	wd_type_draw ((TYP_WINDOW *)w, FALSE ); 
}


/********************************************************************
 *																	*
 * Funkties voor het laden van directories in een window.			*
 *																	*
 ********************************************************************/

/* 
 * Detect whether a file is to be considered s executable.
 * First it is examined whether its filetype is in the list
 * of executable filetypes. Then, it is examined whether execute
 * rights are set (this is done only for names without extensions,
 * i.e. those that do not contain a period).
 * Parameter 'name' is the name proper, not a fullname.
 */

bool dir_isexec(const char *name, XATTR *attr)
{
	return 
	(
		((attr->st_mode & S_IFMT) == S_IFREG) &&
		(
			prg_isprogram(name) 
#if _MINT_
			|| ( mint && (strchr(name, '.') == NULL) && ((attr->st_mode & EXEC_MODE) != 0))
#endif
		)
	);
}


/* 
 * Zet de inforegel van een window. Als n = 0 is dit het aantal
 * bytes en files, als n ongelijk is aan 0 is dit de tekst
 * 'n items selected' 
 */

void dir_info(DIR_WINDOW *w)
{
	_WORD
		l,
		n,
		m;

	long
		total;

	LSUM
		bytes;

#if _MINT_
	LNAME 
		mask = {0};
#else
	SNAME 
		mask = {0};
#endif

	char
		*p;

	if(autoloc)
	{
		strcpy( mask, " (" );
		strcat( mask, automask );
		strcat( mask, ")" );
	}

	if (w->nselected > 0)
	{
		m = MSITEMS;
		bytes = w->selbytes;
		n = w->nselected;
	}
	else
	{
		m = MITEMS;
		bytes = w->visbytes;
		n = w->nvisible;
	}

	size_sum(&total, &bytes);
	p = fmt_size(total, &l);

	sprintf
	(
		w->info, 
		get_freestring(m), 
		mask, 
		p, 
		n, 
		get_freestring( (n == 1) ? MISINGUL : MIPLURAL )
	);

	xw_set((WINDOW *) w, WF_INFO, w->info);
}


/*
 * Release the buffer(s) occupied by directory data
 */

static void dir_free_buffer(DIR_WINDOW *w) 
{
	RPNDTA *b = w->buffer;


	if (b)
	{
		long i;

		for (i = 0; i < w->nfiles; i++)
			free((*b)[i]);		/* free NDTA + name */

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
	if (w->buffer != NULL)
	{
		long 
			i;

		LSUM 
			v;

		NDTA
			*b;

		_WORD 
			oa = options.attribs,
			n = 0;

		v.bytes = 0;
		v.kbytes = 0;

		for (i = 0; i < w->nfiles; i++)
		{
			b = (*w->buffer)[i];
			b->selected = FALSE;
			b->newstate = FALSE;

			/* Note: by adding the line of code
			 * options "Show subfolders" and "show parent"
			 * became independent. Ths is new behaviour as of V4.08 */

			if 
			(   
				(   (oa & FA_HIDDEN) != 0 					/* permit hidden */
			        || (b->attrib.attrib & FA_HIDDEN) == 0 	/* or item is not hidden */
				) && 
				(   (oa & FA_SYSTEM) != 0 					/* permit system */
			        || (b->attrib.attrib & FA_SYSTEM) == 0 	/* or item is not system */
				) && 
				(   (oa & FA_DIR) != 0 						/* permit subdirectory */
			        || (b->attrib.attrib & FA_DIR) == 0 	/* or item is not subdirectory */
					|| strcmp(b->name, prevdir) == 0       	/*** or item is parent dir ***/ 
				) && 
				(   (oa & FA_PARDIR) != 0 					/* permit parent dir */  
					|| strcmp(b->name, prevdir) != 0       	/* or item is not parent dir */ 
				) && 
				(   (b->attrib.mode & S_IFMT) == S_IFDIR 	/* permit directory */
				    || cmp_wildcard(b->name, w->fspec)	   	/* or mask match */
				)
			)
			{
 				if((b->attrib.mode & S_IFMT) != S_IFDIR) 
					add_size (&v, b->attrib.size);
			
				b->visible = TRUE;
				n++;
			}
			else
			{
				b->visible = FALSE;
			}
		}


		w->nvisible = n;
		w->visbytes = v;
	}
}


/*
 * Transform data from extended attributes
 */

static void xattr_to_fattr(XATTR *xattr, FATTR *fattr)
{
	fattr->mode = xattr->st_mode;
	fattr->size = xattr->st_size;
	fattr->mtime = dos_mtime(xattr);
	fattr->mdate = dos_mdate(xattr);
	fattr->attrib = xattr->st_attr;
#if _MINT_
	fattr->gid = xattr->st_gid;
	fattr->uid = xattr->st_uid;
#endif
}


/* 
 * Funktie voor het copieren van een dta naar de buffer van een window.
 * Parameters fulln and tgt are used only with links 
 */

#if _MINT_
static _WORD copy_DTA(NDTA **dest, char *fulln, char *name, XATTR *src, XATTR *tgt, _WORD iindex, bool link)	/* link */
#else
static _WORD copy_DTA(NDTA **dest, char *name, XATTR *src, _WORD iindex, bool link)	/* link */
#endif
{
	NDTA
		*new;

	long 
		l = strlen(name);


	new = malloc(sizeof(NDTA) + l + 1);		/* Allocate space for NDTA and name together */

	if (new)
	{
		new->index = iindex;
		new->link  = link;				/* handle links */
	
#if _MINT_
		if ( link )
		{
			char *tgtname = x_fllink(fulln);

			new->item_type = ITM_FILE;

			if ((tgt->st_mode & S_IFMT) == S_IFDIR)
				new->tgt_type = (strcmp(fn_get_name(tgtname), prevdir) == 0) ? ITM_PREVDIR : ITM_FOLDER;
			else
			{
				/* Note: this may considerably slow down opening of windows */

				if(x_netob(tgtname))
					new->tgt_type = ITM_NETOB;
				else if(dir_isexec(fn_get_name(tgtname), tgt))
					new->tgt_type = ITM_PROGRAM;
				else
					new->tgt_type = ITM_FILE;
			}
	
			free(tgtname);
		}
		else
#endif
		{
			if ((src->st_mode & S_IFMT) == S_IFDIR)
				new->item_type = (strcmp(name, prevdir) == 0) ? ITM_PREVDIR : ITM_FOLDER;
			else
			{
				/* Note: This slows down opening of windows !*/

				if( dir_isexec(name, src) )
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
			new->icon = icnt_geticon(name, new->item_type, new->tgt_type);

		new->name = new->alname;		/* the pointer makes it possible to use NDTA in local name space (auto) */
		strcpy(new->alname, name);
		*dest = new;

		return (_WORD)l;
	}

	*dest = NULL;
	return ENSMEM;
}


/* 
 * Funktie voor het inlezen van een directory. Het resultaat is
 * ongelijk nul als er een fout is opgetreden. De directory wordt
 * gesorteerd. 
 */

static _WORD read_dir(DIR_WINDOW *w)
{
	XDIR 
		*dir;
   
	long 
		bufsiz, 
		memused = 0,
		n = 0;

	LSUM
		length; 

	_WORD 
		error = 0, 
		maxl = 0;

	XATTR 
#if _MINT_
		tgtattr = {0},
#endif
		attr = {0};

	length.kbytes = 0;
	length.bytes = 0;

	dir_free_buffer(w); /* clear and release old buffer, if it exists */

	/* HR we only prepare a pointer array :-) */

	bufsiz = (long)options.max_dir;		/* that's how many pointers we allocate */

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

				error = (_WORD)x_xreaddir(dir, &name, sizeof(VLNAME), &attr); 

				if (error == 0)
				{
					bool link = false;
					char *fulln = NULL;

					/* 
					 * Note: number of items in a directory must currently
					 * be limited to max int16 value, because routines for
					 * selection of items return numbers of items as
					 * short integers!
					 */
		
					if (n >= SHRT_MAX )
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
							 * but at execution may suffer from memory
							 * fragmentation
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

					if ((attr.st_mode & S_IFMT) == S_IFLNK)
					{
						/* 
						 * Why get target? So that items get sorted as they should be:
						 * links to folders will be sorted among folders, etc.
						 * Getting the attributes for network objects will
						 * fail, but it doesn't matter.
 						 */

						fulln = fn_make_path(dir->path, name);
						x_attr(0, w->fs_type, fulln, &tgtattr); /* follow the link to show attributes */
						link = true;
					}
					else
						tgtattr = attr;
#endif

					if (strcmp(".", name) != 0)
					{
						/* 
						 * An area is allocated here for NDTA + name
						 * then the area is filled with data.
						 * This routine returns name length in 'error'.
						 */
#if _MINT_
					    error = copy_DTA(*w->buffer + n, fulln, name, &attr, &tgtattr, (_WORD)n, link);
#else
					    error = copy_DTA(*w->buffer + n, name, &attr, (_WORD)n, link);
#endif
					    /* DO NOT put () around w->buffer + n !!! */

					    if (error >= 0)
						{
							add_size(&length, attr.st_size);
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

	w->nfiles = (_WORD)n;
	w->usedbytes = length;
	w->namelength = maxl;

	/* Is everything OK ? */

	if (error != 0)
	{
		/* An error has occured, free memory used */

		dir_free_buffer(w);
		w->nfiles = 0;
		w->nvisible = 0;
		w->usedbytes.kbytes = 0;
		w->usedbytes.bytes = 0;
		w->visbytes.kbytes = 0;
		w->visbytes.bytes = 0;
	}
	else
	{
		/* Sort directory */

		set_visible(w);
		sort_directory(w);
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
	w->selbytes.kbytes = 0;
	w->selbytes.bytes = 0;
	w->refresh = FALSE;
	calc_rc((TYP_WINDOW *)w, &(w->xw_work));
	dir_info(w);
	wd_type_title((TYP_WINDOW *)w);
}


/* 
 * Read a directory into a window buffer.
 * Beware: this routine sets the mouse first to busy and
 * then to arrow shape!
 */

static _WORD dir_readandset(DIR_WINDOW *w)
{
	_WORD error;

	hourglass_mouse();

	error = read_dir(w);

	if(error == 0)
		dir_setinfo(w);			/* line length is calculated here */

	arrow_mouse();

	return error;
}


/*
 * Aux. function for the two routines further below. This routine
 * also resets the fulled flags if the number of items or the length
 * of the longest name has changed.
 * Use wd_type_draw() after this function! 
 */

void dir_reread(DIR_WINDOW *w)
{
	_WORD
		error, 
		oldn = w->nvisible, 
		oldl = w->llength;

	error = dir_readandset(w);

	xform_error(error);

	if(w->nvisible != oldn || w->llength != oldl)
		wd_type_nofull((WINDOW *)w);
}


/*
 * Unconditional update of a directory window
 */

void dir_refresh_wd(DIR_WINDOW *w)
{
	dir_reread(w);
	wd_type_draw ((TYP_WINDOW *)w, TRUE );
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
		{
			dir_reread((DIR_WINDOW *)w);
			wd_type_draw ((TYP_WINDOW *)w, TRUE );
		}

		w = xw_next(w);
	}
}


/*
 * Remove a trailing backslash from a path,
 * except if the path is that of a root directory
 */

void dir_trim_slash( char *path )
{
	char *b = strrchr(path, '\\');

	if ( b && b[1] == '\0' && !isroot(path) )
		*b = '\0';
}


/* 
 * Refresh a directory window given by path, or else top that window
 * If required directory is found, return TRUE
 * action: DO_PATH_TOP = top, DO_PATH_UPDATE = update
 */

bool dir_do_path( char *path, _WORD action )
{
	WINDOW
		*w;

	const char 
		*wpath;


 	w = xw_first();

	while( w )
	{
		if 
		(
			xw_type(w) == DIR_WIND && 
			(wpath = wd_path(w)) != NULL && 
			path && 
			(strcmp( path, wpath ) == 0) 
		)
		{
			if ( action == DO_PATH_TOP )
				wd_type_topped( w );
			else			
				dir_refresh_wd( (DIR_WINDOW *)w );

			return TRUE;
		}

		w = xw_next(w);
	}

	return FALSE;
}


/*
 * Funktie om te controleren of het pad van file fname gelijk is
 * aan path.
 */

static bool cmp_path(const char *path, const char *fname)
{
	const char 
		*h;

	long 
		l;


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
	if (type == WD_UPD_ALLWAYS)
		((DIR_WINDOW *)w)->refresh = TRUE;
	else
	{
		if (cmp_path(((DIR_WINDOW *)w)->path, fname1) != FALSE)
			((DIR_WINDOW *)w)->refresh = TRUE;

		if ((type == WD_UPD_MOVED) && (cmp_path(((DIR_WINDOW *)w)->path, fname2) != FALSE))
			((DIR_WINDOW *)w)->refresh = TRUE;
	}
}


/*
 * Update a directory window if it is marked for update
 * Also inform signed-on AV-clients that this directory should be updated.
 * Note: notification of AV-clients may slow down file copying.
 * If there are no AV-protocol clients, skip this completely for speed
 */

static void dir_do_update(WINDOW *w)
{
	if (((DIR_WINDOW *)w)->refresh)
	{
		dir_refresh_wd((DIR_WINDOW *)w);

#if _MORE_AV
		if ( avclients )
			va_pathupdate(w);
#endif
	}
}


/* 
 * Funktie voor het zetten van de mode van een window.
 * Display a directory window in text mode or icon mode.
 * Icons must be previosuly determined.
 * Mode is taken directly from options.mode 
 */

void dir_disp_mode(WINDOW *w)
{
#if 0 /* so long as there are no menus in dir window, this can be shorter */
	RECT work;

	wd_type_nofull(w); 
	xw_getwork(w, &work); /* work area is modified by menu height */
	calc_rc((TYP_WINDOW *)w, &work);
	set_sliders((TYP_WINDOW *)w);
	wd_type_redraw(w, &work); 		
#endif
	wd_type_nofull(w); 
	calc_rc((TYP_WINDOW *)w, &(w->xw_work));
	set_sliders((TYP_WINDOW *)w);
	wd_type_redraw(w, &(w->xw_work)); 		
}


/*
 * Funktie voor het zetten van de iconen van objecten in een window.
 */

static void dir_geticons(WINDOW *w)
{
	RPNDTA 
		*pb;

	NDTA 
		*b;

	long 
		i,
		n;


	pb = ((DIR_WINDOW *)w)->buffer;
	n = ((DIR_WINDOW *)w)->nfiles;

	for (i = 0; i < n; i++)
	{
		b = (*pb)[i];
		b->icon = icnt_geticon(b->name, b->item_type, b->tgt_type);
	}
}

void dir_seticons(WINDOW *w)
{
	dir_geticons(w);

	if (options.mode != TEXTMODE) 
		wd_type_draw ((TYP_WINDOW *)w, FALSE );
}


/*
 * Prepare and display a directory window in icon or text mode.
 * In icon mode determine which icon should be used for each item
 * (all items are scanned, not only visible ones)
 */

void dir_mode(WINDOW *w)
{
	if ( options.mode )	
	{
		/* 
		 * Icon mode; find appropriate icons.
		 * This can take a long time, change mouse form to busy 
		 */

		hourglass_mouse();
		dir_geticons(w);
		arrow_mouse();
	}

	/* Now really make the change */

	dir_disp_mode(w);
}



/********************************************************************
 *																	*
 * Funkties voor het zetten van het file mask van een window.		*
 *																	*
 ********************************************************************/


/*
 * Funkties voor het zetten van deusile attributen van een window.	
 * Routine set_visible uses options.attribs, etc. 
 */

void dir_newdir(WINDOW *w)
{
	wd_type_nofull(w);
	set_visible((DIR_WINDOW *)w);
	wd_reset(w);
	dir_setinfo((DIR_WINDOW *)w);
	sort_directory((DIR_WINDOW *)w);
	wd_type_draw((TYP_WINDOW *)w, TRUE); 
}


/*
 * Set the filemask of a directory window
 * (open the dialog, etc.)
 */

void dir_filemask(DIR_WINDOW *w)
{
	char *newmask;
	_WORD oa = options.attribs;

	if ((newmask = wd_filemask(w->fspec)) != NULL)
	{
		free(w->fspec);

		if(*newmask)
		{
			w->winfo->flags.setmask = 1;
			w->fspec = newmask;
		}
		else
		{
			w->winfo->flags.setmask = 0;
			w->fspec = strdup(fsdefext); /* filesystem default mask */

			if(w->fspec == NULL)	/* probably not enough memory */
				w->fspec = newmask; /* better empty string than NULL pointer */
			else
				free(newmask);
		}

		if(options.attribs == oa) /* otherwise dir_newdir will be done elsewhere */
			dir_newdir((WINDOW *)w); 
	}
}


/*
 * Create a new folder or a symbolic link.
 */


static _WORD dir_makenew(DIR_WINDOW *dw, _WORD title)
{
	VLNAME name; /* but only LNAME length will be used */
	char *thename;
	_WORD button, error = 0;

	obj_unhide(newfolder[DIRNAME]);

	rsc_title(newfolder, NDTITLE, title);
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
				if ((title == DTNEWLNK) && *openline)
					error = x_mklink(thename, openline);
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

		if (error == EACCDN)
			alert_iprint(MDEXISTS);
		else
			xform_error(error);
	}

	return error;
}


/*
 * Handle the dialog for creating new folders.
 */

void dir_newfolder(WINDOW *w)
{
	cv_fntoform(newfolder, DIRNAME, empty);
	rsc_title(newfolder, NDTITLE, DTNEWDIR);
	obj_hide(newfolder[OPENNAME]);
	dir_makenew((DIR_WINDOW *)w, DTNEWDIR);
}


#if _MINT_

/*
 * A small aux. routine to save a couple of bytes in dir_newlink();
 * It changes the size of the NEWDIR dialog when appropriate.
 */

static void newlinksize(_WORD dy)
{
	newfolder[0].ob_height += dy;
	newfolder[NEWDIROK].ob_y += dy;
	newfolder[NEWDIRC].ob_y += dy;
	newfolder[OPENNAME].ob_y += dy;
}


/*
 * Create a symbolic link (open the dialog to enter data).
 * Note: appropriate file system is checked for in wd_hndlmenu.
 * It is assumed there that if filesystem is not TOS/FAT, links can 
 * always be created- which in fact may not always be true
 * and should better be checked.
 * Take care to preserve the content of openline
 * so that the next Open... will have the field filled.
 */

void dir_newlink(WINDOW *w, char *target)
{
	char dsk[] = {0, 0};

	_WORD dy = newfolder[NEWDIRTO].ob_y - newfolder[DIRNAME].ob_y;

	/* The string pointed to by dirname is a LNAME */

	*dsk = *target; /* to get the name of a disk volume */
	
	strcpy(dirname, get_freestring(TLINKTO));
	strncat
	(
		dirname,
		(isroot(target)) ? dsk : fn_get_name(target), 
		lmax(XD_MAX_SCRLED - strlen(dirname) - 1L, 0) 
	); 
	strcpy(envline, openline); /* reuse existing space to save openine */
	strsncpy(openline, target, sizeof(VLNAME));	/* this is a fullpath */

	/* Increase dialog size so that FTEXT for target path can be shown */

	newlinksize(dy);

	obj_unhide(newfolder[OPENNAME]);
	obj_unhide(newfolder[NEWDIRTO]);

	/* These will always be scrolled editable texts, so xd_init_shift can be simply applied */

	xd_init_shift(&newfolder[DIRNAME], dirname);
	xd_init_shift(&newfolder[OPENNAME], openline);

	dir_makenew((DIR_WINDOW *)w, DTNEWLNK);

	/* Decrease dialog size back to usual */

	newlinksize(-dy);
	obj_hide(newfolder[NEWDIRTO]);
	strcpy(openline, envline);
}

#endif


/* 
 * Funktie voor het berekenen van het totale aantal regels in een window.
 * This routine calculates number of lines, columns and text columns.
 * It does -not- calculate the number of visible columns. 
 */

void calc_nlines(DIR_WINDOW *dw)
{
	_WORD
		dc,
		nvisible = dw->nvisible;

	if (options.mode == TEXTMODE)
	{
		_WORD mcol, ll = linelength(dw);

		dw->llength = ll;	/* length of an item line */
		ll += CSKIP;

		if ( options.aarr )
		{
			/* at least 8/8 - 6/8 = 2/8 of dir item name have to be visible */

			mcol = (xd_screen.w / dir_font.cw - BORDERS) / ll; /* max. columns to fit on screen */
			dc = minmax(1, min( (dw->ncolumns + dw->llength * 6 / 8) / ll, mcol), nvisible);
		}
		else
			dc = 1;

		dw->dcolumns = dc;
		dw->columns = max(1, ll * dc + BORDERS - CSKIP);
	}
	else
	{
		/* at least 8/8 - 5/8 = 3/8 of icon column have to be visible */

		if (options.aarr)
 			dc = (dw->xw_work.w + iconh * 5 / 8);		
		else
 			dc = (xd_screen.w - XOFFSET);		

		dc = minmax(1, dc / iconw, nvisible); /* will never be less than 1 */
		dw->dcolumns = dc;
		dw->columns = dc;
	}

	dw->nlines = (nvisible - 1) / dc + 1;
}


/* 
 * Calculate the length of a line in a window. Returned value
 * depends on filename length and visible directory fields.
 * In the TOS version of TeraDesk name length is a constant, 
 * in the MiNT version it is the length of the longest filename
 * in the directory. 
 */

_WORD linelength(DIR_WINDOW *w)
{
	_WORD 
		l,						/* length of filename             */
		f,						/* length of other visible fields */
		of = options.fields;	/* which fields are shown */	

  
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
 * Compute screen position of a directory item in text mode.
 * See also icn_comparea.
 * Note: parameter 'work' is needed because work area may not be equal to
 * real work area- but this would happen only if there were a menu
 * in the window
 */

static void dir_comparea
(
	DIR_WINDOW *dw,		/* pointer to directory window */
	_WORD item,			/* item index */
	RECT *r,			/* position and size of item rectangle on the screen */ 
	RECT *dummy,		/* for compatibility with icn_comparea */ 
	RECT *work			/* window work area rectangle */
)
{
	_WORD 
		ll = dw->llength + CSKIP,
		col; 


	(void)dummy;
	/* Find item's column and its x-position. Avoid division by zero */

	col = dw->nlines ? (_WORD)(item / dw->nlines) : 0;

	r->x = work->x  + (col * ll + TOFFSET - dw->px) * dir_font.cw; 
	r->y = DELTA + work->y + (_WORD) ( (item - dw->nlines * col ) - dw->py) * (dir_font.ch + DELTA);
	r->w = dir_font.cw * dw->llength;
	r->h = dir_font.ch;
}


/*
 * Macro used in dir_line.
 * FILL_UNTIL fills a string with ' ' until a counter reaches a certain value.
 * Note: there is no point in creating a routine to do this; the resulting 
 * code would be just a few bytes smaller, but would be slower.
 */


#define FILL_UNTIL(dest, i, value)			while (i < (value))	\
											{					\
												*dest++ = ' ';	\
												i++;			\
											}

/*
 * Convert a GEMDOS time to a string (format HH:MM:SS) or
 * convert a GEMDOS date to a string (format DD-MM-YY).
 * This string is always 9 characters long, including a trailing space. 
 * Attention: no termination 0-byte!
 *
 * Parameters:
 *
 * tstr		- destination string
 * t		- current time or date in GEMDOS format
 * parent	- if TRUE, just fill with blanks
 * s 		- '-' = date; ':' = time
 * Result   - Pointer to the END of tstr.
 */

static char *datimstr(char *tstr, unsigned short t, bool parent, char s)
{
	unsigned short 
		tdh,		/* day or hour */ 
		tmm,		/* month or minute */ 
		tys,		/* year or second */ 
		h;


	if(parent)			/* just fill with blanks for a parent directory */
	{
		_WORD i;

		for(i = 0; i < 9; i++)
			*tstr++ = ' '; /* tstr[i] */
	}
	else
	{
		h = t >> 5;

		if(s == ':')	/* time */
		{
			tys = (t & 0x1F) * 2;
			tmm = h & 0x3F;
			tdh = (h >> 6) & 0x1F;
		}
		else			/* date */
		{
			tdh = t & 0x1F;
			tmm = h & 0x0F;
			tys = (((h >> 4) & 0x7F) + 80) % 100;
		}

		tstr = digit(tstr, tdh);	/* days or hours     */
		*tstr++ = s;
		tstr = digit(tstr, tmm);	/* months or minutes */
		*tstr++ = s;
		tstr = digit(tstr, tys);	/* years or seconds  */
		*tstr++ = ' ';
	}

	return tstr;
}



/*
 * Represent object size as a right-justified string. 
 * If object size is larger than DISP_KBMB, 
 * divide by 1024 or 1048576 and add suffix 'K' or 'M'.
 * Return pointer to the END of the string.
 * Attention: no termination 0 byte.
 * This string always includes a trailing blank
 * Note: 'size' is limited to 2 GB
 */

static char *sizestr(char *tstr, long size)
{
	_WORD
		j,
		i = 0;

	char 
		*t,
		*d = tstr;

	size &= 0x7FFFFFFFL;

	if ( size > DISP_KBMB )
		size = ((1 - KBMB) - size) / KBMB;

	t = fmt_size(size, &j);
	j = SIZELEN - j;

	FILL_UNTIL(d, i, j);

	while (*t)
		*d++ = *t++;

	*d++ = ' ';

	return d;
}


#if _MINT_

/* 
 * Create a string for displaying user/group id (range -999:9999).
 * Leading zeros are shown; no termination zero byte.
 * Return pointer to -end- of string.
 * Hopefully this will be faster than sprintf( ... %i ...).
 */

static char *uidstr(char *idstr, _WORD id)
{
	char
		*idstri = idstr;

	_WORD
		i,
		k = 1000;


	for( i = 0; i < 4; i++ )
	{
		if(id < 0)
		{
			*idstri = '-';
			id = -id;
		}
		else
		{
			*idstri = id / k;
			id -= k * (*idstri);
			*idstri += '0';
		}

		k /= 10;

		idstri++;
	}

	return idstri;

}

#endif


/* 
 * Make a 0-terminated string containing brief information about
 * a directory item: size, date and time. Take care to adjust field length
 * in the resource to what is written here. Contents of the TBYTES string
 * and NCNINFO in the resource must match, including the trailing blank. 
 */

void dir_briefline(char *tstr, XATTR *att)
{
	char
		*b = get_freestring(TBYTES),
		*d = tstr;

	_WORD 
		i,
		l = (_WORD)strlen(b);


	if((att->st_mode & S_IFMT) == S_IFDIR)
	{
		i = 0;
		l += (SIZELEN + 1);
		FILL_UNTIL(d, i, l);
	}
	else
	{
		d = sizestr(d, att->st_size); /* note: this will be in B, KB or MB */
		strcpy(d, b);
		d += l;
	}

	d = datimstr(d, dos_mdate(att), FALSE, '-');
	d = datimstr(d, dos_mtime(att), FALSE, ':');
	*(--d) = 0;
}


/*
 * Make a string with the contents of a line in a directory window.
 * If there is no content, produce a string of spaces.
 */

void dir_line(DIR_WINDOW *dw, char *s, _WORD item)
{
	NDTA 
		*h;						/* pointer to directory item data */

	char 
		*d; 					/* current position in the string */

	const 
		char *p;				/* current position in the source */

	_WORD 
		i,						/* length counter */ 
		of = options.fields,	/* which fields are shown */
		parent = FALSE;			/* flag that this is a .. (parent) dir */

#if _MINT_
	static const _WORD 
		rbits[] = {S_IRUSR,S_IWUSR,S_IXUSR,S_IRGRP,S_IWGRP,S_IXGRP,S_IROTH,S_IWOTH,S_IXOTH};

	static const char 
		fl[] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};
#endif

	/* These characters mark folders and programs in a display. 
	 * Beware! must be:
	 * mark[ITM_FOLDER]  = '\007';
	 * mark[ITM_PREVDIR] = '\007';
	 * mark[ITM_PROGRAM] = '-';
	 */

	static const char 
		mark[] = {' ',' ',' ',' ','\007','-',' ','\007',' ',' '};
 
	mode_t hmode;


	if (item < dw->nvisible)
	{
		h = (*dw->buffer)[item]; /* file data */
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
		if ( (dw->fs_type & (FS_LFN | FS_CSE)) != 0)
		{
			/* 
			 * This is not TOS fs with  8+3 names. 
			 * (long or case-sensitive names are permitted)
			 * Copy the filename. '..' is handled like everything else 
			 */
			_WORD v = minmax(MINLENGTH, dw->namelength, MAXLENGTH) + 2;

			while ((*p) && (i < MAXLENGTH))
			{
				*d++ = *p++;
				i++;
			}

			/* this fill includes two separating blanks */

	        FILL_UNTIL(d, i, v)
		}
		else
#endif
		{	
			/* 
			 * This is TOS fs with 8+3 names
			 * Copy the filename. Handle '..' as a special case. 
			 */
	
			if (parent)
			{
				/* This is the parent directory (..). Just copy. */
	
				while (*p)
				{
					*d++ = *p++;
					i++;
				}
			}
			else
			{
				/* Copy the name, until the start of the extension found. */
				
				while ((*p) && (*p != '.'))
				{
					*d++ = *p++;
					i++;
				}
	
				FILL_UNTIL(d, i, 9); /* fill name with blanks */

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

			FILL_UNTIL(d, i, 14); /* includes two separating blanks */

		}  /* mint fs? */

		/* Compose size string */

		if ( of & WD_SHSIZ )	/* show file size? */
		{
			if (hmode == S_IFDIR) /* this is not a subdirectory */
			{
				i = 0;
				FILL_UNTIL(d, i, SIZELEN + 2);
			}
			else
			{
				d = sizestr(d, h->attrib.size);
				*d++ = ' ';	/* separate size from the next fields */	  
			}
		}

		/* Compose date string */

		if ( of & WD_SHDAT )
		{
			d = datimstr(d, h->attrib.mdate, parent, '-');
			*d++ = ' ';
		}

		/* Compose time string */

		if ( of & WD_SHTIM )
		{
			d = datimstr(d, h->attrib.mtime, parent, ':');
			*d++ = ' ';
		}

		/* Compose attributes */

		if ( of & WD_SHATT )
		{
			switch((unsigned short)hmode)
			{
				/* these attributes handle diverse item types  */
				case S_IFDIR :	/* directory (folder) */
				{
					*d++ = 'd';
					break;
				}
				case S_IFREG :	/* regular file */
				{
					*d++ = '-';
					break;
				}
#if _MINT_
				case S_IFCHR :	/* BIOS special character file */
				{
					*d++ = 'c';
					break;
				}
				case S_IFIFO :	/* pipe */
				{
					*d++ = 'p';
					break;
				}
				case S_IFLNK :	/* symbolic link */
				{
					*d++ = 'l';
					break;
				}
				case __S_IFMEM :	/* shared memory or process data */
				{
					*d++ = 'm';
					break;
				}
#endif
				default : 
				{
					*d++ = '?';
				}
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
				unsigned short aa = h->attrib.attrib;

				/* This is a FAT/VFAT filesystem */

				*d++ = (aa & FA_SYSTEM)   ? 's' : '-';
				*d++ = (aa & FA_HIDDEN)   ? 'h' : '-';
				*d++ = (aa & FA_RDONLY) ? '-' : 'w';
				*d++ = (aa & FA_CHANGED)  ? 'a' : '-';
			}

			*d++ = ' ';
			*d++ = ' ';		
		}

#if _MINT_
		if (((dw->fs_type & FS_UID) != 0) && (of & WD_SHOWN) )
		{
			d = uidstr(d, (_WORD)h->attrib.gid); /* first group id */
			*d++ = '/';
			d = uidstr(d, (_WORD)h->attrib.uid); /* then user id */

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
 * Determine the first icon to (maybe) display.
 * If a macro is used instead of a function, a few bytes
 * are saved in size, and a little bit gained in speed.
 */

#if __USE_MACROS

#define start_ic(w,sl) (long)(sl * w->columns + w->px) 

#else

static long start_ic(DIR_WINDOW *w, _WORD sl)
{
	return (long)(sl * w->columns + w->px);
}

#endif

/*
 * Tell how many icons have to be drawn in a window.
 * Note: this routine does not actuallu -count- the icons, 
 * but calculates their number. This may sometimes give an 
 * incorrect (too big) number.
 */

static long count_ic( DIR_WINDOW *dw, _WORD sl, _WORD lines )
{
	long n, start;

	n = dw->columns * lines;
	start = start_ic(dw, sl);

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
	_WORD sc,				/* first icon column to display */ 
	_WORD ncolumns, 		/* number of icon columns to display */
	_WORD sl, 			/* first icon line to display */
	_WORD lines, 			/* number of icon lines to display */
	bool smode,		/* draw all icons + background if false */
	RECT *work			/* work area of the window */
)
{
	long 
		oi = 0,		/* index of an object in the tree */
		lo,			/* offset of labels area from root object */
		n,			/* approximate number of objects to draw */ 
		start,		/* first icon to be drawn */ 
		i;			/* counter */

	OBJECT 
		*obj;		/* pointer to the root object */

	NDTA 
		*h;			/* pointer to directory items */

	INAME
		*labels;	/* pointers to icons labels */

	_WORD 
		icon_no,
		j,
		ci,			/* column in which an icon is drawn */
		row;

	bool
		hidden;		/* flag for a hidden object */


	/* First icon to be drawn */

	start = start_ic(dw, sl);

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
		row = (_WORD)((sl - dw->py) * iconh);

		/* Set background object */
		wd_set_obj0( obj, smode, row, lines, work );

		/* Note: code below must agree whith dir_drawsel() */

		for (i = 0; i < n; i++)
		{
			bool selected;

			j = (_WORD)(start + i);
			ci = j % dw->columns;

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
				cramped_name(h->name, labels[i], sizeof(INAME));
#else
				strcpy(labels[i], h->name); /* shorter, and safe in single-TOS */
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
					(ci - dw->px) * iconw + XOFFSET,
					(j / dw->columns - sl) * iconh + YOFFSET,
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

void draw_tree(OBJECT *tree, RECT *clip)
{
	objc_draw(tree, ROOT, MAX_DEPTH, clip->x, clip->y, clip->w, clip->h);
}


/*
 * Draw icons in a directory window in icon mode
 */

static void draw_icons(DIR_WINDOW *dw, _WORD sc, _WORD columns, _WORD sl, _WORD lines, RECT *area,
					  bool smode, RECT *work)
{
	OBJECT *tree;

	if ((tree = make_tree(dw, sc, columns, sl, lines, smode, work)) != NULL)
	{
		draw_tree(tree, area);
		free(tree);
	}
}


/*
 * Compute screen position of a directory item in icon mode.
 * See also dir_comparea.
 * Note: parameter 'work' is needed because work area may not be equal to
 * real work area- but this would happen only if there were a menu
 * in the window
 */

static void icn_comparea
(
	DIR_WINDOW *dw, /* pointer to a window */
	_WORD item, 		/* item index in the directory */
	RECT *r1,		/* icon (image?) rectangle */ 
	RECT *r2, 		/* icon text rectangle */
	RECT *work		/* inside area of the window */
)
{
	_WORD 
		columns = dw->columns, 
		s, 
		x, 
		y;

	CICONBLK 
		*h;			/* ciconblk (the largest) */

	s = (_WORD)(item - columns * dw->py); /* item index starting from 1st visible */
	x = s % columns - dw->px; 
	y = s / columns;

	if ((s < 0) && (x != 0))
	{
		y -= 1;
		x = columns + x;
	}

	x = work->x + x * iconw + XOFFSET;
	y = work->y + y * iconh + YOFFSET;

	h = icons[(*dw->buffer)[item]->icon].ob_spec.ciconblk;

	r1->x = h->monoblk.ib_xicon;
	r1->y = h->monoblk.ib_yicon;
	r1->w = h->monoblk.ib_wicon;
	r1->h = h->monoblk.ib_hicon;
	r1->x += x;
	r1->y += y;

	r2->x = h->monoblk.ib_xtext;
	r2->y = h->monoblk.ib_ytext;
	r2->w = h->monoblk.ib_wtext;
	r2->h = h->monoblk.ib_htext;
	r2->x += x;
	r2->y += y;
}


/* 
 * dir_prtline calls VDI routines to actually display a directory line 
 * on screen, in text or in icon mode.
 * This routine is called once for each directory entry.
 * "line" is in fact item index (0 to w->nvisible)
 */
 
void dir_prtline(DIR_WINDOW *dw, _WORD line, RECT *area, RECT *work)
{
	/*
	 * Don't do anything in an iconified window
	 * (and perhaps this is not even needed ?) 
	 */

	if ( (dw->winfo)->flags.iconified != 0 ) 	
		return; 

	if (options.mode == TEXTMODE)
	{
		RECT r, rc, in; 
		NDTA *d = NULL;
		VLNAME s;
		_WORD i;
		
		/* Compute position of a line */

		dir_comparea(dw, line, &r, &r, work);

		/* Clear with pattern the background rectangle if needed */

		if ( line < dw->nvisible )
		{
			d = (*dw->buffer)[(long)line];
			rc.x = r.x - TOFFSET * dir_font.cw;
			rc.y = r.y;
			rc.w = r.w + BORDERS * dir_font.cw;
			rc.h = r.h;

			if((clearline || d->selected) && xd_rcintersect(area, &rc, &in) )
				pclear(&in);
		}

		/* Write the line itself */

		if (xd_rcintersect(area, &r, &in))
		{
			/* Compose a directory line */

			dir_line(dw, s, line);

			/* First column of text to be displayed */

			i = 0;
			xd_clip_on(&in);

			/* Consider only lines with some text */

			if ( d )
			{
				_WORD effects = FE_NONE; /* write normal text until told otherwise */

				/* Show links in italic type, and hidden items grayed */

				if (d->link)
					effects |= FE_ITALIC;

				if ((d->attrib.attrib & FA_HIDDEN) != 0)
					effects |= FE_LIGHT;

				if (effects != FE_NONE)
					vst_effects(vdi_handle, effects); /* set only if needed */	

				w_transptext(r.x, r.y, &s[i]); /* write the string */
				
				if (effects != FE_NONE)
					vst_effects(vdi_handle, FE_NONE);	/* back to normal */

				if (d->selected)
					invert(&in);	/* mark selected item, if needed */
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

static long dir_pymax(DIR_WINDOW *w)
{
	long py = w->py + (long)w->nrows + w->nlines * (w->dcolumns - 1);

	return lmin(py, w->nvisible);
}


/* 
 * Funktie om een pagina naar boven te scrollen. 
 */

void dir_prtcolumn(DIR_WINDOW *dw, _WORD column, _WORD nc, RECT *area, RECT *work)
{
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
			_WORD i, i0, col;

			col = (column - TOFFSET) / (dw->llength + CSKIP);	
			i0 = (_WORD)(dw->nlines * col + dw->py);

			for (i = 0; i < dw->nrows; i++)
				dir_prtline(dw, i0 + i, &in, work);
		}
	}
	else
	{
		r.w = nc * iconw; 
		r.x = work->x + (column - dw->px) * iconw + XOFFSET;

		if (xd_rcintersect(area, &r, &in))
			draw_icons(dw, column, nc, (_WORD)(dw->py), dw->rows, &in, FALSE, work);
	}
}


/*
 * Display a line containing several directory columns
 */

void dir_prtcolumns(DIR_WINDOW *w, long line, RECT *in, RECT *work)
{
	_WORD j, k = 1;

	if ( options.mode == TEXTMODE )
		k = w->dcolumns;

	for ( j = 0; j < k; j++ )
		dir_prtline((DIR_WINDOW *)w, (_WORD)(line + w->nlines * j), in, work);
}


/*
 * Renew the path of a directory window. 
 * Memory for old path is deallocated. Memory for new path
 * has to be allocated before calling this routine.
 * This routine substitutes earlier routine dir_readnew().
 */

static void dir_newpath(DIR_WINDOW *w, char *newpath)
{
	free(w->path);

	w->path = newpath;
	w->py = 0;
	w->px = 0;

	dir_refresh_wd(w);
}




/*
 * Remove a directory and its window from memory
 */

static void dir_rem(DIR_WINDOW *w)
{
	free(w->path);
	free(w->fspec);
	dir_free_buffer(w);

	w->winfo->used = FALSE;

	wd_reset(NULL);
	xw_delete((WINDOW *)w);	
}


/*
 * Either close a directory window completely, or go one level up.
 * If mode > 0 the window is closed completely.
 * If mode < 0 the window is closed to the parent directory
 * If mode = 0 the window is closed either completely or to the parent
 * directory, depending on the state of the 'Show parent' option.
 */

void dir_close(WINDOW *w, _WORD mode)
{
	char 
		*thepath = (char *)((DIR_WINDOW *)w)->path,
		*p;

	if ( w )
	{
		((TYP_WINDOW *)w)->winfo->flags.iconified = 0;
		autoloc_off();

		/* 
		 * Closing a directory window behaves differently, 
		 * depending on whether "Show parent" option is active.
		 */ 

		if( (mode == 0) && (options.attribs & FA_PARDIR) != 0 )
		{

		/*	if(isroot(thepath))  */ /* this produced problems in V4.07 */
				mode = 1;
		}

		if (isroot(thepath) || (mode > 0) )
		{
			/* 
			 * Either this is a root window, or it should be closed entirely;
			 * In any case, close it.
			 */

			xw_close(w);
			dir_rem((DIR_WINDOW *)w);
		}
		else
		{
			/* 
			 * Move one level up the directory tree;
			 * First, extract parent path from the window's path specification.
			 * New memory is allocated for that; 
			 */

			if ((p = fn_get_path(thepath)) != NULL)
			{
				/* Successful; free previous path; set sliders to prev. positions */

				free(thepath);

				((DIR_WINDOW *)w)->path = p;
				((DIR_WINDOW *)w)->px = ((DIR_WINDOW *)w)->par_px; 
				((DIR_WINDOW *)w)->py = ((DIR_WINDOW *)w)->par_py;

				dir_reread((DIR_WINDOW *)w);
				wd_reset((WINDOW *)w); /* remove selection; autolocator off */

				if(((DIR_WINDOW *)w)->par_itm < 0)
				{
					autoloc_upd = TRUE; /* to activate update in w_page() */
					dir_showfirst((DIR_WINDOW *)w, (_WORD)(-(((DIR_WINDOW *)w)->par_itm)));
					autoloc_upd = FALSE;
				}
				else
				{
					wd_type_draw((TYP_WINDOW *)w, TRUE );
				}
			}
		}
	}
}


/********************************************************************
 *																	*
 * Funkties voor het openen van een dir window.  					*
 *																	*
 ********************************************************************/

/* 
 * Open een window. path en fspec moet een gemallocde string zijn.
 * Deze mogen niet vrijgegeven worden door de aanroepende funktie.
 * Bij een fout worden deze strings vrijgegeven. 
 */

static WINDOW *dir_do_open(WINFO *info, char *path, char *fspec, _WORD px, _WORD py, int *error) 
{
	DIR_WINDOW
		*w;

	RECT
		oldsize;

	WDFLAGS
		oldflags;


	wd_in_screen( info ); /* modify position to come into screen */

	/* Create a window. Note: window structure is completely zeroed here */

	if
	(
		(*path == 0) || 
		(
			w = (DIR_WINDOW *)xw_create(DIR_WIND, &wd_type_functions, DFLAGS, &dmax, sizeof(DIR_WINDOW), NULL, error)
		) == NULL
	)
	{
		free(path);
		free(fspec);
		return NULL;
	}

	autoloc_off();

	/* Fields not explicitely specified will remain zeroed */

	info->typ_window = (TYP_WINDOW *)w;
	w->itm_func = &itm_func;
	w->px = px;
	w->py = py;
	w->path = (char *)path;
	w->fspec = fspec;
	w->winfo = info;

	/* In order to calculate columns and rows properly... */

	oldsize.x = info->x; /* remember old size in char-cell units */
	oldsize.y = info->y;
	oldsize.w = info->w;
	oldsize.h = info->h;
	
	wd_restoresize(info); /* temporarily- noniconified size */

	oldflags = info->flags;

	wd_setnormal(info); /* temporarily neither fulled nor iconified state */

	/* Now read the contents of the directory */

	noicons = FALSE; /* so that missing icons be reported */

	*error = dir_readandset(w);

	if (*error != 0)
	{
		dir_rem(w);
		return NULL;
	}
	else
	{
		/* 
		 * oldsize and oldflags now contain information about
		 * iconified or fulled size or state, if any
		 */

		wd_iopen((WINDOW *)w, &oldsize, &oldflags); 

		return (WINDOW *)w;
	}
}


/*
 * Set the sliders in  directory window to show the first selected item
 */

static void dir_showfirst(DIR_WINDOW *dw, _WORD i)
{
	long 
		dwpy,
		nlines = lmax(1L, dw->nlines);

	_WORD 
		dwpx;


	if ( options.mode == TEXTMODE )
	{
		dwpx = (_WORD)((i / nlines) * (dw->llength + CSKIP));
		dwpy = i % nlines;
	}
	else
	{
		dwpx = i % dw->columns;
		dwpy = (long)(i / dw->columns);
	}

	w_page((TYP_WINDOW *)dw, dwpx, dwpy);
}


/*
 * Create a directory window
 * Note: all elements will be deallocated at window closing,
 * must be kept until then.
 * In case of an error, path is freed.
 */

bool dir_add_window
(
	char *path,	/* path of the window to open */ 
	char *thespec,/* file mask specification */
	const char *name	/* name to be selected in the open window, NULL if none */
) 
{
	char *fspec;		/* current (default) file specification */
	
	WINDOW
		*w;			/* pointer to the window being opened */

	WINFO
		*dirwj;

	_WORD 
		j = 0;		/* counter */ 
	int error;		/* error code */


	/* Find an unused directory window slot */

	dirwj = dirwindows;

	while ((j < MAXWINDOWS - 1) && dirwj->used )
	{
		j++;
		dirwj++;
	}

	if((error = x_checkname(path, thespec)) != 0)
	{
		/* path or filespec is too long or wrong somehow */

		xform_error(error);
	}
	else if(dirwj->used)
	{
		/* All windows are used, can't open any more */

		alert_iprint(MTMWIND); 
	}
	else
	{
		/* OK; allocate space for file specification */

		if ( thespec )
			fspec = thespec;
		else
			fspec = strdup(fsdefext);

		if(fspec)
		{
			/* Ok, now we can really open the window, in WINFO slot "j" */

			if ( (w = dir_do_open(dirwj, path, fspec, 0, 0, &error)) == NULL)
			{
				xform_error(error);
				return FALSE;
			}
			else
			{
				/* 
				 * Code below shows search result; if a name is specified,
				 * this means that a search has been performed
				 */

				if ( name )
				{ 
					_WORD
						nv,    /* number of visible items in the directory */
						i;     /* item counter */

					NDTA
						*h;

					DIR_WINDOW 
						*dw = (DIR_WINDOW *)(dirwj->typ_window);

					nv = dw->nvisible; 

					/* Find the first item that matches the name */

					for ( i = 0; i < nv; i++ )
					{
						h = (*dw->buffer)[i]; 

						if ( strcmp( h->name, name ) == 0 )
						{
							dir_showfirst(dw, i);

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

	free(path);
	free(thespec);
	return FALSE;
}


/*
 * For the sake of size optimization, a call with fewer parameters
 */

bool dir_add_dwindow(char *path)
{
	return dir_add_window(path, NULL, NULL);
}


/* 
 * Open a directory window upon [Alt][A] to [Alt][Z];
 * If 'w' is not given, open in a new window.
 * Return TRUE if key is ALT_A to ALT_Z
 */

bool dir_onalt(_WORD key, WINDOW *w)
{
	char
		*newpath;

	_WORD
		i;


	if(key >= ALT_A && key <= ALT_Z)
	{
		i = key - (XD_ALT | 'A');
		if (check_drive(i))
		{
			if ((newpath = strdup(adrive)) != NULL)
			{
				newpath[0] = (char)i + 'A';

				if(w)
				{
					dir_newpath((DIR_WINDOW *)w, newpath);
				}
				else
					dir_add_dwindow(newpath);
			}
		}

		return TRUE;
	}

	return FALSE;
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
	{ CFG_HDR, "dir", { 0 } },
	{ CFG_BEG, NULL, { 0 } },
	{ CFG_D,   "indx", { &that.index		} },
	{ CFG_S,   "path", { that.path		} },
	{ CFG_S,   "mask", { that.spec		} },
	{ CFG_D,   "xrel", { &that.px		} },
	{ CFG_L,   "yrel", { &that.py		} },
	{ CFG_END, NULL, { 0 } },
	{ CFG_LAST, NULL, { 0 } }
};


/*
 * Load or save one open directory window
 */

TYP_WINDOW *wd_that(WINFO *wi, SINFO2 *that);

void dir_one(XFILE *file, int lvl, int io, int *error)
{
	if (io == CFG_SAVE)
	{
		_WORD i = 0;
		DIR_WINDOW *dw;

		/* Identify window's index in WINFO by pointer to that window */

		while ((dw = (DIR_WINDOW *)(dirwindows[i].typ_window )) != (DIR_WINDOW *)that.w)
			i++;

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
		*error = CfgLoad(file, dirw_table, (_WORD)sizeof(VLNAME), lvl); 

		if ( (*error == 0 ) && (that.path[0] == 0 || that.spec[0] == 0 || that.index >= MAXWINDOWS) )
			*error = EFRVAL;

		if (*error == 0)
		{
			/* Note: a few bytes may be wasted in the allocation for 'spec' */

			char *path = malloc(strlen(that.path) + 1);
			char *spec = malloc(lmax(strlen(that.spec), strlen(fsdefext)) + 1);

			if (path && spec)
			{
				WINFO *dirwi = &dirwindows[that.index];

				/* 
				 * Note1: window's index in WINFO is always taken from the file;
				 * Otherwise, position and size would not be preserved
				 * Note2: in case of error path & spec are deallocated in dir_do_open 
				 */

				strcpy(path, that.path);
				strcpy(spec, (dirwi->flags.setmask) ? that.spec : fsdefext);

				dir_do_open
				(
					dirwi,
					path,
					spec,
					that.px,
					(_WORD)(that.py),
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


void dir_config(XFILE *file, int lvl, int io, int *error)
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

static _WORD dir_find(WINDOW *w, _WORD x, _WORD y)
{
	_WORD 
		hx,	
		hy, 
		cw,
		ch,
		item;

	RECT 
/* see below
		work,
*/
		r1, 
		r2;

	DIR_WINDOW 
		*dw;

	void (*dw_comparea)(DIR_WINDOW *w, _WORD item, RECT *r1, RECT *r2, RECT *work);

	/*
	 * Don't do anything in iconified window
	 * (and perhaps this is not even needed ?) 
	 */

	if ( (((TYP_WINDOW *)w)->winfo)->flags.iconified != 0 ) 	
		return -1; 

	dw = (DIR_WINDOW *)w;

#if 0 /* no need if there are no menus in dir windows */
	xw_getwork(w, &work); /* work area modified by menu height */
#endif
	wd_cellsize((TYP_WINDOW *)w, &cw, &ch, TRUE);

#if 0 /* see above */
	hx = (x - work.x) / cw;
	hy = (y - work.y) / ch;
#endif
	hx = (x - w->xw_work.x) / cw;
	hy = (y - w->xw_work.y) / ch;

	if ( hx < 0 || hy < 0 ) 
		return -1;

	if (options.mode == TEXTMODE)
	{
		hx = max(0, (hx - TOFFSET + dw->px)/(dw->llength + CSKIP));

		item = (_WORD)(dw->nlines * hx + dw->py) + hy;
		dw_comparea = dir_comparea;
	}
	else
	{
		if ( hx >= dw->columns )
			return -1;

		item = (_WORD)(hx + dw->px + (dw->py + hy) * dw->columns);
		dw_comparea = icn_comparea;
	}
		
	if (item >= dw->nvisible)
		return -1;

#if 0 /* see above */
	dw_comparea(dw, item, &r1, &r2, &work);
#endif
	dw_comparea(dw, item, &r1, &r2, &(w->xw_work));

	/* note r2 is available only in icon mode */

	if
	(
		xd_inrect(x, y, &r1) || 
		(
			options.mode != TEXTMODE && 
			xd_inrect(x, y, &r2)
		)
	)
		return item;
	else
		return -1;
}


/* 
 * Funktie die aangeeft of een directory item geselecteerd is. 
 */

static bool dir_state(WINDOW *w, _WORD item)
{
	return (*((DIR_WINDOW *) w)->buffer)[item]->selected;
}


/* 
 * Funktie die het type van een directory item teruggeeft. 
 */

static ITMTYPE dir_itmtype(WINDOW *w, _WORD item)
{
	return (*((DIR_WINDOW *) w)->buffer)[item]->item_type;
}



static ITMTYPE dir_tgttype(WINDOW *w, _WORD item)
{
	return (*((DIR_WINDOW *) w)->buffer)[item]->tgt_type;
}


/* 
 * Funktie die de naam van een directory item teruggeeft. 
 */

static const char *dir_itmname(WINDOW *w, _WORD item)
{
	return (*((DIR_WINDOW *) w)->buffer)[item]->name;
}


/*
 * Return pointer to a string with full path + name of an item in a window.
 * Note: this routine allocates space for the fullname string.
 * If the fullname becomes too long it will not be allocated.
 */

static char *dir_fullname(WINDOW *w, _WORD item)
{
	NDTA *itm = (*((DIR_WINDOW *)w)->buffer)[item];

	if (itm->item_type == ITM_PREVDIR)
		return fn_get_path(((DIR_WINDOW *)w)->path);
	else
		return fn_make_path(((DIR_WINDOW *)w)->path, itm->name);
}


/* 
 * Funktie die de dta van een directory item teruggeeft. 
 */

static _WORD dir_attrib(WINDOW *w, _WORD item, _WORD mode, XATTR *attr)
{
	NDTA *itm = (*((DIR_WINDOW *)w)->buffer)[item];
	char *name;
	_WORD error;

	if ((name = x_makepath(((DIR_WINDOW *)w)->path, itm->name, &error)) != NULL)
	{
		error = (_WORD)x_attr(mode, ((DIR_WINDOW *)w)->fs_type, name, attr);
		free(name);
	}

	return error;
}


/*
 * Is the item perhaps a link?
 */

static bool dir_islink(WINDOW *w, _WORD item)
{
#if _MINT_
	return (*((DIR_WINDOW *) w)->buffer)[item]->link;
#else
	(void) w;
	(void) item;
	return FALSE;
#endif
}


/* 
 * Funktie voor het zetten van de nieuwe status van de items in een window 
 */

static void dir_setnws(DIR_WINDOW *w, bool draw)
{
	RPNDTA
		*pb;

	_WORD
		i,
		hh,
		n = 0;
 
	LSUM 
		bytes;

	
	bytes.bytes = 0;
	bytes.kbytes = 0;

	hh = w->nvisible;
	pb = w->buffer;

	for (i = 0; i < hh; i++)
	{
		NDTA *h = (*pb)[i];

		h->selected = h->newstate;

		/* Count numbers and sum sizes- but dont add directory sizes */

		if (h->selected)
		{
			n++;
			if((h->attrib.mode & S_IFMT) != S_IFDIR)
				add_size(&bytes, h->attrib.size);
		}
	}

	w->nselected = n; 
	w->selbytes = bytes;

	if ( draw )
		dir_info(w);
}


/* 
 * Funktie voor het selecteren en deselecteren van items waarvan
 * de status veranderd is. 
 * See also dsk_drawsel() ! Functionality is somewhat duplicated
 * for the sake of colour icons when the background object has to be
 * redrawn as well. An attempt is made to optimize the drawing for speed,
 * depending on the number of icons to be drawn (switch mode at MSEL)
 */

#define MSEL 8 /* if so many icons change state, draw complete window */

static void dir_drawsel(DIR_WINDOW *w)
{
	long
		i,
		h;

	RPNDTA
		*pb = w->buffer;

	RECT
		r1,
		r2,
		d,
		work;

#if 0 /* this can be a bit simpler if there are no menus in dir windows */
	xw_getwork((WINDOW *)w, &work); /* work area modified by menu height */
#endif
	work = w->xw_work;

	xd_begupdate();
	xd_mouse_off();

	if(options.mode != TEXTMODE)
	{
		/* Display as icons */ 

		OBJECT 
			*tree;
		_WORD 
			j,
			j0 = (_WORD)start_ic(w, (_WORD)(w->py)), 
			n = 0, 
			ncre = 0;

		NDTA 
			*b;

		bool
			all = TRUE,		/* draw all icons at once */ 
			sm = TRUE;		/* don't draw background and unchanged icons */


		/* Attempt to optimize (slow) redraw of animated colour icons */

		if ( colour_icons )
		{
			/* Count how many icons have to be redrawn */

			ncre = (_WORD)count_ic(w, (_WORD)w->py, w->rows);

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

		if ((tree = make_tree(w, w->px, w->ncolumns, (_WORD)w->py, w->rows, sm, &work)) == NULL)
			return;

		/* Now draw it */

		if ( colour_icons )
		{
			_WORD oi = 1; 

			/* Background object will always be visible for colour icons */

			wxub.ob_type &= 0xFF00;
			wxub.ob_type |= G_BOX;

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
					tree[j + 1].ob_state = (b->newstate) ? OS_SELECTED : OS_NORMAL;
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
	else
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
					dir_comparea(w, (_WORD)i, &r1, &r1, &work);

					if (xd_rcintersect(&r1, &r2, &d))
						invert(&d);
				}
			}

			xd_clip_off();
			xw_getnext((WINDOW *)w, &r2);
		}
	}


	xd_mouse_on();
	xd_endupdate();
}


/*
 * Funktie voor het selecteren van een item in een window.
 * mode = 0: selecteer selected en deselecteer de rest.
 * mode = 1: inverteer de status van selected.
 * mode = 2: deselecteer selected.
 * mode = 3: selecteer selected.
 * mode = 4: select all (except parent directory). 
 */

static void dir_select(WINDOW *w, _WORD selected, _WORD mode, bool draw)
{
	_WORD i, h;
	RPNDTA *pb;

	/* 
	 * Don't do anything in iconified window.
	 * Note: simulated dir windows do not 'exist' so are not recognized here
	 */

	if ( xw_exist(w) && (((TYP_WINDOW *)w)->winfo)->flags.iconified != 0 ) 
/*	
		return; 
*/
		draw = FALSE; /* maybe better so? */

	h = ((DIR_WINDOW *)w)->nvisible;
	pb = ((DIR_WINDOW *)w)->buffer;

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
		dir_drawsel((DIR_WINDOW *)w);

	dir_setnws((DIR_WINDOW *) w, draw);
}


/*
 * Select items in a directory window according to name mask in 'automask'
 */

void dir_autoselect(DIR_WINDOW *w)
{
	_WORD 
		k,
		k1 = 0,
		nk = w->nvisible - 1;
 
	RPNDTA
		*pb = w->buffer;


	if(pb == NULL)
		return;

	for(k = nk; k >= 0; k--) /* this loops ends with k being 0 */
	{
		NDTA *b = (*pb)[k];

		b->selected = FALSE;
		b->newstate = FALSE;

		if(cmp_wildcard(dir_itmname((WINDOW *)w, k), automask))
		{
			b->newstate = TRUE;
			k1 = k;
		}
	}

	dir_setnws(w, TRUE);
	dir_showfirst(w, k1);

	selection.w = (WINDOW *)w;
	selection.n = w->nselected; 

	if(selection.n == 1)					
		selection.selected = k1;
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
	_WORD *s,			/* index of first item */ 
	_WORD *n			/* number of visible items */
)
{
	_WORD h;

	*s = (_WORD)dw->py;

	if (options.mode == TEXTMODE)
		h = (_WORD)dir_pymax(dw) - *s;
	else
	{
		*s *= dw->columns;
		h = dw->columns * dw->rows;
	}

	if ((*s + h) > dw->nvisible)
		h = max(0, dw->nvisible - *s); 

	*n = h;
}


/* 
 * Rubberbox funktie met scrolling.
 * This is for directory windows only. See also icon.c 
 * Note1: window can be scrolled in x and y simultaneously
 * Note2: if set_rect_default() is removed, rubberbox will not be correctly
 * redrawn when a window is scrolled.
 */

static void rubber_box(DIR_WINDOW *w, RECT *work, _WORD x, _WORD y, RECT *r)
{
	_WORD 
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

	bool 
		released, 
		redraw;


	wd_cellsize((TYP_WINDOW *)w, &absdeltax, &absdeltay, TRUE);

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

		if ((x2 <= work->x) && (w->px > 0) && (x1 < SHRT_MAX - absdeltax) )
		{
			redraw = TRUE;
			scrollx = WA_LFLINE;
			deltax = absdeltax;
		}

		if ((y2 <= work->y) && (w->py > 0) && (y1 < SHRT_MAX - absdeltay) )
		{
			redraw = TRUE;
			scrolly = WA_UPLINE;
			deltay = absdeltay;
		}

		if ((x2 >= rx) && ((w->px + w->ncolumns) < w->columns) && (x1 > -SHRT_MAX + absdeltax) )
		{
			redraw = TRUE;
			scrollx = WA_RTLINE;
			deltax = -absdeltax;
		}

		if ((y2 >= ry) && ((w->py + w->nrows) < w->nlines) && (y1 > -SHRT_MAX + absdeltay) )
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

static void dir_rselect(WINDOW *w, _WORD x, _WORD y)
{
	long 
		i, h;

	RECT 
/* no need, see below
		work,
*/
		r1, r2, r3;


	NDTA
		*b;

	RPNDTA 
		*pb;

	void 
		(*dw_comparea)(DIR_WINDOW *w, _WORD item, RECT *r1, RECT *r2, RECT *work);


	/* Don't do anything in an iconified window */

	if ((((TYP_WINDOW *)w)->winfo)->flags.iconified != 0  ) 
		return;

#if 0 /* no need if there is no menu in dir window */
	xw_getwork(w, &work); /* work area modified by menu height */
	rubber_box((DIR_WINDOW *) w, &work, x, y, &r1);
#endif
	rubber_box((DIR_WINDOW *)w, &(w->xw_work), x, y, &r1);

	h = (long)(((DIR_WINDOW *)w)->nvisible);
	pb = ((DIR_WINDOW *)w)->buffer;

	for (i = 0; i < h; i++ )
	{
		b = (*pb)[i];

		if (options.mode == TEXTMODE)
			dw_comparea = dir_comparea;
		else
			dw_comparea = icn_comparea;

#if 0 /* see above */
		dw_comparea((DIR_WINDOW *) w, (_WORD)i, &r2, &r3, &work);
#endif
		dw_comparea((DIR_WINDOW *) w, (_WORD)i, &r2, &r3, &(w->xw_work));

		/* Note: r3 is available only in icons mode */

		if(rc_intersect2(&r1, &r2) || (options.mode != TEXTMODE && rc_intersect2(&r1, &r3))) 
			b->newstate = !(b->selected);
#if 0 /* Not needed? */
		else
			b->newstate = b->selected;
#endif
	}

	dir_drawsel((DIR_WINDOW *)w);
	dir_setnws((DIR_WINDOW *)w, TRUE);
}


static void get_itmd(DIR_WINDOW *wd, _WORD obj, ICND *icnd, _WORD mx, _WORD my, RECT *work)
{
	RECT
		ir;

	_WORD
		*icndcoords = &icnd->coords[0];


	if (options.mode == TEXTMODE)
	{
		dir_comparea(wd, obj, &ir, &ir, work);

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

		icnd->item = obj;
		icnd->np = 5;
	}
	else
	{
		_WORD 
			columns = wd->columns, 
			s = obj - (_WORD)(wd->py * columns);

		RECT 
			tr;

		icn_comparea(wd, obj, &ir, &tr, work);

		ir.x -= mx;
		ir.y -= my;
		ir.w -= 1;
		tr.x -= mx;
		tr.y -= my;
		tr.w -= 1;
		tr.h -= 1;

		icnd->item = obj;
		icnd->m_x = work->x + (s % columns - wd->px) * iconw + iconw / 2 - mx;
		icnd->m_y = work->y + (s / columns) * iconh + iconh / 2 - my;

		icn_coords(icnd, &tr, &ir); 
	}
}


/*
 * Create a list of (visible ?) selected items
 * This routine will return a NULL list pointer if no list has been created.
 * Note: at most 32767 items can be returned because 'nselected'  and
 * 'nvisible' are 16 bits long
 */

static _WORD *get_list(DIR_WINDOW *w, _WORD *nselected, _WORD *nvisible)
{
	_WORD 
		n = 0, 		/* number of selected items */
		nv = 0, 	/* number of visible items */
		*sel_list,	/* pointer to list of selected items */
		*list,		/* pointer to an item in the list of selected items */ 
		s,			/* first visible item */ 
		h,			/* estimated number of visible items */
		i, 			/* item index */
		m;			/* number of visible items in window specification */

	RPNDTA 
		*d;


	/* Find selected items */

	d = w->buffer;
	m = w->nvisible;

	if((w->xw_xflags & XWF_SIM) != 0)
	{
		s = 0;
		h = 1;
	}
	else
	{
		calc_vitems(w, &s, &h);
		h += s;
	}

	for (i = 0; i < m; i++)
	{
		if ((*d)[i]->selected)
		{
			n++;

			if ((i >= s) && (i < h))
				nv++;
		}
	}

	*nvisible = nv;

	/* Return FALSE if nothing has been selected */

	if (n)
	{
		/* Fill the list */

		list = malloc_chk(n * sizeof(*list));

		sel_list = list;

		if (list)
		{
			*nselected = n;

			for (i = 0; i < m; i++)
			{
				if ((*d)[i]->selected)
					*list++ = i;
			}

			return sel_list;
		}
	}

	*nselected = 0;

	return NULL;
}


/* 
 * Routine voor het maken van een lijst met geselekteerde items. 
 */

static ICND *dir_xlist
(
	WINDOW *w,
	_WORD *nselected,
	_WORD *nvisible,
	_WORD **sel_list,
	_WORD mx,
	_WORD my
)
{
	ICND
		*icns,
		*icnlist;

	RPNDTA
		*d;

	long
		i;


#if 0 /* No need if there is no menu in dir windows */
	RECT work;

	xw_getwork(w, &work); /* work area modified by menu height */
#endif

	/* if *nvisible is FALSE get_list() will return NULL */

	if ((*sel_list = get_list((DIR_WINDOW *)w, nselected, nvisible)) != NULL)
	{

/* this can never happen ?

		if (nselected == 0)
		{
			*icns = NULL;
			return TRUE;
		}
*/
		/* *nvisible will always be larger than 0 here */

		icnlist = malloc_chk(*nvisible * sizeof(ICND));

		if (icnlist)
		{
			_WORD s, n, h;

			icns = icnlist;

			calc_vitems((DIR_WINDOW *)w, &s, &n);

			h = s + n;
			d = ((DIR_WINDOW *)w)->buffer;

			for (i = s; (_WORD)i < h; i++)
			{
				if ((*d)[i]->selected)
/* see above
					get_itmd((DIR_WINDOW *)w, (_WORD)i, icnlist++, mx, my, &work);
*/
					get_itmd((DIR_WINDOW *)w, (_WORD)i, icnlist++, mx, my, &(w->xw_work));
			}

			return icns;
		}
	
		free(*sel_list);
	}

	return NULL;
}


/* 
 * Routine voor het maken van een lijst met geseleketeerde items. 
 * Note: at most 32767 items can be returned because 'n' is 16 bits long
 */

static _WORD *dir_list(WINDOW *w, _WORD *n)
{
	_WORD	dummy;

	return get_list((DIR_WINDOW *)w, n, &dummy);
}


/*
 * Open an item in a directory window. If ALT is pressed, open
 * item in a new window, or whatever. If ALT is -not- pressed,
 * item may be opened in the same window if it is of the right type.
 * IF LSHIFT is pressed and the item is a link to a directory,
 * the item will be opened in the actual directory path, not
 * in the path of the link.
 */

static bool dir_open(WINDOW *w, _WORD item, _WORD kstate)
{
	if((kstate & K_ALT) == 0)
	{
		ITMTYPE
			thetype = dir_tgttype(w, item);

		if (thetype == ITM_FOLDER)
		{
			char
				*newpath = NULL,
				*name = dir_fullname(w, item);

			long
				py;

			_WORD
				px;

			if(dir_islink(w, item) && (kstate & K_LSHIFT) )
			{
				newpath = x_fllink(name); /* NULL if name is NULL */
				free(name);
				px = 0;
				py = 0;
				item = 0;
			}
			else
			{
				newpath = name;
				px = ((DIR_WINDOW *)w)->px,
				py = ((DIR_WINDOW *)w)->py;
			}

			if(newpath)
			{
				autoloc_off();
				dir_newpath((DIR_WINDOW *)w, newpath);
				((DIR_WINDOW *)w)->par_px = px;
				((DIR_WINDOW *)w)->par_py = py;
				((DIR_WINDOW *)w)->par_itm = item;
			}

			return FALSE;
		}

		if (thetype == ITM_PREVDIR)
		{
			dir_close(w, -1);
			return FALSE;
		}
	}
		
	return item_open(w, item, kstate, NULL, NULL);
}


static bool dir_copy
(
	WINDOW *dw,
	_WORD dobject,
	WINDOW *sw,
	_WORD n,
	_WORD *list,
	ICND *dummyicns,
	_WORD dummyx,
	_WORD dummyy,
	_WORD kstate
)
{
	(void)dummyicns;
	(void)dummyx;
	(void)dummyy;
	return item_copy(dw, dobject, sw, n, list, kstate);
}


/* 
 * Simulate (partially) an open directory window containing just one item 
 * so that a number of operations can be executed using existing routines
 * that expect an item selected in a directory window.
 * Note: this window will have a NULL pointer to WINFO structure, and so
 * it can be distinguished from a real window.
 * Also, 'sim window' flag is set.
 * This routine does -not- allocate or deallocate any memory
 * that has to be freed later. The simulated window is -not- included
 * in the chained list of windows and so will not be detected by xw_exist().
 * Beware that 'path' and 'name' must exist as long as data from this window
 * is used.
 */

void dir_simw(DIR_WINDOW *dw, char *path, const char *name, ITMTYPE type)
{
	NDTA buffer;
	NDTA *pbuffer[1]; 

	XATTR
		att;

	char
		*fname;

	/* clear the structure */

	memclr(dw, sizeof(DIR_WINDOW));
	memclr(&buffer, sizeof(NDTA));
	memclr(&att, sizeof(XATTR));

	/* If path is not given, dw.fs_type should become nonzero below */

	dw->xw_type = DIR_WIND;					/* this is a directory window */
	dw->xw_xflags |= XWF_SIM;				/* and a simulated one */
	dw->path = path;						/* directory path */
	dw->fs_type = x_inq_xfs(path);			/* filesystem? */
	dw->itm_func = &itm_func;				/* pointers to specific functions */
	dw->nfiles = 1;							/* number of files in window */
	dw->nvisible = 1;						/* one visible item */
	dw->buffer = &pbuffer;					/* location of NDTA pointer array */

	pbuffer[0] = &buffer;					/* point to buffer */

	buffer.name = name;						/* filename only */
	buffer.item_type = type;				/* type of item in the dir */
	buffer.tgt_type = type; 				/* not exactly correct yet... */

	/* Get this object's extendend attributes. Ignore errors */

	fname = dir_fullname((WINDOW *)dw, 0);

	if(fname)
	{
		x_attr(1, dw->fs_type, fname, &att); /* of the object itself */

		if((att.st_mode & S_IFMT) == S_IFLNK)
		{
			char *tname = x_fllink(fname);

			if(tname)
			{
				buffer.link = TRUE;
				buffer.tgt_type = diritem_type(tname);
				free(tname);
			}
		}

		free(fname);
	}

	xattr_to_fattr(&att, &buffer.attrib);	/* item attributes */ 

	/* Now "select" the one and only object in this "window" */

	dir_select( (WINDOW *)dw, 0, 3, FALSE);
}


/*
 * Try to find what item type should be assigned to a name,
 * so that this item can be opened in the appropriate way.
 * This routine is used in open.c, and only after
 * the target of a possible link has been determined.
 * It is also used in icon.c and va.c
 */

ITMTYPE diritem_type(char *fullname )
{
	XATTR attr;
	_WORD error;

	attr.st_attr = 0;

	/* Is this a network object ? */

	if( x_netob(fullname) )
		return ITM_NETOB;

	/* Is this name that of a volume root directory ? */

	if ( isroot(fullname) )
		return ITM_DRIVE;

	/* Is this name that of the parent directory ? */

	if ( strcmp(fn_get_name(fullname), prevdir) == 0 )
		return ITM_PREVDIR;

	/* Get info on the object itself, don't follow links */

	if((error = x_checkname(fullname, NULL)) == 0)
	{
		_WORD fs_type = x_inq_xfs(fullname);

		if (x_attr(  1, fs_type, fullname, &attr )  >= 0 )
		{
			if ( attr.st_attr & FA_DIR ) 
				return ITM_FOLDER;
			else if ( !( attr.st_attr & FA_LABEL) )
			{
				if ( dir_isexec(fn_get_name(fullname), &attr) )
					return ITM_PROGRAM;
				else
					return ITM_FILE;
			}
		}
		else
			alert_iprint(MNOITEM); /* item not found or item type unknown */
	}
	else
		xform_error(error);

	return ITM_NOTUSED;
}
