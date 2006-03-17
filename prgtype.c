/*
 * Teradesk. Copyright (c)       1993, 1994, 2002  W. Klaren.
 *                                     2002, 2003  H. Robbers,
 *                         2003, 2004, 2005, 2006  Dj. Vukovic
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
#include <library.h>
#include <xdialog.h>
#include <mint.h>
#include <limits.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "lists.h" 
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"
#include "prgtype.h"
#include "filetype.h" 
#include "font.h"
#include "window.h"
#include "icon.h"
#include "icontype.h"



PRGTYPE
	pwork,				/* work area for editing prgtypes */ 
	*prgtypes; 			/* List of executable file types */

void dir_refresh_all(void);


/*
 * Actually copy data for one program type to another location 
 * (note: must not just copy *t = *s because t->next should be preserved)
 */

void copy_prgtype( PRGTYPE *t, PRGTYPE *s )
{
	PRGTYPE *next = t->next; 
	*t = *s;
	t->next = next;	
}


/*
 * Find (or create!) information about an executable file or filetype;
 * input: filename of an executable file or a filename mask;
 * output: program type data
 * if filetype has not been defined, default values are set.
 * Also, if there is no list, set default values.
 * Note: a full path can be given as an argument, but only
 * the name proper is considered.
 */

void prg_info
( 
	PRGTYPE **list, 		/* list of defined program types */
	const char *prgname,	/* name or filetype to search for */ 
	int dummy,				/* not used, for compatibility */
	PRGTYPE *pt				/* output information */ 
)
{
	if ( (list == NULL) || !find_wild( (LSTYPE **)list, (char *)prgname, (LSTYPE *)pt, copy_prgtype ) )
	{
		/* If program type not defined or name not given: default */

		pt->appl_type = PGEM;	/* GEM program */
		pt->flags = PD_PDIR;	/* Default directory is program directory */
		pt->limmem = 0L;		/* No memory limit in multitasking */
	}
	return;
}


/*
 * Check if filename is to be considered as that of a program;
 * return true if name matches one of the masks defined
 * for executable files (programs)
 * Note: use of find_lsitem() here would bring a minor saving in
 * size 916 bytes) but would slow the program down.
 */

boolean prg_isprogram(const char *fname)
{
	PRGTYPE *p = prgtypes;
	char *name = fn_get_name(fname);

	while (p)
	{
		if (cmp_wildcard(name, p->name))
			return TRUE;
		p = p->next;
	}

	return FALSE;
}


/*
 * Add one program filetype to end of list, explicitely specifying 
 * each parameter; If mint is active, maybe set name to lowercase?
 */

static PRGTYPE *ptadd_one
(
	char *filetype,		/* pointer to filetype mask */
	int type, 			/* program type */
	int flags,			/* program usage flags */
	long limmem			/* memory limit for this program type */
)
{
	strsncpy ( (char *)pwork.name, filetype, sizeof(SNAME) );

/* Better let the user decide whether to do this

#if _MINT_
	if ( mint && !magx )
		strlwr(pwork.name);
#endif

*/

	pwork.appl_type = type;
	pwork.flags = flags;
	pwork.limmem = limmem;

	return (PRGTYPE *)lsadd_end( (LSTYPE **)(&prgtypes), sizeof(PRGTYPE), (LSTYPE *)(&pwork), copy_prgtype );
}


/* 
 * Remove all defined program types 
 */

static void rem_all_prgtypes(void)
{
	lsrem_all_one( (LSTYPE **)(&prgtypes) );
}


/*
 * Handling of the dialog for setting characteristics 
 * of one executable (program) filetype; return data in *pt;
 * if operation is cancelled, entry values in *pt should be unchanged.
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
		stat = FALSE,	/* accept or not */
		stop = FALSE;	/* loop until true */

	int 
		lbl,				/* text "filetype" of "application" */
		title = DTADDPRG,	/* resource index of title to be used for dialog */
		button;				/* code of pressed button */


	/* Determine which title to put on dialog, depending on use */

	lbl = TFTYPE;

	if ((use & LS_EDIT) != 0)
	{
		if ( use & LS_APPL )
		{
			addprgtype[PRGNAME].ob_flags &= ~EDITABLE;
			title = DTSETAPT;
			lbl = TAPP;
		}
		else
			title = DTEDTPRG;
	}

 	rsc_title(addprgtype, APTITLE, title);
	rsc_title(addprgtype, PTTEXT, lbl);

	/* Copy all data to dialog */

	cv_fntoform(addprgtype, PRGNAME, pt->name);
	xd_set_rbutton(addprgtype, APTPAR2, APGEM + (int)(pt->appl_type) );
	xd_set_rbutton
	(
		addprgtype,
		APTPAR1,
		(pt->flags & (PD_PDIR | PD_PPAR)) / PD_PDIR + ATWINDOW
	);

	set_opt(addprgtype, pt->flags, PT_ARGV, ATARGV);
	set_opt(addprgtype, pt->flags, PT_BACK, ATBACKG);

#if _MINT_
	/* 
	 * these settings have no effect in single-tos but are left editable
	 * so that the same config file can be edited in mutltitasking/single
	 */

	set_opt(addprgtype, pt->flags, PT_SING, ATSINGLE);
	ltoa(pt->limmem / 1024L, addprgtype[MEMLIM].ob_spec.tedinfo->te_ptext, 10);

