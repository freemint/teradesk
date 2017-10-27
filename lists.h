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


/* 
 * Common part of FTYPE, PRGTYPE, ICONTYPE and APPLINFO structures 
 * is defined here, so that same routines can operate on all
 */

typedef struct lstype
{
	SNAME filetype;	
	struct lstype *next;
} LSTYPE;

extern LSTYPE *selitem;

/* 
 * Pointers to functions for manipulating 
 * filetype, icontype, programtype and applications lists 
 */

typedef struct ls_func
{
	void (*lscopy)(LSTYPE *t, LSTYPE *s);
	void (*lsrem)(LSTYPE **list, LSTYPE *item);
	void (*lsitem)(LSTYPE **list, char *name, int use, LSTYPE *lwork);
	LSTYPE *(*lsfinditem)(LSTYPE **list, char *name, int *ord);
	bool (*ls_dialog)(LSTYPE **list, int pos, LSTYPE *item, int use);
}LS_FUNC;

bool find_wild ( LSTYPE **list, char *name, LSTYPE *work, void *func );
int find_selected(void);
LSTYPE *get_item(LSTYPE **list, int item);
LSTYPE *find_lsitem(LSTYPE **list, char *name, int *pos);
void lsrem(LSTYPE **list, LSTYPE *item);
void lsrem_all(LSTYPE **list, void *rem_func);
void lsrem_all_one(LSTYPE **list);
void lsrem_three(LSTYPE **clist, void *remfunc);
LSTYPE *lsadd( LSTYPE **list, size_t size, LSTYPE *pt, int pos, void *copy_func );
LSTYPE *lsadd_end( LSTYPE **list, size_t size, LSTYPE *pt, void *copy_func );
bool copy_all(LSTYPE **copy, LSTYPE **list, size_t size, void *copy_func ); 
int cnt_types(LSTYPE **list );
bool check_dup( LSTYPE **list, char *name, int pos );
int list_edit(LS_FUNC *lsfunc,	LSTYPE **lists, int nl, size_t size, LSTYPE *lwork, int use );

#define NLINES 4	/* Number of lines in a filetype-selector dialog */

#define LS_EDIT 1		/* edit existing item in a list */
#define LS_ADD  2		/* add new item to a list */
#define LS_WSEL 4		/* there has been a selection from the window */

#define LS_FMSK 16		/* use to set filemasks */
#define LS_DOCT 32		/* use to set app doctypes */
#define LS_PRGT 64		/* use to set program types */
#define LS_ICNT 128		/* use to set window icon types */
#define LS_APPL 256		/* use to set applications */
#define LS_FIIC 512		/* work on list of icons assigned to files  */
#define LS_FOIC 1024	/* work on list of icons assigned to folders */
#define LS_PRIC 2048	/* work on list of icons assigned to programs */

#define LS_SELA 8192	/* Select a one-time-use application */

