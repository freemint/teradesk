/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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

#define END			32767


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
	*files,			/* pointer to list of icons assigned to files      */ 
	*folders; 		/* pointer to list of icons assigned to folders    */



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
 * Find an icon defined by associated file(type) in the files or folders list;
 * return identifier of the icon.
 * Note: this function searches for an icon sequentially through a list.
 * On an low-end Atari, finding icons near the end of a largelist can take
 * noticeable time. Routine cmp_wildcards() is rather slow.
 */

static int find_icon(const char *name, ICONTYPE *list)
{
	ICONTYPE *p = list;

	while (p)
	{
		if (cmp_wildcard(name, p->type) == TRUE)
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
	int group,			/* is this files or folders icon group */
	ICONTYPE *it 		/* output information */
)
{
	int iconid;

	if ( !find_wild( (LSTYPE **)list, filetype, (LSTYPE *)it, copy_icntype ) )
	{
		/* 
	 	 * If icon type not defined or name not given:
	 	 * set general file or folder icon
	 	 */

		if ( group & LS_FIIC )
		{
			if ( prg_isprogram(it->type) )
				iconid = APINAME;
			else
				iconid = FIINAME;
		}
		else
			iconid = FOINAME;
		it->icon = rsrc_icon_rscid ( iconid, iname ); 
	}
	
	return;
}


/*  
 * Find icons in the icons resource file by name, not index.
 * Parameter "link" currently not used  
 */

int icnt_geticon(const char *name, ITMTYPE type, boolean link)
{
	int icon;

	/* Find a related icon, depending on item type (folder/file/program) */

	if ((type == ITM_PREVDIR) || (type == ITM_FOLDER))
	{
		/* Find an icon in the folder group. If not found, use default */

		if ((icon = find_icon(name, folders)) < 0)
			icon = rsrc_icon_rscid ( FOINAME, iname ); 
	}
	else
	{
		/* 
		 * Find an icon in the file group (match the name against wildcard masks). 
		 * If not found, use default file and program icons
		 */

		if ((icon = find_icon(name, files)) < 0)
			icon = rsrc_icon_rscid ( prg_isprogram(name) ? APINAME : FIINAME, iname ); 
	}


	/*
	 * Better not: this below means that if icon is not found, then
	 * the first icon in the resource file is assigned, whatever that first
	 * icon may be (i.e. uncontrolled).
	 * If below is disabled, -1 is returned in case of error
	 *

	if (icon < 0)
		icon = 0;
	*/

	return icon;
}


/*
 * Add an icon assignment into the list, explicitely giving parameters
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

	return (ICONTYPE *)lsadd( (LSTYPE **)list, sizeof(ICONTYPE), (LSTYPE *)(&iwork), END, copy_icntype );
}


/* 
 * Handle the dialog for assigning an icon to a file;
 * assignment of icon to a device (desktop icon) uses the same dialog 
 * in resource but a different routine.
 * list = list into which the item is placed;
 * pos = position in the list (ordinal) where to place the item
 * it = item to be placed in te list
 * use = purpose of use of dialog (add/edit)
 */

static boolean icntype_dialog( ICONTYPE **list, int pos, ICONTYPE *it, int use)
{
	int button;
	int title; 

	SLIDER sl_info;

	SNAME thename;

	/* Which title to use? */

	if ( use & LS_EDIT )
		title = DTEDTICT;
	else
		title = DTADDICT;

	/* Files list or folders list? */

	if ( use & LS_FIIC )
	{
		obj_hide(addicon[ICSHFLD]);
		obj_unhide(addicon[ICSHFIL]);
	}
	else
	{
		obj_unhide(addicon[ICSHFLD]); 
		obj_hide(addicon[ICSHFIL]);
	}

	rsc_title(addicon, AITITLE, title);

	cv_fntoform(addicon + ICNTYPE, it->type);

	/* Set some fields as visible or invisible */

	obj_hide(addicon[CHNBUTT]);
	obj_unhide(addicon[ADDBUTT]);
	obj_hide(addicon[ICBTNS]);
	obj_hide(addicon[DRIVEID]);
	obj_hide(addicon[ICNLABEL]); 
	obj_unhide(addicon[ICNTYPE]);
	obj_unhide(addicon[ICNTYPT]);

	icn_sl_init(it->icon, &sl_info);

	button = sl_dialog(addicon, ICNTYPE, &sl_info); 

	obj_hide(addicon[ICSHFLD]); 
	obj_hide(addicon[ICSHFIL]);

	cv_formtofn( thename, addicon + ICNTYPE);

	if 
	( 
		(button == ADDICNOK ) && 
		(check_dup( (LSTYPE **)list, thename, pos)) 
	)
	{
		strcpy(it->type, thename);
		it->icon = sl_info.line;

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
	obj_select(setmask[ITFILES]); /* list_edit always starts from the 1st list */
	obj_deselect(setmask[ITFOLDER]);

	/*  Edit icontypes list: add/delete/change */

	button = list_edit
	         ( 
				&itlist_func, 
				(LSTYPE **)(&files), 
				(LSTYPE **)(&folders), 
				sizeof(ICONTYPE), 
				(LSTYPE *)(&iwork), 
				LS_ICNT
	         ); 

	if ( button == FTOK ) 
		wd_seticons();

	/* Hide radio buttons for selecting icons list */

	obj_hide(setmask[ICATT]); 
}


/* Initiate (empty) lists of assigned files and folders */

void icnt_init(void)
{
	files = NULL;
	folders = NULL;
}


/*
 * Clear all icontypes
 */

static void rem_all_icontypes(void)
{
	lsrem_all((LSTYPE **)(&files), lsrem);
	lsrem_all((LSTYPE **)(&folders), lsrem);

}

/*
 * Set up some predefined lists of icons-files and icons-folders assignments.
 * This can be tricky: Lists of assignments from searched from the first
 * to the last item. If the most general wildcards such as "*" and "*.*" 
 * are defined among the first ones, other matches later in the list will 
 * never be found. So, at the least, the most general wildcards should be
 * placed near the end of the list, if at all. Anyway, this can lead to
 * a confusion, so perhaps it is best to disable all of these predefined
 * icontypes which were set below; Teradesk anyway assigns some default
 * icons in such cases. To enable below set ENABLE_PREDEFIC to 1.
 */

#define ENABLE_PREDEFIC 0

void icnt_default(void)
{
#if ENABLE_PREDEFIC

	INAME iname; 
	int ilist[3];
#endif

	rem_all_icontypes();


#if ENABLE_PREDEFIC

	ilist[0] = 	rsrc_icon_rscid ( FOINAME, iname ); /*   *              */
	ilist[1] =	rsrc_icon_rscid ( FIINAME, iname ); /*   *.*            */
#if _PREDEF
	ilist[2] =	rsrc_icon_rscid ( APINAME, iname );  /*   PRG, APP, TOS  */
#endif

	itadd_one(&folders, (char *)presets[0],  ilist[0]);	/*	*		*/ 
	itadd_one(&files, (char *)presets[1],   ilist[1]);	/* 	*.*		*/
#if _PREDEF
	itadd_one(&files, (char *)presets[2], ilist[2]);	/*	*.PRG	*/
	itadd_one(&files, (char *)presets[3], ilist[2]);	/*	*.APP	*/
	itadd_one(&files, (char *)presets[4], ilist[2]);	/*	*.GTP	*/
	itadd_one(&files, (char *)presets[5], ilist[2]);	/*	*.TOS	*/
#endif

#endif /* ENABLE_PREFEDIC */


}


static ICONTYPE 
	*pthis,		/* pointer to current member of files or folders icon group */ 
	**ppthis;	/* pointer to address of current member of files or folders icon group */


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
			*error = CfgSave(file, icnt_table, lvl + 1, CFGEMP);
			pthis = pthis->next;
		}
	}
	else
	{
		/* Clear work area, so that data not explicitelly set will be zero */

		memset(&iwork, 0, sizeof(iwork));

		/* Load configuration data for one icontype */

		*error = CfgLoad(file, icnt_table, (int)sizeof(SNAME) - 1, lvl + 1); 

		/* Add to the list */

		if (*error == 0 )
		{
			if ( iwork.type[0] == 0 || iwork.icon_name[0] == 0)
				*error = EFRVAL;
			else
			{
				iwork.icon = rsrc_icon(iwork.icon_name);

				/* Icons not found in the iconfile will be ignored */

				if (iwork.icon >= 0)
				{
					if (
						lsadd(	(LSTYPE **)ppthis,
							sizeof(ICONTYPE),
							(LSTYPE *)&iwork,
							END,
							copy_icntype
				      		) == NULL
						)
							*error = ENOMSG; /* there was an alert in lsadd */
				}
			}
		}
	}
}


