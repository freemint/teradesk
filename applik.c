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


#include <np_aes.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <tos.h>
#include <vdi.h>
#include <mint.h>
#include <library.h>
#include <xdialog.h>
#include <limits.h>


#include "resource.h"
#include "desk.h"
#include "error.h"
#include "startprg.h"
#include "xfilesys.h"
#include "file.h"
#include "events.h"
#include "config.h"	
#include "prgtype.h"
#include "font.h"
#include "window.h"
#include "copy.h"
#include "dir.h"
#include "lists.h"
#include "slider.h" 
#include "filetype.h" 
#include "applik.h"
#include "open.h"
#include "va.h"


APPLINFO
	awork,			/* work area for editing */ 
	*applikations;	/* list of installed apps data */

int
	naap;			/* number of apps assigned to a filetype */

extern PRGTYPE 
	*prgtypes;   	/* list of executable filetypes */

static char 
	*defcml = "%f"; /* default command line for programs */

static SNAME
	prevcall;		/* name of the last started ttp */

static boolean
	nodocs = FALSE;	/* to copy documenttypes when copying apps */

int trash_or_print(ITMTYPE type);	/* from ICON.C */


/* 
 * Find an application specified by its name among installed ones; 
 * Comparison is made upon full path+name, and an exact match is required,
 * contrary to routine find_lsitem in lists.c which works upon the short name
 * (part of the LSTYPE/FTYPE/ICONTYPE/PRGRYPE) and allows wildcards.
 * Return NULL if appliaction not found.
 * Name matching is NOT case sensitive.
 */

APPLINFO *find_appl(APPLINFO **list, const char *program, int *pos) 
{
	APPLINFO *h = *list; 

	if (pos)
		*pos = -1;			/* allow NULL when position not needed */

	if ( program )	
	{
		while (h)
		{
			if (pos)
				(*pos)++;	

			if ( (stricmp(program, h->name) == 0) )
				return h;

			h = h->next;
		}
	}

	return NULL;
}


/* 
 * Remove an app definition, its command line and list of documentypes;
 * This code  operates in a way similar to general "lsrem" routine in 
 * lists.c, except that it deallocates name, commandline and documenttypes 
 * as well.
 * Note: beware of recursive calls: lsrem_all-->rem_appl-->lsrem_all-->lsrem
 */

void rem_appl(APPLINFO **list, APPLINFO *appl)
{
	APPLINFO 
		*f = *list,		/* pointer to first, then current item in the list */ 
		*prev = NULL;	/* pointer to previous item in the list */


	/* Search for a pointer match */

	while (f && (f != appl))
	{
		prev = f;
		f = f->next;
	}

	/* If match found, remove that item from the list */

	if ( f )
	{
		if (prev == NULL)
			*list = f->next;
		else
			prev->next = f->next;

		/* Deallocate strings pointed to from this structure */

		free(f->name);
		free(f->cmdline);
		free(f->localenv);

		/* Remove associated documenttypes */

		lsrem_all_one((LSTYPE **)(&(f->filetypes))); /* deallocate list of documenttypes */
		free(f);
	}
}


/*
 * Copy an application definition from one location to another;
 * Space for both has to be already allocated (size of APPLINFO); 
 * for name, commandline, environment and documenttypes, memory is allocated.  
 * This routine actually copies the list of associated filetypes,
 * as well as the other strings (i.e. it doesn't just copy pointers).
 * If 'nodocs' is true, documenttypes, command line and environment 
 * are not copied.
 * Note: NULL pointers are checked for in strdup() and returned
 */
 
static void copy_app( APPLINFO *t, APPLINFO *s )
{
	copy_prgtype ( (PRGTYPE *)t, (PRGTYPE *)s );

	t->name = strdup(s->name);
	t->fkey = s->fkey;
	t->cmdline = NULL;
	t->localenv = NULL;
	t->filetypes = NULL;

	if(nodocs)
		return;

	/* Note: If lsadd() fails, cmdline and localenv will remain orphaned */

	if(t->name)
	{
		t->cmdline = strdup(s->cmdline);
		t->localenv = strdup(s->localenv);

		copy_all ( (LSTYPE **)(&(t->filetypes)), (LSTYPE **)(&(s->filetypes)), sizeof(FTYPE), copy_ftype);
	}
}


/*
 * Find (or create!) information about an application in a list;
 * input: filename
 * output: APPLINFO data structure
 * if filetype has not been defined, default values are set
 */

void appinfo_info
( 
	APPLINFO **list,	/* list of installed apps */ 
	char *name, 		/* full name for shich to search */
	int dummy,			/* currently not used */
	APPLINFO *appl		/* output information */ 
)
{
	APPLINFO
		*a;

	/* Is this application already installed ? */

	a = find_appl( list, name, NULL );	/* HR 120803 allow NULL when pos not needed */

	if ( a )
	{
		/* Yes, use its data */

		copy_app(appl, a);
	}
	else
	{
		/* 
		 * Not found, set default program type according to name. 
		 * This routine also initiates the value of ->flags
		 */

		prg_info( &prgtypes, name, 0, (PRGTYPE *)appl );

		/* Pass the name, if given; otherwise set an empty string */

		appl->name = strdup ((name) ? name : empty);

		/* 
		 * Create default command line and environment; 
		 * set no documenttypes and F-key 
		 */

		appl->cmdline = strdup(defcml);
		appl->localenv = strdup(empty);
		appl->filetypes = NULL; 
		appl->fkey = 0;
		appl->flags &= ~(AT_EDIT | AT_AUTO | AT_SHUT | AT_VIDE | AT_SRCH | AT_FFMT | AT_VIEW | AT_COMP | AT_CONS | AT_RBXT);
	}
}


/*
 * Check duplicate application assignment; this routine differs
 * from check_dup in lists.c in that the full app path+name is compared.
 */

