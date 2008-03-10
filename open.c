/* 
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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
#include <library.h>
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
#include "prgtype.h"

boolean
	onone = FALSE,		/* true if only one instance should run */
	onfile = FALSE; 	/* true if app is started to open a file */

char
	*hyppage = NULL;	/* name of the hypertext page to display */


int trash_or_print(ITMTYPE type);


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

	if(chk_xd_open(openw, &owinfo) >= 0)
	{
		button = xd_form_do (&owinfo, ROOT);
		thebutton = button;

		switch(button)
		{
			case OWRUN:
			{
				prg_info(NULL, empty, 0, (PRGTYPE *)&awork);
				log_shortname(awork.shname, awork.name);

				if (!prgtype_dialog(NULL, 0, (PRGTYPE *)&awork, LS_APPL | LS_EDIT))
					button = 0;

				break;
			}
			case OWUSE:
			{
				log_shortname(fwork.filetype, awork.name);
				selitem = NULL;
				app_install(LS_SELA, &applikations); /* selitem is nonnull only if successful here */
			
				if (!selitem)
					button = 0;			
			}
			default:
			{
				break;
			}
		}

		xd_buttnorm(&owinfo, thebutton);
		xd_close(&owinfo);
	}

	return button;
}


/*
 * Open an item selected in a window or else explicitely specified.
 * If there is an explicit specification in "theitem", then "inw"
 * and "initem" are ignored. Action depends on the type of object
 * being detected. Also, if "theitem" is a program, "thecommand"
 * may contain a command line, if not NULL
 */

