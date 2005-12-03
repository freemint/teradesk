/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                         2003, 2004, 2005  Dj. Vukovic
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
#include <library.h>
#include <limits.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "lists.h" /* must be before slider.h */
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"	
#include "prgtype.h"
#include "font.h"
#include "window.h"
#include "icon.h"
#include "icontype.h"
#include "filetype.h" 


typedef struct icontype
{
	SNAME type;				/* associated filetype */
	struct icontype *next;	/* pointer to next item in the list */
	int icon;				/* icon id. */
	INAME icon_name;		/* icon name */
} ICONTYPE;

typedef struct
{
	int icon;
	int resvd;
} ITYPE;

static ICONTYPE 
	iwork, 			/* work area for editing icon data */
	*iconlists[3];	/* pointers to lists of files, folders and programs */

static ITMTYPE
	defictype;

extern XDINFO 
	icd_info;

extern int 
	sl_noop;

/*
 * Note: Indexes below must agree with the order of radiobuttons
 * for selecting icon group in the dialog 
 */

#define FILE_LIST 	0	/* index for files icons list in iconlists[] */
#define FOLD_LIST	1	/* index for folders icons list in iconlists [] */
#define PROG_LIST	2	/* index for programs icons list in iconlists [] */ 

/*
 * Copy icon-file assignment data to another location 
 * (note: must not just copy *t = *s because t->next should be preserved)
 */

static void copy_icntype( ICONTYPE *t, ICONTYPE *s )
{
	ICONTYPE *next = t->next;
	*t = *s;
	t->next = next;
}


/* 
 * Find an icon defined by associated file(type) in an icontypes list;
 * return identifier of the icon.
 * Note: this function searches for an icon sequentially through a list.
 * On a low-end Atari, finding icons near the end of a large list can take
 * noticeable time. Routine cmp_wildcards() is rather slow.
 */

static int find_icon(const char *name, ICONTYPE *list)
{
	ICONTYPE *p = list;

	while (p)
	{
		if (cmp_wildcard(name, p->type))
			return min(n_icons - 1, p->icon);

		p = p->next;
	}
	return -1;
}


/*
 * Find (or create!) information about an icon type in a list;
 * input: filename or a filename mask;
 * output: icontype data
 * if filetype has not been defined, default values are set
 */

static void icnt_info
( 
	ICONTYPE **list, 	/* pointer to list in which the icon is searched for */
	char *filetype,		/* file(type) for which the associated icon is searched */ 
	int group,			/* is this files or folders or programs icon group */
	ICONTYPE *it 		/* output information */
)
{
	if ( !find_wild( (LSTYPE **)list, filetype, (LSTYPE *)it, copy_icntype ) )
	{
		/* Attempt o divine the icon from a possibly defined desktop icon */

		it->icon = icn_iconid(filetype);

		if(it->icon < 0)
		{
			/* No suitable icon on the desktop, set general icon */

			int 
				iconid = FIINAME, 
				lg = group & (LS_FIIC | LS_FOIC | LS_PRIC);

			if (lg == LS_FOIC)
				iconid = FOINAME;
			else if (lg == LS_PRIC)
				iconid = APINAME;

			it->icon = rsrc_icon_rscid( iconid, iname ); 
		}
	}
	
	return;
}


/*  
 * Find icons in the icons resource file by name, not index.
 * If icontype is not equal to icon target type, link is assumed.
 */

int icnt_geticon(const char *name, ITMTYPE type, ITMTYPE tgt_type)
{
	int icon, deficon, i;
	boolean more = (type != tgt_type && tgt_type != ITM_NOTUSED);
	ITMTYPE thetype = type;

	/* 
	 * Find a related icon, depending on item type (folder/file/program) 
	 * Links are handled in a somewhat dirty (but efficient) way:
	 * if icon for the specified name is not found, change item type
	 * to target item type and try again.
	 */

	again:; /* if item is a link, return here to check target */

	switch(thetype)
	{
		case ITM_PREVDIR:
		case ITM_FOLDER:
			i = 1;
			deficon = FOINAME;
			break;
		case ITM_PROGRAM:
			i = 2;
			deficon = APINAME;
			break;
		default:	/* ITM_FILE or ITM_LINK */
			i = 0;
			deficon = FIINAME;
	}

	if ((icon = find_icon(name, iconlists[i])) < 0)
	{
		/* Specific icon not found, but this is a link */

		if(more)
		{
			/* Go back and find default icon for this item type */

			thetype = tgt_type;
			more = FALSE;
			goto again;			
		}

		icon = rsrc_icon_rscid ( deficon, iname ); 
	}

	return icon;
}


/* 
 * Handle the dialog for assigning an icon to a file;
 * assignment of icon to a device (desktop icon) uses the same dialog 
 * in resource but a different handling routine.
 * list = list into which the item is placed;
 * pos = position (ordinal) in the list where to place the item
 * it = item to be placed in the list
 * use = purpose of use of dialog (add/edit)
 */

