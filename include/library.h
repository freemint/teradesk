/*
 * Utility functions for Teradesk. Copyright 1993 - 2002  W. Klaren,
 *                                           2002 - 2003  H. Robbers
 *                                           2003 - 2007  Dj. Vukovic
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

#ifndef __LIBRARY__
#define __LIBRARY__ 1

#include <stdbool.h>
#ifdef __PUREC__
#include <portaes.h>
#include <portvdi.h>
#else
#include <portab.h>
#include <gem.h>
typedef _WORD _CDECL (*PARMBLKFUNC)(PARMBLK *pb);
#undef _AESrscfile
#define	_AESrscfile   ((*((OBJECT ***)&aes_global[5])))
#undef _AESrscmem
#define	_AESrscmem    ((*((void **)&aes_global[7])))
#define bfobspec BFOBSPEC
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <tos.h>


/*
 * Define _MINT_ as 1 to compile for multitasking; 0 for singletos only
 */
#ifdef _TOS_ONLY
#define _MINT_ 0				/* 0 for single-TOS-only desktop */
#else
#define _MINT_ 1				/* 1 for full-featured desktop */
#endif

/*
 * Define _MORE_AV as 1 for fuller support of AV-protocol features
 * at the expense of larger memory needs.
 * (sensible mostly for the multitasking version)
 */
#define _MORE_AV _MINT_			/* 0 for Single-TOS-only desktop */

/*
 * Disk-volume label editing feature is disabled for the time being,
 * as it does not behave consistently in Mint vs. Magic and may be
 * confusing.
 */
#define _EDITLABELS 0

/*
 * Define _OVSCAN as 1 to compile code for overscan (Lacescan) support.
 * Relevant only for ST-series machines.
 * It is of no use for anyone who doesn't have this hardware installed,
 * and therefore of no use on a Coldfire system.
 */
#if defined(__COLDFIRE__) || defined(__mcoldfire__)
#define _OVSCAN 0
#else
#define _OVSCAN 1
#endif

/*
 * Define CFGEMP as 1 if you want to write empty (i.e. value = 0) config fields
 * CFGEMP is passed to every CfgSave call; so it is still
 * configurable per individual config table
 */
#define CFGEMP 0

/*
 * Define PALETTES as 0 if you don't want the palette load/save stuff
 * (palette won't be saved at all then)
 */
#define PALETTES 1

/*
 * Define _PREDEF as 1 to compile setting of some predefined values
 * (for file types, keyboard shortcuts, etc.) - or define as 0 for
 * minimum executable size
 * (save about 0.5 KB but have to set everything manually then)
 */
#define _PREDEF 1

/*
 * Define _SHOWFIND as 1 to activate code which opens a text window
 * at appropriate position in the file which is the result of a string
 * search- but this may be an overdoing, therefore disabled here.
 */
#define _SHOWFIND 0

/*
 * Define _MENUDEL as 1 to delete some menu items in TOS 1.4 and older which
 * have a limited screen buffer size. If TeraDesk will not be used with these
 * tos versions, it can be set to 0 to reduce slightly the size of the program.
 * Probably safe to set to 0 for a Coldfire system.
 */
#if defined(__COLDFIRE__) || defined(__mcoldfire__)
#define _MENUDEL 0
#else
#define _MENUDEL 1
#endif

/*
 * Define _FONT_SEL as 1 to compile the system-wide font selector (FONT protocol)
 */
#define _FONT_SEL 1

/*
 * This is for debugging during development. TERADESK.LOG will be created.
 * Note that the content of the log file is not defined; the developer may
 * add specific code as needed.
 */
#define _LOGFILE 0


/* Strings of specific lengths for icon labels, file types, etc. */

typedef char INAME[13];	  /* Icon name/label length */
typedef char SNAME[18];   /* filetype mask; must be compatible with (longer than) dialog field width */
typedef char LNAME[128];  /* filename */
typedef char VLNAME[256]; /* a very long string e.g. a path */
typedef char XLNAME[2048];/* an extremely long name or other string */

typedef struct
{
	long name;
	long value;
} COOKIE;

/* Some general-purpose string-manipulation functions */

char *strsncpy(char *dst, const char *src, size_t len);	
char *strcpyj(char *dst, const char *src, size_t len);
char *strcpyq(char *d, const char *s, char qc);
char *strcpyuq(char *d, const char *s);
char *strcpyrq(char *d, const char *s, char qc, char **fb);
size_t strlenq(const char *name);

_WORD min(_WORD x, _WORD y);
_WORD max(_WORD x, _WORD y);
_WORD minmax(_WORD lo, _WORD i, _WORD hi);
long lmin(long x, long y);
long lmax(long x, long y);
long lminmax(long lo, long i, long hi);
void bell(void);
_WORD touppc(_WORD c);
char *digit(char *s, _WORD x);
void *memclr(void *s, size_t len); 


/* Funkties voor filenamen */

const char *nonwhite (const char *s);
void strip_name (char *dst, const char *src);
void cramped_name(const char *s, char *t, size_t w);

/* Funkties voor cookie-jar */

long find_cookie( long name );
_WORD install_cookie( long name,long value,COOKIE *buffer,long l );

/* GEM uitbreidingen */

_WORD aprintf( _WORD def,const char *string, ... );

/* Funkties voor het bepalen van de TOS-versie */

extern bool have_ssystem;
extern _WORD tos_version;	/* tos version, hex encoded */ 
extern _WORD aes_version;	/* aes version, hex encoded */

_WORD get_tosversion( void );
_WORD get_aesversion( void );


char *ultoa(unsigned long n, char *buffer, int radix);
char *ltoa(long n, char *buffer, int radix);
char *itoa(int n, char *buffer, int radix);

#endif
