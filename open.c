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
#include <mint.h>
#include <xdialog.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "open.h"
#include "xfilesys.h"
#include "printer.h"
#include "font.h"
#include "config.h"
#include "window.h" /* before dir.h and viewer.h */
#include "dir.h"
#include "file.h"
#include "viewer.h"
#include "lists.h"
#include "slider.h"
#include "filetype.h"
#include "applik.h"
#include "library.h"
#include "prgtype.h"

extern APPLINFO awork;
extern FTYPE fwork;
extern LSTYPE *selitem;

boolean onfile = FALSE; /* true if app is started to open a file */

/*
 * Handle the Show/Edit/Run... dialog. Return button index.
 * If "Show" or "Edit" or "Cancel" is selected, just return the code;
 * Otherwise open additional dialog(s).
 */

int open_dialog(void)
{
	XDINFO
		owinfo;

	int 
		thebutton,
		button = 0; /* assuming that no button will ever have index 0 */

	xd_open(openw, &owinfo);
	button = xd_form_do (&owinfo, ROOT);
	thebutton = button;

	switch(button)
	{
		case OWRUN:
			prg_info(NULL, empty, 0, (PRGTYPE *)&awork);
			log_shortname(awork.shname, awork.name);
			if (!prgtype_dialog(NULL, 0, (PRGTYPE *)&awork, LS_APPL | LS_EDIT))
				button = 0;
			break;
		case OWUSE:
			log_shortname(fwork.filetype, awork.name);
			app_install(LS_SELA); /* selitem is nonzero only if successful here */
			if (!selitem)
				button = 0;			
		default:
			break;
	}

	xd_buttnorm( &owinfo, thebutton);
	xd_close(&owinfo);


	return button;
}


/*
 * Open an item selected in a window or else explicitely specified.
 * If there is an explicit specification in "theitem", then "inw"
 * and "initem" are ignored. Action depends on the type of object
 * being detected. Also, if "theitem" is a program, "thecommand"
 * may contain a command line, if not NULL
 */

