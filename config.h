/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                                     2003  Dj. Vukovic
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
	CFG_HIGHEST			/* (not used)   */
} CFG_TYPE;

typedef enum
{	
	CFG_INHIB = 0x01,	/* accept input; dont write (compatability flag) */
	CFG_NOFMT = 0x02,	/* ignore default formatting, set explicitely */
} CFG_FLAG;

typedef struct
{
	char type;			/* CFG_TYPE */
	char flag;			/* CFG_FLAG */
	char *s;			/* format   */
	void *a;			/* data     */
} CfgEntry;


/* Mnemonic for easier reading of code: value for parameter "io" of CfgNest */

#define CFG_LOAD 0
#define CFG_SAVE 1

/* 
 * Maximum acceptable length of a line in configuration file defined below.
 * Should be longer than maximum path length by the length of
 * a keyword and a reasonable number of tabs. Currently PATH_MAX is
 * defined as 128. Probably about 20 characters more should be enough: 
 */
 
#define MAX_CFGLINE PATH_MAX+20

/*
 * Maximum possible keyword length is defined below
 */

#define MAX_KEYLEN 20


typedef void CfgNest(XFILE *file, char *key, int lvl, int io, int *error);

int	CfgLoad(XFILE *f, CfgEntry *tab, int maxs, int lvl);
int CfgSave(XFILE *f, CfgEntry *tab, int lvl, boolean emp);
int handle_cfg(XFILE *f, CfgEntry *tab, int maxs, int lvl, boolean emp, int io, void *ini, void *def);
int handle_cfgfile( char *name,	CfgEntry *tab, char *ident,	int io);
char *nonwhite ( char *s);