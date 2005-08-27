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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <tos.h>
#include <vdi.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>
#include <internal.h>


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
#include "dir.h"
#include "lists.h"
#include "slider.h" 
#include "filetype.h" 
#include "library.h"
#include "applik.h"
#include "open.h"
#include "va.h"


APPLINFO
	awork,			/* work area for editing */ 
	*applikations;	/* list of installed apps data */

extern PRGTYPE 
	*prgtypes;   	/* list of executable filetypes */

extern Options 
	options;		/* need to know cfg file version */

char 
	*defcml = "%f"; /* default command line for programs */

int trash_or_print(ITMTYPE type);
void fix_prgtype_v360(PRGTYPE *p); /* Compatibility issue; remove after V3.60 */


/* 
 * Find an application specified by its name among installed ones; 
 * Comparison is made upon full path+name, and an exact match is required,
 * contrary to routine find_lsitem in lists.c which works upon the short name
 * (part of the LSTYPE/FTYPE/ICONTYPE/PRGRYPE) and allows wildcards.
 * Return NULL if appliaction not found.
 */

APPLINFO *find_appl(APPLINFO **list, const char *program, int *pos) 
{
	APPLINFO *h = *list; 

	if (pos)
		*pos = -1;			/* allow NULL when pos not needed */

	if ( program )	
	{
		while (h)
		{
			if (pos)
				(*pos)++;	
			if ( (strcmp(program, h->name) == 0) )
				return h;
			h = h->next;
		}
	}

	return NULL;
}


/* 
 * Remove an app definition, its command line and list of documentypes;
 * Removed old code completely, new code made to operate similar to general 
 * "lsrem" routine in lists.c, except that it deallocates name, commandline 
 * and documenttypes as well.
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

		free(f->name);
		free(f->cmdline);
		free(f->localenv);

		lsrem_all((LSTYPE **)(&(f->filetypes)), lsrem); /* deallocate list of documenttypes */
		free(f);
	}
}


/*
 * Copy an application definition from one location to another;
 * Space for both has to be already allocated (size of APPLINFO); 
 * for name, commandline, environment and documenttypes, 
 * memory is allocated here.  
 */
 
static void copy_app( APPLINFO *t, APPLINFO *s )
{
	APPLINFO *next = t->next;
	memclr(t, sizeof(APPLINFO) );
	t->next = next;

	copy_prgtype ( (PRGTYPE *)t, (PRGTYPE *)s );

	t->fkey = s->fkey;

	/* Note: NULL pointers are checked for in strdup() and returned */

	t->name = strdup(s->name);
	t->cmdline = strdup(s->cmdline);
	t->localenv = strdup(s->localenv);

	/* Note: copying below will commence even if allocation above failed ! */

	copy_all ( (LSTYPE **)(&(t->filetypes)), (LSTYPE **)(&(s->filetypes)), sizeof(FTYPE), copy_ftype);
}


/*
 * Find (or create!) information about an application in a list;
 * input: filename ;
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

		appl->name = strdup ( (name) ? name : empty);

		/* 
		 * Create default command line and environment; 
		 * set no documenttypes and F-key 
		 */

		appl->cmdline = strdup(defcml);
		appl->localenv = strdup(empty);
		appl->filetypes = NULL; 
		appl->fkey = 0;
		appl->flags &= ~(AT_EDIT | AT_AUTO | AT_SHUT | AT_VIDE | AT_SRCH | AT_FFMT | AT_CONS | AT_VIEW );
	}
}


/*
 * Check duplicate application assignment; this routine differs
 * from check_dup in lists.c in that the full app path+name is compared.
 * Perhaps this routine should be improved to check for duplicate
 * documenttype assignments (one document should not be assigned
 * to several applications)
 */

boolean check_dup_app( APPLINFO **list, APPLINFO *appl, int pos )
{
	int 
		i = 0;			/* item counter */

	APPLINFO 
		*f = *list;		/* pointer to current item in the list */


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
	APPLINFO *f = *list;

	while (f)
	{
		if ( f->fkey == fkey )
			f->fkey = 0;
		f = f->next;
	}
}


/*
 * Check if a function key has been assigned to another application 
 * Return TRUE if it is OK to set F-key now
 */

