/*	AES.H
	
	GEM AES Definitions
	
	Copyright (c) Borland International 1990
	All Rights Reserved.
	
	mrt 1992 H. Robbers Amsterdam:
	
	space --> tabs.
	prefixes in structs removed.
	lists of #defines replaced by enums.
	types in aggregates amalgamated.
	VRECT (x,y,jx,jy); juxtapositioned coordinates (x1,y1,x2,y2)

	12-07-93  PC 1.1
	12-04-99  Multi Threading AES and further modernization
				and paramaterize 16 bit types.
	18-04-99  Reinstate prefixes; now under condition.

	As you will notice, I use a different name for int in the prototypes.
	These are used program internally only.
	If your compiler thinks 32 bits are better, then USE that, or
	get yourself another compiler.
	And if that is not of convenience you should rethink the whole idee
	of promoting int to 32,
	Of course for the other compiler you must recompile the package.
	
	The integral item's that are passed directly to the OS are parameterized.
	These are G_i, G_u, G_l & G_ul.
	The sizes of pointers are not paramaterized.
	These 32 bits for addresses are here to stay. ;-)
*/

/* define G_NOPREFIX if you do not want to use structure local
	prefixes */

#if  !defined __AES__
#define __AES__

/****** GEMparams *******************************************************/


#include <gempb.h>		/* These hold the basics used in the lib itself */

typedef enum
{
	GAI_WDLG = 0x0001,		/* wdlg_xx()-Funktionen vorhanden */
	GAI_LBOX = 0x0002,		/* lbox_xx()-Funktionen vorhanden */
	GAI_FNTS = 0x0004,		/* fnts_xx()-Funktionen vorhanden */
	GAI_FSEL = 0x0008,		/* neue Dateiauswahl vorhanden */

	GAI_MAGIC= 0x0100,		/* MagiC-AES vorhanden */
	GAI_INFO = 0x0200,		/* appl_getinfo() vorhanden */
	GAI_3D   = 0x0400,		/* 3D-Look vorhanden */
	GAI_CICN = 0x0800,		/* Color-Icons vorhanden */
	GAI_APTERM = 0x1000,	/* AP_TERM wird unterstÅtzt */
	GAI_GSHORTCUT = 0x2000,	/* Objekttyp G_SHORTCUT wird unterstÅtzt */
	GAI_WHITEBAK = 0x4000	/* WHITEBAK objects */
} GAI;

typedef struct
{
	G_l	magic;							/* muû $87654321 sein */
	void *membot;						/* Ende der AES-Variablen */
	void *aes_start;					/* Startadresse */

	G_l	magic2;							/* ist 'MAGX' */
	G_l	date;							/* Erstelldatum: ttmmjjjj */
	void (*chgres)( G_i res, G_i txt );	/* Auflîsung Ñndern */
	G_l	(**shel_vector)(void);			/* ROM-Desktop */
	G_b *aes_bootdrv;					/* Hierhin kommt DESKTOP.INF */
	G_i	*vdi_device;					/* vom AES benutzter Treiber */
	void *reservd1;
	void *reservd2;
	void *reservd3;

	G_i version,						/* Versionsnummer */
	    release;						/* Release-Status */

} AESVARS;

typedef	void *DOSVARS;						/* wird in diesem Source nicht benutzt */

typedef struct
{
	G_l		config_status;
	DOSVARS	*dosvars;
	AESVARS	*aesvars;
} MAGX_COOKIE;

/* MultiTOS Drag&Drop definitions */

typedef enum
{
	DD_OK,
	DD_NAK,
	DD_EXT,
	DD_LEN,

	DD_TIMEOUT = 4000,			/* Timeout in ms */
	DD_EXTSIZE = 32,			/* LÑnge des Formatfelds */
	DD_NAMEMAX = 128,			/* maximale LÑnge eines Formatnamens */

	DD_NUMEXTS = 8,				/* Anzahl der Formate */
	DD_HDRMIN  = 9,				/* minimale LÑnge des Drag&Drop-Headers */

	DD_HDRMAX  = ( 8 + DD_NAMEMAX )	/* maximale LÑnge */
} DD_S;

#define DD_FNAME	"u:/pipe/dragdrop.aa"

/****** Application definitions *****************************************/

typedef struct
{
	G_i	 dst_apid;
	G_i	 unique_flg;
	void *attached_mem;
	G_i	 *msgbuf;
} XAESMSG;

G_w G_decl G_nv(appl_init);
G_w G_decl G_n(appl_read)( G_w ap_rid, G_w ap_rlength, void *ap_rpbuff dglob);
G_w G_decl G_n(appl_write)( G_w ap_wid, G_w ap_wlength, void *ap_wpbuff  dglob);
G_w G_decl G_n(appl_find)( const char *ap_fpname  dglob);
G_w G_decl G_n(appl_tplay)( void *ap_tpmem, G_w ap_tpnum, G_w ap_tpscale  dglob);
G_w G_decl G_n(appl_trecord)( void *ap_trmem, G_w ap_trcount  dglob);
G_w G_decl G_nv(appl_exit);
G_w G_decl G_n(appl_search)( G_w ap_smode, char *ap_sname, G_w *ap_stype,
						G_w *ap_sid  dglob);
G_w G_decl G_n(appl_getinfo)( G_w ap_gtype, G_w *ap_gout1, G_w *ap_gout2,
						 G_w *ap_gout3, G_w *ap_gout4  dglob);

#define	appl_bvset( disks, harddisks ) /* Funktion ignorieren (GEM fÅr Dose): void appl_bvset( G_w disks, G_w harddisks ); */

#define	vq_aes()	( appl_init() >= 0 )	/* WORD	vq_aes( void ); */

/****** Event definitions ***********************************************/

