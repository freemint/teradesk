/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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
	long size;
	unsigned int mode;
	unsigned int mtime, mdate;
	unsigned int attrib;
#if _MINT_
	unsigned int gid, uid;
#endif
} FATTR;

typedef struct
{
	bool selected;
	bool newstate;
	bool visible;
	bool link;
	int index;
	struct fattr attrib;
	ITMTYPE item_type;
	ITMTYPE tgt_type;	/* target object type */
	int icon;			/* id. of an icon assignded to this object */
	const char *name;	/* pointer to a name string of this object */
	char alname[];		/* to be allocated together with NDTA */
} NDTA;


/* Note: take care of compatibility between TXT_WINDOW, DIR_WINDOW, TYP_WINDOW */

typedef NDTA *RPNDTA[];			/* () ref NDTA */ /* array of pointers */

typedef struct
{
	ITM_INTVARS;			/* Interne variabelen bibliotheek. */
	WD_VARS;				/* other common header data */

	/* three window-type structures are identical up to this point */

#if _MINT_
	char info[212];			/* info line of window */
#else
	char info[80];			/* info line of window */
#endif
	const char *fspec;		/* filename mask for the window */
	LSUM usedbytes;			/* total size of files in the dir. */
	LSUM visbytes;			/* total size of visible files */
	LSUM selbytes;			/* total size of selected items */
	int fs_type;			/* We need to know the filesystem type for formatting purposes. */
	int nfiles;				/* number of files in directory */
	int nvisible;			/* number of visible files in directory */
	int nselected;			/* number of selected items in directory */
	int namelength;			/* length of longest name in the directory */
	int llength;			/* length of a directory line in text mode */
	int reserved;			/* currently unused */
	int par_px;				/* Position of the slider in the parent window */
	long par_py;			/* Position of the slider in the parent window */
	long par_itm;			/* index of this dir in the parent dir */
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

	bool refresh;
	bool va_refresh;

} DIR_WINDOW;

#define DELTA   2	/* distance between two dir lines in pixels */
#define BORDERS 4	/* Total length of left and right border of a dir line (system-font char widths) */
#define XOFFSET	8	/* for positoning of icons in dir window */
#define YOFFSET	4	/* for positioning of icons in dir window */ 
#define CSKIP   2   /* skip space between directory columns */

#define DO_PATH_TOP 	0
#define DO_PATH_UPDATE	1


extern XDFONT dir_font;
extern WINFO dirwindows[MAXWINDOWS];		/* some information about open windows */
extern RECT dmax;	/* maximum window size */

CfgNest dir_one;

bool dir_add_window(const char *path, const char *thespec, const char *name);
bool dir_add_dwindow(const char *path);
bool dir_onalt(int key, WINDOW *w);
void dir_close(WINDOW *w, int mode);
const char *dir_path(WINDOW *w);
void dir_filemask(DIR_WINDOW *w);
void dir_newfolder(WINDOW *w);
void dir_sort(WINDOW *w);
void dir_seticons(WINDOW *w);
void dir_autoselect(DIR_WINDOW *w);
void dir_briefline(char *tstr, XATTR *att);
void dir_line(DIR_WINDOW *dw, char *s, int item);
void dir_disp_mode(WINDOW *w);
void dir_mode(WINDOW *w);
void dir_newdir( WINDOW *w );
void dir_reread( DIR_WINDOW *w );
void calc_nlines(DIR_WINDOW *w);		
int linelength(DIR_WINDOW *w);
void dir_columns(DIR_WINDOW *dw);
void dir_info(DIR_WINDOW *w);
bool dir_isexec(const char *name, XATTR *attr);
void dir_prtline(DIR_WINDOW *dw, int line, RECT *area, RECT *work);
void dir_prtcolumn(DIR_WINDOW *dw, int column, int nc, RECT *area, RECT *work);
void dir_prtcolumns(DIR_WINDOW *w, long line, RECT *in, RECT *work);
void dir_refresh_wd(DIR_WINDOW *w);
void dir_refresh_all(void);
void dir_trim_slash ( char *path );
bool dir_do_path( char *path, int action );
OBJECT *make_tree(DIR_WINDOW *dw, int sc, int ncolumns, int sl, int lines, bool smode, RECT *work);
void draw_tree(OBJECT *tree, RECT *clip);
void dir_simw(DIR_WINDOW *dw, char *path, char *name, ITMTYPE type);
ITMTYPE diritem_type( char *fullname );
void dir_newlink(WINDOW *w, char *target);
