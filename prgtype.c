/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren.
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

#include <np_aes.h>	
#include <stdlib.h>
#include <string.h>
#include <vdi.h> 
#include <xdialog.h>
#include <mint.h>

#include "desk.h"
#include "error.h"
#include "resource.h"
#include "lists.h" 
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"
#include "prgtype.h"
#include "filetype.h" 
#include "font.h"
#include "window.h"

#define END			32767


extern Options options;	/* need to know cfg file version */
extern char *presets[];

PRGTYPE
	pwork,				/* work area for editing prgtypes */ 
	*prgtypes; 			/* List of executable file types */


/*
 * Actually copy data for one program type to another location 
 * (note: must not just copy *t = *s because t->next should be preserved)
 */

void copy_prgtype( PRGTYPE *t, PRGTYPE *s )
{
	strsncpy ( t->name, s->name, sizeof(SNAME) );
	t->appl_type = s->appl_type;
	t->limmem = s->limmem; 
	t->flags = s->flags;	/* redundant, for compatibility */
}

/*
 * Find (or create!) information about an executable file or filetype;
 * input: filename of an executable file or a filename mask;
 * output: program type data
 * if filetype has not been defined, default values are set.
 * Because of many changes, old code was not commented out 
 * but completely removed
 */

void prg_info
( 
	PRGTYPE **list, 		/* list of defined program types */
	const char *prgname,	/* name or filetype to search for */ 
	int dummy,				/* not used, for compatibility */
	PRGTYPE *pt				/* output information */ 
)
{
	if ( !find_wild( (LSTYPE **)list, (char *)prgname, (LSTYPE *)pt, copy_prgtype ) )
	{
		/* If program type not defined or name not given: default */

		pt->appl_type = PGEM;	/* GEM program */
		pt->flags &= ~(PT_ARGV | PT_SING);
		pt->flags |= PT_PDIR;

		pt->limmem = 0L;		/* No memory limit in multitasking */
	}
	return;
}

/*
 * Check if filename is to be considered as that of a program;
 * return true if executable (program);
 */

boolean prg_isprogram(const char *name)
{
	PRGTYPE *p;

	p = prgtypes;

	while (p != NULL)
	{
		if (cmp_wildcard(name, p->name) == TRUE)
			return TRUE;
		p = p->next;
	}

	return FALSE;
}


/*
 * Add one program filetype to end of list, explicitely specifying 
 * each parameter; If mint is active, set name to lowercase
 */

static PRGTYPE *ptadd_one
(
	char *filetype,		/* pointer to filetype mask */
	int type, 			/* program type */
	int flags,
	long limmem			/* memory limit for this program type */
)
{
	strsncpy ( (char *)pwork.name, filetype, sizeof(pwork.name) );

#if _MINT_
	if ( mint && !magx )
		strlwr(pwork.name);
#endif

	pwork.appl_type = type;
	pwork.flags = flags;
	pwork.limmem = limmem;

	return (PRGTYPE *)lsadd( (LSTYPE **)(&prgtypes), sizeof(PRGTYPE), (LSTYPE *)(&pwork), END, copy_prgtype );
}

/* Remove all defined program types */

static void rem_all_prgtypes(void)
{
	rem_all( (LSTYPE **)(&prgtypes), rem );
}


/*
 * Handling of the dialog for setting characteristics 
 * of one executable (program) filetype; return data in *pt;
 * if operation is cancelled, entry values in *pt should be unchanged.
 * Note: because of many rather drastic changes  
 * old code was not commented out but completely removed
 */

