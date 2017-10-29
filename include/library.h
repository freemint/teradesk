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

#define __LIBRARY__

#include <stdbool.h>
#include <portaes.h>
#include <portvdi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <tos.h>

typedef struct
{
        _WORD x;
        _WORD y;
        _WORD w;
        _WORD h;
} RECT;

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