boolean item_open(WINDOW *inw, int initem, int kstate, char *theitem, char *thecommand)
{
	char

		*realname,		/* link target name */ 
		*path;			/* constructed path of the item to open */

	LNAME 
		epath,			/* Path of the item specified in "Open" */ 
		ename;			/* name of the item specified in "Open" */

	char
		*blank,			/* pointer to a ' ' in the name */ 
		*cmline = NULL;	/* Command passed to application from "Open" dialog" */

	int 
		button;			/* index of the button pressed */

	ITMTYPE 
		type;			/* item type (file, folder, program...) */

	APPLINFO 
		*appl;			/* Pointer to information on the app to run */

	boolean 
		alternate= FALSE, 
		deselect = FALSE;

	WINDOW
		*w;				/* "inw", locally (i.e. maybe changed) */

	int 
		item = initem;	/* "item", locally (i.e. maybe changed) */


	if ( (kstate & 8) != 0 )
		alternate = TRUE;
	
	if ( inw && !theitem )
	{
		/* 
		 * An item is specified by selection in a window;
		 * get its full name
		 */

		/* Note: it is possible that realname == NULL (for trashcan, printer...) */

		realname = itm_tgtname(inw, initem);

		/* Try to divine which type of item this is */

		type = itm_type( inw, initem );

		/* Is this really needed ? */

		if ( realname && type != ITM_TRASH && type != ITM_PRINTER && type != ITM_NOTUSED )
			type = diritem_type( (char *)realname );

		 /* If "Alternate" is pressed a program is treated like ordinary file */

		if ( alternate && (type == ITM_PROGRAM ) )
			type = ITM_FILE;
	}
	else
	{
		/* Open a form to explicitely enter item name */

		if ( !theitem )
		{
			rsc_title( newfolder, NDTITLE, DTOPENIT );
			obj_hide(newfolder[DIRNAME]);
			obj_unhide(newfolder[OPENNAME]);
			xd_init_shift(&newfolder[OPENNAME], openline);

			button = xd_dialog( newfolder, ROOT );
		}

		/* Note: theitem must come first below */

		if ( theitem || (button == NEWDIROK) )
		{
			/* 
			 * For some reason if(!theitem){}else{}  here
			 * gives shorter code than if(theitem){}else{}
			 */

			if ( !theitem ) 
			{
				/* 
				 * Object specified on a command line is opened.
				 * First remove leading and trailing blanks from the line 
				 */

				strip_name(openline, openline);

				/* Continue only if something remains on the line */

				if ( strlen(openline) == 0 )	
					return FALSE;

				/* 
				 * Try to see if there is a command attached.
				 * Separate comand from item name by inserting a '0'
				 * instead of the (first) space between the two.
				 * If there is no command, 'cmline' will point
				 * to an empty string.
				 */

				cmline = empty; /* first, cmline points to an empty string */

				if ( (blank = strchr(openline,' ') ) != NULL )
				{
					*blank = 0;
					cmline = blank;
					cmline++; /* now cmline points to after the first blank */
				}

				/* Convert item name to uppercase */
#if _MINT_
				if (!mint)
#endif
					strupr(openline);

				/* Is this name (path) too long? */

				if ( strlen(openline) >= PATH_MAX )
				{
					alert_iprint(TPTHTLNG);
					return FALSE;
				}

				/* Find the real name of the item */

				if ((realname = x_fllink( openline )) == NULL )
					return FALSE;
					
				/* Restore complete line (for the next opening) */

				if ( blank != NULL )
					*blank = ' ';

			}
			else
			{
				if ( (realname = x_fllink(theitem)) == NULL )
					return FALSE;

				if ( thecommand && *thecommand )
					cmline = thecommand;
			}
		}
		else
			return FALSE;

		/* 
		 * Try to divine which type of item this is. This is done
		 * by analyzing the name and by examining the object's
		 * attributes- if the object does not exist, there will
		 * be an error warning
		 */

		type = diritem_type( (char *)realname );
	}
	
	/* Is this name (path) too long? */

	if ( realname && (strlen(realname) >= PATH_MAX) )
	{
		alert_iprint(TPTHTLNG);
		return FALSE;
	}

	/* Now thet the type of the item is known, do something */

	switch(type)
	{
		case ITM_TRASH:
		case ITM_PRINTER:

			/* Object is a trah can or a printer and can not be opened */

			alert_iprint(MICNOPEN); 
			break;

		case ITM_NOTUSED:

			/* Can't do anything with unknown type of item */

			break;

		default:
	
			/* Separate "realname" into "ename" and "epath" */

			split_path(epath, ename, realname);

			/* 
			 * Simulate some structures of a directory window so that existing
			 * routines expecting an item selected in a window can be used
			 */

			dir_simw( &((DIR_WINDOW *)w), epath, ename, type, (size_t)1, (int)0 );
			item = 0;
			break;		
	}

	free(realname);

	/* 
	 * Object real name and type are determined by now.
	 * Action according to type of the item follows...
	 */

	switch (type)
	{
		case ITM_DRIVE:

			/* Object is a disk volume */

			if ( ( path = itm_fullname(w, item) ) != NULL )
			{
				if (check_drive( (path[0] & 0xDF) - 'A') == FALSE)
				{
					free(path);
					return FALSE;
				}
				else
					deselect = dir_add_dwindow(path); 
			}
			else
				deselect = FALSE;
			break;

		case ITM_PREVDIR:

			/* Object is a parent folder */

			if ((path = fn_get_path(wd_path(w))) != NULL)
				deselect = dir_add_dwindow(path); 
			else
				deselect = FALSE;
			break;

		case ITM_FOLDER:

			/* Object is a folder */

			if ((path = itm_fullname(w, item)) != NULL)
				deselect = dir_add_dwindow(path); 
			else
				deselect = FALSE;
			break;

		case ITM_PROGRAM:

			/* Object is a program */

			if (( path = itm_fullname(w, item) ) != NULL )
			{
				deselect = app_exec(path, NULL, NULL, (int *)cmline, cmline ? -1 : 0, kstate);						
				free(path);
			}

			break;

		case ITM_FILE:

			/* Object is a file */

			onfile = TRUE;
			if ((alternate == FALSE) && (appl = app_find(itm_name(w, item))) != NULL)
				deselect = app_exec(NULL, appl, w, &item, 1, kstate);
			else
			{
				/* File type assignment has been bypassed: now show/edit/cancel */

				if ( theitem )
					button = OWSHOW;
				else
				{
					memset(&awork, 0, sizeof(APPLINFO));
					awork.name = itm_fullname(w, item);
					button = open_dialog();
				}

				switch (button)
				{
					case OWSHOW:
						/* Call the viewer program or open a text window */
						if ( (deselect = app_specstart(AT_VIEW, w, &item, 1, kstate)) == 0 )
							deselect = txt_add_window(w, item, kstate, NULL);
						break;
					case OWEDIT:
						/* Call the editor program */
						if ( (deselect = app_specstart(AT_EDIT, w, &item, 1, kstate)) == 0 )
							alert_iprint(TNOEDIT);
						break;
					case OWRUN:
						/* Run this file as a program/application */
						onfile = FALSE;
						deselect = app_exec(NULL, &awork, NULL, NULL, 0, kstate);
						break;
					case OWUSE:
						/* Open this file with the selected application */
						deselect = app_exec(NULL, (APPLINFO *)selitem, w, &item, 1, kstate);
						break;
					default:
						/* Do nothing */
						break;
				}
				free(awork.name);
			}
			onfile = FALSE;
			break;

		default: 

			/* Object is a trashcan, printer, or not recognized */

			deselect = FALSE;
	}
	return deselect;
}