boolean check_dup_app( APPLINFO **list, APPLINFO *appl, int pos )
{
	APPLINFO 
		*f = *list;		/* pointer to current item in the list */

	int 
		i = 0;			/* item counter */


	while (f)
	{
		if ( i != pos )
		{
			if ( strcmp( f->name, appl->name ) == 0 )
			{
				alert_printf( 1, AFILEDEF, f->shname );
				return FALSE;
			}
		}

		i++;
		f = f->next;
	}

	return TRUE;
}


/* 
 * Put into dialog field the F-key assigned to application 
 */

static void set_fkey(int fkey)
{
	char 
		*applfkey = applikation[APFKEY].ob_spec.tedinfo->te_ptext;


	if (fkey)						/* assigned */
		itoa(fkey, applfkey, 10);	/* convert integer to string */
	else
		*applfkey = 0;				/* empty string */
}


/*
 * Reset F-key "fkey" assigned to any of the installed applications
 */

static void unset_fkey(APPLINFO **list, int fkey)
{
	APPLINFO 
		*f = *list;

	while (f)
	{
		if ( f->fkey == fkey )
			f->fkey = 0;

		f = f->next;
	}
}


/*
 * Check if a function key has already been assigned to another application. 
 * Return TRUE if it is OK to set F-key now
 */

static boolean check_fkey(APPLINFO **list, int fkey, int pos)
{
	APPLINFO 
		*f = *list;		/* pointer to current item in the list */

	int 
		i;				/* item counter */


	if ( fkey != 0 )
	{
		i = 0;

		while (f)
		{
			if ( (i != pos) && (f->fkey == fkey) )
			{
				/* Some other app is associated with this key */

				if (  alert_printf( 2, ADUPFLG, f->shname ) == 1 )
					return TRUE;
				else
					return FALSE;
			}

			i++;
			f = f->next;
		}
	}

	return TRUE;
}


/*
 * Check if a special application is already marked with "flag" at a position
 * other than "pos". If it is, an alert appears asking for confirmation 
 * to change the settings. 
 * Use pos=-1 to check if it is marked at all (no alert appears then).
 * Return FALSE if an application is already marked.
 * Return TRUE if it is OK to mark an application with this flag.
 * Note: bit flags can be combined.
 */

static boolean check_specapp( APPLINFO **list, int flag, int pos )
{
	APPLINFO
		*f = *list;

	int
		button = 0,
		i = 0;

	while( f )
	{
		if ( (i != pos) && ( (f->flags & flag) != NULL ) )
		{
			if ( pos != -1 )
				button = alert_printf( 2, ADUPFLG, f->shname );

			if ( button != 1 )
				return FALSE;
		}

		i++;
		f = f->next;
	}

	return TRUE;
}


/*
 * Reset special-app flags specified in "flags"
 * for all applications in the list
 */

static void unset_specapp(APPLINFO **list, int flags)
{
	APPLINFO
		*app = *list;
	

	while(app)
	{
		app->flags &= ~flags;
		app = app->next;
	}
}


/*
 * Update information about an installed application
 */

void app_update(wd_upd_type type, const char *fname1, const char *fname2)
{
	APPLINFO *info;

	if ((info = find_appl(&applikations, fname1, NULL)) != NULL)/* allow NULL when pos not needed */
	{
		if (type == WD_UPD_DELETED)
			rem_appl(&applikations, info);

		if (type == WD_UPD_MOVED)
		{
			char *h = strdup(fname2);

			if (h)
			{
				free(info->name);
				info->name = h;
			}
		}
	}
}


/* 
 * Create a "short" name of an application; it will be used for informational
 * purposes in dialogs opened to edit documenttypes and application type, and 
 * in some alerts. "dest" must be allocated beforehand and be at least a SNAME
 */ 

void log_shortname( char *dest, char *appname )
{
	*dest = 0;
	
	if (appname)
#if _MINT_
		cramped_name(fn_get_name(appname), dest, sizeof(SNAME));
#else
		strcpy(dest, fn_get_name(appname)); /* safe in single-TOS */
#endif
}


/* 
 * app_dialog: edit setup of a single application. 
 * Edited data is manipulated and returned in *appl; 
 * if operation is canceled, entry values in *appl should be unchanged;
 * **applist serves for checking against duplicates, etc.
 */

