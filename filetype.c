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


#include <np_aes.h>	
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <xdialog.h>
#include <mint.h>

#include "desk.h"
#include "error.h"
#include "resource.h" /* includes desktop.h */
#include "lists.h" /* must be above slider.h */
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"	
#include "filetype.h"
#include "font.h"
#include "window.h"


FTYPE
	fwork,			/* temp. area for currently edited filetype */ 
	*filetypes; 	/* List of defined filetype masks */



/*
 * Some (mostly same) filetype masks are used 
 * in filetype.c, prgtype.c and icontype.c, 
 * so they are here defined once for all
 */

char *presets[10] = {"*", "*.*", "*.PRG", "*.APP", "*.GTP", "*.TOS", "*.TTP", "*.ACC", "*.TXT", "*.IMG" };


/*  
 * Copy one filemask dataset to another; this routine doesn't do
 * anything special except copying the string, but is formed to be
 * compatible with similar routines for other lists
 * (note: must not just copy *t = *s because t->next should be preserved)
 */

void copy_ftype(FTYPE *t, FTYPE *s)
{
	strsncpy ( t->filetype, s->filetype, sizeof(SNAME) );
}


/*
 * Add one filetype mask to end of list, explicitely setting data; 
 * here, only the name is entered, but for other lists the
 * corresponding routines enter additional data as appropriate
 */

static FTYPE *ftadd_one(char *filetype)
{
	strsncpy( fwork.filetype, filetype, sizeof(fwork.filetype));

	/* 
	 * note: in other similar routines name is converted to lowercase
	 * if mint is active; should it be so here too? (was not in old code).
	 * Currently disabled, i.e. always uppercase
	 */

#if _MINT_
/*
	if ( mint && !magic )
		strlwr(fwork.filetype);
*/
#endif

	return (FTYPE *)lsadd( (LSTYPE **)(&filetypes), sizeof(LSTYPE), (LSTYPE *)(&fwork), END, copy_ftype); 
}


/*
 * Find (or create, if nonexistent!) information about an filetype in a list;
 * input: filename or a filename mask;
 * output: filetype (i.e. FTYPE) data
 * this routine does not do much except copy the name; 
 * it has been created for compatibility with routines for other lists
 * which manipulate additional data
 */

static void ftype_info
( 
	FTYPE **list, 		/* list of defined masks */
	char *filetype,		/* filetype to search for */ 
	int dummy,			/* not used, exists for compatibility */
	FTYPE *ft 			/* output information */
)
{
	find_wild( (LSTYPE **)list, filetype, (LSTYPE *)ft, NULL );
	return;
}


/*
 * Remove the complete list of filetypes;
 */

static void rem_all_filetypes(void)
{
	rem_all( (LSTYPE **)(&filetypes), rem ); 
}


/*
 * Handle the dialog for entering a filetype for file mask; 
 * return data in *ft; if operation is canceled, entry values
 * in *ft are unchanged.
 * This routine was practically rewritten, 
 * old code was removed completely
 * 
 */

