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


/* Windowtypes voor xw_create(). */

#define DESK_WIND		16
#define DIR_WIND		17
#define TEXT_WIND		18
#define ACC_WIND		19 
#define CON_WIND		20 /* currently not used */

#define TFLAGS			(NAME|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|ICONIFIER)
#define DFLAGS			(NAME|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|ICONIFIER|INFO)

/* Interne variabelen item windows. */

#define ITM_INTVARS		XW_INTVARS;			/* Interne variabelen bibliotheek. */	\
						ITMFUNC *itm_func	/* Item functions. */

/* Icon en item types */

typedef enum
{
	ITM_NOTUSED = 0,			/* niet gebruikt */
	ITM_DRIVE,
	ITM_TRASH,
	ITM_PRINTER,
	ITM_FOLDER,
	ITM_PROGRAM,
	ITM_FILE,
	ITM_PREVDIR,
	ITM_LINK,		/* only for showing object info !!!! */
	ITM_NETOB		/* network object ( http:// or ftp:// or mailto: ) */
} ITMTYPE;

/* Update type */

typedef enum
{
	WD_UPD_DELETED,
	WD_UPD_COPIED,
	WD_UPD_MOVED,
	WD_UPD_ALLWAYS
} wd_upd_type;

typedef struct
{
	_WORD (*itm_find) (WINDOW *w, _WORD x, _WORD y);
	bool (*itm_state) (WINDOW *w, _WORD item);
	ITMTYPE (*itm_type) (WINDOW *w, _WORD item);
	ITMTYPE (*itm_tgttype) (WINDOW *w, _WORD item);
	const char *(*itm_name) (WINDOW *w, _WORD item);
	char *(*itm_fullname) (WINDOW *w, _WORD item);
	_WORD (*itm_attrib) (WINDOW *w, _WORD item, _WORD mode, XATTR *attr);
	bool (*itm_islink) (WINDOW *w, _WORD item);
	bool (*itm_open) (WINDOW *w, _WORD item, _WORD kstate);
	bool (*itm_copy) (WINDOW *dw, _WORD dobject, WINDOW *sw,
	        _WORD n, _WORD *list, ICND *icns, _WORD x, _WORD y, _WORD kstate);
	void (*itm_select) (WINDOW *w, _WORD selected, _WORD mode, bool draw);
	void (*itm_rselect) (WINDOW *w, _WORD x, _WORD y);
	ICND *(*itm_xlist) (WINDOW *w, _WORD *ns, _WORD *nv, _WORD **list, _WORD mx, _WORD my);
	_WORD *(*itm_list) (WINDOW *w, _WORD *n);

	const char *(*wd_path) (WINDOW *w);

	/* Funkties voor het verversen van de inhoud van een window */

	void (*wd_set_update) (WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);
	void (*wd_do_update) (WINDOW *w);
	void (*wd_seticons) (WINDOW *w);
} ITMFUNC;


typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
} ITM_WINDOW;


typedef struct
{
	WINDOW *w;
	_WORD selected;
	_WORD n;
} SEL_INFO;

/*
 * note: information on iconified state is duplicated here for purpose
 * of saving/reloading; otherwise it is known to xdialog via
 * xw_iflag in WINDOW structure
 */

typedef struct
{
	unsigned int fulled : 1;		/* window fulled */
	unsigned int iconified: 1;		/* window iconified */ 
	unsigned int fullfull: 1;		/* window very much fulled (whole screen) */
	unsigned int setmask: 1;		/* non-default filename mask */
	unsigned int resvd: 12;			/* currently unused */ 
} WDFLAGS;



/*
 * Some fields in the common part of DIR_WINDOW and TXT_WINDOW structures
 */

#define WD_VARS 	_WORD scolumns;  \
					_WORD rows;	   \
					_WORD columns;   		/* number of columns in window content (chars or icons) */ \
					_WORD ncolumns;  		/* visible width (chars) */   \
					_WORD dcolumns;		/* number of dir. columns, otherwise 0 */ \
					_WORD px;        		/* h.slider position  */ \
					long py;       		/* v.slider position  */ \
					long nrows;	   		/* visible height (lines) */  \
					long nlines;   		/* total number of lines in window content (lines or icons) */ \
					char title[80];		/* window title */ \
					char *path;			/* name of directory or file */ \
					struct winfo *winfo

/*
 * Identical part of DIR_WINDOW and TXT_WINDOW structures
 * Note: take care of compatibility between these window types
 */

typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
	WD_VARS;					/* other common header data */
} TYP_WINDOW;

typedef struct winfo
{
	_WORD x;						/* positie van het window */
	_WORD y;
	_WORD w;						/* afmetingen van het werkgebied */
	_WORD h;
	_WORD ix; /* position and size at iconify time */
	_WORD iy; 
	_WORD iw;
	_WORD ih;
	TYP_WINDOW *typ_window;
	WDFLAGS flags;
	bool used;
} WINFO;


/* Note: see window.c; CFG_NEST positions */

typedef struct
{
	_WORD i, x, y, ww, wh, ix, iy, iw, ih;
	WDFLAGS flags;
	XDFONT font;
	WINFO *windows;
} NEWSINFO1;


typedef struct
{
	_WORD 
		index,
		px, 
		hexmode, 
		tabsize;
	long
		py; 
	VLNAME 
		path, 
		spec;
	WINDOW 
		*w;
} SINFO2;

extern bool	clearline;
extern bool onekey_shorts;	/* true if single-key menu shortcuts exist */

extern NEWSINFO1 thisw;
extern SINFO2 that;
extern SEL_INFO selection;

