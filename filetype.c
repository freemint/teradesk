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

#include "resource.h"					/* includes desktop.h */
#include "desk.h"
#include "error.h"
#include "lists.h"						/* must be above slider.h */
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"
#include "filetype.h"
#include "font.h"
#include "window.h"


FTYPE fwork;							/* temp. area for currently edited filetype */
FTYPE *filetypes;							/* List of defined filetype masks */
FTYPE *fthis;
FTYPE **ffthis;



/*
 * Some (mostly same) filetype masks are used 
 * in filetype.c, prgtype.c and icontype.c, 
 * so they are here defined once for all
 * Note: items [0] and [1] MUST be * and *.*; they will be used
 * as default filename extensions in mint and singleTOS.
 * Items [10] and [11] are used in the autolocator.
 */

const char *presets[12] = { "*", "*.*", "*.PRG", "*.APP", "*.GTP", "*.TOS", "*.TTP", "*.ACC", "*.TXT", "*.IMG", ".*", "\0" };
const char fas[] = { FA_RDONLY, FA_CHANGED, FA_HIDDEN, FA_SYSTEM, FA_DIR, FA_PARDIR };


/*  
 * Copy one filemask dataset to another; this routine doesn't do
 * anything special except copying the string, but is formed to be
 * compatible with similar routines for other lists
 * (note: must not just copy *t = *s because t->next should be preserved)
 */

void copy_ftype(LSTYPE *lt, LSTYPE *ls)
{
	FTYPE *t = (FTYPE *) lt;
	FTYPE *s = (FTYPE *) ls;

	strsncpy(t->filetype, s->filetype, sizeof(SNAME));
}


/*
 * Add one filetype mask to end of list, explicitely setting data; 
 * here, only the name is entered, but for other lists the
 * corresponding routines enter additional data as appropriate
 */

static FTYPE *ftadd_one(const char *filetype)
{
	strsncpy(fwork.filetype, filetype, sizeof(fwork.filetype));

	/* 
	 * Note: in other similar routines name is converted to lowercase
	 * if mint is active; should it be so here too? (was not in old code).
	 * Currently disabled, i.e. always uppercase
	 */

#if 0									/* Better let the user decide */
#if _MINT_

	if (mint && !magic)
		strlwr(fwork.filetype);
#endif
#endif
	return (FTYPE *) lsadd_end((LSTYPE **) (&filetypes), sizeof(LSTYPE), (LSTYPE *) (&fwork), copy_ftype);
}


/*
 * Find (or create, if nonexistent!) information about an filetype in a list;
 * input: filename or a filename mask;
 * output: filetype (i.e. FTYPE) data
 * this routine does not do much except copy the name; 
 * it has been created for compatibility with routines for other lists
 * which manipulate additional data
 */

static void ftype_info(LSTYPE **llist,	/* list of defined masks */
					   const char *filetype,	/* filetype to search for */
					   _WORD use,		/* what is being done */
					   LSTYPE *item	/* output information */
	)
{
	FTYPE **list = (FTYPE **) llist;
	FTYPE *ft = (FTYPE *) item;

	if (!(use & LS_SELA))
		find_wild((LSTYPE **) list, filetype, (LSTYPE *) ft, copy_ftype);

	return;
}


/*
 * Remove the complete list of filetypes;
 */

static void rem_all_filetypes(void)
{
	lsrem_all_one((LSTYPE **) (&filetypes));
}


/*
 * Handle the dialog for entering a filetype for file mask; 
 * return edited data in *ft; if operation is canceled, entry values
 * in *ft are unchanged.
 */

static bool filetype_dialog(LSTYPE **llist,	/* list in which duplicates are checked for */
							_WORD pos,	/* positin in the list where to add data */
							LSTYPE *item,	/* data to be edited */
							_WORD use	/* use of this dialog (filetype or doctype, add or edit) */
	)
{
	FTYPE **list = (FTYPE **) llist;
	FTYPE *ft = (FTYPE *) item;
	XDINFO info;						/* dialog info structure */
	_WORD title;						/* rsc index of dialog title string */
	_WORD button;						/* code of pressed button */
	bool status = FALSE;				/* changes accepted or not */
	bool stop = FALSE;					/* true to exit from dialog */

	/* Set dialog title (add/edit) */

	if (use & LS_EDIT)
		title = (use & LS_FMSK) ? DTEDTMSK : DTEDTDT;
	else
		title = (use & LS_FMSK) ? DTADDMSK : DTADDDT;

	rsc_title(ftydialog, FTYTITLE, title);
	cv_fntoform(ftydialog, FTYPE0, ft->filetype);

	/* Open the dialog, then loop until stop */

	if (chk_xd_open(ftydialog, &info) >= 0)
	{
		while (!stop)
		{
			button = xd_form_do(&info, ROOT);

			if (button == FTYPEOK)
			{
				/* 
				 * If selected OK, check if this filetype has not already
				 * been entered in this list
				 */

				SNAME ftxt;

				cv_formtofn(ftxt, ftydialog, FTYPE0);

				if (*ftxt != 0)
				{
					if (check_dup((LSTYPE **) list, ftxt, pos))
					{
						strcpy(ft->filetype, ftxt);
						stop = TRUE;
						status = TRUE;
					}
				}
			} else
			{
				stop = TRUE;
			}

			xd_drawbuttnorm(&info, button);
		}

		xd_close(&info);
	}

	return status;
}