boolean app_dialog
(
	APPLINFO **applist, 	/* list to check for duplicates in */
	int pos,				/* position of item in the list */ 
	APPLINFO *appl,			/* edited data */	 
	int use					/* use of this dialog (add/edit) */
)
{
	XDINFO 
		info;				/* info structure for the dialog */

	FTYPE
		*list,				/* working copy of the list of filetypes */
		*newlist;			/* first same at list, then maybe changed in ft_dialog */

	VLNAME
		thisname,			/* name of the current application */
		thispath;			/* path of the current application */

	char
		*newenv,			/* pointer to the changed text */
		*newcmd,			/* pointer to the changed text */ 
		*newpath;			/* pointer to the changed text */

	int
		i,					/* local counter */ 
		startob,			/* start object for editing */
		title,				/* rsc index of dialog title string */
		button, 			/* code of pressed button */
		button2,			/* for a subdialog */
		fkey;				/* code of associated F-key */

	boolean
		stat = FALSE, 
		qquit = FALSE;		/* true when ok to exit loop */

	static const char 
		ord[] = {ISEDIT, ISAUTO, ISSHUT, ISVIDE, ISSRCH, ISFRMT, ISVIEW, ISCOMP, ISRBXT};


	list = NULL;
	newenv = NULL;
	newcmd = NULL;
	newpath = NULL;
	*thispath = 0;
	*thisname = 0;

	/* 
	 * Set dialog title(s); create copy of filetypes list if needed 
	 * i.e. if application setup is edited, copy list of filetypes to temporary;
	 * If application is added, there is nothing to copy
	 */

	if ( use & LS_EDIT )
	{
		title = DTEDTAPP;
		startob = APCMLINE;

		copy_all( (LSTYPE **)(&list), (LSTYPE **)(&appl->filetypes), sizeof(FTYPE), copy_ftype);
  	}
	else
	{
		title = (use & LS_WSEL ) ? DTINSAPP : DTADDAPP; /* Install or edit - set title */
		startob = APPATH;
		list = appl->filetypes;
	}

	newlist = list;

	rsc_title( applikation, APDTITLE, title );

	/*
	 * Copy application name, path, command line and environment to dialog fields;
	 * as application name is stored as path+name it has to be split-up
	 * in order to display path separately
	 */

	if(*(appl->name))
	{
		int error = x_checkname(appl->name, NULL);

		xform_error(error);

		if(!error)
			split_path( thispath, thisname, appl->name );
	}

	cv_fntoform(applikation, APNAME, thisname); /* xd_init_shift() inside */
	cv_fntoform(applikation, APPATH, thispath);
	strsncpy(envline, appl->localenv, sizeof(VLNAME));
	xd_init_shift(&applikation[APLENV], envline); 
	strsncpy(cmdline, appl->cmdline, sizeof(VLNAME)); 
	xd_init_shift(&applikation[APCMLINE], cmdline);

	/* Display alerts if strings were too long */

	if ( strlen(appl->cmdline) > XD_MAX_SCRLED || (strlen(appl->localenv) > XD_MAX_SCRLED) )
		alert_iprint(TCMDTLNG);

	/* Put F-key value, if any, into the dialog */

	set_fkey(appl->fkey);

	/* Open the dialog for editing application setup */

	if(chk_xd_open(applikation, &info) >= 0)
	{
		while (!qquit)
		{
			/* Get button code */

			button = xd_form_do(&info, startob);

			/* Get new application path and name from the dialog */

			cv_formtofn(thispath, applikation, APPATH);
			cv_formtofn(thisname, applikation, APNAME);
		 
			/* Create "short" name of this application */

			log_shortname( appl->shname, thisname );

			/* Do something appropriate for the button pressed */
		
			switch (button)
			{
				case SETUSE:
				{
					/* 
					 * Set special use of this application;
					 * Adjust limit value of i in two loops below
					 * to be equal to number of checkboxes in the 
					 * 'Special Use' dialog (currently 8).
					 * Dimension of ord[] above must be changed appropriately.
					 */
					for ( i = 0; i < 9; i++ )
						set_opt(specapp, appl->flags, AT_EDIT << i, (int)ord[i] );
	
					button2 = chk_xd_dialog(specapp, ROOT);
	
					if ( button2 == SPECOK )
					{
						for ( i = 0; i < 9; i++ )
							get_opt(specapp, &(appl->flags), AT_EDIT << i, (int)ord[i] );
					}	
	
					break;
				}
				case APPFTYPE:
				{
					/*
					 * Set associated document types; 
					 * must recreate dialog title afterwards because the same
					 * dialog is used
					 */
	
					ft_dialog( (const char *)(appl->shname), &newlist, (LS_DOCT | (use & LS_SELA)) );
					rsc_title(setmask, DTSMASK, title);
					break;
				}
				case APPPTYPE:			
				{
					/* 
					 * Define application execution parameters (program type)
					 * note: default values have already been set 
					 */
	
					prgtype_dialog( NULL, INT_MAX, (PRGTYPE *)appl, LS_APPL | LS_EDIT );
					break;
				}
				case APOK:
				{
					/* 
					 * Changes are accepted, do everything needed to exit dialog
	 				 * First, check for duplicate app installation 
					 */
	
					if ( !check_dup_app(applist, appl, pos ) )
						break;
	
					/* Then, check for duplicate F-key assignment */
	
					fkey = atoi(applikation[APFKEY].ob_spec.tedinfo->te_ptext);
	
					if ( !check_fkey(applist, fkey, pos) )
						break;
	
					/* Check for duplicate spec. application assignment */
	
					if ( !check_specapp( applist, (appl->flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_VIEW | AT_COMP | AT_CONS | AT_RBXT)), pos) )
						break;
	
					/* 
					 * Build full application name again,
					 * because it might have been changed by editing. 
					 * Use "thispath" and "thisname" as they are not needed anymore.
					 * Note that actual file is NOT renamed, just 
					 * the entry in the applications list, but a check is made
					 * if a file with the new name exists (it must exist)
					 */
	
					if ( *thispath  == 0 || *thisname == 0 )
					{
						alert_iprint(MFNEMPTY); /* can not be empty! */
						break;
					}
	
					if (make_path( thispath, thispath, thisname ) != 0)
					{
						alert_iprint(TPTHTLNG);
						break;
					}
	
					/* Does this application file exist at all ? */
	
					if ( !x_exist(thispath, EX_FILE) )
					{
						alert_printf(1, APRGNFND, appl->shname);
						break;
					}
	
					/* 
					 * If application path+name have been changed,
					 * allocate a new one 
					 */
	
					if ( strcmp(thispath, appl->name) != 0 )
					{
						if ( (newpath = strdup(thispath)) != NULL )
						{
							free(appl->name);
							appl->name = newpath;
						}
						else
							goto exit2;
					}
	
					/* 
					 * If command line string has been changed (only then),
					 * allocate a new one 
					 */
				 
					if ( (strcmp(cmdline, appl->cmdline) != 0) )
					{
						if ((newcmd = strdup(cmdline)) != NULL)
						{
							free(appl->cmdline);
							appl->cmdline = newcmd;
						}
						else
							goto exit2;
					}
	
					/* Same for local environment */
	
					strip_name(envline, envline);
					if ( (strcmp(envline, appl->localenv) != 0) )
					{
						if ((newenv = strdup(envline)) != NULL)
						{
							free(appl->localenv);
							appl->localenv = newenv;
						}
						else
							goto exit2;
					}
	
					/* 
					 * How about documenttypes? If there has been a change in
					 * address, point to new list, clear old one; otherwise 
					 * clear the copy. Note: currently there is no check
					 * whether the same documenttype has been assigned to another
					 * application, it is only checked (in ft_dialog) whether 
					 * there are duplicate documenttypes set for this application.
					 * Anyway, it is possible to assign one documenttype to
					 * several applications.
					 */	
	
					if ( newlist != list )
					{
						/* 
						 * There have been changes in ft_dialog
						 * (known because list address has changed)
						 * remove original list, accept newlist, the
						 * one pointed to by "list" has been destroyed
						 * in ft_dialog
						 */
	
						lsrem_all_one((LSTYPE **)(&appl->filetypes));
						appl->filetypes = newlist;
					}
					else
					{
						/* there has been no changes, destroy copy of list */
	
						lsrem_all_one((LSTYPE **)(&list));
					}
	
					/* Set also the f-key. (remove all other assignments of this key) */
	
					unset_fkey(applist, fkey);
					appl->fkey = fkey;
	
					/* Reset special app flags from other applications */
	
					unset_specapp( applist, (appl->flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_VIEW | AT_COMP | AT_CONS | AT_RBXT)) );
	
					qquit = TRUE;
					stat = TRUE;
					break;
				}
				default:
				{
					exit2:

					/* Anything else, like exit without OK */
	
					/* Copy of documenttype list has to be destroyed */
	
					lsrem_all_one((LSTYPE **)(&newlist));
					qquit = TRUE;
				}
			}

			/* Set all buttons to normal state */

			xd_change(&info, button, NORMAL, (qquit == FALSE) ? 1 : 0);
		}

		/* Close the dialog and exit */

		xd_close(&info);
	}

	return stat;
}