static boolean icntype_dialog( ICONTYPE **list, int pos, ICONTYPE *it, int use)
{
	int 
		button, 
		theic = it->icon, 
		title;

	SLIDER 
		sl_info;

	SNAME 
		thename;


	/* Which title to use? */

	if ( use & LS_EDIT )
		title = DTEDTICT;
	else
		title = DTADDICT;

	/* Files list or folders or programs list? */

	obj_hide(addicon[ICSHFIL]);
	obj_hide(addicon[ICSHFLD]);
	obj_hide(addicon[ICSHPRG]);

	switch( use & (LS_FIIC | LS_FOIC | LS_PRIC) )
	{
		case LS_FOIC:
			obj_unhide(addicon[ICSHFLD]); 
			break;
		case LS_PRIC:
			obj_unhide(addicon[ICSHPRG]); 
			break;
		default:	/* LS_FIIC */
			obj_unhide(addicon[ICSHFIL]);
	}

	/* Set dialog title and initial name */

	rsc_title(addicon, AITITLE, title);

	cv_fntoform(addicon, ICNTYPE, it->type);

	/* Set some fields as visible or invisible */

	obj_hide(addicon[CHNBUTT]);
	obj_hide(addicon[ICBTNS]);
	obj_hide(addicon[DRIVEID]);
	obj_hide(addicon[ICNLABEL]); 
	obj_unhide(addicon[ICNTYPE]);
	obj_unhide(addicon[ICNTYPT]);
	obj_unhide(addicon[ADDBUTT]);
	obj_hide(addicon[INAMBOX]);

	sl_noop = 0;
	button = icn_dialog(&sl_info, &theic, ICNTYPE, options.win_pattern, options.win_color);
	xd_close(&icd_info);

	cv_formtofn(thename, addicon, ICNTYPE);

	/* Button is OK but check for duplicate assignment, then set what needed */

	if((button == ADDICNOK) && (check_dup((LSTYPE **)list, thename, pos)))
	{
		strcpy(it->type, thename);
		it->icon = theic;

		strsncpy
		(
			it->icon_name,	
			icons[it->icon].ob_spec.ciconblk->monoblk.ib_ptext,
			sizeof(it->icon_name)
		);

		return TRUE;
	}
	else
		return FALSE;
}


/*
 * Use these iconlist-specific functions to manipulate icon lists: 
 */

#pragma warn -sus

static LS_FUNC itlist_func =
{
	copy_icntype,
	lsrem,
	icnt_info, 
	find_lsitem,
	icntype_dialog
};


#pragma warn .sus

/* 
 * Handle the dialog for maintaining lists of icons assigned to files 
 */

void icnt_settypes(void)
{
	int 
		button;


	/* Set dialog title, show radio buttons for selecting icon list */

	rsc_title(setmask, DTSMASK, DTITYPES);		
	obj_unhide(setmask[ICATT]);
	xd_set_rbutton(setmask, ICATT, ITFILES);

	/*  Edit icontypes list: add/delete/change */

	button = list_edit
	         ( 
				&itlist_func, 
				(LSTYPE **)iconlists, 
				3,
				sizeof(ICONTYPE), 
				(LSTYPE *)(&iwork), 
				LS_ICNT
	         ); 

	if ( button == FTOK ) 
		wd_seticons();

	/* Hide radio buttons for selecting icons list */

	obj_hide(setmask[ICATT]); 
}


/* 
 * Initiate (empty) lists of assigned files, folders and programs
 */

void icnt_init(void)
{
	iconlists[FILE_LIST] = NULL;
	iconlists[FOLD_LIST] = NULL;
	iconlists[PROG_LIST] = NULL;
}


/*
 * Clear all icontypes (files, folders and programs)
 */

void rem_all_icontypes(void)
{
	lsrem_three((LSTYPE **)iconlists, lsrem);
}


/* This routine is not currently used in TeraDesk

/*
 * Add an icon assignment into the list, explicitely giving parameters.
 */

static ICONTYPE *itadd_one(ICONTYPE **list, char *filetype, int icon)
{
	/* Define filemask */

	strsncpy ( (char *)iwork.type, filetype, sizeof(iwork.type) );

#if _MINT_
	if ( mint && !magic )
		strlwr(iwork.type);
#endif

	/* Index of that icon in (c)icons.rsc */

	iwork.icon = icon;

	/* Copy icon name (i.e. icon name in (c)icons.rsc) */

	strsncpy(iwork.icon_name, icons[icon].ob_spec.ciconblk->monoblk.ib_ptext, sizeof(INAME));	

	/* Add that to list */

	return (ICONTYPE *)lsadd( (LSTYPE **)list, sizeof(ICONTYPE), (LSTYPE *)(&iwork), INT_MAX, copy_icntype );
}

*/



#if !__USE_MACROS

/*
 * Set default icon assignment: no icontypes
 */

void icnt_default(void)
{
	rem_all_icontypes();
}

#endif


/*
 * Move an icontype assignment from one group list to another.
 * If assignment to the new list fails, old assignment is not removed.
 */

static void icnt_move(int to, int from, ICONTYPE *it)
{
	if
	(
		lsadd
		(
			(LSTYPE **)(&iconlists[to]),
			sizeof(ICONTYPE),
			(LSTYPE *)it,
			INT_MAX,
			copy_icntype
   		) != NULL
	) 
		lsrem((LSTYPE **)(&iconlists[from]), (LSTYPE *)it);
}