/* 
 * Default funktie voor het zetten van een filemask. Bij het
 * aanroepen moet het huidige masker in mask staan. Na afloop zal
 * hierin het nieuwe masker staan. Als het resultaat TRUE is, dan
 * is op OK gedrukt, als het resultaat FALSE is, dan is op Cancel
 * gedrukt, of er is een fout opgetreden.
 *
 * mask = old filemask or NULL if new mask not to be set
 * return: new filemask or NULL if not set.
 */

char *wd_filemask(const char *mask)
{
	return ft_dialog(mask, &filetypes, LS_FMSK);
}


/*
 * Use these filetype-list-specific functions to manipulate filetype lists: 
 */

static LS_FUNC ftlist_func = {
	copy_ftype,
	lsrem,
	ftype_info,
	find_lsitem,
	filetype_dialog
};


/* 
 * Handling of the dialog for setting filetype masks (add/delete/change);
 * Note: this dialog is also called when an application is installed
 * (to set document types for that application). A recursion occurs then,
 * because list_edit(...) routine is called from within itself:
 * app_install-->list_edit-->app_dialog-->ft_dialog-->list_edit
 */

char *ft_dialog(const char *mask,		/* file mask which will be current from now on */
				FTYPE **flist,			/* list of defined filetypes/masks */
				_WORD use				/* determines if sets filemasks or documenttypes */
	)
{
	OBJECT savedial;					/* to save the dialog before recursive calls */
	_WORD j;							/* loop counter */
	_WORD luse;							/* local value of use */
	_WORD button;						/* code of pressed button */
	_WORD ftb;							/* button, too   */
	SNAME newmask;						/* newly specified filemask */
	char *result = NULL;				/* to be the return value of this routine */

	static const char ois[] = { 0, 0, MSKHID, MSKSYS, MSKDIR, MSKPAR };
	static const _WORD items[] = { MSKATT, FILETYPE, FTTEXT, 0 };

	/* 
	 * If necessary, save the previous state of this dialog's root object
	 * in order to return to proper state after a recursive call
	 * (this will happen when assigning application documenttypes) 
	 */

	if (use & LS_DOCT)
	{
		savedial = *setmask;

		ftb = xd_get_rbutton(setmask, FTPARENT);
		xd_set_rbutton(setmask, FTPARENT, FTYPE1);
	}

	luse = use & ~LS_WSEL;

	/* Make visible what is relevant for each particular use of this dialog */

	if (mask)
	{
		cv_fntoform(setmask, FILETYPE, mask);
		obj_unhide(setmask[FILETYPE]);
		obj_unhide(setmask[FTTEXT]);
	}

	/*
	 * As there are finally only two uses for this routine, an 
	 * if-then-else could have been used below instead of switch...
	 */

	switch ((luse & ~LS_SELA))
	{
	case LS_FMSK:
		/* Use this dialog to set file mask */

		rsc_title(setmask, DTSMASK, DTFTYPES);
		rsc_title(setmask, FTTEXT, TFTYPE);
		obj_unhide(setmask[MSKATT]);
		setmask[FILETYPE].ob_flags |= OF_EDITABLE;

		/* Enter values of file attributes flags into dialog */

		for (j = 2; j < 6; j++)
			set_opt(setmask, options.attribs, fas[j], ois[j]);

		break;
	case LS_DOCT:
		/* 
		 * Use this dialog to set document types for an application;
		 * must deselect add/delete/change buttons because of a 
		 * recursive call of setmask dialog when selecting document types
		 */

		rsc_title(setmask, DTSMASK, DTDTYPES);
		rsc_title(setmask, FTTEXT, TAPP);
		setmask[FILETYPE].ob_flags &= ~OF_EDITABLE;
		obj_deselect(setmask[FTADD]);
		obj_deselect(setmask[FTDELETE]);
		obj_deselect(setmask[FTCHANGE]);
		break;
	default:
		break;
	}

	/* Edit the filemasks list: add/delete/change entry */

	button = list_edit(&ftlist_func, (LSTYPE **) (flist), 1, sizeof(FTYPE), (LSTYPE *) (&fwork), luse);

	rsc_hidemany(setmask, items);

	if (button == FTOK)
	{
		/* If changes are accepted... */

		if (luse & LS_FMSK)
		{
			for (j = 2; j < 6; j++)
				get_opt(setmask, &options.attribs, fas[j], ois[j]);

			if (mask != NULL)
			{
				cv_formtofn(newmask, setmask, FILETYPE);

				if ((result = malloc_chk(strlen(newmask) + 1)) != NULL)
					strcpy(result, newmask);

				return result;
			}
		}
	}

	/* 
	 * If necessary, restore previous state of dialog root object 
	 * (needed after a recursive call)
	 */

	if (use & LS_DOCT)
	{
		*setmask = savedial;
		xd_set_rbutton(setmask, FTPARENT, ftb);
	}

	/* No mask set */

	return NULL;
}