/* 
 * Use these list-specific functions to manipulate applications lists: 
 */

#pragma warn -sus
static LS_FUNC aplist_func =
{
	copy_app,		/* lscopy */
	rem_appl,		/* lsrem */
	appinfo_info,	/* lsitem */
	find_appl,		/* lsfinditem */
	app_dialog		/* ls_dialog */
};
#pragma warn .sus


/* 
 * Handle installing, editing, selecting and removing applications.
 * If "Install Applicaton" menu is selected without any program file being
 * selected in a dir window, a dialog is shown with a list of installed
 * applications, and options to add, remove or edit them, for which 
 * another dialog is opened.
 * If installation is to be done upon an applicaton selected in a dir window,
 * a dialog opens for directly setting application parameters.
 */

void app_install(int use, APPLINFO **applist)
{
	int
		title, 
		theuse = use | LS_APPL;


	/* Set dialog title */

	if ( use & LS_SELA )
	{
		title = DTSELAPP;
		rsc_title(setmask, FTTEXT, TAPP ); 
		setmask[FILETYPE].ob_flags &= ~(EDITABLE | HIDETREE);
		obj_unhide(setmask[FTTEXT]);

		if(selitem)
			cv_fntoform(setmask, FILETYPE, (*applist)->shname);
		else
			*(setmask[FILETYPE].ob_spec.tedinfo->te_ptext) = 0; 
	}
	else
		title = DTINSAPP; 

	rsc_title(setmask, DTSMASK, title);

	/* 
	 * Edit applications list: add/delete/change... 
	 * Accepting or canceling changes is handled within list_edit,
	 * so there is nothing to do after that
	 */

	list_edit( &aplist_func, (LSTYPE **)applist, 1, sizeof(APPLINFO), (LSTYPE *)&awork, theuse); 

	rsc_title(setmask, FTTEXT, TAPP); 
	obj_hide(setmask[FILETYPE]);
	obj_hide(setmask[FTTEXT]);
}


/* 
 * Find installed application which is specified for "file" filetype.
 * Return pointer to information block for that app.
 * If more than one application us specified for the filetype,
 * the listbox dialog is opened so that the application can be selected.
 */

APPLINFO *app_find(const char *file, boolean dial)
{
	APPLINFO 
		*h = applikations, 
		*d = NULL;		/* temporary list of assigned apps */

	FTYPE 
		*t;				/* list of documenttypes */


	naap = 0;			/* number of assigned apps */
	selitem = NULL;		/* nothing found yet */
	nodocs = TRUE;		/* do not copy documenttypes */

	/* Find all applications associated with this filetype; form a list */

	while (h)
	{
		t = h->filetypes;

		while (t)
		{
			/* 
			 * Match name pattern; for network objects allow only 
			 * applications assigned for network objects
			 * Or better not. It would reduce flexibility!
			 */

			if(cmp_wildcard(file, t->filetype) /* && (!x_netob(file) || x_netob(t->filetype)) */ )
			{
				if
				(
					lsadd_end
					( 
						(LSTYPE **)&d, 
						sizeof(awork), 
						(LSTYPE *)h, 
						copy_app 
					) == NULL 
				)
					goto nomore;

				naap++;
				break; /* stop scanning documenttypes for this app */
			}

			t = t->next;
		}

		h = h->next;
	}

	nomore:;

	selitem = (LSTYPE *)d;

	/* If more than one app found, open the dialog, but disable editing */

	if(naap > 1 && dial)
	{
		obj_disable(setmask[FTADD]);
		obj_disable(setmask[FTDELETE]);
		obj_disable(setmask[FTCHANGE]); 

		app_install(LS_SELA, &d); /* returns pointer to selected app in selitem */

		obj_enable(setmask[FTADD]);
		obj_enable(setmask[FTDELETE]);
		obj_enable(setmask[FTCHANGE]); 
	}

	/* Now find (by name) the selected application in the original list */

	if(selitem)
		(APPLINFO *)selitem = find_appl(&applikations, ((APPLINFO *)selitem)->name, NULL);

	/* Remove temporary list of applications */

	lsrem_all((LSTYPE **)&d, rem_appl);

	nodocs = FALSE;
	return (APPLINFO *)selitem;
}


