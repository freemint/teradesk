/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
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


typedef struct fattr
{
	unsigned int mode;
	long size;
	unsigned int mtime, mdate;
	unsigned int attrib;
} FATTR;

typedef struct
{
	boolean selected,
	        newstate,
	        visible,
	        link;
	int index;
	struct fattr attrib;
	ITMTYPE item_type;
	int icon;
/*	const char *icname;  HR 120803 never set */
	const char *name;
	char alname[];		/* to be allocated together with NDTA */
} NDTA;

/* Note: take care of compatibility between TXT_WINDOW, DIR_WINDOW, TYP_WINDOW */

#define OLD_DIR 0

typedef NDTA *RPNDTA[];			/* () ref NDTA */ /* array of pointers */

typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
	WD_VARS;					/* other common header data */

	struct winfo *winfo;		/* pointer to WINFO structure. */

	/* three window-type structures are identical up to this point */

	char info[60];				/* info line of window */

	const char *path;
	const char *fspec;

	int fs_type;				/* We need to know the filesystem type for formatting purposes. */
	int nfiles;					/* number of files in directory */
	int nvisible;				/* number of visible files in directory */
	int nselected;
	long usedbytes;
	int namelength;				/* length of longest name in the directory */
#if OLD_DIR
	NDTA *buffer;
#else
	RPNDTA *buffer;			/* HR 120803: change to pointer to pointer array */
							/* ref to row of ref to NDTA */
							/* ref () ref NDTA */
							/* ptr to ptr_array */
	/* I first defined simply NDTA **buffer, which is not the same,
	   doesnt look correct and indeed didnt work.
	   I didnt see a way to define type ref () ref NDTA (algol 68) in C
	   without the intermediate RPNDTA type .
	   But at least this way it works.
	*/
#endif
	boolean refresh;

} DIR_WINDOW;

#define DELTA   2	/* distance between two dir lines in pixels */
#define BORDERS 4	/* Total length of left and right border of a dir line (system-font char widths) */
#define XOFFSET	8	/* for positoning of icons in dir window */
#define YOFFSET	4	/* for positioning of icons in dir window */ 


void dir_init(void);
void dir_default(void);

#if !TEXT_CFG_IN
int dir_load_window(XFILE *file); 	
#endif

int dir_save (XFILE *file, WINDOW *w, int lvl);
boolean dir_add_window(const char *path, const char *name);
void dir_close(WINDOW *w, int mode);

const char *dir_path(WINDOW *w);
void get_dir_line(WINDOW *dw, char *s, int item);
void dir_always_update (WINDOW *w);

void calc_nlines(DIR_WINDOW *w);		
int linelength(DIR_WINDOW *w);			
void dir_title(DIR_WINDOW *w);
void dir_prtline(DIR_WINDOW *dw, int line, RECT *area, RECT *work);
void do_draw(DIR_WINDOW *dw, RECT *r, OBJECT *tree, boolean clr,
					boolean text, RECT *work); 
void dir_prtcolumn(DIR_WINDOW *dw, int column, RECT *area, RECT *work);
void dir_refresh_wd(DIR_WINDOW *w);
void dir_readnew(DIR_WINDOW *w);
OBJECT *make_tree(DIR_WINDOW *dw, int sl, int lines, boolean smode, RECT *work);
void dir_simw(DIR_WINDOW *dw, char *path, char *name, ITMTYPE type, size_t size, int attrib);
ITMTYPE diritem_type( char *fullname );

