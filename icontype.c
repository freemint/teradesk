/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2013  Dj. Vukovic
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */


#include <library.h>
#include <xdialog.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "lists.h"						/* must be before slider.h */
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
	SNAME type;							/* associated filetype */
	struct icontype *next;				/* pointer to next item in the list */
	_WORD icon;							/* icon id. */
	INAME icon_name;					/* icon name */
} ICONTYPE;

typedef struct
{
	_WORD icon;
	_WORD resvd;
} ITYPE;

static ICONTYPE iwork;					/* work area for editing icon data */
static ICONTYPE *iconlists[3];			/* pointers to lists of files, folders and programs */
static ICONTYPE *pthis;					/* pointer to current member of files, folders or programs icon group */
static ICONTYPE **ppthis;				/* pointer to address of current member of icon group */

static ITMTYPE defictype;


/*
 * Note: Indexes below must agree with the order of radiobuttons
 * for selecting icon group in the dialog 
 */

#define FILE_LIST 	0					/* index for files icons list in iconlists[] */
#define FOLD_LIST	1					/* index for folders icons list in iconlists [] */
#define PROG_LIST	2					/* index for programs icons list in iconlists [] */

/*
 * Copy icon-file assignment data to another location 
 * (note: must not just copy *t = *s because t->next should be preserved)
 */

static void copy_icntype(LSTYPE *lt, LSTYPE *ls)
{
	ICONTYPE *t = (ICONTYPE *) lt;
	ICONTYPE *s = (ICONTYPE *) ls;
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
 * Use of find_lsitem() here does not bring any reduction in size,
 * but slows things even further.
 */

static _WORD find_icon(const char *name, ICONTYPE *list)
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

static void icnt_info(LSTYPE **llist,	/* pointer to list in which the icon is searched for */
					  const char *filetype,	/* file(type) for which the associated icon is searched */
					  _WORD group,		/* is this files or folders or programs icon group */
					  LSTYPE *item		/* output information */
	)
{
	ICONTYPE **list = (ICONTYPE **) llist;
	ICONTYPE *it = (ICONTYPE *) item;

	if (!find_wild((LSTYPE **) list, filetype, (LSTYPE *) it, copy_icntype))
	{
		/* Attempt to divine the icon from a possibly defined desktop icon */

		it->icon = icn_iconid(filetype);

		if (it->icon < 0)
		{
			/* No suitable icon on the desktop, set general icon */

			_WORD iconid = FIINAME;

			_WORD lg = group & (LS_FIIC | LS_FOIC | LS_PRIC);

			if (lg == LS_FOIC)
				iconid = FOINAME;
			else if (lg == LS_PRIC)
				iconid = APINAME;

			it->icon = rsrc_icon_rscid(iconid, iname);
		}
	}

	return;
}


/*  
 * Find icons in the icons resource file by name, not index.
 * If icontype is not equal to icon target type, link is assumed.
 */

_WORD icnt_geticon(const char *name, ITMTYPE type,	/* type of the item */
				   ITMTYPE tgt_type		/* type of the tartet item of a link */
	)
{
	_WORD icon, deficon, i;
	ITMTYPE thetype = type;
	bool more = (type != tgt_type && tgt_type != ITM_NOTUSED);


	/* 
	 * Find a related icon, depending on item type (folder/file/program) 
	 * Links are handled in a somewhat dirty (but efficient) way:
	 * if icon for the specified name is not found, change item type
	 * to target item type and try again.
	 */

  again:;								/* if item is a link, return here to check target */

	switch (thetype)
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
	default:							/* ITM_FILE or ITM_LINK */
		i = 0;
		deficon = FIINAME;
		break;
	}

	if ((icon = find_icon(name, iconlists[i])) < 0)
	{
		/* Specific icon not found, but this is a link, so try again */

		if (more)
		{
			/* Go back and find default icon for this item type */

			thetype = tgt_type;
			more = FALSE;
			goto again;
		}

		icon = rsrc_icon_rscid(deficon, iname);
	}

	return icon;
}


/* 
 * Handle the dialog for assigning an icon to a file;
 * assignment of icon to a device (desktop icon) uses the same dialog 
 * in resource but a different handling routine.
 */