/*
 * Find the (short) name of the first application associated with a filename
 */

char *app_find_name(const char *fname, boolean full)
{
	APPLINFO
		*theapp;


	theapp = app_find(fname, (full) ? TRUE : FALSE);

	if (theapp)
		return (full) ? theapp->name : theapp->shname;
	else
		return NULL;
}


/* 
 * Find installed application for which is specified the "fkey" function key;
 * Return pointer to information block for that app
 */

APPLINFO *find_fkey(int fkey)
{
	APPLINFO
		*h = applikations;


	while (h)
	{
		if (h->fkey == fkey)
			return h;

		h = h->next;
	}

	return NULL;
}


/*
 * Copy a command line to another destination (allocate space) and 
 * change all quotes to the specified character. Actual command is copied
 * to index #1 of destination; location #0 is a placeholder
 * for commandline length.
 * Note: 3 bytes are added in strlenq()
 */
 
static char *requote_cmd(char *cmd)
{
	char
		*fb, 
		*q = malloc_chk(strlenq(cmd));


	if(q)
	{
		*q = '*';
		strcpyrq(q + 1, cmd, '"', &fb);
	}

	return q;
}


/*
 * Build a command line from the format string and the selected objects.
 * Commad line variables understood: %f (fullname), %n (name), %p (path)
 * Return pointer to an allocated space containing the complete command line,
 * including length placeholder in the first byte. Arguments in this
 * command line are separated by blanks.
 * Note: parameter 'sellist' is locally modified.
 */

static char *app_build_cml
(
	const char *format,	/* (Template for) the command lint */
	WINDOW *w,			/* Window where the selection is made */
	int *sellist,		/* List of selected items */
	int n				/* Number of selected items */
)
{
	char 
		*d,					/* pointer to a position in 'build' */
		*tmp,				/* pointer to a temporary storage */
		*build,				/* buffer for building the command name */
		*realname,			/* name to be used- either item name or link target */
		*qformat,			/* requoted command line */
		h, 					/* pointer to a character in command format */
		hh;					/* lowercase of the above */

	int 
		i, 					/* item counter */
		error,				/* anything */
		mes;				/* message identification */


	build = NULL;
	realname = NULL;
	error = -1;

	/* Convert any quotes in the command line to double quotes */

	if((qformat = requote_cmd((char *)format)) != NULL)
	{
		char *c = qformat;			/* pointer to a position in qformat */
		long ml;					/* needed buffer length for the command line */
		ITMTYPE type;				/* current item type */

		/* 
		 * Allocate a little more memory, because a couple of characters
		 * may be added to the command line for each name in the list.
		 * Also, a command line may contain other text beside the names.
		 */

		ml = (n + 2) * sizeof(VLNAME) + strlen(qformat);

		if ( (build = malloc_chk(ml)) == NULL )
			goto error_exit2; 

		memclr(build, ml);
		d = build;

		/* Process input (i.e. format line) until 0-byte encountered */

		while ((h = *c++) != 0)
		{
			if (h == '%') 
			{
				/* A command line variable may have been encountered */

				h = *c++;
				hh = tolower(h);

				/* Handled: %f, %n, %p */

				if ( (hh == 'f') || (hh == 'n') || (hh == 'p') )
				{
					/*
					 * Command line variables are ignored if there are no files,
					 * but other content of the command line is processed
					 */

					if ( n > 0 )
					{
						/* Add each item from the list... */

						for (i = 0; i < n; i++)
						{
							type = itm_type(w, *sellist);

							if (type != ITM_NETOB && (mes = trash_or_print(type)) != 0)
							{
								alert_cantdo(mes, MNODRAGP);
								continue;
							}
							else
							{
								char *np;

								/* separate arguments by blanks */	

								if (i)
									*d++ = ' ';

								/* Get the full real name of an item (allocate) */

								if ((realname = itm_tgtname(w, *sellist)) == NULL)
									goto error_exit2;

								tmp = realname;	/* for hh == 'f' or hh == 'p' */

								np = fn_get_name(realname);

								/* 
								 * Note: if %n is specified and realname is
								 * a root path, an empty string will be set
								 */

								if(hh == 'n')
									tmp = np;

								if(hh == 'p')
									*np = 0;

								/* (now tmp points to a position in realname) */

								if (isupper(h))
									strlwr(tmp);

								/* Would the command be too long ? */

								if(strlenq(tmp) + strlen(build) >= ml)
								{
									xform_error ( ECOMTL ); 
									goto error_exit2;
								}
							
								d = strcpyq(d, tmp, '"');

								/* This frees the area that tmp points to */

								free(realname);
								realname = NULL;
							}

							sellist++;

						} /* for... */
					}
					else
					{
						/* must get rid of blanks after %f, etc. */
						c = nonwhite(c);
					}						
				}
				else  /* neither %f nor %n nor %p */
				{
					*d++ = h;
				}
			}
			else /* not a command line variable */
			{
				*d++ = h;
			}
		} /* while */

		/* add zero termination byte */

		*d++ = 0;

		/* Everything is OK, copy to destination as much as is needed */

		tmp = strdup( build );
		error = 0;
	}
 
	/* Come here at end or with errors */

	error_exit2:;

	/* Free whatever was temporarily allocated */

	free(build);
	free(qformat);
	free(realname);

	return (error == 0) ?  tmp : NULL;
}


/* 
 * Attempt to extract a valid path from a command line. Return the pointer to 
 * a new string containing this path. Memory is allocated in this routine.
 * If a string resembling a path is not found in the line, return NULL
 */