/* 
 * Table for configuring one group of window icons (icontypes; files or folders)
 */

static CfgEntry icngrp_table[] =
{
	{CFG_HDR, 0, "*" },
	{CFG_BEG},
	{CFG_NEST,0, "itype", one_itype  },		/* Repeating group */
	{CFG_END},
	{CFG_LAST}
};


/* 
 * Save or load "files" or "folders" group of icon definitions 
 */

static CfgNest icngrp_cfg
{
	if ( io == CFG_LOAD )
		lsrem_all((LSTYPE **)(*ppthis), lsrem);

	*error = handle_cfg(file, icngrp_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, NULL, NULL);
}


/* 
 * Save or load "files" group of window icontypes 
 */

static CfgNest file_cfg
{
	pthis = files;
	ppthis = &files;
	icngrp_table[0].s = "files";

	icngrp_cfg(file, key, lvl, io, error);
}


/* 
 * Save or load "folders" group of window icontypes 
 */

static CfgNest folder_cfg
{
	pthis = folders;
	ppthis = &folders;
	icngrp_table[0].s = "folders";

	icngrp_cfg(file, key, lvl, io, error);
}


/*
 * Table for configuring all window icons (icontypes)
 */

static CfgEntry icontypes_table[] =
{
	{CFG_HDR, 0, "icontypes" },
	{CFG_BEG},
	{CFG_NEST,0, "files",     file_cfg  },		/* group */
	{CFG_NEST,0, "folders", folder_cfg  },		/* group */
	{CFG_ENDG},
	{CFG_LAST}
};


/* 
 * Save or load all window icons (icontypes) definitions 
 */

CfgNest icnt_config
{
	*error = handle_cfg(file, icontypes_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, rem_all_icontypes, icnt_default);
}


