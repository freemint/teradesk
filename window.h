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


/* Windowtypes voor xw_create(). */

#define DESK_WIND		16
#define DIR_WIND		17
#define TEXT_WIND		18
#define ACC_WIND		19 
#define CON_WIND		20 /* currently not used */


#define TFLAGS			(NAME|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|ICONIFY)
#define DFLAGS			(NAME|CLOSER|FULLER|MOVER|INFO|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|ICONIFY)


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
	ITM_LINK		/* only for showing object info !!!! */
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
	int (*itm_find) (WINDOW *w, int x, int y);
	boolean (*itm_state) (WINDOW *w, int item);
	ITMTYPE (*itm_type) (WINDOW *w, int item);
	ITMTYPE (*itm_tgttype) (WINDOW *w, int item);
	int (*itm_icon) (WINDOW *w, int item);
	const char *(*itm_name) (WINDOW *w, int item);
	char *(*itm_fullname) (WINDOW *w, int item);
	int (*itm_attrib) (WINDOW *w, int item, int mode, XATTR *attr);
	boolean (*itm_islink) (WINDOW *w, int item);
	boolean (*itm_open) (WINDOW *w, int item, int kstate);
	boolean (*itm_copy) (WINDOW *dw, int dobject, WINDOW *sw,
	        int n, int *list, ICND *icns, int x, int y, int kstate);
	void (*itm_showinfo) (WINDOW *w, int n, int *list, boolean search);

	void (*itm_select) (WINDOW *w, int selected, int mode, boolean draw);
	void (*itm_rselect) (WINDOW *w, int x, int y);
	boolean (*itm_xlist) (WINDOW *w, int *ns, int *nv, int **list, ICND **icns, int mx, int my);
	boolean (*itm_list) (WINDOW *w, int *n, int **list);

	const char *(*wd_path) (WINDOW *w);

	/* Funkties voor het verversen van de inhoud van een window */

	void (*wd_set_update) (WINDOW *w, wd_upd_type type, const char *fname1, const char *fname2);
	void (*wd_do_update) (WINDOW *w);

	/* Funkties die aangeroepen worden als filetype of sorteer methode worden gezet. */

	void (*wd_filemask) (WINDOW *w);
	void (*wd_newfolder) (WINDOW *w);
	void (*wd_disp_mode) (WINDOW *w, int mode);	/* 0 = text, 1 = icons */
	void (*wd_sort) (WINDOW *w, int sort);		/* 0 = name, 1 = extension, 2 = date, 3 = size, 4 = unsorted */
	void (*wd_fields) (WINDOW *w, int fields);	/* bits 0:3=show size,date,time,attr. */
	void (*wd_seticons) (WINDOW *w);
} ITMFUNC;


typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
} ITM_WINDOW;


typedef struct
{
	WINDOW *w;
	int selected;
	int n;
} SEL_INFO;

/*
 * note: information on iconified is duplicated here for purpose
 * of saving/reloading; otherwise it is known to xdialog via
 * xw_iflag in WINDOW structure
 */

typedef struct
{
	unsigned int fulled : 1;
	unsigned int iconified: 1; 
	unsigned int resvd: 14; 
} WDFLAGS;


/*
 * Some fields in the commonpart of DIR_WINDOW and TXT_WINDOW structures
 */

#define WD_VARS 	int scolumns;  \
					int rows;	   \
					int columns;   \
					long nrows;	  /* code is shorter if this is long */   \
					int ncolumns;  \
					int px;        \
					long py;       \
					long nlines;   \
					char title[80]

/*
 * Identical part of DIR_WINDOW and TXT_WINDOW structures
 */


/* Note: take care of compatibility between TXT_WINDOW, DIR_WINDOW, TYP_WINDOW */

typedef struct
{
	ITM_INTVARS;				/* Interne variabelen bibliotheek. */
	WD_VARS;					/* other common header data */
	struct winfo *winfo;

	/* 
	 * three window-type structures are identical up to this point:
	 * DIR_WINDOW, TXT_WINDOW and TYP_WINDOW
	 */

}TYP_WINDOW;


typedef struct winfo
{
	int x;						/* positie van het window */
	int y;
	int w;						/* afmetingen van het werkgebied */
	int h;
	int ix; /* position and size at iconify time */
	int iy; 
	int iw;
	int ih;
	WDFLAGS flags;
	boolean used;
	TYP_WINDOW *typ_window;
}WINFO;


typedef struct
{
	int i, x, y, ww, wh, ix, iy, iw, ih;
	WDFLAGS flags;
	FDATA font;
	WINFO *windows;
} NEWSINFO1;