static bool icntype_dialog(LSTYPE **llist,	/* list into which the item is placed */
						   _WORD pos,	/* position (ordinal) in the list where to place the item */
						   LSTYPE *item,	/* item to be placed in the list */
						   _WORD use	/* purpose of use of dialog (add/edit) */
	)
{
	ICONTYPE **list = (ICONTYPE **) llist;
	ICONTYPE *it = (ICONTYPE *) item;
	_WORD il;
	_WORD button;
	_WORD theic = it->icon;
	_WORD title;
	SLIDER sl_info;
	SNAME thename;

	static const _WORD items1[] = { ICSHFIL, ICSHFLD, ICSHPRG, 0 };
	static const _WORD items2[] = { CHNBUTT, ICBTNS, DRIVEID, ICNLABEL, INAMBOX, 0 };

	/* Which title to use? */

	if (use & LS_EDIT)
		title = DTEDTICT;
	else
		title = DTADDICT;

	/* Files list or folders or programs list? */

	rsc_hidemany(addicon, items1);

	switch (use & (LS_FIIC | LS_FOIC | LS_PRIC))
	{
	case LS_FOIC:
		il = ICSHFLD;
		break;
	case LS_PRIC:
		il = ICSHPRG;
		break;
	default:							/* LS_FIIC */
		il = ICSHFIL;
		break;
	}

	obj_unhide(addicon[il]);

	/* Set dialog title and initial name */

	rsc_title(addicon, AITITLE, title);

	cv_fntoform(addicon, ICNTYPE, it->type);

	/* Set some fields as visible or invisible */

	rsc_hidemany(addicon, items2);

	obj_unhide(addicon[ICNTYPE]);
	obj_unhide(addicon[ICNTYPT]);
	obj_unhide(addicon[ADDBUTT]);

	sl_noop = 0;
	button = icn_dialog(&sl_info, &theic, ICNTYPE, options.win_pattern, options.win_colour);
	xd_close(&icd_info);

	cv_formtofn(thename, addicon, ICNTYPE);

	/* Button is OK but check for duplicate assignment, then set what needed */

	if ((button == ADDICNOK) && (check_dup((LSTYPE **) list, thename, pos)))
	{
		strcpy(it->type, thename);
		it->icon = theic;

		strsncpy(it->icon_name, icons[it->icon].ob_spec.ciconblk->monoblk.ib_ptext, sizeof(it->icon_name));

		return TRUE;
	}

	return FALSE;
}


/*
 * Use these iconlist-specific functions to manipulate icon lists: 
 */

static LS_FUNC itlist_func = {
	copy_icntype,
	lsrem,
	icnt_info,
	find_lsitem,
	icntype_dialog
};


/* 
 * Handle the dialog for maintaining lists of icons assigned to files 
 */

void icnt_settypes(void)
{
	_WORD button;

	/* Set dialog title, show radio buttons for selecting icon list */

	rsc_title(setmask, DTSMASK, DTITYPES);
	obj_unhide(setmask[ICATT]);
	xd_set_rbutton(setmask, ICATT, ITFILES);

	/*  Edit icontypes list: add/delete/change */

	button = list_edit(&itlist_func, (LSTYPE **) iconlists, 3, sizeof(ICONTYPE), (LSTYPE *) (&iwork), LS_ICNT);

	if (button == FTOK)
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

void icnt_default(void)
{
	lsrem_three((LSTYPE **) iconlists, lsrem);
}


#if 0									/* This routine is not currently used in TeraDesk */

/*
 * Add an icon assignment into the list, explicitely giving parameters.
 */

static ICONTYPE *itadd_one(ICONTYPE **list, char *filetype, _WORD icon)
{
	/* Define filemask */

	strsncpy(iwork.type, filetype, sizeof(iwork.type));

/* Let the user decide */
#if _MINT_
	if (mint && !magic)
		strlwr(iwork.type);
#endif

	/* Index of that icon in (c)icons.rsc */

	iwork.icon = icon;

	/* Copy icon name (i.e. icon name in (c)icons.rsc) */

	strsncpy(iwork.icon_name, icons[icon].ob_spec.ciconblk->monoblk.ib_ptext, sizeof(iwork.icon_name));

	/* Add that to list */

	return (ICONTYPE *) lsadd_end((LSTYPE **) list, sizeof(ICONTYPE), (LSTYPE *) (&iwork), copy_icntype);
}

#endif



/*
 * Move an icontype assignment from one group list to another.
 * If assignment to the new list fails, old assignment is not removed.
 */

static void icnt_move(_WORD to, _WORD from, ICONTYPE *it)
{
	if (lsadd_end((LSTYPE **) (&iconlists[to]), sizeof(ICONTYPE), (LSTYPE *) it, copy_icntype) != NULL)
		lsrem((LSTYPE **) (&iconlists[from]), (LSTYPE *) it);
}


/*
 * Move program icons from/to files or programs group, depending on whether
 * their name mask is currently that of a program or a file.
 * Icons for program-types files can be set only based on item names or
 * name masks. Execute rights are not considered here; they do not make sense
 * in this context anyway.
 */

void icnt_fix_ictypes(void)
{
	ICONTYPE *it, *next;

	it = iconlists[FILE_LIST];

	while (it)
	{
		next = it->next;

		/* Note: a mask only is given to prg_isprogram() below */

		if (prg_isprogram(it->type))
			icnt_move(PROG_LIST, FILE_LIST, it);

		it = next;
	}

	it = iconlists[PROG_LIST];

	while (it)
	{
		next = it->next;

		/* Note: a mask only is given to prg_isprogram() below */

		if (!prg_isprogram(it->type))
			icnt_move(FILE_LIST, PROG_LIST, it);

		it = next;
	}
}


/*
 * Table for configuring one window icon (icontype)
 */

static CfgEntry const icnt_table[] = {
	CFG_HDR("itype"),
	CFG_BEG(),
	CFG_S("mask", iwork.type),
	CFG_S("name", iwork.icon_name),
	CFG_END(),
	CFG_LAST()
};


/* 
 * Save or load definiton of one icontype
 */

static void one_itype(XFILE *file, int lvl, int io, int *error)
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
	} else
	{
		/* Clear work area, so that data not explicitelly set will be zero */

		memclr(&iwork, sizeof(iwork));

		/* Load configuration data for one icontype */

		*error = CfgLoad(file, icnt_table, (_WORD) sizeof(SNAME), lvl);

		/* Add to the list */

		if (*error == 0)
		{
			if (iwork.type[0] == 0 || iwork.icon_name[0] == 0)
			{
				*error = EFRVAL;
			} else
			{
				if ((iwork.icon = rsrc_icon(iwork.icon_name)) < 0)
					iwork.icon = default_icon(defictype);

				if (lsadd_end((LSTYPE **) ppthis, sizeof(ICONTYPE), (LSTYPE *) & iwork, copy_icntype) == NULL)
					*error = ENOMSG;	/* there was an alert in lsadd */
			}
		}
	}
}


