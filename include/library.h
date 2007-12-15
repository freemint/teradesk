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

#ifndef _BOOLEAN_
#include <boolean.h>
#endif

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
char *strcpyuq(char *d, char *s);
char *strcpyrq(char *d, const char *s, char qc, char **fb);
size_t strlenq(const char *name);

int min(int x, int y);
int max(int x, int y);
int minmax(int lo, int i, int hi);
long lmin(long x, long y);
long lmax(long x, long y);
long lminmax(long lo, long i, long hi);
void bell(void);
int touppc(int c);
char *digit(char *s, int x);
void *memclr(void *s, size_t len); 


/* Funkties voor filenamen */

char *nonwhite ( char *s);
void strip_name (char *dst, const char *src);
void cramped_name(const char *s, char *t, size_t w);

/* Funkties voor cookie-jar */

long find_cookie( long name );
int install_cookie( long name,long value,COOKIE *buffer,long l );

/* GEM uitbreidingen */

int aprintf( int def,const char *string, ... );

/* Funkties voor het bepalen van de TOS-versie */

int get_tosversion( void );
int get_aesversion( void );


#endif