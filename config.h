/*
 * Teradesk. Copyright (c)         1993, 1994, 2002  W. Klaren,
 *                                       2002, 2003  H. Robbers,
 *                           2003, 2004, 2005, 2006  Dj. Vukovic
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
	CFG_BD,				/* %d on int16, 0 & 1 only      */
	CFG_D,				/* %d on int16, nonnegative only      */
	CFG_DDD,			/* 3 x %d on int 16, nonnegative only */
	CFG_X,				/* %x on int16  */
	CFG_L,				/* %ld on int32, nonnegative only     */
	CFG_S,				/* %s           */
	CFG_NEST,			/* group        */
	CFG_INHIB = 0x20,	/* inhibit output, modifier for the above */
	CFG_NOFMT = 0x40,	/* ignore default format; modifier */
} CFG_TYPE;

#define CFG_MASK 0x1F;	/* to extract 31 entry type without modifiers */

typedef struct
{
	char type;			/* CFG_TYPE */
	char *s;			/* format   */
	void *a;			/* data     */
} CfgEntry;


/* Mnemonic for easier reading of code: value for parameter "io" of CfgNest */

#define CFG_LOAD 0
#define CFG_SAVE 1

/* 
 * Maximum acceptable length of a line in configuration file defined below.
 * Should be longer than maximum path length by the length of
 * a keyword and a reasonable number of tabs. Currently max. path is
 * set to 254. Probably about 20 characters more should be enough: 
 */
 
#define MAX_CFGLINE 275

/*
 * Maximum possible keyword length is defined below
 */

#define MAX_KEYLEN 20
#define CFGSKIP 0x0002

extern int chklevel;


typedef void CfgNest(XFILE *file, int lvl, int io, int *error);

int	CfgLoad(XFILE *f, CfgEntry *tab, int maxs, int lvl);
int CfgSave(XFILE *f, CfgEntry *tab, int lvl, boolean emp);
int handle_cfg(XFILE *f, CfgEntry *tab, int lvl0, int emp, int io, void *ini, void *def);
int handle_cfgfile( char *name,	CfgEntry *tab, const char *ident,	int io);
char *nonwhite ( char *s);