typedef enum
{
	MN_SELECTED		=10,
	WM_REDRAW		=20,
	WM_TOPPED,
	WM_CLOSED,
	WM_FULLED,
	WM_ARROWED,
	WM_HSLID,
	WM_VSLID,
	WM_SIZED,
	WM_MOVED,
	WM_NEWTOP,
	WM_UNTOPPED		=30,	/* GEM  2.x	*/
	WM_ONTOP,    			/* AES 4.0	*/
	WM_UNKNOWN,				/* AES 4.1	*/
	WM_BACK,				/* WINX 	*/
	WM_BOTTOMED = WM_BACK,	/* AES 4.1	*/
	WM_ICONIFY,				/* AES 4.1 	*/
	WM_UNICONIFY,			/* AES 4.1	*/
	WM_ALLICONIFY,			/* AES 4.1	*/
	WM_TOOLBAR,				/* AES 4.1	*/
	AC_OPEN			=40,
	AC_CLOSE,
	CT_UPDATE		=50,
	CT_MOVE,
	CT_NEWTOP,
	CT_KEY,
	AP_TERM			=50,
	AP_TFAIL,
	AP_RESCHG		=57,
	SHUT_COMPLETED	=60,
	RESCH_COMPLETED,
	AP_DRAGDROP		=63,
	SH_WDRAW		=72,	/* MultiTOS    */
	SC_CHANGED		=80,	/* MultiTOS    */
	PRN_CHANGED		=82,
	FNT_CHANGED,
	THR_EXIT		=88,	/* MagiC 4.5	*/
	PA_EXIT,				/* MagiC 3     */
	CH_EXIT			=90,	/* MultiTOS    */
	WM_M_BDROPPED	=100,	/* KAOS 1.4    */
	SM_M_SPECIAL,			/* MAG!X       */
	SM_M_RES2,				/* MAG!X       */
	SM_M_RES3,				/* MAG!X       */
	SM_M_RES4,				/* MAG!X       */
	SM_M_RES5,				/* MAG!X       */
	SM_M_RES6,				/* MAG!X       */
	SM_M_RES7,				/* MAG!X       */
	SM_M_RES8,				/* MAG!X       */
	SM_M_RES9,				/* MAG!X       */
	WM_SHADED		=22360,	/* [WM_SHADED apid 0 win 0 0 0 0] */
	WM_UNSHADED		=22361	/* [WM_UNSHADED apid 0 win 0 0 0 0] */
} MESAG_TY;

/* SM_M_SPECIAL codes */

#define SMC_TIDY_UP		0							/* MagiC 2	*/
#define SMC_TERMINATE	1							/* MagiC 2	*/
#define SMC_SWITCH		2							/* MagiC 2	*/
#define SMC_FREEZE		3							/* MagiC 2	*/
#define SMC_UNFREEZE	4							/* MagiC 2	*/
#define SMC_RES5		5							/* MagiC 2	*/
#define SMC_UNHIDEALL	6							/* MagiC 3.1	*/
#define SMC_HIDEOTHERS	7							/* MagiC 3.1	*/
#define SMC_HIDEACT		8							/* MagiC 3.1	*/
/* Keybord states */

typedef enum
{
	K_RSHIFT=0x0001,
	K_LSHIFT=0x0002,
	K_CTRL  =0x0004,
	K_ALT   =0x0008
} K_STATE;

#if !G_MODIFIED
G_w G_decl G_nv(evnt_keybd);
G_w G_decl G_n(evnt_button)(	G_w ev_bclicks, G_w ev_bmask, G_w ev_bstate,
					G_w *ev_bmx, G_w *ev_bmy, G_w *ev_bbutton,
					G_w *ev_bkstate dglob);
G_w G_decl G_n(evnt_mouse)( G_w ev_moflags, G_w ev_mox, G_w ev_moy,
				G_w ev_mowidth, G_w ev_moheight, G_w *ev_momx,
				G_w *ev_momy, G_w *ev_mobutton,
				G_w *ev_mokstate dglob);
G_w G_decl G_n(evnt_mesag)( G_w *ev_mgpbuff dglob );
G_w G_decl G_n(evnt_timer)( G_i ev_tlocount, G_i ev_thicount dglob );
G_w G_decl G_n(evnt_multi)( G_w ev_mflags, G_w ev_mbclicks, G_w ev_mbmask,
				G_w ev_mbstate, G_w ev_mm1flags, G_w ev_mm1x,
				G_w ev_mm1y, G_w ev_mm1width, G_w ev_mm1height,
				G_w ev_mm2flags, G_w ev_mm2x, G_w ev_mm2y,
				G_w ev_mm2width, G_w ev_mm2height,
				G_w *ev_mmgpbuff, G_i ev_mtlocount,
				G_i ev_mthicount, G_w *ev_mmox, G_w *ev_mmoy,
				G_w *ev_mmbutton, G_w *ev_mmokstate,
				G_w *ev_mkreturn, G_w *ev_mbreturn dglob );
G_w G_decl G_n(evnt_dclick)( G_w ev_dnew, G_w ev_dgetset dglob );
#else
G_w G_decl G_nv(evnt_keybd);
G_w G_decl G_n(evnt_button)(G_w clks,G_w mask,G_w state,
                EVNTDATA *ev dglob);
G_w G_decl G_n(evnt_mouse)(G_w flags, GRECT *g, EVNTDATA *ev dglob);
G_w G_decl G_n(evnt_mesag)( G_w *ev_mgpbuff dglob );
G_w G_decl G_n(evnt_timer)(unsigned long ms dglob);
G_w G_decl G_n(evnt_multi)(G_w flags,G_w clks,G_w mask,G_w state,
               G_w m1flags,GRECT *g1,
               G_w m2flags,GRECT *g2,
               G_w *msgpipe,
               unsigned long ms,
               EVNTDATA *ev,
               G_w *toets,G_w *oclks dglob);