static char *app_parpath(char *cmline)
{
	char 
		*p, 			/* current position in the string */
		*p1,
		fqc;			/* first quote character */

	VLNAME 
		thepath;		/* extracted path */

	boolean 
		q = FALSE;		/* quoting in progress */



	p = cmline;			/* start looking here */
	p1 = NULL;
	fqc = 0;

	while(*p)
	{
		/* 
		 * Beginning of a path was found (see below), now find the end 
		 * (end is recognized by any character with code lesser than '!'
		 * that is not between quotes)
		 */

		if(p1 && ((*p == 0) || (q && *p == fqc && p[1] != fqc) || (!q && *p < '!')))
		{
			strsncpy(thepath, p1, p - p1 + 1);
			return fn_get_path(thepath); /* allocates space */			
		} 

		/* 
		 * Find the beginning of the first path. It must start
		 * with a drive specification (A:\ to Z:\)
		 */

		if( p1 == NULL && isdisk(p) )
			p1 = p;

		/* Toggle quoting */

		if ((*p == fqc) || (!fqc && (*p == 39 || *p == '"')) && p[1] != *p)
		{
			/* This is a single quote */

			fqc = 0;

			if(!q)
				fqc = *p;

			q = !q;
		}
		else if(q && *p == fqc && p[1] == fqc)
			p++; 

		p++;
	}

	return NULL;
}


/*
 * Start a program or an installed application.
 *
 * Parameters:
 *
 * program	- name of program to start. Is allowed to be NULL if
 *			  'app' is not NULL
 * app		- application to start. Is allowed to be NULL if
 *			  'program' is not NULL
 * w		- window from which objects were dragged to the
 *			  application, or NULL if no objects were dragged to
 *			  the program (i.e. program was started directly)
 * sellist	- list of selected objects in 'w'
 *            or (char *)sellist to pass an explicit command line if n = -1
 * n		- number of selected objects in 'w', or 0 if no objects
 *			  are dragged to the program, or -1 if a verbatim command line
 *            is passed
 * kstate   - keyboard state (shift,alternate...)
 *
 * Result: TRUE if succesfull, FALSE if not.
 */

