/*
 * Thread safe and debug friendly memory allocator
 *
 * (c) 2001 - 2003 by Henk Robbers @ Amsterdam
 *
 */ 


#if __GNUC__
#include <osbind.h>
#include <sys/types.h>
#else
#include <tos.h>
#include <stdio.h>
#endif


#include <string.h>

#include "ahcm.h"

/* NOTE! AHCM will only be pure and reentrant if DEBUG is 0 */
#define MDEBUG 0
#define TESTSHRINK 0
#define USEM1M1 1			/* If 0, XA_match can be tested */
#define LARGE 1
#define IFIRST 0

#if MDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

XA_memory XA_heap_base = {nil, nil, nil, 8192, 13, 0, 0};		/* flags 3 is pref TT-ram */
/*                                       ^^^^
 * If you've made a type error           here,
 *   AHCM will nicely round it up for you :-)
 */
XA_memory XA_local_base = {nil, nil, nil, 8192, 13, 0, 0};

static
void *new_block(XA_memory *base, long size, bool svcur)
{
	XA_block *new, *last, *first;

	if (base->mode)
		new = Mxalloc(size, base->mode);
	else
		new = Malloc(size);

 	if (new)
	{
		XA_unit *fr;
		DBG((" - new_block %ld :: %ld\n", new, size);)
#if IFIRST
		first = base->first;
		if (first)
		{
			first->prior = new;
			base->first = new;
			new->prior = nil;
			new->next = first;
#else
		last = base->last;
		if (last)
		{
			last->next = new;
			base->last = new;
			new->next = nil;
			new->prior = last;
#endif
		othw
			base->last = base->first = new;
			new->next = new->prior = nil;
		}
		new->used.first = new->used.last = new->used.cur = nil;
		fr = new->area;
		new->free.first = new->free.last = new->free.cur = fr;
		fr->next = fr->prior = nil;
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
#if MDEBUG
	XA_list *free = &blk->free;
	XA_unit *this = free->first;
	if (!free->first or !free->last)
		printf("free_block; wrong free list f:%ld, l:%ld\n",free->first,free->last);
	else if (this->next or this->prior)
		printf("free_block; wrong free unit p:%ld, n:%ld\n",this->prior,this->next);
	else if (this->size ne blk->size - blockprefix)
		printf("free_block; wrong free size: %ld, b:%ld, diff:%ld\n",
			this->size, blk->size, blk->size - (this->size + blockprefix));
#endif
	if (next)
		next->prior = prior;
	else
		base->last = prior;
	if (prior)
		prior->next = next;
	else
		base->first = next;
	DBG((" - Free block %ld :: s%ld\n", blk, blk->size);)
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
			{
				DBG((" - f0 %ld %ld:: %ld\n",blk, blk->free.cur, blk->free.cur->size);)
				return blk;
			}

	blk = base->first;
	while (blk)
	{
		XA_unit *fr = blk->free.first;
		while (fr)
		{
			DBG((" - f1 %ld :: %ld\n", fr, fr->size);)
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
	return nil;
}

/* Remember that blocks are always a whole multiple of roundup size !!
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
	#if MDEBUG
			DBG(("large block %ld,%ld\n", s, l);)
	#endif
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
#if MDEBUG
	else
		printf("Not split %ld :: s%ld\n", cur, cur->size);
#endif
	return cur;
}

static
void insfirst(XA_list *list, XA_unit *this)
{
	DBG(("insfirst %ld :: %ld\n", this, this->size);)
	list->cur = this;
	this->prior = nil;
	this->next = list->first;
	if (list->first)
		list->first->prior = this;
	else							/* if last is nil, first also nil */
		list->last = this;
	list->first = this;
}

static
void insbefore(XA_list *list, XA_unit *this, XA_unit *before)
{
	DBG(("insbefore %ld\n", before);)
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
	DBG(("inslast %ld :: %ld\n", this, this->size);)
	list->cur = this;
	this->next = nil;
	this->prior = list->last;
	if (list->last)
		list->last->next = this;
	else							/* if first is nil, last also nil */
		list->first = this;
	list->last = this;
}

static
void insafter(XA_list *list, XA_unit *this, XA_unit *after)
{
	DBG(("insafter %ld\n", after);)
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
		if ((long) this + this->size eq (long) next)
		{
			DBG((" - combine %ld :: %ld + %ld :: %ld = %ld :: %ld\n",
				this, this->size, next, next->size, this, this->size + next->size);)
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
	DBG(("sortins %ld\n", this);)

	have = list->cur;
	if (!have)
		have = list->first;
	if (!have)
	{
		DBG((" - c0\n");)
		insfirst(list, this);
	}
	else
	if (have->area < this->area)
	{
		DBG((" - c1\n");)
		while (have and have->area < this->area)
			have = have->next;
		if (!have)
			inslast(list, this);
		else
			insbefore(list, this, have);
	othw
		DBG((" - c2\n");)
		while (have and have->area > this->area)
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
	DBG((" - a1\n");)
	amalgam(list, this);
	if (this->prior)
	{
		DBG((" - a2\n");)
		amalgam(list, this->prior);
	}
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
#if MDEBUG
	DBG(("round %ld to %ld\n", size, y);)
#endif
		return y;
	}
	else
		return size;					/* already round */
}

/* The smaller the requested area, the less likely it is
   allocated & freed only once in a while. */
void *XA_alloc(XA_memory *base, size_t size, XA_key key, XA_key type)
{
	XA_block *blk = nil;
	bool large = false;
	long s = ((size+3)&-4) + unitprefix;

	if (!base)
		base = &XA_heap_base;

#if MDEBUG
	DBG(("malloc %ld, %d\n", size, key);)
#endif
	if (s > base->chunk - blockprefix)
	{
		long ns = XA_round(base, s + blockprefix);
		blk = new_block(base, ns, false);
		if (!blk)
			return nil;
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
	return nil;
}

void *XA_calloc(XA_memory *base, size_t items, size_t chunk, XA_key key, XA_key type)
{
	void *new = nil;
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
DBG((" - free_unit\n");)
	extract(&this->used, at);
	sortins(&this->free, at),
	combine(&this->free, at);

	if (this->used.first eq nil)
		free_block(base, this);
#if LARGE
	else if (this->size > base->chunk)		/* 'large' block ? */
	{
		/* shrink a large block if it now has a large free unit at the end of the block */

		at = this->free.last;		/* free list is sorted on address */
		if (at)
		{	
			if (    at->size > base->chunk - blockprefix	/* 'large' free unit ? */
			    and (long)at + at->size eq (long)this + this->size		/* at end of block ? */
			   )
			{
				long s = 0;
				
				/* self repair the multiple chunksize prerogative */
				while ((long)this + s < (long)at + unitprefix)
					s += base->chunk;
				s = this->size - s;
				this->size -= s;
				at->size -= s;
#if MDEBUG
				{
					int r = Mshrink(0, this, this->size);
					DBG(("%d shrunk s%ld this%ld at%ld\n", r, s, this->size, at->size);)
				}
#else
				Mshrink(0, this, this->size);
#endif
			}
		}
	}
#endif
}

void XA_free(XA_memory *base, void *area)
{
	XA_block *blk;
	if (!area)
		return;
	if (!base)
		base = &XA_heap_base;

	blk = base->cur;
	if (blk)
	{
		if (blk->used.cur)
			if (blk->used.cur->area eq area)
			{
				DBG(("blk free cur %ld %ld:: %ld\n",blk, blk->used.cur, blk->used.cur->size);)
				free_unit(base, blk, blk->used.cur);
				return;
			}
	}

	blk = base->first;
	DBG((" - bfirst %ld\n",blk);)
	while (blk)
	{
		long x = (long)area,
		     b = (long)blk->area + unitprefix;
		/* Is the area in this block ? */
		if (   x >= b
		    && x < b + blk->size - (blockprefix + unitprefix)
		    )
		{
			XA_unit *at;

			if (blk->used.cur)
				if (blk->used.cur->area eq area)
				{
					DBG(("free cur %ld %ld:: %ld\n",blk, blk->used.cur, blk->used.cur->size);)
					free_unit(base, blk, blk->used.cur);
					return;
				}

			at = blk->used.first;
			while (at)
			{
				if (at->area eq area)
				{
					free_unit(base, blk, at);
					return;
				}
				at = at->next;
			}
		}
		blk = blk->next;
	}
}

bool XA_match(XA_unit *at, XA_key key, XA_key type)
{
	bool m =(    key  eq -1
	         or (key  ne -1 and key  eq at->key)
	         )
	    and (    type eq -1
	         or (type ne -1 and type eq at->type)
	         )
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
	if (key eq -1 and type eq -1)
	{
		while(blk)
		{
			XA_block *next = blk->next;
			Mfree(blk);
			blk = next;
		}
		base->first = base->last = base->cur = nil;
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
#if MDEBUG
				DBG(("at->stack %d, base->stack %d\n", at->stack, base->stack);)
#endif
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

typedef struct record
{
	long size;
	char data[0];
} REC;


void XA_follow(XA_memory *base, XA_block *blk, XA_list *list, XA_report *report)
{
	/* Go up and down the list, finish at the same address. */
	XA_unit *un = list->first, *n = un, *p;
	while (un)
	{
		n = un;
		un = un->next;
	}
	if (n ne list->last)
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
	if (n ne list->first)
	{
		report(base, blk, n, "first found");
		report(base, blk, list->first, "list->first");
	}
}

void XA_sanity(XA_memory *base, XA_report *report)
{
	XA_block *blk;
#if MDEBUG
	printf("\nSanity check\n\n");
#endif
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
#if MDEBUG
		printf("Follow free list\n");
#endif
		XA_follow(base, blk, &blk->free, report);
#if MDEBUG
		printf("Follow used list\n");
#endif
		XA_follow(base, blk, &blk->used, report);
		blk = blk->next;
	}
}

void XA_set_base(XA_memory *base, size_t chunk, short round, short flags)
{
	if (!base)
		base = &XA_heap_base;

	base->first = base->last = base->cur = nil;

/* 0: default, negative: dont change */
	if (!chunk)
		base->chunk = XA_heap_base.chunk;
	elif (chunk > 0)
		base->chunk = chunk;

	if (!round)
		base->round = XA_heap_base.round;
	elif (round > 0)
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
	return XA_alloc(nil, size, 0, 0);
}

void *calloc(size_t items, size_t chunk)
{
	return XA_calloc(nil, items, chunk, 0, 0);
}

void free(void *addr)
{
	XA_free(nil, addr);
}
#endif

void _FreeAll(void)
{
#if ! MDEBUG						/* problems with printf at this stage */
	XA_free_all(nil, -1, -1);
#endif
}
