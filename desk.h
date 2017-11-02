/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren.
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2014  Dj. Vukovic
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


#ifdef MEMDEBUG
/* Note: a specific directory required below: */
#include <d:\tmp\memdebug\memdebug.h>
#endif


#undef O_APPEND

/* 
 * Some convenient macros for manipulating dialog objects
 */

#define obj_hide(x)		x.ob_flags |= OF_HIDETREE
#define obj_unhide(x)	x.ob_flags &= ~OF_HIDETREE  
#define obj_select(x)	x.ob_state |= OS_SELECTED
#define obj_deselect(x)	x.ob_state &= ~OS_SELECTED
#define obj_enable(x)	x.ob_state &= ~OS_DISABLED
#define obj_disable(x)	x.ob_state |= OS_DISABLED

/* Maximum number of Teradesk's windows of one type */

#define MAXWINDOWS 8 

/* Diverse options which are bitflags */


/* Copy and print options; these go into options.cprefs */

#define CF_COPY			0x0001	/* confirm copy              */
#define CF_DEL			0x0002	/* confirm delete            */
#define CF_OVERW		0x0004	/* confirm overwrite         */
#define CF_PRINT		0x0008 	/* confirm print             */
#define CF_TOUCH		0x0010  /* confirm touch (not used)  */
#define CF_KEEPS		0x0020	/* keep selection after copy */
#define CF_TRUNN		0x0040	/* truncate long names if needed */
#define P_GDOS			0x0080	/* use GDOS device for printing; currently NOT used */
								/* unused 0x0100 */
								/* unused 0x0200 */
#define CF_CTIME		0x0400	/* change date & time */
#define CF_CATTR		0x0800  /* change file attributes */
#define CF_SHOWD		0x1000 	/* always show copyinfo dialog */
#define P_HEADER		0x2000	/* print header and formfeed */
#define CF_FOLL			0x4000	/* follow links */

/* Other diverse dialog options */

#define DIALPOS_MODE	0x0040	/* OFF = mouse, ON = center    */
#define DIAL_MODE   	0x0003  /* 0x0001 = XD_BUFFERED (flying), 0x0002 = XD_WINDOW */

#define TEXTMODE		0		/* display directory as text */
#define ICONMODE		1		/* display directory as icons */

/* These options are for menu items */

#define WD_SORT_NAME	0x00	/* sort directory by file name  */
#define WD_SORT_EXT		0x01	/* sort directory by file type  */
#define WD_SORT_DATE	0x02	/* sort directory by file date  */
#define WD_SORT_LENGTH	0x03	/* sort directory by file size  */
#define WD_NOSORT		0x04	/* don't sort directory         */
#define WD_REVSORT	 	0x10	/* reverse sort order (bitflag) */
#define WD_NOCASE	 	0x20	/* sort case insensitive (bitflag) */

/* Option bitflags for elements to be shown in directory windows */

#define WD_SHSIZ 0x0001 /* show file size  */
#define WD_SHDAT 0x0002 /* show file date  */
#define WD_SHTIM 0x0004 /* show file time  */
#define WD_SHATT 0x0008 /* show attributes */
#define WD_SHOWN 0x0010 /* show owner      */

/* Option bitflags for some settable video modes and related information in options.vprefs */

#define VO_BLITTER	0x0001 	/* video option blitter ON  */
#define VO_OVSCAN  	0x0002 	/* video option overscan ON */
							/* unused: 0x0004 0x0008 0x0010 0x0020 0x0040 0x0080 */
#define SAVE_COLOURS	0x0100	/* save palette */

/* Saving options; these go into options.sexit */

#define SAVE_CFG	0x0001	/* Save configuration at exit */
#define SAVE_WIN	0x0002	/* Save open windows at exit */

/* Option bitflags for other diverse settings; these go into options.xprefs */

#define S_IGNCASE	0x0001	/* Ignore string case when searching */
#define S_SKIPSUB	0x0002	/* Skip subdirectories when searching */
							/* Unused: 0x0002, 0x0004 */