boolean app_exec
(
	const char *program,
	APPLINFO *app,
	WINDOW *w,
	int *sellist,
	int n,
	int kstate
)
{
	APPLINFO 
		*appl;					/* Application info of program. */

	PRGTYPE
		*thework;				/* pointer to edit area for program type */

	long
		cmllen,					/* length of the command line */
		limmem;					/* mem.limit in multitask (bytes) */

	const char 
		*cl_format,				/* Format of commandline. */
		*name,					/* Program (real) name. */
		*par_path = NULL,		/* Path of the 1st parameter */
		*def_path;				/* Default path of program. */

	char
		*theenv = NULL,			/* local environment for the application */
		*thecommand = NULL;		/* command line */

	ApplType 
		appl_type;				/* Type of program (GEM or not) */

	SNAME
		thiscall;				/* name of this ttp */

	boolean 
		argv,					/* Use ARGV protocol flag. */
		single,					/* don't multitask (Magic) */
		back,					/* run in background when possible */
		result = FALSE;			/* return result */


	/* Default command line format... */

	cl_format = defcml;

	/* 
	 * Maybe a link was specified. Find the real object, because
	 * its type has to be determined. If the object is not a link,
	 * its name will be just copied in x_fllink().
	 * Remember to deallocate 'name' later.
	 */

	name = x_fllink((char *)program);

	/* If application is NULL, find out if 'program' is installed as an application. */

	appl = (app == NULL) ? find_appl(&applikations, name, NULL) : app; 

	if (appl == NULL && name != NULL)
	{
		/* 
		 * We do not have any application data, just the program name.
		 * Abort if a program file with this name does not exist. 
		 */

		log_shortname((char *)thiscall, (char *)name);

		if (!x_exist(name, EX_FILE))
		{
			alert_printf(1, APRGNFND, thiscall);
			goto errexit;
		}

		/* 
		 * 'program' is not installed as an application. 
		 * Use default settings for this executable file type.
		 * Default data are filled into pwork.
		 * Default command line is a full filename passed.
		 * Default path is either the program directory path or the top window 
		 * path (which may also default to program directory path if no
		 * directory window is topped).
		 */

		prg_info(&prgtypes, name, 0, &pwork);
		thework = &pwork;
	}
	else
	{
		/* 
		 * 'app' is not NULL or 'program' is installed as an
		 * application. Use the settings set by the user in the
		 * 'Install application' dialog. 
		 * Local environment is passed only if that string is not empty,
		 * otherwsise it is set to NULL.
		 */

		if (appl->cmdline && appl->cmdline[0]) /* there is a command line */
			cl_format = appl->cmdline;

		free(name);
		name = x_fllink(appl->name); /* allocate new name */

		theenv = (appl->localenv && appl->localenv[0]) ? appl->localenv : NULL;
		thework = (PRGTYPE *)appl;

		/* 
		 * If the application file does not exist, ask the user to
		 * locate it or to remove it. 
		 */

		if(name)
		{
			log_shortname((char *)thiscall, (char *)name);

			if (!x_exist(name, EX_FILE))
			{
				char *fullname;
				int button;

				if ((button = alert_printf(1, AAPPNFND, thiscall)) == 2)	/* remove */
					rem_appl(&applikations, appl); 
		
				if (button > 1)						/* remove or cancel */
					goto errexit;

				/* Locate the file using file selector */

				if ((fullname = locate(name, L_PROGRAM)) == NULL)
					goto errexit;

				/* 
				 * Fullname is not NULL.
				 * Maybe a link was selected. Locate real object.
				 * If this fails, old settings are kept
				 */

				free(name); /* have fullname now; don't need 'name' */

				if ( (name = x_fllink(fullname)) != NULL )
				{
					/* 
					 * Replace existing application name.
					 * then create a short name here, or applications list 
					 * in the dialog will look wrong  
					 */

					free(appl->name);
					appl->name = strdup(name);
					log_shortname( appl->shname, appl->name );
				}

				free(fullname);
			}
		}
	}

	/* 
	 * Set some program-type details as proper boolean values 
	 * note: form of the assignment given below appears to give the smallest binary
	 */

	argv = FALSE;
	single = FALSE;
	back = FALSE;

	if((thework->flags & PT_ARGV) != 0)
		argv = TRUE;

	if ((thework->flags & PT_SING) != 0)
		single = TRUE;

	if((thework->flags & PT_BACK) != 0)
		back = TRUE;

	limmem = thework->limmem;
	appl_type = thework->appl_type;

	/* 
	 * Further action depends on the type of the program and whether
	 * there are filenames passed to it.
	 * The commandline string will be formed in this segment.
	 */

	if ( (n == 0) && ( (appl_type == PTTP) || (appl_type == PGTP) ) )
	{
		/* 
		 * If there are no files passed to the program (n is 0) and
		 * the program is a TTP or GTP program, then ask the user to
		 * enter a command line.
		 * Remember the command line if the same ttp program
		 * is started again (remember name of the program as well)
		 */

		log_shortname( (char *)thiscall, (char *)name );

		if ( *prevcall == 0 || strcmp( thiscall, prevcall ) != 0 )
		{
			*ttpline = 0;	/* clear the dialog field */
			strcpy( prevcall, thiscall );
		}

		/* Don't start the application after all if not OK'd */

		if (chk_xd_dialog(getcml, CMDLINE) == CMLOK)
		{
			/* 
			 * Ttpline below is from the dialog. Copy any possible length,
			 * and put command line length (or 127) into first byte
			 */

			thecommand = requote_cmd(ttpline);
		}
	}
	else if(n == -1)
	{
		thecommand = requote_cmd((char *)sellist);
		*prevcall = 0;
	}
	else
	{
		/* 
		 * There are filenames passed, or this is not a TTP/GTP program;
		 * build the command line. Note: this command line may contain
		 * quotes and spaces 
		 */

		thecommand = app_build_cml(cl_format, w, sellist, n);
		*prevcall = 0;
	}

	if(thecommand != NULL)
	{
		/* Set the appropriate default directory */

		if ( (thework->flags & PD_PDIR) != 0 )
		{
			/* default path is to the program directory */
			def_path = NULL;
		}
		else if ( ((thework->flags & PD_PPAR) != 0) && thecommand[0] )
		{
			/* default path is to the first parameter of the command */

			par_path = app_parpath((char *)(&thecommand[1]));
			def_path = par_path;
		}
		else
		{
			/* default path is to the top window */
			def_path = wd_toppath();
		}

		/* 
		 * Do not use ARGV on programs not installed as applications
		 * if the command line is sufficiently short. Do not use ARGV
		 * if there is no command line at all.
		 * (by now 'thecommand' is allocated in any of the branches above)
		 * I.e. -some- form of command line always exist, even if an empty string.
		 * Alternatively, force the use of ARGV if so said.
		 */

		strip_name(thecommand + 1, thecommand + 1);
		cmllen = strlen(thecommand + 1);
		thecommand[0] = (cmllen > 125) ? 127 : (char)cmllen;

		if ( !cmllen || (!find_appl(&applikations, name, NULL) && (!fargv && (cmllen < 126))) )
			argv = FALSE;

		/* Check if the commandline is too long (max 125 if ARGV is not recognized)  */

		if ( !argv && (cmllen > 125)  )
			xform_error ( ECOMTL ); 
		else	/* No error, start the program. */
		{
			if ( name )
			{
				/* Note: something is wrong here! see va_start_prg in va.c */

				if (!va_start_prg(name, appl_type, &thecommand[1]))
					start_prg(name, thecommand, def_path, appl_type, argv, single, back, limmem, theenv, kstate);

				result = TRUE; /* Only now can it be true */
			}
		}
	}	

	/* Program may jump here in case of an error */

	errexit:;

	onfile = FALSE;

	free(name);
	free(thecommand);
	free(par_path);

	return result;
}


/********************************************************************
 *																	*
 * Funkties voor initialisatie, laden en opslaan.					*
 *																	*
 ********************************************************************/

/* 
 * Initialize the list of installed applications 
 */

void app_init(void)
{
	applikations = NULL;
#ifdef MEMDEBUG
	atexit(app_default);
#endif
	*prevcall = 0;
}


/* 
 * Remove all applications from the list.
 * Note: recursive calls occur here: 
 * app_default-->rem_all-->rem_appl-->rem_all-->rem
 */

void app_default(void)
{
	lsrem_all((LSTYPE **)(&applikations), rem_appl);
}


/*
 * A structure used for saving and loading application data
 */

typedef struct
{
	VLNAME 
		name,
		cmdline,
		localenv;
	      
	FTYPE 
		ftype;
} SINFO;

static
SINFO this;


/*
 * Save or load program type configuration for one application
 */

static CfgNest this_atype
{
	*error = 0;

	prg_table[0].s = "atype";
	prg_table[2].type |= CFG_INHIB;

	if ( io == CFG_SAVE )
	{
		copy_prgtype( &pwork, (PRGTYPE *)(&awork) );
		*error = CfgSave(file, prg_table, lvl, CFGEMP);
	}
	else
	{
		memclr(&pwork, sizeof(pwork));

		*error = CfgLoad(file, prg_table, (int)sizeof(SNAME), lvl); 

		if ( pwork.appl_type > PTTP ) /* PTTP is the last one */
			*error = EFRVAL; 		
		else
			copy_prgtype( (PRGTYPE *)(&awork), &pwork );
	}
}


/*
 * Clear a list of associated doctypes
 */

static void rem_all_doctypes(void)
{
	lsrem_all_one((LSTYPE **)(&awork.filetypes));
}