G_w G_decl G_n(evnt_dclicks)(G_w new,G_w getset dglob);
#endif
#if G_EXT
void G_n(EVNT_multi)( G_w evtypes, G_w nclicks, G_w bmask, G_w bstate,
							MOBLK *m1, MOBLK *m2, G_ul ms,
							EVNT *event dglob );
#endif

/* this is our special invention to increase evnt_multi performance */

typedef struct /* Special type for EventMulti */
{
	/* input parameters */
	G_i     flags, bclicks, bmask, bstate, m1flags;
	        m1x, m1y,
	        m1width, m1height, m2flags,
	        m2x, m2y,
	        m2width, m2height;
	G_u     tlocount, thicount;
	/* output parameters */
	G_i     wich,
			mox, moy,
			mobutton, mokstate,
	        kreturn, breturn;
	/* message buffer */
	G_i     mgpbuf[8];
} EVENT;

G_w G_decl G_n(EvntMulti)( EVENT *evnt_struct dglob);


/****** Object definitions **********************************************/
typedef enum
{
	G_BOX=20,
	G_TEXT,
	G_BOXTEXT,
	G_IMAGE,
	G_USERDEF,
	G_IBOX,
	G_BUTTON,
	G_BOXCHAR,
	G_STRING,
	G_FTEXT,
	G_FBOXTEXT,
	G_ICON,
	G_TITLE,
	G_CICON,

/* HR: for completeness */

	G_SWBUTTON,		/* MAG!X       */
	G_POPUP,		/* MAG!X       */
	G_WINTITLE,		/* MagiC 3.1	*/
	G_EDIT,
	G_SHORTCUT,		/* MagiC 6 */
	G_SLIST,		/* XaAES extended object - scrolling list */
	G_UNKNOWN,
	G_MAX			/* Maximum number of object types */
} OBJECT_TY;

/* Object flags */

typedef enum
{
	NONE	  =0x0000,
	SELECTABLE=0x0001,
	DEFAULT	  =0x0002,
	EXIT      =0x0004,
	EDITABLE  =0x0008,
	RBUTTON	  =0x0010,
	LASTOB    =0x0020,
	TOUCHEXIT =0x0040,
	HIDETREE  =0x0080,
	INDIRECT  =0x0100,
	FL3DNONE  =0x0000,
	FL3DIND   =0x0200,	/* 3D Indicator			  AES 4.0		*/
	FL3DBAK   =0x0400,	/* 3D Background		  AES 4.0		*/
	FL3DACT   =0x0600,	/* 3D Activator			  AES 4.0		*/
	FL3DMASK  =0x0600,
	SUBMENU   =0x0800
} OBJECT_FLAG;

/* Object states */

typedef enum
{
	NORMAL	=0x00,
	SELECTED=0x01,
	CROSSED =0x02,
	CHECKED =0x04,
	DISABLED=0x08,
	OUTLINED=0x10,
	SHADOWED=0x20,
	WHITEBAK=0x40,		/* TOS     & MagiC */
	DRAW3D  =0x80		/* GEM 2.x         */
} OBJECT_STATE;

/* objc_sysvar */

typedef enum
{
	LK3DIND     =1,	/* AES 4.0     */
	LK3DACT,		/* AES 4.0     */
	INDBUTCOL,		/* AES 4.0     */
	ACTBUTCOL,		/* AES 4.0     */
	BACKGRCOL,		/* AES 4.0     */
	AD3DVALUE,		/* AES 4.0     */
	MX_ENABLE3D =10	/* MagiC 3.0   */
} OB_SYSVAR;

/* Object colors */
#if !defined __COLORS
#define __COLORS		/*	using AES-colors and BGI-colors
							is not possible	*/

typedef enum
{
	WHITE,
	BLACK,
	RED,
	GREEN,
	BLUE,
	CYAN,
	YELLOW,
	MAGENTA,
	LWHITE,
	LBLACK,
	LRED,
	LGREEN,
	LBLUE,
	LCYAN,
	LYELLOW,
	LMAGENTA
} AES_COLOR;
#endif

#define ROOT             0
#define MAX_LEN         81              /* max string length */
#define MAX_DEPTH        8              /* max depth of search or draw */


#define IBM             3               /* font types */
#define SMALL           5

typedef enum
{
	ED_START,							/* editable text field definitions */
	ED_INIT,
	ED_CHAR,
	ED_END
} EDOB_TY;

typedef enum
{
	TE_LEFT,							/* editable text justification */
	TE_RIGHT,
	TE_CNTR
} TE_JUST;

typedef struct orect
{
#if G_PRFXS
	struct	orect   *o_link;
	G_i		o_x,o_y,o_w,o_h;
#else
	struct	orect   *link;
	G_i		x,y,w,h;
#endif
} ORECT;

typedef GRECT RECT;

typedef struct
{
	G_i x,y,
		jx,jy;		/* VDI rectangle */
} VRECT;

/* Object structures */

#if G_PRFXS
typedef struct
{
	char	*te_ptext,     
			*te_ptmplt,
			*te_pvalid;
	G_i		te_font,
			te_junk1,
			te_just,
			te_color,
			te_junk2,
			te_thickness,
			te_txtlen,
			te_tmplen;
} TEDINFO;
#else
typedef struct
{
	char	*text,       /* ptr to text (must be 1st)    */
			*tmplt,      /* ptr to template              */
			*valid;      /* ptr to validation            */
	G_i		font,        /* font                         */
			junk1,       /* junk 	                     */
			just,        /* justification: left, right...*/
			color,       /* color information            */
			junk2,       /* junk 	                     */
			thickness,   /* border thickness             */
			txtlen,      /* text string length           */
			tmplen;      /* template string length       */
} TEDINFO;
#endif


