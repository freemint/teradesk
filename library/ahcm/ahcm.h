/*
 * A Home C
 *
 * (c) 2000 - 2003 by Henk Robbers @ Amsterdam
 */
 
#ifndef _XA_MEMORY_H
#define _XA_MEMORY_H

#include <prelude.h>
#include <stdlib.h>		/* size_t */

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

extern XA_memory XA_default_base;

typedef void XA_report(XA_memory *base, XA_block *blk, XA_unit *unit, char *txt);

void 	XA_set_base	(XA_memory *base, size_t chunk, short round, short flags);
void *	XA_alloc	(XA_memory *base, size_t amount, XA_key key, XA_key type);
void *	XA_calloc	(XA_memory *base, size_t items, size_t chunk, XA_key key, XA_key type);
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
void free(void *);
void _FreeAll(void);
#else
/* In this case, only files that include ahcm.h
   invoke AHCM. Other objects are not affected.
   Calls to standard C malloc in libraries are not
   replaced.
*/
#define calloc(n,l) XA_calloc(nil, (n), (l), 0)
#define malloc(l)   XA_alloc (nil, (l), 0)
#define free(a)     XA_free  (nil, (a))
#define _freeAll()	XA_free_all(nil, -1, -1)
#endif

#define unitprefix sizeof(XA_unit)
#define blockprefix sizeof(XA_block)


#endif

