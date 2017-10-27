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
 * (c) 2001 - 2005 by Henk Robbers @ Amsterdam
 *
 */

#if __GNUC__
#include <osbind.h>
#include <sys/types.h>
#else
#include <tos.h>
/*
#include <stdio.h>
*/
#endif


#include <string.h>

#include "ahcm.h"

#define USEM1M1 1			/* If 0, XA_match can be tested */
#define LARGE 1
#define IFIRST 0

typedef struct record
{
	long size;
	char data[0];
} REC;

XA_memory XA_heap_base = {NULL, NULL, NULL, 8192, 13, 0, 0};		/* mode 3 is pref TT-ram */
/*                                       ^^^^
 * If you've made a type error           here,
 *   AHCM will nicely round it up for you :-)
 */
XA_memory XA_local_base = {NULL, NULL, NULL, 8192, 13, 0, 0};

XA_memory XA_file_base = {NULL, NULL, NULL, 16384, 13, 0, 0};

static
void *new_block(XA_memory *base, long size, bool svcur)
{
	XA_block *new;
#if IFIRST
	XA_block *first;
#else
	XA_block *last;
#endif

	if (base->mode)
		new = Mxalloc(size, base->mode);
	else
		new = Malloc(size);

 	if (new)
	{
		XA_unit *fr;

#if IFIRST
		first = base->first;
		if (first)
		{
			first->prior = new;
			base->first = new;
			new->prior = NULL;
			new->next = first;
#else
		last = base->last;
		if (last)
		{
			last->next = new;
			base->last = new;
			new->next = NULL;
			new->prior = last;
#endif
		} else {
			base->last = base->first = new;
			new->next = new->prior = NULL;
		}
		new->used.first = new->used.last = new->used.cur = NULL;
		fr = new->area;
		new->free.first = new->free.last = new->free.cur = fr;
		fr->next = fr->prior = NULL;
		fr->size = size - blockprefix;
		new->size = size;
		if (svcur)
			base->cur = new;
	}
	return new;
}

static
void free_block(XA_memory *base, XA_block *blk)
{
	XA_block *next = blk->next, *prior = blk->prior;

	if (next)
		next->prior = prior;
	else
		base->last = prior;
	if (prior)
		prior->next = next;
	else
		base->first = next;

	base->cur = prior ? prior : next;
	Mfree(blk);
}

static
void extract(XA_list *list, XA_unit *this)
{
	XA_unit *next = this->next, *prior = this->prior;
	if (next)
		next->prior = prior;
	else
		list->last = prior;
	if (prior)
		prior->next = next;
	else
		list->first = next;
	list->cur = prior ? prior : next;
}

static
XA_block *find_free(XA_memory *base, long s)
{
	XA_block *blk;

	blk = base->cur;
	if (blk)
		if (blk->free.cur)
			if (blk->free.cur->size >= s)
				return blk;

	blk = base->first;
	while (blk)
	{
		XA_unit *fr = blk->free.first;
		while (fr)
		{
			if (fr->size >= s)
			{
				base->cur = blk;
				blk->free.cur = fr;
				return blk;
			}
			fr = fr->next;
		}
		blk = blk->next;
	}
	return NULL;
}

/* Option LARGE:
   Remember that blocks are always a whole multiple of roundup size !!
   If a block is larger then chunk size, we put a free area
   at the front of the large block. No space is wasted and when
   the allocated large unit is freed, the large block can be
   shrunken to the chunk size.
*/
static
XA_unit *split(XA_list *list, long s, bool large)
{
	XA_unit *cur = list->cur;
	long l = cur->size - s;

	if (l > 2*unitprefix)
	{
		XA_unit *new  = cur,
		        *next = cur->next;
#if LARGE
		if (large)
		{
			(long)cur += l;
			cur->next = next;
			cur->prior = new;
			if (next)
				next->prior = cur;
			else
				list->last = cur;
			new->next = cur;
		}
		else
#endif
		{
			(long)new += s;
			new->next = next;
			new->prior = cur;
			if (next)
				next->prior = new;
			else
				list->last = new;
			cur->next = new;
		}
		new->size = l;
		cur->size = s;
		new->key = -1;
		new->type = -1;

		list->cur = new;		/* last split off becomes cur. */
	}

	return cur;
}

static
void insfirst(XA_list *list, XA_unit *this)
{
	list->cur = this;
	this->prior = NULL;
	this->next = list->first;
	if (list->first)
		list->first->prior = this;
	else							/* if last is NULL, first also NULL */
		list->last = this;
	list->first = this;
}

static
void insbefore(XA_list *list, XA_unit *this, XA_unit *before)
{
	this->next = before;
	if (before->prior)
		before->prior->next = this;
	else
		list->first = this;
	this->prior = before->prior;
	before->prior = this;
}

static
void inslast(XA_list *list, XA_unit *this)
{
	list->cur = this;
	this->next = NULL;
	this->prior = list->last;
	if (list->last)
		list->last->next = this;
	else							/* if first is NULL, last also NULL */
		list->first = this;
	list->last = this;
}

static
void insafter(XA_list *list, XA_unit *this, XA_unit *after)
{
	this->prior = after;
	if (after->next)
		after->next->prior = this;
	else
		list->last = this;
	this->next = after->next;
	after->next = this;
}

static
void amalgam(XA_list *list, XA_unit *this)
{
	if (this->next)
	{
		XA_unit *next = this->next;
		if ((long) this + this->size == (long) next)
		{
			this->size += next->size;
			next = next->next;
			if (next)
				next->prior = this;
			else
				list->last = this;
			this->next = next;
		}
	}
	list->cur = this;
}

static
void sortins(XA_list *list, XA_unit *this)
{
	XA_unit *have;

	have = list->cur;
	if (!have)
		have = list->first;
	if (!have)
		insfirst(list, this);
	else
	if (have->area < this->area)
	{
		while (have && have->area < this->area)
			have = have->next;
		if (!have)
			inslast(list, this);
		else
			insbefore(list, this, have);
	} else {
		while (have && have->area > this->area)
			have = have->prior;
		if (!have)
			insfirst(list, this);
		else
			insafter(list, this, have);
	}
}

static
void combine(XA_list *list, XA_unit *this)
{
	amalgam(list, this);
	if (this->prior)
		amalgam(list, this->prior);
}

static
size_t XA_round(XA_memory *base, size_t size)
{
	long y;
	short x = base->round;

	if (x < 12) x = 12;
	if (x > 16) x = 16;

	y = (1L << x) - 1;		/* build a mask */

	if (size & y)
	{
		y = ((size >> x) + 1) << x;		/* must roundup */
		return y;
	}
	else
		return size;					/* already round */
}

static
bool in_list(XA_list *list, void *area)
{
	XA_unit *at;

	if (list->cur)
		if (list->cur->area == area)
			return true;

	at = list->first;
	while (at)
	{
		if (at->area == area)
			return true;

		at = at->next;
	}

	return false;
}

static
XA_block *find_unit(XA_memory *base, void *area)
{
	XA_block *blk = base->cur;

	if (blk)
		if (blk->used.cur)
			if (blk->used.cur->area == area)
				return blk;

	blk = base->first;

	while (blk)
	{
		long x = (long)area,
		     b = (long)blk->area + unitprefix;

		/* Is the area in this block ? */
		if ( x >= b && x <  b + blk->size - (blockprefix + unitprefix) )
		    if (in_list(&blk->used, area))
				return blk;

		blk = blk->next;
	}

	return NULL;
}

/* The smaller the requested area, the less likely it is
   allocated & freed only once in a while. */
void *XA_alloc(XA_memory *base, size_t size, XA_key key, XA_key type)
{
	XA_block *blk = NULL;
	bool large = false;
	long s = ((size+3)&-4) + unitprefix;

	if (!base)
		base = &XA_heap_base;

	if (s > base->chunk - blockprefix)
	{
		long ns = XA_round(base, s + blockprefix);
		blk = new_block(base, ns, false);
		if (!blk)
			return NULL;
		large = true;
	}
	else
		blk = find_free(base, s);

	if (!blk)
	{
		base->chunk = XA_round(base, base->chunk);
		blk = new_block(base, base->chunk, true);
	}

	if (blk)
	{
		XA_unit *this;

		this = split(&blk->free, s, large);
		extract(&blk->free, this);
#if IFIRST
		insfirst(&blk->used, this);
#else
		inslast(&blk->used, this);
#endif
		this->key = key;
		this->type = type;

#if TESTSHRINK
		if (large)
		{
			XA_unit *test = blk->free.first;
			if (test)
			{
				/* By putting the excess unit in the used list I can easily
				   test the shrink process. */
				extract(&blk->free, test);
				insfirst(&blk->used, test);
			}
		}
#endif
		return this->area;
	}
	return NULL;
}

void *XA_calloc(XA_memory *base, size_t items, size_t chunk, XA_key key, XA_key type)
{
	void *new = NULL;
	long l = items*chunk;
	if (l)
	{
		new = XA_alloc(base, l, key, type);
		if (new)
			memset(new,0,l);
	}
	return new;
}


static
void free_unit(XA_memory *base, XA_block *this, XA_unit *at)
{
	extract(&this->used, at);
	sortins(&this->free, at),
	combine(&this->free, at);

	if (this->used.first == NULL)
		free_block(base, this);
#if LARGE
	else
	if (this->size > base->chunk)		/* 'large' block ? */
	{
		/* shrink a large block if it now has a large free unit at the end of the block */

		at = this->free.last;		/* free list is sorted on address */
		if (at)
		{
			if (    at->size > base->chunk - blockprefix	/* 'large' free unit ? */
			    && (long)at + at->size == (long)this + this->size		/* at end of block ? */
			   )
			{
				long s = 0, t = blockprefix;
				/* self repair the multiple chunksize prerogative */
				while ((long)this + s < (long)at + unitprefix)
					s += base->chunk;

				if (s < this->size)
				{
					t = this->size - s;
					this->size -= t;
					at->size -= t;
					Mshrink(this, this->size);
				}
			}
		}
	}
#endif
}

void *XA_realloc(XA_memory *base, void *area, size_t size, XA_key key, XA_key type)
{
	XA_block *blk;

	if (area == NULL)
		return XA_alloc(base, size, key, type);

	if (!base)
		base = &XA_heap_base;

	blk = find_unit(base, area);
	if (blk)
	{
		XA_unit *this = area;
		long old_size, new_size;
		(long)this -= unitprefix;
		new_size = ((size+3)&-4) + unitprefix;
		old_size = this->size;

		if (new_size < old_size)
		{
			XA_unit *frf;

			if (old_size - new_size < 2*unitprefix)
				return area;

			blk->used.cur = this;
			/* split off excess && integrate in free list */
			split(&blk->used, new_size, false);
			if (this->next)
				free_unit(base, blk, this->next);


#if LARGE
		/* reasonable optinization when realloc was used in environment having
		   extreme differences between allocation sizes (large blocks involved)
		*/
			frf = blk->free.first;
			if (frf && frf < this)
			{
				if (    blk->size >  base->chunk
				    && new_size  <  base->chunk - (unitprefix + blockprefix)
				    && frf->size >= new_size
				    && frf->size - new_size > 2*unitprefix
				   )
				{
					blk->free.cur = frf;
					split(&blk->free, new_size, false);
					extract(&blk->free, frf);
					inslast(&blk->used, frf);
					memmove(frf->area, area, size);
					free_unit(base, blk, this);
					this = frf;
				}
			}
#endif
			this->key = key;
			this->type = type;
			return this->area;
		}
		else
		if (new_size > old_size)
		{
			XA_unit *follow = this;
			long diff = new_size - old_size, fs;

			(long)follow += old_size;
			fs = follow->size;

			/* is the following physical unit free and large enough to hold the extension */

			if (    old_size + fs > new_size
				&& fs - diff > 2*unitprefix
			    && in_list(&blk->free, follow->area)
			   )
			{
				XA_unit *followup = follow;

				/* just move up the boundery between this and the following free area */

				(long)followup += diff;
				memmove(followup, follow, unitprefix);
				followup->size -= diff;
				this->size += diff;

				if (followup->prior)
					followup->prior->next = followup;
				else
					blk->free.first = followup;

				if (followup->next)
					followup->next->prior = followup;
				else
					blk->free.last = followup;

				blk->free.cur = followup;
				this->key = key;
				this->type = type;
				return this->area;
			}
			else
			{
				void *new;
				new = XA_alloc(base, size, key, type);
				if (new)
				{
					memmove(new, area, old_size - unitprefix);
					free_unit(base, blk, this);
				}
				return new;
			}
		}
		else
			return area;				/* same size, do nothing */
	}

	return NULL;
}

void XA_free(XA_memory *base, void *area)
{
	XA_block *blk;
	if (!area)
		return;
	if (!base)
		base = &XA_heap_base;

	blk = find_unit(base, area);
	if (blk)
	{
		XA_unit *this = area;
		(long)this -= unitprefix;
		free_unit(base, blk, this);
	}
}

bool XA_match(XA_unit *at, XA_key key, XA_key type)
{
	bool m =(   key  == -1
	         || key  == at->key )
	    && (   type == -1
	         || type == at->type )
		;
	return m;
}

void XA_free_all(XA_memory *base, XA_key key, XA_key type)
{
	XA_block *blk;

	if (!base)
		base = &XA_heap_base;
	blk = base->first;

#if USEM1M1					/* This is only for being able to test XA_match */
	if (key == -1 && type == -1)
	{
		while(blk)
		{
			XA_block *next = blk->next;
			Mfree(blk);
			blk = next;
		}
		base->first = base->last = base->cur = NULL;
	}
	else
#endif
	{
/* So I can detect leaking */
		while (blk)
		{
			XA_block *bx = blk->next;
			XA_unit *at = blk->used.first;
			while (at)
			{
				XA_unit *ax = at->next;
				if (XA_match(at, key, type))
					free_unit(base, blk, at);
				at = ax;
			}
			blk = bx;
		}
	}
}

void XA_up(XA_memory *base)
{
	if (!base)
		base = &XA_local_base;
	++base->stack;
}

void *XA_new(XA_memory *base, size_t size, XA_key key)
{
	if (!base)
		base = &XA_local_base;
	return XA_alloc(base, size, key, base->stack);
}

void XA_down(XA_memory *base)
{
	XA_block *blk;

	if (!base)
		base = &XA_local_base;

	blk = base->first;

	while (blk)
	{
		XA_block *bx = blk->next;
		XA_unit *at = blk->used.first;
		while (at)
		{
			XA_unit *ax = at->next;

			if (at->type >= base->stack)
				free_unit(base, blk, at);
			at = ax;
		}
		blk = bx;
	}

	if (base->stack > 0)
		--base->stack;
}

bool XA_leaked(XA_memory *base, XA_key key, XA_key type, XA_report *report)
{
	XA_block *blk; bool reported = false;
	if (!base)
		base = &XA_heap_base;
	blk = base->first;

	while (blk)
	{
		XA_unit *at = blk->used.first;
		while (at)
		{
			if (XA_match(at, key, type))
			{
				report(base, blk, at, "leak");
				reported = true;
			}
			at = at->next;
		}
		blk = blk->next;
	}
	return reported;
}

void XA_follow(XA_memory *base, XA_block *blk, XA_list *list, XA_report *report)
{
	/* Go up and down the list, finish at the same address. */
	XA_unit *un = list->first, *n = un;
	while (un)
	{
		n = un;
		un = un->next;
	}
	if (n != list->last)
	{
		report(base, blk, n, "last found");
		report(base, blk, list->last, "list->last");
	}
	un = list->last;
	n = un;
	while (un)
	{
		n = un;
		un = un->prior;
	}
	if (n != list->first)
	{
		report(base, blk, n, "first found");
		report(base, blk, list->first, "list->first");
	}
}

void XA_sanity(XA_memory *base, XA_report *report)
{
	XA_block *blk;

	if (!base)
		base = &XA_heap_base;
	/* follow sizes and see if they fit. */
	blk = base->first;
	while (blk)
	{
		REC *at = (REC *)blk,
		    *to = at,
		    *pr;
		(long)at += blockprefix;
		(long)to += blk->size;

		while (at < to)
		{
			pr = at;
			(long)at += at->size;
		}

		if (at != to)
			report(base, blk, (XA_unit *)pr, "insane");
		XA_follow(base, blk, &blk->free, report);
		XA_follow(base, blk, &blk->used, report);
		blk = blk->next;
	}
}

void XA_set_base(XA_memory *base, size_t chunk, short round, short flags)
{
	if (!base)
		base = &XA_heap_base;

	base->first = base->last = base->cur = NULL;

/* 0: default, negative: dont change */
	if (!chunk)
		base->chunk = XA_heap_base.chunk;
	else if (chunk > 0)
		base->chunk = chunk;

	if (!round)
		base->round = XA_heap_base.round;
	else if (round > 0)
		base->round = round;

	if (base->round < 12) base->round = 12;
	if (base->round > 16) base->round = 16;

	base->mode = flags;
}

void XA_list_free(XA_memory *base, XA_report *report)
{
	XA_block *blk;
	if (!base)
		base = &XA_heap_base;

	blk = base->first;

	while (blk)
	{
		XA_unit *at = blk->free.first;
		while (at)
		{
			report(base, blk, at, "free");
			at = at->next;
		}
		blk = blk->next;
	}

}

#if XA_lib_replace

void *malloc(size_t size)
{
	return XA_alloc(NULL, size, 0, 0);
}

void *calloc(size_t items, size_t chunk)
{
	return XA_calloc(NULL, items, chunk, 0, 0);
}

void *realloc(void *addr, size_t size)
{
	return XA_realloc(NULL, addr, size, 0, 0);
}

void free(void *addr)
{
	XA_free(NULL, addr);
}
#endif

void _FreeAll(void)
{
	XA_free_all(NULL, -1, -1);
}


void *xmalloc(size_t size, XA_key key)
{	return XA_alloc(NULL, size, key, 0);	}

void *xcalloc(size_t items, size_t chunk, XA_key key)
{	return XA_calloc(NULL, items, chunk, key, 0);}

void *xrealloc(void *old, size_t size, XA_key key)
{	return XA_realloc(NULL, old, size, key, 0);	}


void *fmalloc(size_t size, XA_key key)
{	return XA_alloc(&XA_file_base, size, key, 0);	}

void *fcalloc(size_t items, size_t chunk, XA_key key)
{	return XA_calloc(&XA_file_base, items, chunk, key, 0);	}

void *frealloc(void *old, size_t size, XA_key key)
{	return XA_realloc(&XA_file_base, old, size, key, 0);	}
void ffree(void *area)
{	XA_free(&XA_file_base, area);	}
void ffree_all(XA_key key)
{	XA_free_all(&XA_file_base, key, -1);	}