/*
 * Move program icons from/to files or programs group, depending on whether
 * their name mask is currently that of a program or a file
 */

void icnt_fix_ictypes(void)
{
	ICONTYPE *it, *next;

	it = iconlists[FILE_LIST];
	while(it)
	{
		next = it->next;
		if(prg_isprogram(it->type))
			icnt_move( PROG_LIST, FILE_LIST, it);
		it = next;
	}

	it = iconlists[PROG_LIST];
	while(it)
	{
		next = it->next;
		if(!prg_isprogram(it->type))
			icnt_move(FILE_LIST, PROG_LIST, it);
		it = next;
	}
}


static ICONTYPE 
	*pthis,		/* pointer to current member of files, folders or programs icon group */ 
	**ppthis;	/* pointer to address of current member of icon group */


/*
 * Table for configuring one window icon (icontype)
 */

static CfgEntry icnt_table[] =
{
	{CFG_HDR, 0, "itype" },
	{CFG_BEG},
	{CFG_S,   0, "mask", iwork.type		 },
	{CFG_S,   0, "name", iwork.icon_name },
	{CFG_END},
	{CFG_LAST}
};


/* 
 * Save or load definiton of one icontype
 */

static CfgNest one_itype 
{
	*error = 0;

	if (io == CFG_SAVE)
	{
		while ((*error == 0) && pthis)
		{
			iwork = *pthis;
			*error = CfgSave(file, icnt_table, lvl, CFGEMP);
			pthis = pthis->next;
		}
	}
	else
	{
		/* Clear work area, so that data not explicitelly set will be zero */

		memclr(&iwork, sizeof(iwork));

		/* Load configuration data for one icontype */

		*error = CfgLoad(file, icnt_table, (int)sizeof(SNAME) - 1, lvl); 

		/* Add to the list */

		if (*error == 0 )
		{
			if ( iwork.type[0] == 0 || iwork.icon_name[0] == 0)
				*error = EFRVAL;
			else
			{
				if ( (iwork.icon  = rsrc_icon(iwork.icon_name)) < 0 )
					iwork.icon = default_icon(defictype);	

				if 
				(
					lsadd
					(
						(LSTYPE **)ppthis,
						sizeof(ICONTYPE),
						(LSTYPE *)&iwork,
						INT_MAX,
						copy_icntype
			      	) == NULL
				)
					*error = ENOMSG; /* there was an alert in lsadd */
			}
		}
	}
}


/* 
 * Table for configuring one group of window icons (files/folders/programs)
 */

static CfgEntry icngrp_table[] =
{
	{CFG_HDR, 0, NULL }, /* keyword will be substituted */
	{CFG_BEG},
	{CFG_NEST,0, "itype", one_itype  },		/* Repeating group */
	{CFG_END},
	{CFG_LAST}
};


/* 
 * Save or load "files", "folders" or "programs" group of icon definitions 
 */

static CfgNest icngrp_cfg
{
	if ( io == CFG_LOAD )
		lsrem_all((LSTYPE **)(*ppthis), lsrem);

	*error = handle_cfg(file, icngrp_table, lvl, (CFGEMP | ((pthis) ? 0 : CFGSKIP)), io, NULL, NULL);
}


/* 
 * Save or load "files" group of window icontypes 
 */

static CfgNest file_cfg
{
	pthis = iconlists[0];
	ppthis = &iconlists[0];
	icngrp_table[0].s = "files";
	defictype = ITM_FILE;
	icngrp_cfg(file, lvl, io, error);
}


/* 
 * Save or load "folders" group of window icontypes 
 */

static CfgNest folder_cfg
{
	pthis = iconlists[1];
	ppthis = &iconlists[1];
	icngrp_table[0].s = "folders";
	defictype = ITM_FOLDER;
	icngrp_cfg(file, lvl, io, error);
}


/* 
 * Save or load "programs" group of window icontypes 
 */

static CfgNest program_cfg
{
	pthis = iconlists[2];
	ppthis = &iconlists[2];
	icngrp_table[0].s = "programs";
	defictype = ITM_PROGRAM;
	icngrp_cfg(file, lvl, io, error);
}


/*
 * Table for configuring all window icons (icontypes)
 */

static CfgEntry icontypes_table[] =
{
	{CFG_HDR, 0, "icontypes" },
	{CFG_BEG},
	{CFG_NEST,0, "files",     file_cfg    },		/* group */
	{CFG_NEST,0, "folders",   folder_cfg  },		/* group */
	{CFG_NEST,0, "programs",  program_cfg },		/* group */
	{CFG_ENDG},
	{CFG_LAST}
};


/* 
 * Save or load all window icons (icontypes) definitions 
 */

CfgNest icnt_config
{
	*error = handle_cfg(file, icontypes_table, lvl, CFGEMP, io, rem_all_icontypes, icnt_default);

/* OK; removed
	icnt_fix_ictypes(); /* Compatibility issue, may be removed after V3.60 */
*/
}