boolean prgtype_dialog
( 
	PRGTYPE **list, 	/* list to check duplicate entries in */
	int pos, 			/* position in the list where to enter data */
	PRGTYPE *pt,		/* data to be edited */ 
	int use				/* use of dialog (add or edit program type or app type) */
)
{
	XDINFO
		info;			/* dialog info structure */

	boolean
		stat,			/* accept or not */
		stop = FALSE;	/* loop until true */

	int 
		title,			/* resoruce index of title to be used for dialog */
		button;			/* code of pressed button */

	/* Determine which title to put on dialog, depending on use */

	if ( !(use & LS_EDIT) )
	{
		title = DTADDPRG;
	}
	else
		title = (use & LS_APPL) ? DTSETAPT : DTEDTPRG;

 	rsc_title(addprgtype, APTITLE, title);

	/* Copy all data to dialog */

	cv_fntoform(addprgtype + PRGNAME, pt->name);
	xd_set_rbutton(addprgtype, APTPAR2, APGEM + (int)(pt->appl_type) );

	xd_set_rbutton(addprgtype, APTPAR1, (pt->flags & PT_PDIR) ? ATPRG : ATWINDOW);

	set_opt(addprgtype, pt->flags, PT_ARGV, ATARGV);

#if _MINT_
	/* 
	 * these settings have no effect in single-tos but are left editable
	 * so that the same config file can be edited in mutltitasking/single
	 */

	set_opt(addprgtype, pt->flags, PT_SING, ATSINGLE);
	ltoa(pt->limmem / 1024L, addprgtype[MEMLIM].ob_spec.tedinfo->te_ptext, 10);

#else
		addprgtype[MEMLIM].ob_state |= HIDETREE;
		addprgtype[ATSINGLE].ob_state |= HIDETREE;
#endif
	
	/* Open the dialog, then loop until exit button */

	xd_open( addprgtype, &info );

	while ( !stop )
	{
		button = xd_form_do( &info, ROOT );

		/* If ok, and there is a filetype, and is not a duplicate entry */

		if ( (button == APTOK) )
		{	
			SNAME thename;

			cv_formtofn(thename, &addprgtype[PRGNAME]);
 
			if ( strlen(thename) != 0 )
			{
				if (check_dup((LSTYPE **)list, thename, pos) ) 
				{
					/* Get all data back from the dialog */

					strcpy( pt->name, thename);
					pt->appl_type = (ApplType)(xd_get_rbutton(addprgtype, APTPAR2) - APGEM);
					get_opt(addprgtype, &pt->flags, PT_ARGV, ATARGV);
					get_opt(addprgtype, &pt->flags, PT_PDIR, ATPRG);
					get_opt(addprgtype, &pt->flags, PT_SING, ATSINGLE);
				
#if _MINT_
					pt->limmem = 1024L * atol(addprgtype[MEMLIM].ob_spec.tedinfo->te_ptext);
#endif
					stat = TRUE;
					stop = TRUE;
				}
			}
		}
		else
			stop = TRUE;

		xd_change(&info, button, NORMAL, TRUE);
	}
	xd_close(&info);
	return stat;
}


/* Use these kind-specific functions to manipulate program types list: */

#pragma warn -sus
static LS_FUNC ptlist_func =
{
	copy_prgtype,
	rem,
	prg_info,
	find_lsitem,
	prgtype_dialog
};
#pragma warn .sys


/*
 * Handling of the dialog for setting program (executable) file options;
 * Note: because of many rather drastic changes  
 * old code was not commented out but completely removed
 */

void prg_setprefs(void)
{
	int 
		button,			/* code of pressed button */
		prefs;			/* temporary program preferences */

			
	prefs = options.cprefs & 0xFDDF;

	set_opt(setmask, options.cprefs, TOS_KEY, PKEY);
	set_opt(setmask, options.cprefs, TOS_STDERR, PSTDERR);

	rsc_title(setmask, DTSMASK, DTPTYPES);	
	setmask[PGATT].ob_flags &= ~HIDETREE;
	rsc_title(addprgtype, PTTEXT, TFTYPE);

	/*  Edit programtypes list: add/delete/change */

	button = list_edit( &ptlist_func, (LSTYPE **)(&prgtypes), NULL, sizeof(PRGTYPE), (LSTYPE *)(&pwork), LS_PRGT); 

	setmask[PGATT].ob_flags |= HIDETREE;

	if ( button == FTOK )
	{
		prefs |= ((setmask[PKEY].ob_state & SELECTED) != 0) ? TOS_KEY : 0;
		prefs |= ((setmask[PSTDERR].ob_state & SELECTED) != 0) ? TOS_STDERR : 0;

		options.cprefs = prefs;
		wd_seticons();
	}
}