#if G_PRFXS
typedef struct
{
	G_i		*ib_pmask,
			*ib_pdata;
	char	*ib_ptext;
	G_i		ib_char,
			ib_xchar,
			ib_ychar,
			ib_xicon,
			ib_yicon,
			ib_wicon,
			ib_hicon,
			ib_xtext,
			ib_ytext,
			ib_wtext,
			ib_htext;
} ICONBLK;
#else
typedef struct
{
	G_i		*mask,
			*data;
	char	*text;
	G_i		ch,
			xchar,
			ychar,
			xicon,
			yicon,
			wicon,
			hicon,
			xtext,
			ytext,
			wtext,
			htext;
} ICONBLK;
#endif

typedef struct cicon_data
{
        G_i                num_planes;
        G_i                *col_data;
        G_i                *col_mask;
        G_i                *sel_data;
        G_i                *sel_mask;
        struct cicon_data  *next_res;
} CICON;


typedef struct cicon_blk
{
        ICONBLK            monoblk;
        CICON              *mainlist;
} CICONBLK;

typedef struct
{
#if G_PRFXS
	G_i	*bi_pdata,
		bi_wb,
		bi_hl,
		bi_x,
		bi_y,
		bi_color;
#else
	G_i	*data,               /* ptr to bit forms data        */
		wb,                  /* width of form in bytes       */
		hl,                  /* height in lines              */
		x,                   /* source x in bit form         */
		y,                   /* source y in bit form         */
		color;               /* foreground color             */
#endif
} BITBLK;


struct __parmblk;

#if !((defined  __STDC__) && CDECL)
/*	   using this structure is not possible
 *      if ANSI keywords only is ON, and __Cdecl is non empty
 */
typedef struct
{
	int __Cdecl (*ub_code)(struct __parmblk *parmblock);
	long      ub_parm;
} USERBLK;
#endif

typedef struct
{
	unsigned character   :  8;
	signed   framesize   :  8;
	unsigned framecol    :  4,
			 textcol     :  4,
			 textmode    :  1,
			 fillpattern :  3,
			 interiorcol :  4;
} bfobspec;

typedef union obspecptr
{
	long     index;
	union obspecptr *indirect;
	bfobspec obspec;
	TEDINFO	*tedinfo;
	ICONBLK	*iconblk;
	CICONBLK *ciconblk;
	BITBLK	*bitblk;
#if !((defined  __STDC__) && CDECL)
/*	   using this structure is not possible
 *      if ANSI keywords only is ON, and __Cdecl is non empty
 */
	USERBLK *userblk;
#endif
	char	*free_string;
} OBSPEC;

#if G_PRFXS
typedef struct
{
	G_i		ob_next, ob_head, ob_tail;
	unsigned
	G_i		ob_type, ob_flags, ob_state;
	OBSPEC	ob_spec;
	G_i		ob_x, ob_y, ob_width, ob_height;
} OBJECT;
#else
typedef struct
{
	G_i		next,		/* -> object's next sibling     */
			head,		/* -> head of object's children */
			tail;		/* -> tail of object's children */
	unsigned
	G_i		type,		/* object type: BOX, CHAR,...   */
			flags,		/* object flags                 */
			state;		/* state: SELECTED, OPEN, ...   */
	OBSPEC	spec;		/* "out": -> anything else      */
/* The following is in fact a GRECT, but is too offten used to change it */
	G_i		x,			/* upper left corner of object  */
			y,			/* upper left corner of object  */
			w,			/* object width                 */
			h;			/* object height                */
} OBJECT;
#endif

#if G_PRFXS
typedef struct __parmblk
{
	OBJECT  *pb_tree;
	G_i		pb_obj,
			pb_prevstate,
			pb_currstate;
	G_i     pb_x, pb_y, pb_w, pb_h;
	G_i     pb_xc, pb_yc, pb_wc, pb_hc;
	G_ul    pb_parm;
} PARMBLK;
#else
typedef struct __parmblk
{
	OBJECT  *tree;
	G_i		obj,
			prevstate,
			currstate;
	GRECT	size,
			clip;
	union
	{
		long    parm;
		char *text;
	} P;
} PARMBLK;
#endif

#if G_PRFXS
typedef struct
{
        OBJECT  *mn_tree;
        G_i     mn_menu;
        G_i     mn_item;
        G_i     mn_scroll;
        G_i		mn_keystate;
} MENU;
#else
typedef struct
{
        OBJECT  *tree;
        G_i     menu;
        G_i     item;
        G_i     scroll;
        G_i		keystate;
} MENU;
#endif

typedef struct
{
        long   Display;
        long   Drag;
        long   Delay;
        long   Speed;
        G_i    Height;
} MN_SET;

typedef struct
{	
	char	*string;		/* etwa "TOS|KAOS|MAG!X"          */	
	G_i     num,			/* Nr. der aktuellen Zeichenkette */
			maxnum;			/* maximal erlaubtes <num>        */
} SWINFO;

typedef struct
{
     OBJECT	*tree;		/* Popup- MenÅ                    */
     G_i	 obnum;		/* aktuelles Objekt von <tree>    */
} POPINFO;

/****** Menu definitions ************************************************/

/* menu_bar modes */

typedef enum
{
	MENU_HIDE,					/* TOS         */
	MENU_SHOW,					/* TOS         */
	MENU_INSTL = 100			/* MAG!X       */
} MBAR_DO;

G_w G_decl G_n(menu_bar)( OBJECT *me_btree, MBAR_DO me_bshow dglob);
G_w G_decl G_n(menu_icheck)( OBJECT *me_ctree, G_w me_citem, G_w me_ccheck dglob);
G_w G_decl G_n(menu_ienable)( OBJECT *me_etree, G_w me_eitem, G_w me_eenable dglob);
G_w G_decl G_n(menu_tnormal)( OBJECT *me_ntree, G_w me_ntitle, G_w me_nnormal dglob);
G_w G_decl G_n(menu_text)( OBJECT *me_ttree, G_w me_titem, const char *me_ttext dglob);
G_w G_decl G_n(menu_register)( G_w me_rapid, const char *me_rpstring dglob);
G_w G_decl G_n(menu_popup)( MENU *me_menu, G_w me_xpos, G_w me_ypos, MENU *me_mdata dglob);
G_w G_decl G_n(form_popup)( OBJECT *tree, G_w x, G_w y dglob);
#if G_EXT
G_w G_decl G_n(menu_click)(G_w val, G_w setflag dglob);
G_w G_decl G_n(menu_attach)( G_w me_flag, OBJECT *me_tree, G_w me_item, MENU *me_mdata dglob);
G_w G_decl G_n(menu_istart)( G_w me_flag, OBJECT *me_tree, G_w me_imenu, G_w me_item dglob);
G_w G_decl G_n(menu_settings)( G_w me_flag, MN_SET *me_values dglob);
#endif