#define TOS_KEY		0x0020	/* 0 = continue, 1 = wait */
#define TOS_STDERR	0x0200	/* 0 = no redirection, 1 = redirect handle 2 to 1. */

/*
 * It is assumed that the first menu item for which a keyboard shortcut
 * can be defined is MOPEN and the last is MSAVEAS; the interval
 * is defined below by MFIRST and MLAST. 
 * Index of the fist relevant menu title is likewise defined.
 * Beware of dimensioning options.kbshort[64]; it should be larger
 * than NITEM below; currently, about 58 out of 64 are used.
 */

#define MFIRST MOPEN			/* first menu item for which a shortcut can be */
#define MLAST  MSAVEAS			/* last menu item ... */
#define TFIRST TLFILE			/* title under which the first item is */
#define NITEM MLAST - MFIRST	/* number of, -1 */

/* Executable file (program) types */

typedef enum
{
	PGEM = 0, 
	PGTP, 
	PACC, 
	PTOS, 
	PTTP
} ApplType;


/* Configuration options structure */

typedef struct
{	
	/* Desktop */

	_WORD version;				/* cfg. file version */
	_WORD cprefs;				/* copy prefs: CF_COPY|CF_DEL|CF_OVERW|CF_PRINT|CF_TOUCH|CF_CTIME|CF_CATTR|CF_SHOWD|P_HEADER|CF_FOLL */
	_WORD xprefs;				/* more preferences: S_IGNCASE | S_SKIPSUB | TOS_KEY | TOS_STDERR  */
	_WORD dial_mode;			/* dialog mode (window/flying) */
	_WORD sexit;				/* save on exit */
	_WORD kbshort[NITEM + 2];	/* keyboard shortcuts */

	/* Sizes */

	_WORD bufsize;				/* copy buffer size  */
	long max_dir;				/* maximum no of entries for 1 hierarchic level */
	_WORD plinelen;				/* printer line length */
	_WORD tabsize;				/* global tab size */
	_WORD cwin;					/* compare match window */

	/* View */

	_WORD mode;					/* text or icon mode */
	_WORD sort;					/* sorting rule */
	_WORD aarr;					/* auto arrange directory items */
	_WORD fields;				/* shown file data elements */
	_WORD attribs;				/* shown attributes of visible directory items */

	/* Video */

	_WORD vprefs;           	/* video preferences  */
	_WORD vrez;             	/* video resolution */
	_WORD dsk_colour;			/* desktop colour */
	_WORD win_colour;			/* window colour */
	_WORD dsk_pattern;			/* desktop pattern  */
	_WORD win_pattern;			/* window pattern */
} Options;


typedef struct
{
	_WORD item;					/* nummer van icoon */
	_WORD m_x;					/* coordinaten middelpunt t.o.v. muis, alleen bij iconen */
	_WORD m_y;
	_WORD np;						/* number of points in object's contour */
	_WORD coords[18];				/* coordinaten van schaduw */
} ICND;

extern Options
	options;

extern char 
	*global_memory;

extern const char
	*nopage,
	*empty,
	*bslash,
	*adrive,
	*prevdir;

extern long 
	global_mem_size;

extern _WORD 
	vdi_handle,		/* workstation handle */ 
	ap_id,			/* application id of TeraDesk itself */ 
	nfonts;			/* number of available fonts */

/* Flags to show a specific OS or AES type (detected from cookies) */

#if _MINT_
extern bool mint;	/* mint or magic present */
extern bool naes;	/* naes present */
extern bool geneva;	/* geneva present */

#endif

extern bool
	shutdown,		/* shutdown has been initiated */
	startup;		/* startup is in progress */


void *malloc_chk(size_t size);
_WORD chk_xd_dialog(OBJECT *tree, _WORD start);
_WORD chk_xd_open(OBJECT *tree, XDINFO *info);
void set_opt(OBJECT *tree, _WORD flags, _WORD opt, _WORD button ); 
void get_opt(OBJECT *tree, _WORD *flags, _WORD opt, _WORD button );
_WORD hndlmessage(_WORD *message);
bool wait_to_quit(void);


