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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <tos.h>
#include <vdi.h>
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "resource.h"
#include "startprg.h"
#include "xfilesys.h"
#include "file.h"
#include "config.h"	
#include "prgtype.h"
#include "font.h"
#include "window.h"
#include "lists.h"
#include "slider.h" 
#include "filetype.h" 
#include "library.h"
#include "applik.h"
#include "va.h"
#include "edit.h"	  


APPLINFO
	awork,					/* work area for editing */ 
	*applikations;			/* list of installed apps data */

extern PRGTYPE *prgtypes;   /* list of executable filetypes */

extern Options options;		/* need to know cfg file version */

extern char *editor;	


/* 
 * Find an application specified by its name among installed ones; 
 * Comparison is made upon full path+name, and an exact match is required,
 * contrary to routine find_lsitem in lists.c which works upon short name
 * (part of the LSTYPE/FTYPE/ICONTYPE/PRGRYPE) and allows wildcards.
 */

APPLINFO *find_appl(APPLINFO **list, const char *program, int *pos) 
{
	APPLINFO *h = *list; 

	if (pos)
		*pos = -1;			/* allow NULL when pos not needed */

	if ( program != NULL )	
	{
		while (h != NULL)
		{
			if (pos)
				(*pos)++;		/* HR 120803 */
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
 * "rem" routine in lists.c, except that it deallocates name, commandline 
 * and documenttypes as well.
 * Note: beware of recursive calls: rem_all-->rem_appl-->rem_all-->rem
 */

void rem_appl(APPLINFO **list, APPLINFO *appl)
{

	APPLINFO 
		*f = *list,		/* pointer to first, then current item in the list */ 
		*prev;			/* pointer to previous item in the list */

	prev = NULL;

	/* Search for a pointer match */

	while ((f != NULL) && (f != appl))
	{
		prev = f;
		f = f->next;
	}

	/* If match found, remove that item from the list */

	if ( (f == appl) && (f != NULL) )
	{
		if (prev == NULL)
			*list = f->next;
		else
			prev->next = f->next;

		free_item( f->name ); 	/* deallocate app name */
		free_item( f->cmdline);	/* and command line */
		rem_all((LSTYPE **)(&(f->filetypes)), rem); /* deallocate documenttyoe list */
		free_item(f);
	}
}


/*
 * Copy an application dataset from one location to another;
 * Space for both has to be already allocated; 
 * for name, commandline and documenttypes, memory is allocated here 
 */
 
static void copy_app( APPLINFO *t, APPLINFO *s )
{
	copy_prgtype ( (PRGTYPE *)t, (PRGTYPE *)s );

	t->fkey = s->fkey;
	t->edit = s->edit;
	t->autostart = s->autostart;

	t->name = NULL;
	t->cmdline = NULL;
	t->filetypes = NULL;

	if ( s->name != NULL )
		t->name = strdup(s->name);
	if ( s->cmdline != NULL )
		t->cmdline = strdup(s->cmdline);
	copy_all ( &(t->filetypes), &(s->filetypes), sizeof(FTYPE), copy_ftype);
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

	if ( a != NULL )
	{
		/* Yes, use its data */

		copy_app(appl, a);
	}
	else
	{
		/* Not found, set default program type according to name */

		prg_info( &prgtypes, name, 0, (PRGTYPE *)appl );

		/* 
		 * Pass name, if given, otherwise set empty string 
		 */

		if ( name != NULL )
			appl->name = strdup(name);
		else
			appl->name = strdup("\0");

		/* Create a default command line; set no documenttypes and F-key */

		appl->cmdline = strdup("%f");
		appl->filetypes = NULL; 
		appl->fkey = 0;
		appl->edit = FALSE;
		appl->autostart = FALSE;

		/* Check if allocation of name and commandline strings  was successful */

		if ( appl->name == NULL || appl->cmdline == NULL )
			xform_error ( ENSMEM );
	}
}


/*
 * Check duplicate application assignment; this routine differs
 * from check_dup in lists.c in that the full app path+name is compared
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


	while (f != NULL)
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
 * Put into dialog the F-key assigned to application 
 */

static void set_fkey(int fkey)
{
	char 
		*applfkey = applikation[APFKEY].ob_spec.tedinfo->te_ptext;

	if (fkey == 0)					/* if key not assigned */
		*applfkey = 0;				/* empty string */
	else
		itoa(fkey, applfkey, 10);	/* convert int to string */
}


/*
 * Reset F-key "fkey" assigned to any of the installed applications
 */

static void unset_fkey(APPLINFO **list, int fkey)
{
	APPLINFO *f = *list;

	while (f != NULL)
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

	while (f != NULL)
	{
		if ( i != pos )
		{
			if ( f->fkey == fkey )
			{
				button = alert_printf( 2, ADUPFLG, f->shname );
				if ( button == 1 )
				{
					return TRUE;
				}
				else
					return FALSE;
			}
		}

		i++;
		f = f->next;
	}

	return TRUE;
}


/********************************************************************
 *																	*
 * Funktie voor het verversen van geinstalleerde applikaties.		*
 *																	*
 ********************************************************************/

static boolean app_set_path(APPLINFO *info, const char *newpath)
{
	char *h;

	if ((h = strdup(newpath)) == NULL)
	{
		alert_printf(1, AAPPLPM, fn_get_name(newpath));
		return FALSE;
	}

	free(info->name);
	info->name = h;

	return TRUE;
}


void app_update(wd_upd_type type, const char *fname1, const char *fname2)
{
	APPLINFO *info;

	if ((info = find_appl(&applikations, fname1, NULL)) != NULL)/* allow NULL when pos not needed */
	{
		if (type == WD_UPD_DELETED)
			rem_appl(&applikations, info);
		if (type == WD_UPD_MOVED)
			app_set_path(info, fname2);
	}
}


/********************************************************************
 *																	*
 * Hulpfunkties voor install_appl()									*
 *																	*
 ********************************************************************/


/* 
 * Create "short" name of an application; it will be used
 * for informational purposes only in dialogs opened to edit
 * documenttypes and application type
 */ 

void log_shortname( char *dest, char* appname )
{
	char *n = fn_get_name(appname);

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
		title,				/* rsc index of dialog title string */
		button, 			/* code of pressed button */
		fkey;				/* code of associated F-key */

	boolean
		newedit = FALSE,	/* true if app is to be set as an editor */
		wasedit = FALSE,	/* true if application was an editor */
		stat = FALSE, 
		quit = FALSE;		/* true when ok to exit loop */


	LNAME
		thisname,			/* name of the current application */
		thispath;			/* path of the current application */

	char
		*newcmd = NULL,		/* pointer to the changed text */ 
		*newpath = NULL;	/* pointer to the changed text */

	XDINFO 
		info;				/* info structure for the dialog */

	FTYPE
		*list = NULL,		/* working copy of the list of filetypes */
		*newlist;			/* first same at list, then maybe changed in ft_dialog */


	/* 
	 * Set dialog title(s); create copy of filetypes list if needed 
	 * i.e. if application setup is edited, copy list of filetypes to temporary;
	 * If application is added, there is nothing to copy
	 */

	if ( use & LS_EDIT )
	{
		title = DTEDTAPP;
		copy_all( (LSTYPE **)(&list), (LSTYPE **)(&appl->filetypes), sizeof(FTYPE), copy_ftype);
  	}
	else
	{
		if ( appl->name[0] > ' ' && !prg_isprogram(fn_get_name(appl->name)) )
		{
			alert_printf(1, AFNPRG, isroot(appl->name) ? appl->name : appl->shname); /* Not a program filetype ! */
			return FALSE;
		}
		title = (use & LS_WSEL ) ? DTINSAPP : DTADDAPP; /* Install or edit - set title */
		list = appl->filetypes;
	}

	newlist = list;

	rsc_title( applikation, APDTITLE, title );
	rsc_title(addprgtype, PTTEXT, TAPP);

	/* Is this, perhaps, the editor program ? */

	applikation[ISEDIT].ob_state &= ~SELECTED;
	
#if TEXT_CFG_IN
	if (appl->edit)		
	{
		applikation[ISEDIT].ob_state |= SELECTED;
		wasedit = TRUE;
	}
#else
	if ( editor != NULL )
	{
		if ( strcmp( editor, appl->name ) == 0 )
		{
			applikation[ISEDIT].ob_state |= SELECTED;
			wasedit = TRUE;
		}
	}
#endif

	/* Is this an autostart application? */

	set_opt(applikation, appl->autostart, 1, ISAUTO);

	/*
	 * Copy application name, path and command line to dialog fields;
	 * as application name is stored as path+name it has to be split-up
	 * in order to to display path separately
	 */

	split_path( thispath, thisname, appl->name );

	cv_fntoform(&applikation[APNAME], thisname);
	cv_fntoform(&applikation[APPATH], thispath);

	strcpy(applcmdline, appl->cmdline);

	/* Put F-key value, if any, into the dialog */

	set_fkey(appl->fkey);

	/* Open the dialog for editing application setup */

	xd_open(applikation, &info);

	while (quit == FALSE)
	{
		/* Get button code */

		button = xd_form_do(&info, APCMLINE) & 0x7FFF;

		 /* Create "short" name of this application */

		log_shortname( appl->shname, appl->name );

		/* Do something appropriate upon a button pressed */
		
		switch (button)
		{
		case APPFTYPE:

			/* Set associated document types; must recreate dialog title afterwards */

			ft_dialog( (const char *)(appl->shname), &newlist, LS_DOCT );
			rsc_title(setmask, DTSMASK, DTINSAPP);
			break;

		case APPPTYPE:
			
			/* 
			 * Define application execution parameters (program type)
			 * note: default values have already been set 
			 */

			addprgtype[PRGNAME].ob_flags &= ~EDITABLE;
			prgtype_dialog( NULL, END, (PRGTYPE *)appl, LS_APPL | LS_EDIT );
			addprgtype[PRGNAME].ob_flags |= EDITABLE;
			break;

		case APOK:

			/* Changes are accepted, do everything needed to exit dialog */
 
			/* First, check for duplicate app installation */

			if ( !check_dup_app(applist, appl, pos ) )
				break;

			/* Then, check for duplicate F-key assignment */

			fkey = atoi(applikation[APFKEY].ob_spec.tedinfo->te_ptext);

			if ( !check_fkey(applist, fkey, pos) )
				break;

			/* Check for duplicate editor assignment */

			get_opt(applikation, (int *)(&newedit), 1, ISEDIT); 

			if ( !check_edit(applist, newedit, pos) )
				break;

			/* 
			 * Build full application full name again, because
			 * it might have been changed by editing. 
			 * Use "thispath" and "thisname" as they are not needed anymore.
			 * Note that actual file is NOT renamed, just 
			 * the entry in the applications list, but a check is made
			 * if a file with the new name exists (it must exist)
			 */

			cv_formtofn(thispath, &applikation[APPATH]);
			cv_formtofn(thisname, &applikation[APNAME]);

			/* A name and a path must be given */

			if ( *thispath  == 0 || *thisname == 0 )
			{
				alert_iprint(MFNEMPTY);
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
				if ( ( newpath = strdup(thispath) ) == NULL )
				{
					xform_error(ENSMEM);
					goto exit2;
				}
			}

			/* 
			 * If command line string has been changed (only then),
			 * allocate a new one 
			 */
			 
			if ( (strcmp(applcmdline, appl->cmdline) != 0) )
			{
				if ((newcmd = strdup(applcmdline)) == NULL)
				{
					xform_error(ENSMEM);
					goto exit2;
				}
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

				rem_all((LSTYPE **)(&appl->filetypes), rem);
				appl->filetypes = newlist;
			}
			else
			{
				/* there has been no changes, destroy copy of list */

				rem_all((LSTYPE **)(&list), rem);
			}

			/* Change app name and command line if changed */

			if ( newpath != NULL )
			{
				free(appl->name);
				appl->name = newpath;
			}

			if ( newcmd != NULL )
			{
				free(appl->cmdline);
				appl->cmdline = newcmd;
			}
  
			/* Set also the f-key. (remove all other assignments of this key) */

			unset_fkey(applist, fkey);
			appl->fkey = fkey;

			/* Set this app as editor program, if so said */

			if ( newedit )
			{
				unset_edit(applist);
				appl->edit = TRUE;
				edit_set(appl->name);
			}
			else
			{
				appl->edit = FALSE;	
				if ( wasedit )
					edit_set(NULL);
			}

			/* Set autostart flag if needed */

			get_opt(applikation, (int *)(&appl->autostart), 1, ISAUTO);

			quit = TRUE;
			stat = TRUE;
			break;

		default:
		  exit2:

			/* Anything else, like exit without OK */

			/* Copy of documenttype list has to be destroyed */

			rem_all((LSTYPE **)(&newlist), rem);
			quit = TRUE;
			break;
		}

		/* Set all buttons to normal state */

		xd_change(&info, button, NORMAL, (quit == FALSE) ? 1 : 0);
	}

	/* Close the dialog and exit */

	xd_close(&info);

	return stat;
}


/* 
 * Use these list kind-specific functions to manipulate applications lists: 
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
 * applications, and options to add,remove or edit them, for which 
 * another dialog is opened.
 * If installation is to be done upon an applicaton selected in a dir window,
 * a dialog opens for directly setting application parameters.
 */

void app_install(void)
{
	/* Set dialog title */

	rsc_title(setmask, DTSMASK, DTINSAPP);

	/* 
	 * Edit applications list: add/delete/change... 
	 * Accepting or canceling changes is handled within list_edit,
	 * so there is nothing to do after that
	 */

	list_edit( &aplist_func, (LSTYPE **)(&applikations), NULL, sizeof(APPLINFO), (LSTYPE *)(&awork), LS_APPL); 
}


/* 
 * Find installed application which is specified for "file" filetype.
 * Return pointer to information block for that app. 
 */

APPLINFO *app_find(const char *file)
{
	APPLINFO *h;
	FTYPE *t;

	h = applikations;
	while (h != NULL)
	{
		t = h->filetypes;
		while (t)
		{
			if (cmp_wildcard(file, t->filetype) == TRUE) 
				return h;
			t = t->next;
		}
		h = h->next;
	}
	return NULL;
}


/* 
 * Find installed application for which is specified the "fkey" function key;
 * Return pointer to information block for that app
 */

APPLINFO *find_fkey(int fkey)
{
	APPLINFO *h;

	h = applikations;

	while (h)
	{
		if (h->fkey == fkey)
			return h;
		h = h->next;
	}

	return NULL;
}


/*
 * Determine the length of the command line.
 */

static long app_get_arglen
(
	const char *format,
	WINDOW *w,
	int *sellist,
	int n
)
{
	const char *c = format;
	char h, *mes;
	long l = 0;
	int i, item;
	ITMTYPE type;

	while (*c)
	{
		if ( (*c++ == '%') && (n > 0) ) 
		{
			h = tolower(*c++);
			if ((h == 'n') || (h == 'f'))
			{
				for (i = 0; i < n; i++)
				{
					item = sellist[i];
					type = itm_type(w, item);

					if ((type == ITM_TRASH) || (type == ITM_PRINTER))
					{
						switch (type)
						{
						case ITM_TRASH:
							rsrc_gaddr(R_STRING, MTRASHCN, &mes);
							break;
						case ITM_PRINTER:
							rsrc_gaddr(R_STRING, MPRINTER, &mes);
							break;
						}
						if (alert_printf(1, ANODRAGP, mes) == 1)
							continue;
						else
							return -1L;
					}
					else
					{
						if (i != 0)
							l++;
						if ((h == 'f') || (type == ITM_DRIVE))
							l += itm_info(w, item, ITM_FNAMESIZE);
						else if (h == 'n')
							l += itm_info(w, item, ITM_NAMESIZE);
					}
				}
			}
			else
				l++;
		}
		else
			l++;
	}

	return l;
}


/*
 * Build a command line from the format string and the selected objects.
 */

static boolean app_set_cml
(
	const char *format,
	WINDOW *w,
	int *sellist,
	int n,
	char *dest		
)
{
	const char 
		*c = format, 
		*s;

	char 
		h, 
		*d = dest, 
		*tmp;

	int 
		i, 
		item;

	ITMTYPE 
		type;


	while (*c == ' ')
		c++;

	while ((h = *c++) != 0)
	{
		if ((h == '%') &&  (n > 0) )
		{
			h = *c++;
			if ((tolower(h) == 'f') || (tolower(h) == 'n'))
			{
				for (i = 0; i < n; i++)
				{
					item = sellist[i];
					type = itm_type(w, item);

					if ((type != ITM_TRASH) && (type != ITM_PRINTER))
					{
						if (i != 0)
							*d++ = ' ';

						if ((tolower(h) == 'f') || (type == ITM_DRIVE))
						{
							if ((tmp = itm_fullname(w, item)) == NULL)
								return FALSE;
						}
						else
						{
							if ((tmp = strdup(itm_name(w, item))) == NULL)
								return FALSE;
						}

						if (isupper(h))
							strlwr(tmp);

						s = tmp;
						while (*s)
							*d++ = *s++;
						free(tmp);
					}
				}
			}
			else
				*d++ = h;
		}
		else
			*d++ = h;
	}

	*d++ = 0;

	return TRUE;
}


/*
 * Check and build the command line from the format string and
 * the selected objects. Space is allocated for the resulting
 * commandline string 
 */

static char *app_build_cml
(
	const char *format,	/* format string */
	WINDOW *w,			/* pointer to a window with selected items */
	int *list,			/* list of item indexes */
	int n				/* number of selected items */
)
{
	long l;
	char *cmdline;

	if ((l = app_get_arglen(format, w, list, n) ) < 0) 
		return NULL;

	l += 2;		/* Length byte and terminating 0. */

	if ((cmdline = malloc(l)) != NULL)
	{
		if (app_set_cml(format, w, list, n, cmdline + 1) == FALSE) 
		{
			free(cmdline);
			return NULL;
		}

		if ((l = strlen(cmdline + 1)) <= 125)
			*cmdline = (char) l;
		else
			*cmdline = 127;
	}

	return cmdline;
}


/*
 * Start a program or an application.
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
 * n		- number of selected objects in 'w', or 0 if no objects
 *			  are dragged to the program.
 * kstate   - 
 * dragged	- currently not used
 *
 * Result: TRUE if succesfull, FALSE if not.
 */

/*
 * modified to use *program to pass explicit 
 * path and name of file to open in case of n=-1 && app != NULL
 * 290903: modified again to instead use (char *)sellist to pass
 * a verbatim command line instead of a filename. Use *program
 * as is should be used- for program name. n=-1 is still used.
 */

boolean app_exec(const char *program, APPLINFO *app, WINDOW *w,
				 int *sellist, int n, int kstate, boolean dragged)
{

	APPLINFO 
		*appl;					/* Application info of program. */

	const char 
		*cl_format,				/* Format of commandline. */
		*name,					/* Program name. */
		*def_path;				/* Default path of program. */

	char 
		*fullname,				/* full name of the app file searched for */
		*newcmd = NULL,			/* same but if built from elements */
		thecommand[132];		/* final command line */
		
	long
		limmem;					/* mem.limit in multitask */

	boolean 
		argv,					/* Use ARGV protocol flag. */
		single,					/* don't multitask (Magic) */
		result;

	ApplType 
		appl_type;				/* Type of program */

	PRGTYPE
		pwork;					/* contains programtype data */

	static SNAME
		prevcall;				/* previously called ttp */

	SNAME
		thiscall;				/* name of this ttp */


	/* Clear the command line which might be passed on to a program */

	thecommand[0] = 0;
	thecommand[1] = 0;

	/* If application is NULL, findout if 'program' is installed as an application. */

	/* HR 120803 allow NULL when pos not needed */

	appl = (app == NULL) ? find_appl(&applikations, program, NULL) : app; 

	if (appl == NULL)
	{
		/* 
		 * 'program' is not installed as an application. 
		 * Use default settings for this executable file type 
		 */

		cl_format = "%f";
		name = (char *) program;

		prg_info(&prgtypes, name, 0, &pwork);
		appl_type = pwork.appl_type;
		argv = pwork.argv;
		single = pwork.single;
		limmem = pwork.limmem;
		def_path = (pwork.path) ? NULL : wd_toppath();

		/* Abort if program does not exist. */

		if (x_exist(name, EX_FILE) == FALSE)
		{
			alert_printf(1, APRGNFND, fn_get_name(name));
			return FALSE;
		}
	}
	else
	{
		/* 
		 * 'app' is not NULL or 'program' is installed as an
		 * application. Use the settings set by the user in the
		 * 'Install application' dialog. 
		 */

		cl_format = appl->cmdline;
		name = appl->name;
		appl_type = appl->appltype;
		argv = appl->argv;
		single = appl->single;
		limmem = appl->limmem;
		def_path = (appl->path == TRUE) ? NULL : wd_toppath();

		/* 
		 * If the application does not exist, ask the user to
		 * locate it or to remove it. 
		 */

		if (x_exist(name, EX_FILE) == FALSE)
		{
			int button;

			if ((button = alert_printf(1, AAPPNFND, fn_get_name(name))) == 3)
				return FALSE;
			else if (button == 2)
			{
				rem_appl(&applikations, appl); 
				return FALSE;
			}
			/* DjV: note: instead of "fullname" there was "newname" here, but 
			 * that seems wrong, as newname (global) should always have pointed to an
			 * editable field in nameconflict dialog
			 */
			else if ((fullname = locate(name, L_PROGRAM)) == NULL)
				return FALSE;
			else 
			{
				free(name);
				appl->name = name = fullname;

				/* Create a short name here, or applications list in the dialog will look wrong  */

				log_shortname( appl->shname, appl->name );
			}
		}
	}

	if (n == 0)
	{
		/* 
		 * If there are no files passed to the program (n is 0) and
		 * the program is a TTP or GTP program, then ask the user to
		 * enter a command line.
	     */

		if ((appl_type == PTTP) || (appl_type == PGTP))
		{
			int i, j;

			/* 
			 * Remember the command line if the same ttp program
			 * is started again (remember name of the program as well)
			 */

			log_shortname( thiscall, name );

			if ( strcmp( thiscall, prevcall ) != 0 )
			{
				cmdline[0] = 0;
				strcpy( prevcall, thiscall );
			}

			if (xd_dialog(getcml, CMDLINE) != CMLOK)
				return FALSE;

			j = strlen(cmdline);

			thecommand[0] = j;
			strcpy( &thecommand[1], cmdline );

		}
		else
			prevcall[0] = 0;
	}
	else if ( n > 0 )
	{
		/* 
		 * There are files passed to the program, build the command line. 
		 * Note: new command line string space is allocated here
		 * (and also in the next app_build_cml). It looks as if this
		 * space must exist at least for some time after the program
		 * has been started- therefore it must not be free'd?
		 * To achieve this, the new command line is copied into 
		 * a permanent location- the one which is also used in the
		 * commandline dialog
		 */ 

		if ((newcmd = app_build_cml(cl_format, w, sellist, n )) == NULL) 
			return FALSE;

		strcpy(thecommand, newcmd);
		prevcall[0] = 0;
	}
	else
	{
		if ( n == -1 )		/* File to open is passed by explicit path or name or other command */
		{
			if ((newcmd = app_build_cml( (char *)sellist, w, sellist, n)) == NULL)
				return FALSE;
			strcpy(thecommand, newcmd);
		}
		prevcall[0] = 0;
	}

	/* Check if the commandline is too long. */

	if (   argv == FALSE
	    && strlen(thecommand + 1) > 125
	   )
	{
		xform_error ( ECOMTL ); 
		result = FALSE;
	}
	else	/* No error, start the program. */
	{
		/* something is wrong here! see va.c */

		if (!va_start_prg(name, &thecommand[1]))
			start_prg(name, thecommand, def_path, appl_type, argv, single, limmem, kstate);

		result = TRUE;
	}

	free(newcmd);
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
	rem_all((LSTYPE **)(&applikations), rem_appl);
	applikations = NULL; /* not needed here? */

}


#if ! TEXT_CFG_IN

#include "app_load.h"		/* has to go anyhow */

#endif



typedef struct
{
	LNAME name,
	      cmdline;
	FTYPE ftype;
} SINFO;

static
SINFO this;


/*
 * Save or load program type configuration for one application
 */

static CfgNest this_atype
{
	*error = 0;

	prg_table[0].s[0] = 'a';
	prg_table[2].flag = CFG_INHIB;

	if ( io == CFG_SAVE )
	{
		copy_prgtype( &pwork, (PRGTYPE *)(&awork) );

		/* Bit/bool should be considered temporary */

		bool_to_bit( &pwork.flags, PT_ARGV, pwork.argv );
		bool_to_bit( &pwork.flags, PT_PDIR, pwork.path );
		bool_to_bit( &pwork.flags, PT_SING, pwork.single);
		bool_to_bit( &pwork.flags, AT_EDIT, awork.edit );
		bool_to_bit( &pwork.flags, AT_AUTO, awork.autostart );

		*error = CfgSave(file, prg_table, lvl + 1, CFGEMP);
	}
	else
	{
		memset(&pwork, 0, sizeof(pwork));

		*error = CfgLoad(file, prg_table, (int)sizeof(SNAME), lvl + 1); 

		if ( pwork.appl_type > PTTP )
			*error = EFRVAL; 		
		else
		{
			/* This bit_to_bool should be considered temporary */

			bit_to_bool( pwork.flags, PT_ARGV, &pwork.argv );
			bit_to_bool( pwork.flags, PT_PDIR, &pwork.path );
			bit_to_bool( pwork.flags, PT_SING, &pwork.single);
			bit_to_bool( pwork.flags, AT_EDIT, &awork.edit);
			bit_to_bool( pwork.flags, AT_AUTO, &awork.autostart);

			copy_prgtype( (PRGTYPE *)(&awork), &pwork );
		}
	}
}

/*
 * Clear a list of associated doctypes
 */
static void rem_all_doctypes(void)
{
	rem_all((LSTYPE **)(&awork.filetypes), rem);
}

/*
 * Load or save configuration for associated documenttypes 
 */

static CfgNest dt_config
{
	fthis = awork.filetypes;
	ffthis = &(awork.filetypes); 
	ft_table[0].s[0] = 'd'; 
	filetypes_table[0].s = "doctypes";
	filetypes_table[2].s[0] = 'd';
	filetypes_table[3].type = CFG_END;

	*error = handle_cfg(file, filetypes_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, rem_all_doctypes, NULL);
}


/*
 * Configuration table for one application
 */

static CfgEntry app_table[] =
{
	{CFG_HDR, 0, "app" },
	{CFG_BEG},
	{CFG_S,   0, "path", this.name		},
	{CFG_S,   0, "cmdl", this.cmdline	},
	{CFG_D,  0, "fkey", &awork.fkey	    },
/*
	{CFG_BD,  0, "edit", &awork.edit	    },
	{CFG_BD,  0, "auto", &awork.autostart   },
*/
	{CFG_NEST,0, "atype",     this_atype    },
	{CFG_NEST,0, "doctypes",  dt_config		},
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configuration for one application
 */

static CfgNest one_app
{
	*error = 0;

	if (io == CFG_SAVE)
	{
		APPLINFO *h = applikations;

		while ((*error == 0) && h)
		{
			awork = *h;
			strcpy(this.name, awork.name);
			strcpy(this.cmdline, awork.cmdline);

			*error = CfgSave(file, app_table, lvl + 1, CFGEMP); 
	
			h = h->next;
		}
	}
#if TEXT_CFG_IN
	else
	{
		memset(&this, 0, sizeof(this));
		memset(&awork,0, sizeof(awork));

		*error = CfgLoad(file, app_table, (int)sizeof(LNAME), lvl + 1); 

		if ( *error == 0 )				/* got one ? */
		{
 			if ( this.name[0] == 0 )
				*error = EFRVAL;
			else
			{
				char *name = malloc(strlen(this.name) + 1);
				char *cmdline = malloc(strlen(this.cmdline) + 1);

				if (name && cmdline)
				{
					strcpy(name, this.name);
					strcpy(cmdline, this.cmdline);

					log_shortname( awork.shname, name );
					awork.name = name;
					awork.cmdline = cmdline;


					/* 
					 * As a guard against duplicate flags/keys assignment
					 * for each application loaded, all previous editor
					 * or Fkey assignments (to that key) are cleared here, 
					 * so only the last assignment remains.
					 * Note: if adding the application to the list fails
					 * a few lines later, this will leave Teradesk without
					 * an editor or this Fkey assignment. Might be called
					 * a bug, but probably not worth rectifying.
					 */

					if ( awork.edit )
					{
						unset_edit(&applikations);
						edit_set(awork.name);
					}
					if ( awork.fkey )
						unset_fkey(&applikations, awork.fkey );

					/* Add this application to list */

					if ( lsadd( 
				             (LSTYPE **)&applikations, 
				              sizeof(awork), 
				              (LSTYPE *)&awork, 
				              END, 
				              copy_app 
				           ) == NULL 
				   		)
					{
						free(name);
						free(cmdline);
						rem_all((LSTYPE **)(&awork.filetypes), rem);
						*error = ENOMSG;
					}
				}	
				else
					*error = ENSMEM;
			}
		}
	}
#endif
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
	*error = handle_cfg(file, applications_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, app_default, app_default);
}


/* 
 * Start applications which have been marked as autostart
 */

void app_autostart(void)
{
	APPLINFO *app = applikations;

	while (app != NULL)
	{
		if ( app->autostart )
		{
			/* Start a marked application */

			app_exec(NULL, app, NULL, NULL, 0, 0, FALSE);

			/* Maybe wait a little bit until next one ?? */

			evnt_timer(500, 0); 		
		}
		app = app->next;
	}
}
