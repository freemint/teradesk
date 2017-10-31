/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2009  Dj. Vukovic
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


/* Note: at most 31 entry type can be defined here */

typedef enum
{
	CFG_LAST,			/* end of table */
	CFG_HDR,			/* header =     */
	CFG_BEG,			/* {            */
	CFG_END,			/* }            */
	CFG_ENDG,			/* } +newline   */
	CFG_FINAL,			/* file end     */
	CFG_B,				/* %c on int8   */
	CFG_H,				/* %d on int8, nonnegative only       */
	CFG_C,				/* %c on int16  */
	CFG_BD,				/* %d on bool, 0 & 1 only      */
	CFG_D,				/* %d on int16, nonnegative only      */
	CFG_DDD,			/* 3 x %d on int 16, nonnegative only */
	CFG_E,				/* %d on enum, nonnegative only      */
	CFG_X,				/* %x on int16  */
	CFG_L,				/* %ld on int32, nonnegative only     */
	CFG_S,				/* %s           */
	CFG_NEST,			/* group        */
	CFG_INHIB = 0x20,	/* inhibit output, modifier for the above */
} CFG_TYPE;

#define _VERIFY_CONCAT(x, y) _VERIFY_CONCAT0(x, y)
#define _VERIFY_CONCAT0(x, y) x##y

#if defined __COUNTER__ && __COUNTER__ != __COUNTER__
# define _VERIFY_COUNTER __COUNTER__
#else
# define _VERIFY_COUNTER __LINE__
#endif
#define _VERIFY_GENSYM(prefix) _VERIFY_CONCAT(prefix, _VERIFY_COUNTER)

#ifdef __GNUC__
#define _VERIFY(R) struct _VERIFY_GENSYM(_verify_type) { unsigned int _verify_error_if_negative: (R) ? 1 : -1; }
#define VERIFY_TYPE(s, e) + (int)(long)(__extension__(_VERIFY(sizeof(s) == sizeof(e)) *)0)
#else
#define _VERIFY(R)
#define VERIFY_TYPE(s, e)
#endif


#ifdef __GNUC__
#define __build_bug(e) (__extension__ sizeof(struct { int:-!!(e); }))
/* &a[0] degrades to a pointer: a different type from an array */
#define __must_be_array(a) + __build_bug(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#else
#define __build_bug(e)
#define __must_be_array(a)
#endif


#define CFG_HDR(s)     { CFG_HDR,   s,    { 0 } }
#define CFG_BEG()      { CFG_BEG,   NULL, { 0 } }
#define CFG_END()      { CFG_END,   NULL, { 0 } }
#define CFG_ENDG()     { CFG_ENDG,  NULL, { 0 } }
#define CFG_FINAL()    { CFG_FINAL, NULL, { 0 } }
#define CFG_B(s,v)     { CFG_B,     s,    { &v VERIFY_TYPE(v, char) } }
#define CFG_H(s,v)     { CFG_H,     s,    { &v VERIFY_TYPE(v, char) } }
#define CFG_C(s,v)     { CFG_C,     s,    { &v VERIFY_TYPE(v, unsigned char) } }
#define CFG_BD(s,v)    { CFG_BD,    s,    { &v VERIFY_TYPE(v, bool) } }
#define CFG_D(s,v)     { CFG_D,     s,    { &v VERIFY_TYPE(v, _WORD) } }
#define CFG_DI(s,v)    { CFG_D | CFG_INHIB,     s,    { &v VERIFY_TYPE(v, _WORD) } }
#define CFG_DDD(s,v)   { CFG_DDD,   s,    { &v VERIFY_TYPE(v, _WORD) } }
#define CFG_E(s,v)     { CFG_E,     s,    { &v VERIFY_TYPE(v, int) } }
#define CFG_X(s,v)     { CFG_X,     s,    { &v VERIFY_TYPE(v, _UWORD) } }
#define CFG_XI(s,v)    { CFG_X | CFG_INHIB,     s,    { &v VERIFY_TYPE(v, _UWORD) } }
#define CFG_L(s,v)     { CFG_L,     s,    { &v VERIFY_TYPE(v, long) } }
#define CFG_S(s,v)     { CFG_S,     s,    { v VERIFY_TYPE(&v, char *) __must_be_array(v) } }
#define CFG_SI(s,v)    { CFG_S | CFG_INHIB,     s,    { v VERIFY_TYPE(&v, char *) __must_be_array(v) } }
#define CFG_NEST(s,f)  { CFG_NEST,  s,    { f } }
#define CFG_LAST()     { CFG_LAST,  NULL, { 0 } }

#define CFG_MASK 0x1F	/* to extract 31 entry type without modifiers */

typedef struct
{
	char type;			/* CFG_TYPE */
	const char *s;		/* format   */
	union {
		void *p;		/* data     */
		char *s;
		_UWORD *u;
		unsigned char *c;
		_WORD *i;
		long *l;
		bool *b;
		int *e;
		void (*f)(XFILE *file, int lvl, int io, int *error);
	} a;
} CfgEntry;


/* Mnemonic for easier reading of code: value for parameter "io" of CFG_NEST */

#define CFG_LOAD 0
#define CFG_SAVE 1

/* 
 * Maximum acceptable length of a line in configuration file defined below.
 * Should be longer than maximum path length by the length of
 * a keyword and a reasonable number of tabs. Currently max. path is
 * set to 254. Probably about 20 characters more should be enough: 
 */
 
#define MAX_CFGLINE 274

/*
 * Maximum possible keyword length is defined below
 */

#define MAX_KEYLEN 20
#define CFGSKIP 0x0002

extern int chklevel;


int CfgLoad(XFILE *f, const CfgEntry *tab, int maxs, int lvl);
int CfgSave(XFILE *f, const CfgEntry *tab, int lvl, bool emp);
int handle_cfg(XFILE *f, const CfgEntry *tab, int lvl0, int emp, int io, void *ini, void *def);
int handle_cfgfile( const char *name, const CfgEntry *tab, const char *ident, int io);