boolean item_open
(
	WINDOW *inw, 		/* window in which the selection is made */
	int initem,			/* ordinal of the selected item from the window */ 
	int kstate,			/* keyboard state while opening the item */ 
	char *theitem, 		/* explicitely specified full item name */
	char *thecommand	/* command line if the item is a program */
)
{
	WINDOW
		*w;				/* "inw", locally (i.e. maybe changed) */

	DIR_WINDOW
		simw;			/* pointer to a simulated dir window */

	APPLINFO 
		*appl;			/* Pointer to information on the app to run */

	char
		*qline = NULL,		/* requoted content of openline */
		*cmline = NULL,		/* Command passed to application from "Open" dialog" */
		*realname = NULL,	/* link target name */ 
		*blank,				/* pointer to a ' ' in the name */ 
		*path;				/* constructed path of the item to open */

	VLNAME 
		epath,			/* Path of the item specified in "Open" */ 
		ename;			/* name of the item specified in "Open" */

	int 
		item,			/* "initem", locally (i.e. maybe changed) */
		error,
		button;			/* index of the button pressed */

	boolean 
		alternate= FALSE, 
		deselect = FALSE;

	ITMTYPE 
		type;			/* item type (file, folder, program...) */


	item = initem;

	autoloc_off();

	if ( (kstate & K_ALT) != 0 )
		alternate = TRUE;
	
	if ( inw && !theitem )
	{
		/* 
		 * An item is specified by selection in a window; get its full name
		 * Note: it is possible that 'realname' be NULL (for trashcan, printer...) 
		 */

		realname = itm_tgtname(inw, initem);

		/* Try to divine which type of item this is */

		type = itm_tgttype( inw, initem );

		/* Is this really needed ? */

		if ( realname && type != ITM_NOTUSED && !trash_or_print(type) )
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
			button = chk_xd_dialog( newfolder, ROOT );
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
				 * Separate this command from item name by inserting a '0'
				 * instead of the (first) space between the two.
				 * If there is no command, 'cmline' will point
				 * to an empty string.
				 */

				cmline = (char *)empty; /* first, cmline points to an empty string */

				qline = malloc_chk(strlenq(openline));
				strcpyrq(qline, openline, '"', &blank); /* 34 = double quote */

				if(blank)
				{
					*blank = 0;
					cmline = blank + 1; /* now cmline points to after the first blank */
				}

				/* Unquote item name */

				strcpyuq(qline, qline);

				/* Convert item name to uppercase, keep the command as it is */
#if _MINT_
				if (!mint)
#endif
					strupr(qline);

				/* Find the real name of the item */

				if ((realname = x_fllink(qline)) == NULL )
				{
					free(qline);
					return FALSE;
				}
					
				/* Restore complete line (for the next opening) */

				if ( blank )
					*blank = ' ';
			}
			else
			{
				/* Item name is explicitely specified in "theitem" */

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
		 * be an error warning.
		 */

		type = diritem_type( (char *)realname );
	}
	
	/* Is this name (path) too long, or wrong in some other way? */

	if(realname && ((error = x_checkname(realname, NULL)) != 0))
	{
		free(realname);
		free(qline);
		xform_error(error);
		return FALSE;
	}

	/* Now that the type of the item is known, do something */

	switch(type)
	{
		case ITM_TRASH:
		case ITM_PRINTER:
		case ITM_NOTUSED:
		{
			/* Object is a trah can or a printer (or unknown) and can not be opened */

			alert_cantdo(trash_or_print(type), MNOOPEN);
			break;
		}
		default:
		{	
			/* Separate "realname" into "ename" and "epath" */

			if(type == ITM_NETOB)
			{
				strcpy(ename, realname);
				strcpy(epath, empty);
			}
			else
				split_path(epath, ename, realname);

			/* 
			 * Simulate some structures of a directory window so that existing
			 * routines expecting an item selected in a window can be used
			 */

			w = (WINDOW *)&simw;
			dir_simw(&simw, epath, ename, type);
			item = 0;
		}
	}

	free(realname);

	/* 
	 * Object real name and type are determined by now.
	 * Action according to type of the item follows...
	 */

	switch (type)
	{
		case ITM_DRIVE:
		{
			/* Object is a disk volume (codes 0 to 25 for check_drive() ) */

			if ( ( path = itm_fullname(w, item) ) != NULL )
			{
				if (check_drive( (path[0] & 0xDF) - 'A') == FALSE)
				{
					free(path);
					free(qline);
					return FALSE;
				}
				else
					deselect = dir_add_dwindow(path); 
			}
			else
				deselect = FALSE;

			break;
		}
		case ITM_PREVDIR:
		{
			/* Object is a parent folder */

			if ((path = fn_get_path(wd_path(w))) != NULL)
				deselect = dir_add_dwindow(path); 
			else
				deselect = FALSE;

			break;
		}
		case ITM_FOLDER:
		{
			/* Object is a folder */

			if ((path = itm_fullname(w, item)) != NULL)
				deselect = dir_add_dwindow(path); 
			else
				deselect = FALSE;

			break;
		}
		case ITM_PROGRAM:
		{
			/* Object is a program */

			if (( path = itm_fullname(w, item) ) != NULL )
			{
				deselect = app_exec(path, NULL, NULL, (int *)cmline, cmline ? -1 : 0, kstate);						
				free(path);
			}

			break;
		}
		case ITM_NETOB:
		case ITM_FILE:
		{
			/* Object is a file */

			naap = 0; /* see app_find() */
			onfile = TRUE;
			deselect = FALSE;

			if (!alternate && (appl = app_find(itm_name(w, item), TRUE)) != NULL)
				deselect = app_exec(NULL, appl, w, &item, 1, kstate);
			else if(naap == 0) 
			{
				/* File type assignment has been bypassed: now show/edit/cancel */

				if ( theitem )
					button = OWSHOW;
				else
				{
					memclr(&awork, sizeof(APPLINFO));
					awork.name = itm_fullname(w, item);
					button = open_dialog();
				}

				switch (button)
				{
					case OWSHOW:
					{
						/* Call the viewer program or open a text window */
						if ( (deselect = app_specstart(AT_VIEW, w, &item, 1, kstate)) == 0 )
							deselect = txt_add_window(w, item, kstate, NULL);
						break;
					}
					case OWEDIT:
					{
						/* Call the editor program */
						if ( (deselect = app_specstart(AT_EDIT, w, &item, 1, kstate)) == 0 )
							alert_iprint(TNOEDIT);
						break;
					}
					case OWRUN:
					{
						/* Run this file as a program/application */
						onfile = FALSE;
						deselect = app_exec(NULL, &awork, NULL, NULL, 0, kstate);
						break;
					}
					case OWUSE:
					{
						/* Open this file with the selected application */
						deselect = app_exec(NULL, (APPLINFO *)selitem, w, &item, 1, kstate);
						break;
					}
					default:
					{
						/* Do nothing */
						break;
					}
				}

				free(awork.name);
			}

			onfile = FALSE;
			break;
		}
		default: 
		{
			/* Object is a trashcan, printer, or not recognized */

			deselect = FALSE;
		}
	}

	free(qline);
	return deselect;
}


/* 
 * Open a page from the hypertext User manual. 
 * The hypertext viewer application is detected and a command is sent to it.
 * The command will typically be a menu text which is supposed to be
 * identical to a page name in the hypertext except for two leading blanks
 * and possibly an appended keyboard shortcut. All these will be stripped
 * in this routine. Therefore, the "page" string must be at least 
 * 6 characters long
 * Note 1: according to AV-protocol documentation, all filenames
 * must be input in capitals with complete paths (at least the documentation
 * for ST-Guide says so. How does this comply with unixoid filesystems? 
 * Maybe should use lowercase after all?)
 * Note 2: ST-guide supports a special path "*:\" meaning "all known paths" 
 */

void opn_hyphelp(void)
{
	APPLINFO *helpprg = app_find("*.HYP", FALSE);

	if(helpprg && hyppage)
	{
		LNAME cmd;

		onfile = TRUE;

		/* ST-Guide accepts only exactly one blank between filename and pagename */

		strcpy(cmd, "*:\\TERADESK.HYP");

		if(*hyppage)
		{
			strcat(cmd, &hyppage[1]);
			cmd[strlen(cmd) - 5] = '\0';
		}

		app_exec(NULL, helpprg, NULL, (int *)cmd, -1, 0);
	}
}