/*
 * Initiaite (empty) list of (executable) program filetypes
 */

void prg_init(void)
{
	prgtypes = NULL;
#ifdef MEMDEBUG
	atexit(rem_all_prgtypes);
#endif
}


/* Set some default predefined program file types */

void prg_default(void)
{
	rem_all_prgtypes();

	/*
	 * removed separate definition of types for mint/single
	 * if mint is active, there is conversion to lowercase in ptadd_one
	 * If mint is active, accessory is defined as program type 
	 */
	ptadd_one(presets[2], PGEM, PT_PDIR | PT_ARGV, 0L);		/*	*.PRG 	*/
	ptadd_one(presets[3], PGEM, PT_PDIR | PT_ARGV, 0L);		/*	*.APP	*/
	ptadd_one(presets[4], PGTP, PT_PDIR | PT_ARGV, 0L);		/*	*.GTP	*/
	ptadd_one(presets[5], PTOS, PT_PDIR, 0L);				/*	*.TOS	*/
	ptadd_one(presets[6], PTTP, PT_PDIR, 0L);				/*	*.TTP	*/

#if _MINT_
	if ( mint )
		ptadd_one(presets[7], PACC, PT_PDIR, 0L);			/*	*.ACC	*/
#endif
}


#define PRG_PATH		8		/* 0 = window, 1 = program */
#define TOS_PATH		16		/* 0 = window, 1 = program */

/*
 * Configuration table for one program type
 */

CfgEntry prg_table[] =
{
	{CFG_HDR, 0, "*type" },
	{CFG_BEG},
	{CFG_S,   0, "name",  pwork.name	},
	{CFG_L,   0, "limm",  &pwork.limmem	},
	{CFG_D,   0, "appt",  &pwork.appl_type	},
	{CFG_X,   0, "flag",  &pwork.flags },
	{CFG_END},
	{CFG_LAST}
};


/*
 * Save or load configuration for program type(s)
 */

static CfgNest one_ptype
{
	*error = 0;

	if (io == CFG_SAVE)
	{
		PRGTYPE *p = prgtypes;

		while ( (*error == 0) && p)
		{
			strcpy(pwork.name, p->name);
			pwork.appl_type = (int) p->appl_type;
			pwork.limmem = p->limmem;
			pwork.flags = p->flags;
	
			*error = CfgSave(file, prg_table, lvl + 1, CFGEMP); 
	
			p = p->next;
		}
	}
	else
	{
		memset(&pwork, 0, sizeof(pwork));

		*error = CfgLoad(file, prg_table, (int)sizeof(SNAME) - 1, lvl + 1); 

		if (*error == 0 )
		{

			if ( pwork.appl_type > PTTP || pwork.name[0] == 0 )
				*error = EFRVAL;		
			else
			{
				if ( 
					lsadd(  (LSTYPE **)&prgtypes, 
		            		sizeof(pwork), 
		            		(LSTYPE *)&pwork, 
		            		END, 
		            		copy_prgtype
				  		) == NULL
					)
						*error = ENOMSG;
			}
		}
	}
}


/*
 * Configuration table for program (executable file) types
 */
 
static CfgEntry prgty_table[] =
{
	{CFG_HDR, 0, "apptypes" },
	{CFG_BEG},
	{CFG_NEST,0, "ptype", one_ptype  },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Configure all programtypes 
 */

CfgNest prg_config
{
	prg_table[0].s[0] = 'p';
	prg_table[2].flag = 0;

	*error = handle_cfg(file, prgty_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, rem_all_prgtypes, prg_default);
}

