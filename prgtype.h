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

typedef struct prgtype
{
 /* ---vvv--- compatible with FTYPE and LSTYPE structures */
	SNAME name;				/* filetype */
	struct prgtype *next;	/* pointer to next item */
 /* ---^^^--- compatible with FTYPE and LSTYPE structures */
	ApplType appl_type;		/* type of program */
	boolean argv;			/* uses ARGV */
	boolean path;			/* program directory is default */
	boolean single;			/* run only in single mode (in Magic) */
	long limmem;			/* memory limit for this type in multitasking */
	int flags;				/* temporary; for load/save same as argv + path + single */
} PRGTYPE;

/* For consistency with other flags which are saved as bits... */

#define PT_ARGV 0x0001
#define PT_PDIR 0x0002
#define PT_SING 0x0004
#define AT_EDIT 0x0008
#define AT_AUTO 0x0010

extern CfgEntry prg_table[];
extern PRGTYPE pwork;

CfgNest prg_config;

void prg_setprefs(void);

void prg_info(PRGTYPE **list, const char *prgname, int dummy, PRGTYPE *pt );
boolean prgtype_dialog( PRGTYPE **list, int pos, PRGTYPE *pt, int use );
void copy_prgtype ( PRGTYPE *t, PRGTYPE *s );

void prg_init(void);
void prg_default(void);

#if !TEXT_CFG_IN
int prg_load(XFILE *file);
#endif

boolean prg_isprogram(const char *name);


