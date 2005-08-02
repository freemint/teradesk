/*
 * Utility functions for Teradesk. Copyright       1993, 2002  W. Klaren,
 *                                                 2002, 2003  H. Robbers
 *                                           2003, 2004, 2005  Dj. Vukovic
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

#ifndef _BOOLEAN_
#include <boolean.h>
#endif

typedef struct
{
	long name;
	long value;
} COOKIE;

/* Some general-purpose functions */

char *strsncpy(char *dst, const char *src, size_t len);	
char *strcpyj(char *dst, const char *src, size_t len);

int min(int x, int y);
int max(int x, int y);
int minmax(int lo, int i, int hi);
long lmin(long x, long y);
long lmax(long x, long y);
int lminmax(long lo, long i, long hi);
void bell(void);
int touppc(int c);
void digit(char *s, int x);
void free_item( void **ptr );
void *memclr(void *s, size_t len); 


/* Funkties voor filenamen */

void make_path( char *name,const char *path,const char *fname );
void split_path( char *path,char *fname,const char *name );
void strip_name (char *dst, const char *src);
void cramped_name(const char *s, char *t, int w);

/* Funkties voor cookie-jar */

long find_cookie( long name );
int install_cookie( long name,long value,COOKIE *buffer,long l );

/* GEM uitbreidingen */

int aprintf( int def,const char *string, ... );

/* Funkties voor het bepalen van de TOS-versie */

int get_tosversion( void );