static boolean check_fkey(APPLINFO **list, int fkey, int pos)
{
	int 
		button,
		i = 0;			/* item counter */

	APPLINFO 
		*f = *list;		/* pointer to current item in the list */

	if ( fkey == 0 )
		return TRUE;

	while (f)
	{
		if ( (i != pos) && (f->fkey == fkey) )
		{
			button = alert_printf( 2, ADUPFLG, f->shname );

			if ( button == 1 )
				return TRUE;
			else
				return FALSE;
		}
		i++;
		f = f->next;
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
	int
		button,
		i = 0;

	APPLINFO
		*f = *list;

	while( f )
	{
		if (i != pos)
		{
			if ( (f->flags & flag) != NULL )
			{
				if ( pos != -1 )
					button = alert_printf( 2, ADUPFLG, f->shname );

				if ( button != 1 )
					return FALSE;
			}
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
	APPLINFO *app = *list;
	
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


/********************************************************************
 *																	*
 * Hulpfunkties voor install_appl()									*
 *																	*
 ********************************************************************/


/* 
 * Create "short" name of an application; it will be used
 * for informational purposes in dialogs opened to edit
 * documenttypes and application type, and in some alerts.
 * "dest" must be allocated beforehand.
 */ 

void log_shortname( char *dest, char *appname )
{
	char *n;

	*dest = 0;
	
	if (appname == NULL)
		return;
	
	n = fn_get_name(appname);

#if _MINT_
	if (mint)
		cramped_name( n, dest, (int)sizeof(SNAME) );
	else
#endif
		strsncpy( dest, n, 13 ); /* 8 (name) + 1 (dot) + 3 (ext) + 1 (term) */ 
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

	LNAME
		thisname,			/* name of the current application */
		thispath;			/* path of the current application */

	char
		*newenv = NULL,		/* pointer to the changed text */
		*newcmd = NULL,		/* pointer to the changed text */ 
		*newpath = NULL;	/* pointer to the changed text */

	XDINFO 
		info;				/* info structure for the dialog */

	FTYPE
		*list = NULL,		/* working copy of the list of filetypes */
		*newlist;			/* first same at list, then maybe changed in ft_dialog */

	static const char 
		ord[7] = {ISEDIT, ISAUTO, ISSHUT, ISVIDE, ISSRCH, ISFRMT, ISVIEW};


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

	split_path( thispath, thisname, appl->name );

	cv_fntoform(applikation, APNAME, thisname);
	cv_fntoform(applikation, APPATH, thispath);
	strsncpy(applcmd, appl->cmdline, XD_MAX_SCRLED); 
	xd_init_shift(&applikation[APCMLINE], applcmd);
	strsncpy(envline, appl->localenv, XD_MAX_SCRLED);
	xd_init_shift(&applikation[APLENV], envline); 

	/* Display alerts if strings were too long */

	if ( strlen(appl->cmdline) > XD_MAX_SCRLED || (strlen(appl->localenv) > XD_MAX_SCRLED) )
		alert_iprint(TCMDTLNG);

	/* Put F-key value, if any, into the dialog */

	set_fkey(appl->fkey);

	/* Open the dialog for editing application setup */

	xd_open(applikation, &info);

	while (!qquit)
	{
		/* Get button code */

		button = xd_form_do(&info, startob) & 0x7FFF;

		/* Get new application path and name from the dialog */

		cv_formtofn(thispath, applikation, APPATH);
		cv_formtofn(thisname, applikation, APNAME);
		 
		/* Create "short" name of this application */

		log_shortname( appl->shname, thisname );

		/* Do something appropriate for the button pressed */
		
		switch (button)
		{
		case SETUSE:
			/* 
			 * Set special use of this application;
			 * Adjust limit value of i in two loops below
			 * to be equal to number of checkboxes in the dialog (currently 7)
			 */
			{
				for ( i = 0; i < 7; i++ )
					set_opt(specapp, appl->flags, AT_EDIT << i, (int)ord[i] );

				button2 = chk_xd_dialog(specapp, ROOT);

				if ( button2 == SPECOK )
				{
					for ( i = 0; i < 7; i++ )
						get_opt(specapp, &(appl->flags), AT_EDIT << i, (int)ord[i] );
				}	

				break;
			}

		case APPFTYPE:

			/* Set associated document types; 
			 * must recreate dialog title afterwards because the same
			 * dialog is used
			 */

			ft_dialog( (const char *)(appl->shname), &newlist, (LS_DOCT | (use & LS_SELA)) );
			rsc_title(setmask, DTSMASK, title);
			break;

		case APPPTYPE:
			
			/* 
			 * Define application execution parameters (program type)
			 * note: default values have already been set 
			 */

			prgtype_dialog( NULL, END, (PRGTYPE *)appl, LS_APPL | LS_EDIT );
			break;

		case APOK:

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

			if ( !check_specapp( applist, (appl->flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_CONS | AT_VIEW )), pos) )
				break;

			/* 
			 * Build full application full name again, because
			 * it might have been changed by editing. 
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

			make_path( thispath, thispath, thisname );

			/* Does this application file exist at all ? */

			if ( !x_exist(thispath, EX_FILE) )
			{
				alert_printf(1, APRGNFND, appl->shname );
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
			 
			if ( (strcmp(applcmd, appl->cmdline) != 0) )
			{
				if ((newcmd = strdup(applcmd)) != NULL)
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
			 * there are duplicate documenttypes set for this application
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

				lsrem_all((LSTYPE **)(&appl->filetypes), lsrem);
				appl->filetypes = newlist;
			}
			else
			{
				/* there has been no changes, destroy copy of list */

				lsrem_all((LSTYPE **)(&list), lsrem);
			}

			/* Set also the f-key. (remove all other assignments of this key) */

			unset_fkey(applist, fkey);
			appl->fkey = fkey;

			/* Reset special app flags from other applications */

			unset_specapp( applist, (appl->flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_CONS | AT_VIEW )) );

			qquit = TRUE;
			stat = TRUE;
			break;

		default:
		  exit2:

			/* Anything else, like exit without OK */

			/* Copy of documenttype list has to be destroyed */

			lsrem_all((LSTYPE **)(&newlist), lsrem);
			qquit = TRUE;
			break;
		}

		/* Set all buttons to normal state */

		xd_change(&info, button, NORMAL, (qquit == FALSE) ? 1 : 0);
	}

	/* Close the dialog and exit */

	xd_close(&info);

	return stat;
}


/* 
 * Use these list-specific functions to manipulate applications lists: 
 */

#pragma warn -sus
static LS_FUNC aplist_func =
{
	copy_app,
	rem_appl,
	appinfo_info,
	find_appl,
	app_dialog
};
#pragma warn .sus


/* 
 * Handle installing, editing and removing applications.
 * If "Install Applicaton" menu is selected without any program file being
 * selected in a dir window, a dialog is shown with a list of installed
 * applications, and options to add, remove or edit them, for which 
 * another dialog is opened.
 * If installation is to be done upon an applicaton selected in a dir window,
 * a dialog opens for directly setting application parameters.
 */

void app_install(int use)
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

	list_edit( &aplist_func, (LSTYPE **)(&applikations), 1, sizeof(APPLINFO), (LSTYPE *)&awork, theuse); 

	rsc_title(setmask, FTTEXT, TAPP); 
	obj_hide(setmask[FILETYPE]);
	obj_hide(setmask[FTTEXT]);
}


/* 
 * Find installed application which is specified for "file" filetype.
 * Return pointer to information block for that app. 
 */

APPLINFO *app_find(const char *file)
{
	APPLINFO *h = applikations;
	FTYPE *t;

	while (h)
	{
		t = h->filetypes;
		while (t)
		{
			if (cmp_wildcard(file, t->filetype)) 
				return h;
			t = t->next;
		}
		h = h->next;
	}
	return NULL;
}


/*
 * Find the (short) name of the application assiciated with a filename
 */

char *app_find_name(const char *fname)
{
	APPLINFO *theapp = app_find(fname);

	if (theapp)
		return theapp->shname;
	else
		return NULL;
}


/* 
 * Find installed application for which is specified the "fkey" function key;
 * Return pointer to information block for that app
 */

APPLINFO *find_fkey(int fkey)
{
	APPLINFO *h = applikations;

	while (h)
	{
		if (h->fkey == fkey)
			return h;
		h = h->next;
	}

	return NULL;
}


/*
 * Build a command line from the format string and the selected objects.
 * Commad line variables understood: %f (fullname), %n (name), %p (path)
 * Return pointer to an allocated space containing the complete command line,
 * including length information in the first byte (if the command is
 * longer than 125 bytes, 127 is entered as length).
 */

static char *app_build_cml
(
	const char *format,
	WINDOW *w,
	int *sellist,
	int n
)
{
	boolean
		quote;

	const char 
		*c = format,
		*s;

	char 
		quotechar = 39, 	/* 34=double quote; 39=single quote */
		h, 
		hh,
		*d,					/* pointer to a position in 'build' */
		*tmp,
		*build = NULL,		/* buffer for building the command name */
		*realname = NULL,	/* name to be used- either item name or link target */
		*fullname = NULL;	/* full name of the item */

	int 
		i, 			/* item counter */
		error,
		mes,
		item;

	long
		ltot,		/* total length of the current command line */
		ml;			/* maximum possible length of a command line */

	ITMTYPE 
		type;


	if ( n == -1 )
	{
		/* Special case: n = -1 : an explicitely given command line- just copy it */

		ltot = strlen(format);
		tmp = malloc_chk(ltot + 2L);

		if ( tmp )
			strcpy(&tmp[1], format);
	}
	else
	{
		/*
		 * Any argument will not be longer than VLNAME-1. Therefore it seems
		 * safe to assume that a complete command line will be shorter
		 * than (n+1) * (sizeof(VLNAME) + 2). A temporary buffer for constructing
		 * the command line is dimensioned so, with four bytes per arg. to spare
		 * for an occasional quote or blank (it can't overflow)
		 */

		ml = (n + 1) * (sizeof(VLNAME) + 4);

		if ( (build = malloc_chk(ml)) == NULL )
			return NULL; 

		/* Keep the first byte for length information */

		d = build;

		*d++ = '*'; /* will be substituted by length info */

		ltot = 1;

		/* Trim leading blanks from input */

		while (*c == ' ')
		c++;

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
						for (i = 0; i < n; i++)
						{
							item = sellist[i];
							type = itm_type(w, item);
							quote = FALSE;	

							if ((mes = trash_or_print(type)) != 0)
							{
								if (alert_printf(1, ANODRAGP, get_freestring(mes)) == 1)
									continue;
								else
									goto error_exit2;
							}
							else
							{
								/* Always separate arguments by blanks */	

								if (i != 0)
								{
									*d++ = ' ';
									ltot++;
								}

								/* Get the full name of an item (allocate fullname) */

								if ((fullname = itm_fullname(w, item)) == NULL)
									goto error_exit2;

								/* 
								 * Is this, maybe, a link?
								 * Item can be a link; if so, resolve it;
								 * if it is not a link, the name will be copied
								 */

								if ((realname = x_fllink(fullname)) == NULL)	
									goto error_exit2;			
							
								if ((hh == 'f') || (type == ITM_DRIVE))
								{
									tmp = realname;
								}
								else if ( hh == 'n' )
								{
									tmp = fn_get_name(realname);
								}
								else if ( hh == 'p' )
								{
									tmp = realname;
									*fn_get_name(realname) = 0;
								}

								/* (now tmp points to a position in realname) */

								if (isupper(h))
									strlwr(tmp);

								if ( ltot + strlen(tmp) >= ml )
								{
									error = ECOMTL;
									goto error_exit1;
								}

								/* Quote the name if it contains blanks */

								if ( strchr(tmp, ' ') != NULL )
								{
									quote = TRUE;
									ltot++;
									*d++ = quotechar; /* quote */
								}

								s = tmp;
								while (*s)
								{
									*d++ = *s++;
									ltot++;
								}

								if (quote)
								{
									ltot++;
									*d++ = quotechar; /* unquote */
								}

								free(fullname);
								free(realname);
							}
						}
					}
				}
				else
				{
					ltot++;
					if ( ltot >= ml )
					{
						error = ECOMTL;
						goto error_exit1;
					}
					*d++ = h;
				}
			}
			else
			{
				ltot++;
				if ( ltot >= ml )
				{
					error = ECOMTL;
					goto error_exit1;
				}
				*d++ = h;
			}
		}

		/* add zero termination byte */

		*d++ = 0;

		/* Copy to destination */

		tmp = strdup( build );

		ltot --; /* subtract 1 for the first character */
	}

	/* Insert length information */

	if (tmp)
	{
		if ( ltot > 125 )
			*tmp = 127;
		else
			*tmp = ltot;
	}

	/* Cleanup and exit */

	free(build);
	return tmp;

	/* Handle errors */

	error_exit1:;

	xform_error( error );
 
	error_exit2:;

	free(build);
	free(fullname);
	free(realname);
	return NULL;
}


