/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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
	long limmem;			/* memory limit for this type in multitasking */
	int flags;				/* argv + path + single */
} PRGTYPE;

/* For consistency with other flags which are saved as bits... */

#define PT_ARGV 0x0001 	/* understands ARGV */
#define PT_PDIR 0x0002	/* program directory is the default; OBSOLETE */ 
#define PT_SING 0x0004	/* do not multitask */
#define AT_EDIT 0x0008	/* set as editor */
#define AT_AUTO 0x0010	/* set as startup/autostart */
#define AT_SHUT 0x0020	/* set as shutdown */
#define AT_VIDE 0x0040	/* set for video mode change */
#define AT_SRCH 0x0080	/* set as file search app */
#define AT_FFMT 0x0100	/* set as floppy format application */
#define AT_VIEW 0x0200	/* set as viewer */
#define AT_COMP 0x0400	/* set as file compare app */
#define AT_RBXT 0x0800	/* right button extension */
#define AT_CONS 0x1000	/* console (placeholder) */
#define PD_PDIR 0x2000	/* Program directory is the default */
#define PD_PPAR 0x4000	/* directory of the first parameter is the default */
#define PT_BACK 0x8000	/* background */

extern CfgEntry prg_table[];
extern PRGTYPE pwork;

CfgNest prg_config;
void prg_setprefs(void);
void prg_info(PRGTYPE **list, const char *prgname, int dummy, PRGTYPE *pt );
bool prgtype_dialog( PRGTYPE **list, int pos, PRGTYPE *pt, int use );
void copy_prgtype ( PRGTYPE *t, PRGTYPE *s );
void prg_init(void);
void prg_default(void);
bool prg_isprogram(const char *name);
void sim_click(void);