/*
 * Load or save configuration for associated documenttypes 
 */

static CfgNest dt_config
{
	char *sss = "dtype";
	fthis = awork.filetypes;
	ffthis = &(awork.filetypes); 
	ft_table[0].s = sss; 
	filetypes_table[0].s = "doctypes";
	filetypes_table[2].s = sss;
	filetypes_table[3].type = CFG_END;

	*error = handle_cfg(file, filetypes_table, lvl, (CFGEMP | ((fthis) ? 0 : CFGSKIP)), io, rem_all_doctypes, NULL);
}


/*
 * Configuration table for one application
 */

static CfgEntry app_table[] =
{
	{CFG_HDR,  "app" },
	{CFG_BEG},
	{CFG_S,    "path", 	this.name	  },
	{CFG_S,    "cmdl", 	this.cmdline  },
	{CFG_S,    "envr", 	this.localenv },
	{CFG_D,    "fkey", 	&awork.fkey	  },
	{CFG_NEST, "atype",    this_atype   },
	{CFG_NEST, "doctypes", dt_config    },
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configuration for one application (save all applications)
 */

static CfgNest one_app
{
	*error = 0;

	if (io == CFG_SAVE)
	{
		APPLINFO *h = applikations;

		while ((*error == 0) && h)
		{
			/* 
			 * It is assumed that strings will never be too long
			 * (length should be checked in the dialog)
			 * Now copy the strings to work area
			 */

			awork = *h; 
			strcpy(this.name, awork.name);
			strcpy(this.cmdline, awork.cmdline);
			strcpy(this.localenv, awork.localenv);

			*error = CfgSave(file, app_table, lvl, CFGEMP); 
	
			h = h->next;
		}
	}
	else
	{
		memclr(&this, sizeof(this));
		memclr(&awork, sizeof(awork));

		*error = CfgLoad(file, app_table, (int)sizeof(VLNAME), lvl); 

		if ( *error == 0 )				/* got one ? */
		{
 			if ( this.name[0] == 0 )
				*error = EFRVAL;
			else
			{
				/* For awork, strings are just pointed to, not copied */

				awork.name = (char *)&this.name;
				awork.cmdline = (char *)&this.cmdline;
				awork.localenv = (char *)&this.localenv;

				log_shortname( awork.shname, awork.name );

				/* 
				 * As a guard against duplicate flags/keys assignment
				 * for each application loaded, all previous spec. app
				 * or Fkey assignments (to that key or spec. app flags) 
				 * are cleared here, so only the last assignment remains.
				 * Note: if adding the application to the list fails
				 * a few lines later, this will leave Teradesk without
				 * a spec app or this Fkey assignment. Might be considered
				 * to be a bug, but probably not worth rectifying.
				 */

				unset_specapp(&applikations, (awork.flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_VIEW | AT_COMP | AT_CONS | AT_RBXT) ) );

				if (awork.fkey > 20)
					awork.fkey = 0;

				unset_fkey(&applikations, awork.fkey );

				/* 
				 * Add this application to list. Note that there is
				 * a flaw in the code here: if lsadd_end() fails because 
				 * of insufficient memory, command line and environment
				 * for the new item may remain allocated and lost
				 */

				if 
				( 
					lsadd_end
					( 
						(LSTYPE **)&applikations, 
						sizeof(awork), 
						(LSTYPE *)&awork, 
						copy_app 
					) == NULL 
				)
				{
					lsrem_all_one((LSTYPE **)(&awork.filetypes));
					*error = ENOMSG;
				}
			}
		}
	}
}


/*
 * Configuration table for all applications
 */

static CfgEntry applications_table[] =
{
	{CFG_HDR,  "applications" },
	{CFG_BEG},
	{CFG_NEST, "app", one_app },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Configure all applications
 */

CfgNest app_config
{
	*error = handle_cfg(file, applications_table, lvl, CFGEMP, io, app_default, app_default);
}


/*
 * Start applications marked for a special use.
 * It can be: an editor, an autostart, a shutdown application, etc.
 * There can be only one editor, but several startup or
 * shutdown applications. 
 * There is a delay of 1 or 2 seconds between autostart apps
 * but a delay of 6 seconds between shutdown apps.
 * 
 * Note: this routine can be improved, to better handle
 * simultaneous or sequential launch of applications.
 *
 * Routine returns the number of application started 
 * (successfully or otherwise)
 */

int app_specstart(int flags, WINDOW *w, int *list, int nn, int kstate)
{
	APPLINFO
		*app = applikations;

	int
		n = 0,
		delayt = 1000;


	if ( (flags & AT_SHUT) || (flags & AT_VIDE) )
		delayt = 6000;
#if _MINT_
	else
		if ( mint || geneva )
			delayt = 2000;
#endif

	while (app != NULL)
	{
		/* 
		 * Note: it i essential that app_specstart() be executed
		 * once at TeraDesk startup in order for the two line below
		 * to be executed and xd_rbdckick set to 0 if necessary.
		 * This i done when TeraDesk searches for applications
		 * marked with AT_AUTO.
		 */

		if(app->flags & AT_RBXT)
			xd_rbdclick = 0;

		if ( ( app->flags & flags) != NULL ) /* correct type of application ? */
		{
			/* Delay a little bit before the next application */

			if ( n )
				wait(delayt); 

			n++;

			/* 
			 * Start a marked application. Beware that in some
			 * situations certain types should not be started twice
			 */

			if
			(
				(startup && (app->flags & AT_AUTO)) ||
				(shutdown && (app->flags & AT_SHUT)) ||
				(app->flags & (AT_VIDE | AT_FFMT))
			)
				onone = TRUE;	/* ignore this app if alrady running */

			app_exec(NULL, app, w, list, nn, kstate);

			onone = FALSE;
		}

		app = app->next;
	}
	return n;
}


