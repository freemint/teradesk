#ifndef PRELUDE_H
#define PRELUDE_H

/* (c) 1991-1998 by H. Robbers te Amsterdam */

/* HR: the word global should really be kept reserved for (future?)
       use as the opposite of auto, local or static. */

#undef TRUE
#undef FALSE
#undef true
#undef false
#undef NULL
#undef null
#undef NIL
#undef nil

#if ! __AC__					/* FOR_A 0; Not for 'A' compiler. */
	#define   and &&
	#define   or  ||
	#define   not !
	#define   eq  ==
	#define   ne  !=
	#define   ge  >=
	#define   le  <=
	#define   mod %
	#define global
	#define esac break;
	#define elif else if
	#define othw }else{
	#define od while(1);		/* ; no mistake */
	typedef double extended;
	typedef float single;

	#if ! __AHCC__
	/* these are otherwise built in */
		#define nil 0L
		typedef double real;
		typedef struct { real re, im; } complex;
		typedef complex compl;
	#endif
#endif

#if ! __bool_true_false_are_defined
/* these are otherwise built in or provided by stdbool.h */
	typedef enum
	{
		false = 0,
		true
	} _Bool, bool;
	#define __bool_true_false_are_defined 1
#elif __AHCC__
	typedef _Bool bool;
#endif

#define NULL 0L
#define null 0L
#define nil 0L
#define NIL 0L
#define FALSE false
#define TRUE true

typedef char *string;
typedef unsigned char *ustring;
typedef unsigned char uchar;
typedef unsigned short uint, ushort;
typedef unsigned long ulong;

/* Pure C doesnt accept short for bit fields */
typedef int bits;				/* use these for bitfields */
typedef unsigned int ubits;

#define K *1024
#define fall_thru

#define EOS 0
#endif