/* Object prototypes */

G_w G_decl G_n(objc_add)( OBJECT *ob_atree, G_w ob_aparent, G_w ob_achild dglob);
G_w G_decl G_n(objc_delete)( OBJECT *ob_dltree, G_w ob_dlobject dglob);
G_w G_decl G_n(objc_draw)( OBJECT *ob_drtree, G_w ob_drstartob,
               G_w ob_drdepth, G_w ob_drxclip, G_w ob_dryclip,
               G_w ob_drwclip, G_w ob_drhclip dglob);
G_w G_decl G_n(objc_find)( OBJECT *ob_ftree, G_w ob_fstartob, G_w ob_fdepth,
               G_w ob_fmx, G_w ob_fmy dglob);
G_w G_decl G_n(objc_offset)( OBJECT *ob_oftree, G_w ob_ofobject,
                 G_w *ob_ofxoff, G_w *ob_ofyoff dglob);
G_w G_decl G_n(objc_order)( OBJECT *ob_ortree, G_w ob_orobject,
                G_w ob_ornewpos dglob);
G_w G_decl G_n(objc_edit)( OBJECT *ob_edtree, G_w ob_edobject,
               G_w ob_edchar, G_w *ob_edidx, G_w ob_edkind dglob);
#if G_EXT
G_w G_decl G_n(objc_xedit)(void *tree,G_w item,G_w edchar,G_w *idx,G_w kind,
			GRECT *r dglob);
#endif
#if !G_MODIFIED
G_w G_decl G_n(objc_change)( OBJECT *ob_ctree, G_w ob_cobject,
                 G_w ob_cresvd, G_w ob_cxclip, G_w ob_cyclip,
                 G_w ob_cwclip, G_w ob_chclip,
                 G_w ob_cnewstate, G_w ob_credraw dglob);
#else
G_w G_decl G_n(objc_change)(void *tree,
	G_w item,G_w resvd,GRECT *r,G_w newst,G_w redraw dglob);
#endif
G_w G_decl G_n(objc_sysvar)
			   (G_w mo, G_w which, G_w ival1,
				G_w ival2, G_w *oval1, G_w *oval2 dglob);


/****** Form definitions ************************************************/

typedef enum
{
	FMD_START,
	FMD_GROW,
	FMD_SHRINK,
	FMD_FINISH
} FMD_TY;

typedef struct	/* form_xdo definitions */
{
	char scancode;
	char nclicks;
	G_i  objnr;
} SCANX;

typedef struct
{
	SCANX	*unsh;
	SCANX	*shift;
	SCANX	*ctrl;
 	SCANX	*alt;
	void	*resvd;
} XDO_INF;

#if G_PRFXS
typedef struct _xted
{
	char *xte_ptmplt;
	char *xte_pvalid;
	G_i	  xte_vislen;
	G_i	  xte_scroll;
} XTED;
#else
typedef struct _xted /* scrollable textedit objects */
{
	char *tmplt;
	char *valid;
	G_i	  vislen;
	G_i	  scroll;
} XTED;
#endif

G_w G_decl G_n(form_do)( OBJECT *fo_dotree, G_w fo_dostartob dglob);
#if G_EXT
G_w G_decl G_n(form_xdo)(OBJECT *tree, G_w item, G_w *curob, XDO_INF *scantab, void *flyinf dglob);
G_w	G_decl G_n(form_xdial)(G_w subfn, GRECT *lg, GRECT *bg, void **flyinf dglob);
#endif
#if !G_MODIFIED
G_w G_decl G_n(form_dial)(G_w flag,G_w lx,G_w ly,G_w lw,G_w lh,
                                   G_w bx,G_w by,G_w bw,G_w bh dglob);
#else
G_w	G_decl G_n(form_dial)(G_w subfn, GRECT *lg, GRECT *bg dglob);
#endif
#if G_EXT
G_w	G_decl G_n(form_xdial)(G_w subfn, GRECT *lg,
				GRECT *bg, void **flyinf dglob);
#endif
G_w G_decl G_n(form_alert)( G_w fo_adefbttn, const char *fo_astring dglob);
G_w G_decl G_n(form_error)( G_w fo_enum dglob);
G_w G_decl G_n(form_center)( OBJECT *fo_ctree, G_w *fo_cx, G_w *fo_cy,
                 G_w *fo_cw, G_w *fo_ch dglob);
G_w G_decl G_n(form_keybd)( OBJECT *fo_ktree, G_w fo_kobject, G_w fo_kobnext,
                G_w fo_kchar, G_w *fo_knxtobject, G_w *fo_knxtchar dglob);
G_w G_decl G_n(form_button)( OBJECT *fo_btree, G_w fo_bobject, G_w fo_bclicks,
                G_w *fo_bnxtobj dglob);

/****** Graph definitions ************************************************/


/* Mouse forms */

typedef enum
{
	ARROW,
	TEXT_CRSR,
	HOURGLASS,
	BUSYBEE,
	BEE = BUSYBEE,
	POINT_HAND,
	FLAT_HAND,
	THIN_CROSS,
	THICK_CROSS,
	OUTLN_CROSS,
	USER_DEF	= 255,
	M_OFF,
	M_ON,
	M_SAVE,
	M_LAST,
	M_RESTORE
} MOUSE_TY;