extern bool autoloc;
extern XUSERBLK wxub;
extern bool autoloc_upd;
extern bool can_touch;

#if _MINT_
extern LNAME automask; /* to compose the autolocator mask */
#else
extern SNAME automask; /* to compose the autolocator mask */
#endif


extern XDFONT *cfg_font;

extern const WD_FUNC wd_type_functions;

void positions(XFILE *file, int lvl, int io, int *error);
void cfg_wdfont(XFILE *file, int lvl, int io, int *error);
void wd_config(XFILE *file, int lvl, int io, int *error);

void autoloc_off(void);
_WORD itm_find(WINDOW *w, _WORD x, _WORD y);
bool itm_state(WINDOW *w, _WORD item);
ITMTYPE itm_type(WINDOW *w, _WORD item);
ITMTYPE itm_tgttype(WINDOW *w, _WORD item);
_WORD itm_icon(WINDOW *w, _WORD item);
const char *itm_name(WINDOW *w, _WORD item);
char *itm_fullname(WINDOW *w, _WORD item);
char *itm_tgtname(WINDOW *w, _WORD item);
_WORD itm_attrib(WINDOW *w, _WORD item, _WORD mode, XATTR *attrib);
bool itm_islink(WINDOW *w, _WORD item);
bool itm_follow(WINDOW *w, _WORD item, bool *link, char **name, ITMTYPE *type);
bool itm_open(WINDOW *w, _WORD item, _WORD kstate);
void itm_select(WINDOW *w, _WORD selected, _WORD mode, bool draw);
void itm_set_menu ( WINDOW *w );
void wd_setselection(WINDOW *w);
void wd_do_dirs(void *func);
void wd_set_update(wd_upd_type type, const char *name1, const char *name2);
void wd_do_update(void);
void wd_update_drv(_WORD drive);
void wd_restoretop(_WORD code, _WORD *whandle, _WORD *wap_id);
bool wd_dirortext(WINDOW *w);
void wd_hndlbutton(WINDOW *w, _WORD x, _WORD y, _WORD n, _WORD button_state, _WORD keystate);
const char *wd_path(WINDOW *w);
const char *wd_toppath(void);
void wd_seticons(void);
void wd_reset(WINDOW *w);
void wd_deselect_all(void);
void wd_del_all(void);
void wd_hndlmenu(_WORD item, _WORD keystate);
void wd_menu_ienable(_WORD item, _WORD enable);
void wd_sizes(void);

void wd_default(void);
bool wd_tmpcls(void);
void wd_reopen(void);
void wd_type_draw(TYP_WINDOW *w, bool message); 
void wd_type_sldraw(WINDOW *w);
bool wd_type_setfont(_WORD button); 
void calc_rc(TYP_WINDOW *w, GRECT *work); 
void wd_wsize(TYP_WINDOW *w, GRECT *input, GRECT *output, bool iswork); 
void wd_calcsize(WINFO *w, GRECT *size); 
_WORD wd_type_hndlkey(WINDOW *w, _WORD scancode, _WORD keystate);
void wd_set_defsize(WINFO *w); 
void wd_type_close( WINDOW *w, _WORD mode);
void wd_type_topped (WINDOW *w);		
void wd_type_bottomed (WINDOW *w);		
void wd_type_arrowed(WINDOW *w, _WORD arrows);
void wd_type_fulled(WINDOW *w, _WORD mbshift);
void wd_type_nofull(WINDOW *w);
void wd_type_hslider(WINDOW *w, _WORD newpos);
void wd_type_vslider(WINDOW *w, _WORD newpos);
void wd_type_moved(WINDOW *w, GRECT *newpos);
void wd_type_sized(WINDOW *w, GRECT *newsize);
void wd_type_redraw(WINDOW *w, GRECT *area);
void wd_type_title(TYP_WINDOW *w);
void set_hslsize_pos(TYP_WINDOW *w);
void set_vslsize_pos(TYP_WINDOW *w);
void set_sliders(TYP_WINDOW *w);
void w_page(TYP_WINDOW *w, _WORD newpx, long newpy);
void w_pageup(TYP_WINDOW *w);
void w_pagedown(TYP_WINDOW *w);
void w_pageleft(TYP_WINDOW *w);
void w_pageright(TYP_WINDOW *w);
void w_scroll(TYP_WINDOW *w, _WORD type); 
bool wd_adapt(WINDOW *w);	
void wd_cellsize(TYP_WINDOW *w, _WORD *cw, _WORD *ch, bool icons);
void wd_set_obj0( OBJECT *obj, bool smode, _WORD row, _WORD lines, GRECT *work );
void set_obji( OBJECT *obj, long i, long n, bool selected, bool hidden, bool link, _WORD icon_no, 
_WORD obj_x, _WORD obj_y, char *name );
void wd_type_iconify(WINDOW *w, GRECT *r);
void wd_type_uniconify(WINDOW *w, GRECT *r);
void wd_restoresize(WINFO *info);
void wd_setnormal(WINFO *info); 
void wd_iopen( WINDOW *w, GRECT *oldsize, WDFLAGS *oldflags);
bool wd_checkopen(int *error);
void wd_noselection(void);
void wd_in_screen ( WINFO *info );
long wd_type_slines(TYP_WINDOW *w);
void wd_drawall(void);
_WORD wd_wcount(void);
bool itm_move(WINDOW *src_wd, _WORD src_object, _WORD old_x, _WORD old_y, _WORD avkstate);
void arrow_mouse(void);
void hourglass_mouse(void);
void w_transptext( _WORD x, _WORD y, char *text);
bool isfileprog(ITMTYPE type);
void wd_forcesize(WINDOW *w, GRECT *size, bool cond);