/* 
 * Initialize (empty) list of filetype masks 
 */

void ft_init(void)
{
	filetypes = NULL;
#ifdef MEMDEBUG
	atexit(rem_all_filetypes);
#endif
}


/* 
 * Set a list of filetype masks with some predefined ones 
 */

void ft_default(void)
{
	_WORD i;

	rem_all_filetypes();

	/* 
	 * Note 1: first two masks ("*" and "*.*") must be set in order to 
	 * show anything in windows 
	 * Note 2: ftadd_one can be used with explicitely entered name, too:
	 * e.g. ftadd_one("*.C");
	 */

#if _PREDEF

	/* Note: for upper/lowercase match see routine ftadd_one */

	for (i = 0; i < 10; i++)
		ftadd_one(presets[i]);
#else

	ftadd_one(presets[0]);				/*      *       */
	ftadd_one(presets[1]);				/*      *.*     */

#endif
}


/*
 * Configuration table for one filetype or doctype
 */

CfgEntry ft_table[] = {
	{ CFG_HDR, NULL, { 0 } },				/* keyword will be substituted */
	{ CFG_BEG, NULL, { 0 } },
	{ CFG_S,   "mask", { fwork.filetype } },
	{ CFG_END, NULL, { 0 } },
	{ CFG_LAST, NULL, { 0 } }
};


/* 
 * This routine handles saving of all defined filetypes, but it handles
 * loading of -only one- 
 */
static void one_ftype(XFILE *file, int lvl, int io, int *error)
{
	*error = 0;

	if (io == CFG_SAVE)
	{
		/* Save data: all defined filetypes */

		while ((*error == 0) && fthis)
		{
			fwork = *fthis;
			*error = CfgSave(file, ft_table, lvl, CFGEMP);
			fthis = fthis->next;
		}
	} else
	{
		/* Load data; one filetype */

		memclr(&fwork, sizeof(fwork));	/* must set ALL of .filetype to 0 !!! */
		*error = CfgLoad(file, ft_table, (int) sizeof(SNAME), lvl);

		if (*error == 0)
		{
			if (fwork.filetype[0] == 0)
			{
				*error = EFRVAL;
			} else
			{
				if (lsadd_end((LSTYPE **) ffthis, sizeof(FTYPE), (LSTYPE *) & fwork, copy_ftype) == NULL)
					*error = ENOMSG;	/* there was an alert in lsadd */
			}
		}
	}
}


/*
 * Configuration table for filetypes (filename masks)
 */

CfgEntry filetypes_table[] = {
	{ CFG_HDR, NULL, { 0 } },								/* keyword will be substituted */
	{ CFG_BEG, NULL, { 0 } },
	{ CFG_NEST, NULL, { one_ftype } },						/* Repeating group */
	{ CFG_ENDG, NULL, { 0 } } ,
	{ CFG_LAST, NULL, { 0 } }
};


/*
 * Load or save all filetypes
 */
void ft_config(XFILE *file, int lvl, int io, int *error)
{
	const char *fff = "ftype";

	fthis = filetypes;
	ffthis = &filetypes;
	ft_table[0].s = fff;
	filetypes_table[0].s = "filetypes";
	filetypes_table[2].s = fff;
	filetypes_table[3].type = CFG_ENDG;

	*error = handle_cfg(file, filetypes_table, lvl, CFGEMP, io, rem_all_filetypes, ft_default);
}