/* 
 * Attempt to extract a valid path from the commandline.
 * Return pointer to a string containing the path.
 * Memory is allocated in this routine.
 * If a string resembling a path is not found, return NULL
 */

static char *app_parpath(char *cmline)
{
	char *p, *p1 = NULL;
	boolean q = FALSE;
	VLNAME thepath;


	p = cmline;

	while(*p)
	{
		/* 
		 * Beginning of a path was found (see below), now find the end 
		 * (end is recognized by any char with code lesser than '!')
		 */

		if(p1 && ((*p == 0) || (q && *p == '"') || (!q && *p < '!')))
		{
			strsncpy(thepath, p1, p - p1 + 1);
			return fn_get_path(thepath); /* allocates space */			
		} 

		/* 
		 * Find the beginning of the first path. It must start
		 * with a drive specification
		 */

		if( p1 == NULL && strlen(p) > 2 && p[0] && p[1] && isalpha(p[0]) && p[1] == ':' && p[2] == '\\')
			p1 = p;

		if (*p == '"')
			q = !q;

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
(const char *program, APPLINFO *app, WINDOW *w, int *sellist, int n, int kstate)
{

	APPLINFO 
		*appl;					/* Application info of program. */

	const char 
		*cl_format,				/* Format of commandline. */
		*name,					/* Program (real) name. */
		*par_path = NULL,		/* Path of the 1st parameter */
		*def_path;				/* Default path of program. */

	char
		*theenv = NULL,			/* local environment for the application */
		*thecommand = NULL;		/* command line */

	long
		cmllen,					/* length of the command line */
		limmem;					/* mem.limit in multitask (bytes) */

	boolean 
		argv,					/* Use ARGV protocol flag. */
		single,					/* don't multitask (Magic) */
		back,					/* run in background when possible */
		result = FALSE;			/* return result */

	ApplType 
		appl_type;				/* Type of program */

	PRGTYPE
		*thework;				/* pointer to edit area for program type */

	static SNAME
		prevcall;				/* name of the last started ttp */

	SNAME
		thiscall;				/* name of this ttp */


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

		if (!x_exist(name, EX_FILE))
		{
			alert_printf(1, APRGNFND, fn_get_name(name));
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

		if (!x_exist(name, EX_FILE))
		{
			char *fullname;
			int button;

			if ((button = alert_printf(1, AAPPNFND, fn_get_name(name))) == 2)	/* remove */
				rem_appl(&applikations, appl); 
		
			if (button > 1)						/* remove or cancel */
				goto errexit;

			/* 
			 * Locate the file using file selector.
			 * Note: instead of "fullname" there was "newname" above, but 
			 * that seems wrong, as newname (global) should always have pointed
			 * to an editable field in nameconflict dialog
			 */

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

	/* Set some program-type details as proper boolean values */

	argv = ((thework->flags & PT_ARGV) != 0) ? TRUE : FALSE;
	single = ((thework->flags & PT_SING) != 0) ? TRUE : FALSE;
	back = ((thework->flags & PT_BACK) != 0) ? TRUE : FALSE; 

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

		if ( strcmp( thiscall, prevcall ) != 0 )
		{
			cmdline[0] = 0;	/* clear the dialog field */
			strcpy( prevcall, thiscall );
		}

		/* Don't start the application after all */

		if (chk_xd_dialog(getcml, CMDLINE) != CMLOK)
			goto errexit;
			
		/* 
		 * Cmdline below is from the dialog. Copy any possible length,
		 * and put command line length (or 127) into first byte
		 */

		cmllen = strlen(cmdline);

		if ( (thecommand = malloc(cmllen + 2L)) == NULL )
			goto errexit;

		thecommand[0] = (cmllen > 125) ? 127 : (char)cmllen;
		strsncpy((char *)(&thecommand[1]), cmdline, (size_t)cmllen + 1);
	}
	else
	{
		/* 
		 * There are filenames passed, or this is not a TTP/GTP program;
		 * build the command line 
		 */

		if ((thecommand = app_build_cml( (n == -1) ? (char *)sellist : cl_format, w, sellist, n)) == NULL)
			goto errexit;

		prevcall[0] = 0;
	}

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

	cmllen = strlen(thecommand + 1);

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
 * Initialize applications list 
 */

void app_init(void)
{
	applikations = NULL;
#ifdef MEMDEBUG
	atexit(app_default);
#endif
}


/* 
 * Remove all applications from the list 
 *
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
	prg_table[2].flag = CFG_INHIB; /* name mask do not make sense here */

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
	lsrem_all((LSTYPE **)(&awork.filetypes), lsrem);
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
	{CFG_HDR, 0, "app" },
	{CFG_BEG},
	{CFG_S,   0, "path", 	this.name	  },
	{CFG_S,   0, "cmdl", 	this.cmdline  },
	{CFG_S,   0, "envr", 	this.localenv },
	{CFG_D,   0, "fkey", 	&awork.fkey	  },
	{CFG_NEST,0, "atype",    this_atype   },
	{CFG_NEST,0, "doctypes", dt_config    },
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

		*error = CfgLoad(file, app_table, (int)sizeof(LNAME), lvl); 

		if ( *error == 0 )				/* got one ? */
		{
 			if ( this.name[0] == 0 )
				*error = EFRVAL;
			else
			{
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
				 * a spec app or this Fkey assignment. Might be called
				 * a bug, but probably not worth rectifying.
				 */

				unset_specapp(&applikations, (awork.flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_CONS | AT_VIEW) ) );

				if (awork.fkey > 20)
					awork.fkey = 0;

				unset_fkey(&applikations, awork.fkey );

				fix_prgtype_v360((PRGTYPE *)(&awork)); /* Compatibility issue, remove after V3.60 */

				/* Add this application to list */

				if 
				( 
					lsadd
					( 
						(LSTYPE **)&applikations, 
						sizeof(awork), 
						(LSTYPE *)&awork, 
						END, 
						copy_app 
					) == NULL 
				)
				{
					lsrem_all((LSTYPE **)(&awork.filetypes), lsrem);
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
	{CFG_HDR, 0, "applications" },
	{CFG_BEG},
	{CFG_NEST,0, "app", one_app },		/* Repeating group */
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
 * Start applications with a special use.
 * It can be: an editor, an autostart, a shutdown application, etc.
 * There can be only one editor, but several startup or
 * shutdown applications. 
 * There is a delay of 1 or 2 seconds between autostart apps
 * but a delay of 10 seconds between shutdown apps.
 * 
 * Note: this routine can be improved, to better handle
 * simultaneous or sequential launch of applications.
 *
 * Routine returns the number of application started 
 * (successfully or otherwise)
 */

int app_specstart(int flags, WINDOW *w, int *list, int nn, int kstate)
{
	APPLINFO *app = applikations;

	int	n = 0, delayt;

	if ( (flags & AT_SHUT) || (flags & AT_VIDE) )
		delayt = 10000;
	else
	{
#if _MINT_
		if ( mint || geneva )
			delayt = 2000;
		else
#endif
			delayt = 1000;
	}

	while (app != NULL)
	{
		if ( ( app->flags & flags) != NULL )
		{
			/* Delay a little bit before the next application */

			if ( n )
				wait(delayt); 
			n++;

			/* Start a marked application */

			app_exec(NULL, app, w, list, nn, kstate);

			/* Some types should not be multiply defined- exit after the first one */

			if ( (flags & (AT_EDIT | AT_SRCH | AT_FFMT | AT_CONS | AT_VIEW ) ) != 0 )
				break;
		}
		app = app->next;
	}
	return n;
}