extern NEWSINFO1 thisw;

extern CfgEntry 
	fnt_table[],
	positions_table[],
	wtype_table[];

extern FONT *cfg_font;

CfgNest positions;
CfgNest cfg_wdfont;
CfgNest wd_config;

int itm_find(WINDOW *w, int x, int y);
boolean itm_state(WINDOW *w, int item);
ITMTYPE itm_type(WINDOW *w, int item);
ITMTYPE itm_tgttype(WINDOW *w, int item);
int itm_icon(WINDOW *w, int item);
const char *itm_name(WINDOW *w, int item);
char *itm_fullname(WINDOW *w, int item);
char *itm_tgtname(WINDOW *w, int item);
int itm_attrib(WINDOW *w, int item, int mode, XATTR *attrib);
boolean itm_islink(WINDOW *w, int item, boolean cond);
boolean itm_open(WINDOW *w, int item, int kstate);

void itm_select(WINDOW *w, int selected, int mode, boolean draw);
void itm_rselect(WINDOW *w, int x, int y);
boolean itm_xlist(WINDOW *w, int *ns, int *nv, int **list, ICND **icns, int mx, int my);
boolean itm_list(WINDOW *w, int *n, int **list);
void itm_set_menu ( WINDOW *w );
void wd_setselection(WINDOW *w);

void wd_set_update(wd_upd_type type, const char *name1, const char *name2);
void wd_do_update(void);
void wd_update_drv(int drive);

void wd_hndlbutton(WINDOW *w, int x, int y, int n, int button_state,
				   int keystate);
const char *wd_path(WINDOW *w);
const char *wd_toppath(void);
void wd_seticons(void);

/* Funkties voor het opvragen van informatie over geselekteerde objecten. */

WINDOW *wd_selected_wd(void);
int wd_nselected(void);
int wd_selected(void);
void wd_reset(WINDOW *w);
void wd_deselect_all(void);
void wd_fields(void);
void wd_del_all(void);
void wd_hndlmenu(int item, int keystate);

void wd_init(void);
void wd_default(void);
int wd_load(XFILE *file);

boolean wd_tmpcls(void);
void wd_reopen(void);

void wd_type_draw(TYP_WINDOW *w, boolean message); 
boolean wd_type_setfont(int title); 
void calc_rc(TYP_WINDOW *w, RECT *work); 
void wd_wsize(TYP_WINDOW *w, RECT *input, RECT *output); 
void wd_calcsize(WINFO *w, RECT *size); 

int wd_type_hndlkey(WINDOW *w, int scancode, int keystate);
void wd_type_hndlbutton(WINDOW *w, int x, int y, int n,
						   int button_state, int keystate);
void wd_set_defsize(WINFO *w); 

void wd_type_topped (WINDOW *w);		
void wd_type_arrowed(WINDOW *w, int arrows);
void wd_type_fulled(WINDOW *w);
void wd_type_hslider(WINDOW *w, int newpos);
void wd_type_vslider(WINDOW *w, int newpos);
void wd_type_moved(WINDOW *w, RECT *newpos);
void wd_type_sized(WINDOW *w, RECT *newsize);
void wd_type_redraw(WINDOW *w, RECT *area);
void do_redraw(WINDOW *w, RECT *r1, boolean clear);

#define HORIZ  1
#define VERTI  2

void set_hslsize_pos(TYP_WINDOW *w);
void set_vslsize_pos(TYP_WINDOW *w);
void set_sliders(TYP_WINDOW *w);
void w_page(TYP_WINDOW *w, int direct);
void w_pageup(TYP_WINDOW *w);
void w_pagedown(TYP_WINDOW *w);
void w_pageleft(TYP_WINDOW *w);
void w_pageright(TYP_WINDOW *w);
void w_scroll(TYP_WINDOW *w, int type); 

void wd_adapt(WINDOW *w);	

void wd_set_obj0( OBJECT *obj, boolean smode, int row, int lines, int yoffset, RECT *work );
void set_obji( OBJECT *obj, long i, long n, boolean selected, int icon_no, 
int obj_x, int obj_y, char *name );
void wd_type_iconify(WINDOW *w);
void wd_type_uniconify(WINDOW *w);
void wd_iopen( WINDOW *w, RECT *size);
void calc_icwsize(void);
void icw_draw (WINDOW *w);
void wd_in_screen ( WINFO *info );
void wd_drawall(void);
int wd_wcount(void);
boolean itm_move(WINDOW *src_wd, int src_object, int old_x, int old_y, int avkstate);