static boolean filetype_dialog
(
	FTYPE **list, 	/* list in which duplicates are checked for */
	int pos,		/* positin in the list where to add data */ 
	FTYPE *ft,		/* data to be edited */ 
	int use			/* use of this dialog (filetype or doctype, add or edit) */
)
{
	int 
		title, 	/* rsc index of dialog title string */
		button;	/* code of pressed button */

	boolean
		stat = FALSE,	/* changes accepted or not */
		stop = FALSE;	/* true to exit from dialog */

	XDINFO
		info;			/* dialog info structure */


	/* Set dialog title (add/edit) */

	if ( use & LS_EDIT )
		title = (use & LS_FMSK) ? DTEDTMSK : DTEDTDT;
	else
		title = (use & LS_FMSK) ? DTADDMSK : DTADDDT;

	rsc_title(ftydialog, FTYTITLE, title); 

	cv_fntoform(ftydialog + FTYPE0, ft->filetype);

	/* Open the dialog, then loop until stop */

	xd_open(ftydialog, &info);

	while (!stop)
	{
		button = xd_form_do( &info, ROOT );

		if ( button == FTYPEOK )
		{
			/* 
			 * If selected OK, check if this filetype has not already
			 * been entered in this list
			 */

			SNAME ftxt;

			cv_formtofn( ftxt, &ftydialog[FTYPE0]);

			if ( *ftxt != 0 )
			{
				if ( check_dup((LSTYPE **)list, ftxt, pos ) )
				{
					strcpy(ft->filetype, ftxt);
					stop = TRUE;
					stat = TRUE;
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


void wd_set_filemask(WINDOW *w)
{
	((ITM_WINDOW *) w)->itm_func->wd_filemask(w);
}


/* 
 * Default funktie voor het zetten van een filemask. Bij het
 * aanroepen moet het huidige masker in mask staan. Na afloop zal
 * hierin het nieuwe masker staan. Als het resultaat TRUE is, dan
 * is op OK gedrukt, als het resultaat FALSE is, dan is op Cancel
 * gedrukt, of er is een fout opgetreden.
 */

char *wd_filemask(const char *mask) 
{
	return ft_dialog( mask, &filetypes, LS_FMSK );
}


/*
 * Use these filetype-list-specific functions to manipulate filetype lists: 
 */

#pragma warn -sus
static LS_FUNC ftlist_func =
{
	copy_ftype,
	rem,
	ftype_info,
	find_lsitem,
	filetype_dialog
};
#pragma warn .sus


/* 
 * Handling of the dialog for setting filetype masks (add/delete/change);
 * Note: this dialog is also called when an application is installed
 * (to set document types for that application). A recursion occurs then,
 * because list_edit(...) routine is called from within itself:
 * app_install-->list_edit-->app_dialog-->ft_dialog-->list_edit
 */
 
char *ft_dialog
( 
	const char *mask,	/* file mask which will be current from now on */ 
	FTYPE **flist,		/* list of defined filetypes/masks */ 
	int use				/* determines if sets filemasks or documenttypes */ 
)
{
	OBJECT
		savedial;	/* to save dialog before recursive call */

	int 
		luse,		/* local value of use */
		button,		/* code of pressed button */
		ftb;		/* button, too   */

	SNAME 
		newmask;	/* newly specified filemask */

	char 
		*result;	/* to be return value of this routine */


	/* 
	 * If necessary, save the previous state of this dialog's root object
	 * (to return to proper state after a recursive call) 
	 */

	if ( use & LS_DOCT )
	{
		memcpy( &savedial, setmask, sizeof(OBJECT));
		ftb = xd_get_rbutton ( setmask, FTPARENT );
		xd_set_rbutton( setmask, FTPARENT, FTYPE1 );
	}

	luse = use & ~LS_WSEL;

	/* Make visible what is relevant for each particular use of this dialog */

	if ( mask != NULL )
	{
		cv_fntoform(setmask + FILETYPE, mask);		
		setmask[FILETYPE].ob_flags &= ~HIDETREE;
		setmask[FTTEXT].ob_flags &= ~HIDETREE;
	}

	/*
	 * As there are finally only two uses for this routine, an 
	 * if-then-else could have been used below instead of switch...
	 */

	switch(luse)
	{
		case LS_FMSK:

			/* Use this dialog to set file mask */

			rsc_title(setmask, DTSMASK, DTFTYPES);
 			rsc_title(setmask, FTTEXT, TFTYPE ); 
			setmask[MSKATT].ob_flags &= ~HIDETREE; 
			setmask[FILETYPE].ob_flags |= EDITABLE;

			/* Enter values of file attributes flags into dialog */

			set_opt( setmask, options.attribs, FA_HIDDEN, MSKHID );
			set_opt( setmask, options.attribs, FA_SYSTEM, MSKSYS );
			set_opt( setmask, options.attribs, FA_SUBDIR, MSKDIR );
			set_opt( setmask, options.attribs, FA_PARDIR, MSKPAR );
			break;

		case LS_DOCT:

			/* 
			 * Use this dialog to set document types for an application;
			 * must deselect add/delete/change buttons because of a 
			 * recursive call of setmask dialog when selecting document types
			 */

			rsc_title(setmask, DTSMASK, DTDTYPES);
			rsc_title(setmask, FTTEXT, TAPP ); 
			setmask[FILETYPE].ob_flags &= ~EDITABLE;
			setmask[FTADD].ob_state &= ~SELECTED; 
			setmask[FTDELETE].ob_state &= ~SELECTED;
			setmask[FTCHANGE].ob_state &= ~SELECTED;
			break;

		default:
			break;
	}

	/* Edit the filemasks list: add/delete/change entry */

	button = list_edit( &ftlist_func, (LSTYPE **)(flist), NULL, sizeof(FTYPE), (LSTYPE *)(&fwork), luse);

	setmask[MSKATT].ob_flags |= HIDETREE;
	setmask[FILETYPE].ob_flags |= HIDETREE;
	setmask[FTTEXT].ob_flags |= HIDETREE;

	if ( button == FTOK )
	{
		/* If changes are accepted... */

		if ( luse & LS_FMSK )
		{
			get_opt( setmask, &options.attribs, FA_HIDDEN, MSKHID );
			get_opt( setmask, &options.attribs, FA_SYSTEM, MSKSYS );
			get_opt( setmask, &options.attribs, FA_SUBDIR, MSKDIR ); 
			get_opt( setmask, &options.attribs, FA_PARDIR, MSKPAR );

			if ( mask != NULL )
			{
				cv_formtofn(newmask, &setmask[FILETYPE]);
				if ((result = malloc(strlen(newmask) + 1)) != NULL)
					strcpy(result, newmask);
				else
					xform_error(ENSMEM);
				return result;
			}
		}
	}

	/* 
	 * If necessary, restore previous state of dialog root object 
	 * (needed after a recursive call)
	 */

	if ( use & LS_DOCT )
	{
		memcpy ( setmask, &savedial, sizeof(OBJECT));
		xd_set_rbutton( setmask, FTPARENT, ftb );
	}

	/* No mask set */

	return NULL;
}


/* Initialize (empty) list of filetype masks */

void ft_init(void)
{
	filetypes = NULL;
#ifdef MEMDEBUG
	atexit(rem_all_filetypes);
#endif
}


/* Set a list of filetype masks with some predefined ones */

void ft_default(void)
{
	int i;

	rem_all_filetypes();

	/* 
	 * Note 1: first two masks must be set in order to show anything in windows 
	 * Note 2: ftadd_one can be used with explicitely entered name, too:
	 * e.g. ftadd_one("*.C");
	 */

	ftadd_one(presets[0]);	/* 		* 		*/			
	ftadd_one(presets[1]);	/* 		*.*		*/

#if _PREDEF

	/* Note: for upper/lowercase match see routine ftadd_one */

	for ( i = 2; i < 10; i++ )
		ftadd_one(presets[i]);
#endif
}


/*
 * Configuration table for one filetype or doctype
 */

CfgEntry ft_table[] =
{
	{CFG_HDR, 0, "*type"  },
	{CFG_BEG},
	{CFG_S,   0, "mask", fwork.filetype },
	{CFG_END},
	{CFG_LAST}
};


/* 
 * This routine handles saving of all defined filetypes, but it handles
 * loading of -only one- 
 */
 
FTYPE 
	*fthis, 
	**ffthis;


CfgNest one_ftype
{
	*error = 0;

	if (io == CFG_SAVE)
	{
		/* Save data: all defined filetypes */

		while ( (*error == 0) && fthis)
		{
			fwork = *fthis;

			*error = CfgSave(file, ft_table, lvl + 1, CFGEMP); 

			fthis = fthis->next;
		}
	}
	else
	{
		/* Load data; one filetype */

		memset( &fwork, 0, sizeof(FTYPE) ); /* must set ALL of .filetype to 0 !!! */

		*error = CfgLoad(file, ft_table, (int)sizeof(SNAME) - 1, lvl + 1); 

		if (*error == 0 )
		{
			if ( fwork.filetype[0] == 0 )
				*error = EFRVAL;
			else
			{
				if (
						lsadd(  (LSTYPE **)ffthis,
		    				sizeof(FTYPE),
		                	(LSTYPE *)&fwork,
		                	END,
		                	copy_ftype) == NULL
					)
						*error = ENOMSG; /* there was an allert in lsadd */
			}
		}
	}
}


/*
 * Configuration table for filetypes (filename masks)
 */

CfgEntry filetypes_table[] =
{
	{CFG_HDR, 0, "*"     },
	{CFG_BEG},
	{CFG_NEST,0, "*type", one_ftype },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Load or save all filetypes
 */

CfgNest ft_config
{
	fthis = filetypes;
	ffthis = &filetypes;

	ft_table[0].s[0] = 'f'; 
	filetypes_table[0].s =    "filetypes";
	filetypes_table[2].s[0] = 'f';
	filetypes_table[3].type = CFG_ENDG;

	*error = handle_cfg(file, filetypes_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, rem_all_filetypes, ft_default);
}



