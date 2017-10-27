/*
 * Thread safe and debug friendly memory allocator
 *
 * This file is part of AHCM. A Home Cooked Memory allocator.
 *
 * AHCM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * AHCM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AHCM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * (c) 2001 - 2004 by Henk Robbers @ Amsterdam
 *
 */

#ifdef _XA_MEMORY_H
#pragma if_endfile			/* AHCC: perform #endif and abandon file immediately */
/* there is also if_endcomp and if_endmake, + the equivalents without if_ */
#else
#define _XA_MEMORY_H

#include <stdlib.h>		/* size_t */
#include <stdbool.h>

#define XA_lib_replace 1

typedef short XA_key;

typedef struct xa_unit
{
	long size;				/* MUST be in front and include unitprefix !! */
	struct xa_unit *next,*prior;
	XA_key key,
	       type;
	char area[0];
} XA_unit;

typedef struct xa_list
{
	XA_unit *first, *cur, *last;
} XA_list;

typedef struct xa_block		/* These are the big ones, at least 8K */
{
	long size;				/* MUST be in front and include blockprefix!! */
	struct xa_block *next, *prior;
	XA_list used, free;
	short mode;
	XA_unit area[0];
} XA_block;

typedef struct xa_memory
{
	XA_block *first, *last, *cur;		/* No free pointer here, blocks that
									   become free are returned to GEMDOS */
	long chunk;
	short round,
	      mode,
	      stack;
} XA_memory;

extern XA_memory XA_heap_base, XA_local_base, XA_file_base;

typedef void XA_report(XA_memory *base, XA_block *blk, XA_unit *unit, char *txt);

void 	XA_set_base	(XA_memory *base, size_t chunk, short round, short flags);
void *	XA_alloc	(XA_memory *base, size_t amount, XA_key key, XA_key type);
void *	XA_calloc	(XA_memory *base, size_t items, size_t chunk, XA_key key, XA_key type);
void *	XA_realloc	(XA_memory *base, void *area, size_t size, XA_key key, XA_key type);
void 	XA_free		(XA_memory *base, void *area);
void 	XA_free_all	(XA_memory *base, XA_key key, XA_key type);
void	XA_up		(XA_memory *base);
void *	XA_new		(XA_memory *base, size_t size, XA_key key);
void	XA_down		(XA_memory *base);
bool	XA_leaked	(XA_memory *base, XA_key key, XA_key type, XA_report *report);
void	XA_sanity	(XA_memory *base, XA_report *report);
void    XA_list_free(XA_memory *base, XA_report *report);

#if XA_lib_replace
/* The below are wrapper functions and do the same
   as the macros further below.
   You can find them in ahcm.c
   By having these functions in ahcm.c
   existing library functions are completely replaced.
*/
	void *calloc(size_t n, size_t sz);
	void *malloc(size_t size);
	void *realloc(void *, size_t);
	void free(void *);
	void _FreeAll(void);
#else
/* In this case, only files that include ahcm.h
   invoke AHCM. Other objects are not affected.
   Calls to standard C malloc in libraries are not
   replaced.
*/
	#define calloc(n,l) XA_calloc(nil, (n), (l), 0, 0)
	#define malloc(l)   XA_alloc (nil, (l), 0, 0)
	#define realloc(p,l) XA_realloc(nil, (p), (l), 0, 0)
	#define free(a)     XA_free  (nil, (a))
	#define _freeAll()	XA_free_all(nil, -1, -1)
#endif

#define unitprefix sizeof(XA_unit)
#define blockprefix sizeof(XA_block)

void *xmalloc(size_t, XA_key);
void *xcalloc(size_t items, size_t chunk, XA_key key);
void *xrealloc(void *old, size_t size, XA_key key);

void *fmalloc(size_t, XA_key);
void *fcalloc(size_t items, size_t chunk, XA_key key);
void *frealloc(void *old, size_t size, XA_key key);
void ffree(void *);
void ffree_all(XA_key key);
#endif