/* 
 * Table for configuring one group of window icons (files/folders/programs)
 */

static CfgEntry const filegrp_table[] = {
	CFG_HDR("files"),
	CFG_BEG(),
	CFG_NEST("itype", one_itype),	/* Repeating group */
	CFG_END(),
	CFG_LAST()
};


static CfgEntry const foldergrp_table[] = {
	CFG_HDR("folders"),
	CFG_BEG(),
	CFG_NEST("itype", one_itype),	/* Repeating group */
	CFG_END(),
	CFG_LAST()
};


static CfgEntry const programgrp_table[] = {
	CFG_HDR("programs"),
	CFG_BEG(),
	CFG_NEST("itype", one_itype),	/* Repeating group */
	CFG_END(),
	CFG_LAST()
};


/* 
 * Save or load "files", "folders" or "programs" group of icon definitions 
 */

static void icngrp_cfg(XFILE *file, const CfgEntry *table, int lvl, int io, int *error)
{
	if (io == CFG_LOAD)
		lsrem_all_one((LSTYPE **) (*ppthis));

	*error = handle_cfg(file, table, lvl, CFGEMP | (pthis ? 0 : CFGSKIP), io, NULL, NULL);
}


/* 
 * Save or load "files" group of window icontypes 
 */

static void file_cfg(XFILE *file, int lvl, int io, int *error)
{
	pthis = iconlists[0];
	ppthis = &iconlists[0];
	defictype = ITM_FILE;
	icngrp_cfg(file, filegrp_table, lvl, io, error);
}


/* 
 * Save or load "folders" group of window icontypes 
 */

static void folder_cfg(XFILE *file, int lvl, int io, int *error)
{
	ppthis = &iconlists[1];
	pthis = iconlists[1];
	defictype = ITM_FOLDER;
	icngrp_cfg(file, foldergrp_table, lvl, io, error);
}


/* 
 * Save or load "programs" group of window icontypes 
 */

static void program_cfg(XFILE *file, int lvl, int io, int *error)
{
	pthis = iconlists[2];
	ppthis = &iconlists[2];
	defictype = ITM_PROGRAM;
	icngrp_cfg(file, programgrp_table, lvl, io, error);
}


/*
 * Table for configuring all window icons (icontypes)
 */

static CfgEntry const icontypes_table[] = {
	CFG_HDR("icontypes"),
	CFG_BEG(),
	CFG_NEST("files", file_cfg),	/* group */
	CFG_NEST("folders", folder_cfg),	/* group */
	CFG_NEST("programs", program_cfg),	/* group */
	CFG_ENDG(),
	CFG_LAST()
};


/* 
 * Save or load all window icons (icontypes) definitions 
 */
void icnt_config(XFILE *file, int lvl, int io, int *error)
{
	*error = handle_cfg(file, icontypes_table, lvl, CFGEMP, io, icnt_default, icnt_default);
}