/* Mouse form definition block */
#if G_PRFXS
typedef struct mfstr
{
	G_i	mf_xhot,
		mf_yhot,
		mf_nplanes,
		mf_fg,
		mf_bg,
		mf_mask[16],
		mf_data[16];
} MFORM;
#else
typedef struct mfstr
{
	G_i	xhot,
		yhot,
		nplanes,
		fg,
		bg,
		mask[16],
		data[16];
} MFORM;
#endif

G_w G_decl G_n(graf_rubberbox)( G_w gr_rx, G_w gr_ry, G_w gr_minwidth,
                    G_w gr_minheight, G_w *gr_rlastwidth,
                    G_w *gr_rlastheight dglob);
G_w G_decl G_n(graf_rubbox)( G_w gr_rx, G_w gr_ry, G_w gr_minwidth,
                    G_w gr_minheight, G_w *gr_rlastwidth,
                    G_w *gr_rlastheight dglob);
#if !G_MODIFIED
G_w G_decl G_n(graf_dragbox)( G_w gr_dwidth, G_w gr_dheight,
                  G_w gr_dstartx, G_w gr_dstarty,
                  G_w gr_dboundx, G_w gr_dboundy,
                  G_w gr_dboundw, G_w gr_dboundh,
                  G_w *gr_dfinishx, G_w *gr_dfinishy dglob);
#else
G_w G_decl G_n(graf_dragbox)(G_w dw,G_w dh,G_w dx,G_w dy,
                 GRECT *g,
                 G_w *ex,G_w *ey dglob);
#endif
G_w G_decl G_n(graf_movebox)( G_w gr_mwidth, G_w gr_mheight,
                  G_w gr_msourcex, G_w gr_msourcey,
                  G_w gr_mdestx, G_w gr_mdesty dglob);
G_w G_decl G_n(graf_mbox)( G_w gr_mwidth, G_w gr_mheight,
                  G_w gr_msourcex, G_w gr_msourcey,
                  G_w gr_mdestx, G_w gr_mdesty dglob);
#if !G_MODIFIED
G_w G_decl G_n(graf_growbox)( G_w gr_gstx, G_w gr_gsty,
                  G_w gr_gstwidth, G_w gr_gstheight,
                  G_w gr_gfinx, G_w gr_gfiny,
                  G_w gr_gfinwidth, G_w gr_gfinheight dglob);
G_w G_decl G_n(graf_shrinkbox)( G_w gr_sfinx, G_w gr_sfiny,
                    G_w gr_sfinwidth, G_w gr_sfinheight,
                    G_w gr_sstx, G_w gr_ssty,
                    G_w gr_sstwidth, G_w gr_sstheight dglob);
#else
G_w G_decl G_n(graf_growbox)(GRECT *sr, GRECT *er dglob);
G_w G_decl G_n(graf_shrinkbox)(GRECT *sr, GRECT *er dglob);
#endif
G_w G_decl G_n(graf_watchbox)( OBJECT *gr_wptree, G_w gr_wobject,
                   G_w gr_winstate, G_w gr_woutstate dglob);
G_w G_decl G_n(graf_slidebox)( OBJECT *gr_slptree, G_w gr_slparent,
                   G_w gr_slobject, G_w gr_slvh dglob);
G_w G_decl G_n(graf_handle)( G_w *gr_hwchar, G_w *gr_hhchar,
                 G_w *gr_hwbox, G_w *gr_hhbox dglob);
#if G_EXT
G_w G_decl G_n(graf_xhandle)(G_w *wchar,G_w *hchar,G_w *wbox,G_w *hbox,G_w *device dglob);
#endif
G_w G_decl G_n(graf_mouse)( G_w gr_monumber, MFORM *gr_mofaddr dglob);
#if !G_MODIFIED
G_w G_decl G_n(graf_mkstate)( G_w *gr_mkmx, G_w *gr_mkmy,
                  G_w *gr_mkmstate, G_w *gr_mkkstate dglob);
#else
G_w G_decl G_n(graf_mkstate)(EVNTDATA *ev dglob);
#endif

/****** Scrap definitions ***********************************************/

G_w G_decl scrp_read( char *sc_rpscrap );
G_w G_decl scrp_write( char *sc_wpscrap );


/****** File selector definitions ***************************************/

G_w G_decl G_n(fsel_input)( char *fs_iinpath, char *fs_iinsel,
                G_w *fs_iexbutton dglob);
G_w G_decl G_n(fsel_exinput)( char *fs_einpath, char *fs_einsel,
                G_w *fs_eexbutton, char *fs_elabel dglob);


/****** Window definitions **********************************************/

typedef enum
{
	NAME	=0x0001,
	CLOSER	=0x0002,
	FULLER	=0x0004,
	MOVER	=0x0008,
	INFO	=0x0010,
	SIZER	=0x0020,
	UPARROW	=0x0040,
	DNARROW	=0x0080,
	VSLIDE	=0x0100,
	LFARROW	=0x0200,
	RTARROW	=0x0400,
	HSLIDE	=0x0800,
	MENUBAR	=0x1000,		/* XaAES     */
	BACKDROP =0x2000,		/* KAOS 1.4    */
	ICONIFIER=0x4000,		/* AES 4.1     */
	SMALLER  =ICONIFIER,
} WIND_ATTR;