#else
	obj_hide(addprgtype[MEMLIM]);
	obj_hide(addprgtype[ATSINGLE]);
#endif
	
	/* Open the dialog, then loop until exit button */

	if(chk_xd_open( addprgtype, &info ) >= 0)
	{
		while ( !stop )
		{
			button = xd_form_do( &info, ROOT );

			/* If ok, and there is a filetype, and is not a duplicate entry */

			if ( (button == APTOK) )
			{	
				SNAME thename;

				cv_formtofn(thename, addprgtype, PRGNAME);
 			
				if ( strlen(thename) != 0 )
				{
					if (check_dup((LSTYPE **)list, thename, pos) ) 
					{
						/* Get all data back from the dialog */

						pt->appl_type = (ApplType)(xd_get_rbutton(addprgtype, APTPAR2) - APGEM);
						pt->flags = (xd_get_rbutton(addprgtype, APTPAR1) - ATWINDOW) * PD_PDIR;

						get_opt(addprgtype, &pt->flags, PT_BACK, ATBACKG);
						get_opt(addprgtype, &pt->flags, PT_ARGV, ATARGV);
						get_opt(addprgtype, &pt->flags, PT_SING, ATSINGLE);

						strcpy( pt->name, thename);
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

			xd_drawbuttnorm(&info, button);
		}
		xd_close(&info);
	}

	addprgtype[PRGNAME].ob_flags |= EDITABLE;
	return stat;
}


/* 
 * Use these listtype-specific functions to manipulate program types list:
 */

#pragma warn -sus
static LS_FUNC ptlist_func =
{
	copy_prgtype,
	lsrem,
	prg_info,
	find_lsitem,
	prgtype_dialog
};
#pragma warn .sys


/*
 * Handling of the dialog for setting program (executable) file options;
 */

void prg_setprefs(void)
{
	int 
		button;			/* code of pressed button */


	set_opt(setmask, options.xprefs, TOS_KEY, PKEY);
	set_opt(setmask, options.xprefs, TOS_STDERR, PSTDERR);
	rsc_title(setmask, DTSMASK, DTPTYPES);	
	obj_unhide(setmask[PGATT]);

	/*  Edit programtypes list: add/delete/change */

	button = list_edit( &ptlist_func, (LSTYPE **)(&prgtypes), 1, sizeof(PRGTYPE), (LSTYPE *)(&pwork), LS_PRGT); 

	obj_hide(setmask[PGATT]);

	if ( button == FTOK )
	{
		get_opt(setmask, &options.xprefs, TOS_KEY, PKEY);
		get_opt(setmask, &options.xprefs, TOS_STDERR, PSTDERR);
		icn_fix_ictype(); 	/* modify types of desktop icons */
		icnt_fix_ictypes();	/* move program icons to appropriate group */
		dir_refresh_all();	/* modify window icons */
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


/* 
 * Set default predefined program file types.
 * In Mint or Magic, accessory is also a program type. 
 */

void prg_default(void)
{
	static const ApplType pt[] = {PGEM, PGEM, PGTP, PTOS, PTTP, PACC};
	static const int dd[] = {PD_PDIR|PT_ARGV,PD_PDIR|PT_ARGV,PD_PDIR|PT_ARGV,PD_PDIR,PD_PDIR,PD_PDIR};
	int i;

	rem_all_prgtypes();

#if _MINT_
	for(i = 0; i < 6; i++)
#else
	for(i = 0; i < 5; i++)
#endif
		ptadd_one((char *)presets[i + 2], pt[i], dd[i], 0L);
}


/*
 * Configuration table for one program type
 */

CfgEntry prg_table[] =
{
	{CFG_HDR, NULL }, /* keyword will be substituted */
	{CFG_BEG},
	{CFG_S,   "name",  pwork.name	},
	{CFG_L,   "limm",  &pwork.limmem	},
	{CFG_D,   "appt",  &pwork.appl_type	},
	{CFG_X,   "flag",  &pwork.flags },
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
			pwork.appl_type = (int)p->appl_type;
			pwork.limmem = p->limmem;
			pwork.flags = p->flags;

			*error = CfgSave(file, prg_table, lvl, CFGEMP); 
	
			p = p->next;
		}
	}
	else
	{
		memclr(&pwork, sizeof(pwork));

		*error = CfgLoad(file, prg_table, (int)sizeof(SNAME), lvl); 

		if (*error == 0 )
		{
			if ( pwork.appl_type > PTTP || pwork.name[0] == 0 )
				*error = EFRVAL;		
			else
			{
				/* Add a program type into the list */

				if
				( 
					lsadd_end
					(  
						(LSTYPE **)&prgtypes, 
		            	sizeof(pwork), 
		            	(LSTYPE *)&pwork, 
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
	{CFG_HDR,  "apptypes" },
	{CFG_BEG},
	{CFG_NEST, "ptype", one_ptype  },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Configure all programtypes 
 */

CfgNest prg_config
{
	prg_table[0].s = "ptype";
	prg_table[2].type &= CFG_MASK;

	*error = handle_cfg(file, prgty_table, lvl, CFGEMP, io, rem_all_prgtypes, prg_default);
} 