typedef enum
{
	WF_KIND=1,
	WF_NAME,
	WF_INFO,
	WF_WORKXYWH,
	WF_CURRXYWH,
	WF_PREVXYWH,
	WF_FULLXYWH,
	WF_HSLIDE,
	WF_VSLIDE,
	WF_TOP,
	WF_FIRSTXYWH,
	WF_NEXTXYWH,
	WF_RESVD,
	WF_NEWDESK,
	WF_HSLSIZE,
	WF_VSLSIZE,
	WF_SCREEN,
	WF_COLOR,
	WF_DCOLOR,
	WF_OWNER,				/* AES 4       */
	WF_BEVENT = 24,
	WF_BOTTOM,
	WF_ICONIFY,				/* AES 4.1     */
	WF_UNICONIFY,			/* AES 4.1     */
	WF_UNICONIFYXYWH,		/* AES 4.1     */
	WF_TOOLBAR = 30,		/* compatible  */
	WF_FTOOLBAR,
	WF_NTOOLBAR,
	WF_MENU,
	WF_WIDGET,
	WF_WHEEL       = 40,
	WF_M_BACKDROP  = 100,	/* KAOS 1.4    */
	WF_M_OWNER,				/* KAOS 1.4    */
	WF_M_WINDLIST,			/* KAOS 1.4    */
	WF_SHADE       =0x575d,	/* WINX 2.3	*/
	WF_STACK	   =0x575e,	/* WINX 2.3	*/
	WF_TOPALL	   =0x575f,	/* WINX 2.3	*/
	WF_BOTTOMALL   =0x5760	/* WINX 2.3	*/
} WIND_CODE;

typedef enum
{
	W_BOX ,
	W_TITLE,
	W_CLOSER,
	W_NAME,
	W_FULLER,
	W_INFO,
	W_DATA,
	W_WORK,
	W_SIZER,
	W_VBAR,
	W_UPARROW,
	W_DNARROW,
	W_VSLIDE,
	W_VELEV,
	W_HBAR,
	W_LFARROW,
	W_RTARROW,
	W_HSLIDE,
	W_HELEV,
	W_SMALLER,		/* AES 4.1     */
	W_BOTTOMER		/* MagiC 3     */
} WIND_COMP;

/* wind_set(WF_BEVENT) */

typedef enum
{
	BEVENT_WORK=1,		/* AES 4.0	*/
	BEVENT_INFO,		/* MagiC 6	*/
} BEVENT;

typedef enum
{
	WA_UPPAGE,
	WA_DNPAGE,
	WA_UPLINE,
	WA_DNLINE,
	WA_LFPAGE,
	WA_RTPAGE,
	WA_LFLINE,
	WA_RTLINE,
	WA_WHEEL
} WIND_ARROW;

typedef enum
{
	WC_BORDER,						/* wind calc flags */
	WC_WORK
} WIND_CALC_C;

typedef enum
{
	END_UPDATE,						/* update flags */
	BEG_UPDATE,
	END_MCTRL,
	BEG_MCTRL
} WIND_UPD_FL;

#if !G_MODIFIED
G_w G_decl G_n(wind_create)( G_w wi_crkind, G_w wi_crwx, G_w wi_crwy,
                 G_w wi_crww, G_w wi_crwh dglob);
G_w G_decl G_n(wind_open)( G_w wi_ohandle, G_w wi_owx, G_w wi_owy,
               G_w wi_oww, G_w wi_owh dglob);
G_w G_decl G_n(wind_calc)( G_w wi_ctype, G_w wi_ckind, G_w wi_cinx,
               G_w wi_ciny, G_w wi_cinw, G_w wi_cinh,
               G_w *coutx, G_w *couty, G_w *coutw,
               G_w *couth dglob);
#else
G_w G_decl G_n(wind_create)(G_w kind, GRECT *r dglob);
G_w G_decl G_n(wind_open  )(G_w whl,  GRECT *r dglob);
G_w G_decl G_n(wind_calc)(G_w ty,G_w srt, GRECT *ri, GRECT *ro dglob);
#endif
G_w G_decl G_n(wind_close)( G_w wi_clhandle dglob);
G_w G_decl G_n(wind_delete)( G_w wi_dhandle dglob);
#if __WGS_ELLIPSISD__
G_w wind_get( G_w wi_ghandle, G_w wi_gfield, ... );
G_w wind_set( G_w wi_shandle, G_w wi_sfield, ... );
#else
G_w G_decl G_n(wind_get)( G_w wi_ghandle, G_w wi_gfield, G_w, G_w, G_w, G_w dglob);
G_w G_decl G_n(wind_set)( G_w wi_shandle, G_w wi_sfield, G_w *, G_w *, G_w *, G_w * dglob);
#endif
#if G_EXT
G_w G_decl G_n(wind_get_grect) (G_w whl, G_w srt, GRECT *g dglob);
G_w G_decl G_n(wind_get_int)   (G_w whl, G_w srt, G_w *g1 dglob);
G_w G_decl G_n(wind_get_ptr)   (G_w whl, G_w srt, void **v dglob);
G_w G_decl G_n(wind_set_string)(G_w whl, G_w srt, char *s dglob);
G_w G_decl G_n(wind_set_grect) (G_w whl, G_w srt, GRECT *r dglob);
G_w G_decl G_n(wind_set_int)   (G_w whl, G_w srt, G_w i dglob);
G_w G_decl G_n(wind_set_ptr_int)(G_w whl,G_w srt, void *s, G_w g dglob);
#endif
G_w G_decl G_n(wind_find)( G_w wi_fmx, G_w wi_fmy dglob);
G_w G_decl G_n(wind_update)( G_w wi_ubegend dglob);
void G_decl G_nv(wind_new);


/****** Resource definitions ********************************************/

typedef enum					/* data structure types */
{
	R_TREE,
	R_OBJECT,
	R_TEDINFO,
	R_ICONBLK,
	R_BITBLK,
	R_STRING,					/* gets pointer to free strings */
	R_IMAGEDATA,				/* gets pointer to free images */
	R_OBSPEC,
	R_TEPTEXT,					/* sub ptrs in TEDINFO */
	R_TEPTMPLT,
	R_TEPVALID,
	R_IBPMASK,					/* sub ptrs in ICONBLK */
	R_IBPDATA,
	R_IPBTEXT,
	R_BIPDATA,					/* sub ptrs in BITBLK */
	R_FRSTR,					/* gets addr of ptr to free strings */
	R_FRIMG,					/* gets addr of ptr to free images  */
} RSH_THING;

#if G_PRFXS
typedef struct rshdr
{
	G_i	rsh_vrsn,
		rsh_object,
		rsh_tedinfo,
		rsh_iconblk,    /* list of ICONBLKS */
		rsh_bitblk,
		rsh_frstr,
		rsh_string,
		rsh_imdata,     /* image data */
		rsh_frimg,
		rsh_trindex,
		rsh_nobs,       /* counts of various structs */
		rsh_ntree,
		rsh_nted,
		rsh_nib,
		rsh_nbb,
		rsh_nstring,
		rsh_nimages,
		rsh_rssize;     /* total bytes in resource */
} RSHDR;
#else
typedef struct rshdr
{
	G_i	vrsn,
		object,
		tedinfo,
		iconblk,    /* list of ICONBLKS */
		bitblk,
		frstr,
		string,
		imdata,     /* image data */
		frimg,
		trindex,
		nobs,       /* counts of various structs */
		ntree,
		nted,
		nib,
		nbb,
		nstring,
		nimages,
		rssize;     /* total bytes in resource */
} RSHDR;
#endif

G_w G_decl G_n(rsrc_load)( const char *re_lpfname dglob);
G_w G_decl G_nv(rsrc_free);
G_w G_decl G_n(rsrc_gaddr)( G_w re_gtype, G_w re_gindex, void *gaddr dglob);
G_w G_decl G_n(rsrc_saddr)( G_w re_stype, G_w re_sindex, void *saddr dglob);
G_w G_decl G_n(rsrc_obfix)( OBJECT *re_otree, G_w re_oobject dglob);
G_w G_decl G_n(rsrc_rcfix)( RSHDR *rc_header dglob);


/****** Shell definitions ***********************************************/

/* tail for default shell */

typedef struct
{
	G_i  dummy;                   /* ein Nullwort               */
	long magic;                   /* 'SHEL', wenn ist Shell     */
	G_i  isfirst;                 /* erster Aufruf der Shell    */
	long lasterr;                 /* letzter Fehler             */
	G_i  wasgr;                   /* Programm war Grafikapp.    */
} SHELTAIL;

/* shel_write modes for parameter "isover" */

typedef enum
{
	SHW_IMMED,			/* PC-GEM 2.x  */
	SHW_CHAIN,			/* TOS         */
	SHW_DOS,			/* PC-GEM 2.x  */
	SHW_PARALLEL=100,	/* MAG!X       */
	SHW_SINGLE			/* MAG!X       */
} SHW_ISOVER;

/* shel_write modes for parameter "doex" */

typedef enum
{
	SHW_NOEXEC,
	SHW_EXEC,
	SHW_EXEC_ACC =3,		/* AES 3.3	*/
	SHW_SHUTDOWN,			/* AES 3.3     */
	SHW_RESCHNG,			/* AES 3.3     */
	SHW_BROADCAST=7,		/* AES 4.0     */
	SHW_INFRECGN =9,		/* AES 4.0     */
	SHW_AESSEND,			/* AES 4.0     */
	SHW_THR_CREATE=20		/* MagiC 4.5	*/
} SHW_DOEX;

/* extended shel_write() modes and parameter structure */

typedef enum
{
	SHW_XMDLIMIT = 256,
	SHW_XMDNICE  = 512,
	SHW_XMDDEFDIR=1024,
	SHW_XMDENV   =2048,
	SHW_XMDFLAGS =4096
} SHW_XMD;

typedef struct {
	char *command;
	long limit;
	long nice;
	char *defdir;
	char *env;
	long flags;		/* ab MagiC 6 */
} XSHW_COMMAND;

#if !((defined  __STDC__) && CDECL)
/*	   using this structure is not possible
 *      if ANSI keywords only is ON, and __Cdecl is non empty
 */
typedef struct {
	long __Cdecl (*procedure)(void *par);
	void *user_stack;
	G_ul stacksize;
	G_i  mo;		/* immer auf 0 setzen! */
	long res1;		/* immer auf 0L setzen! */
} THREADINFO;
#endif

G_w G_decl G_n(shel_read)( char *sh_rpcmd, char *sh_rptail dglob);
G_w G_decl G_n(shel_write)( G_w sh_wdoex, G_w sh_wisgr, G_w sh_wiscr,
                char *sh_wpcmd, char *sh_wptail dglob);
G_w G_decl G_n(shel_get)( char *sh_gaddr, G_u sh_glen dglob);
G_w G_decl G_n(shel_put)( char *sh_paddr, G_u sh_plen dglob);
G_w G_decl G_n(shel_find)( char *sh_fpbuff dglob);
G_w G_decl G_n(shel_envrn)( char **sh_epvalue, char *sh_eparm dglob);
#if G_EXT
G_w G_decl G_n(shel_rdef)( char *fname, char *dir dglob);
G_w G_decl G_n(shel_wdef)( char *fname, char *dir dglob);

#include <aes_xtnd.h>
#endif

extern G_w aes_handle,MagX_version,radio_bgcol;
extern bool MagX,MiNT;
extern MAGX_COOKIE *magic;

void adapt3d_rsrc( OBJECT *objs, G_w no_objs, G_w hor_3d, G_w ver_3d );
void    no3d_rsrc( OBJECT *objs, G_w no_objs, G_w ftext_to_fboxtext );
G_b *is_userdef_title( OBJECT *obj );
G_w	get_aes_info( G_w version, G_w *font_id, G_w *font_height, G_w *hor_3d, G_w *ver_3d );
void substitute_objects( OBJECT *objs, G_w no_objs, G_w aes_flags, OBJECT *rslct, OBJECT *rdeslct );
void substitute_free(void);
#endif

