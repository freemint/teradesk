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
#include <tos.h>
#include <vdi.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "desktop.h"
#include "error.h"
#include "xfilesys.h"
#include "config.h"	
#include "font.h"
#include "window.h"
#include "lists.h" 
#include "slider.h"
#include "filetype.h"
#include "applik.h"
#include "edit.h"
#include "file.h"
#include "prgtype.h"

/* Onder MultiTOS eerst kijken of een applikatie draait (met
   appl_find) Als dit het geval is boodschap sturen anders
   programma starten. */

char *editor;


void edit_set(const char *name)
{
	if (editor != NULL)
		free(editor);
	editor = (char *) name;
}


/* Activate the editor program */

boolean call_editor(WINDOW *w, int selected, int kstate)
{
	int list = selected;

	if (editor != NULL)
		return app_exec(editor, NULL, w, &list, 1, kstate, FALSE);
	else
	{
		alert_iprint(MNEDITOR);
		return FALSE;
	}
}


/* Is editor installed: TRUE if it is */

boolean edit_installed(void)
{
	return (editor == NULL) ? FALSE : TRUE;
}


/*
 * Check if an editor is assigned somewhere else but "pos"
 * Return: TRUE if it is OK to set editor now
 */

boolean check_edit(APPLINFO **list, boolean edit, int pos)
{

	int
		button,
		i = 0;

	APPLINFO
		*f = *list;


	if ( edit )
	{
		while( f != NULL )
		{
			if (i != pos )
			{
				if ( f->edit )
				{
					button = alert_printf( 2, ADUPFLG, f->shname );
					if ( button == 1 )
						return TRUE;
					else
						return FALSE;
				}
			}
			i++;
			f = f->next;
		}
	}
	return TRUE;
}


/* 
 * Unset editor flag wherever it is. 
 * Operate on the applications list being edited, 
 * not on the master list
 */

void unset_edit(APPLINFO **list)
{
	APPLINFO *app = *list;
	
	while(app)
	{
		app->edit = FALSE;
		app = app->next;
	}
}

#if ! TEXT_CFG_IN
/* 
 * Load name of editor program 
 */

int edit_load(XFILE *file)
{
	int error;

	edit_default();

	if ((editor = x_freadstr(file, NULL, sizeof(LNAME), &error)) == NULL)	
		return error;

	if (strlen(editor) == 0)
	{
		free(editor);
		editor = NULL;
	}
#if ! TEXT_CFG_IN
	else
	{
		/* 
		 * find the name of the editor in the application list
		 * and set the edit flag 
		 */

		APPLINFO *app;
		
		app = find_appl(&applikations, editor, NULL);
		if (app)
		{
			unset_edit(&applikations);
			app->edit = true;
		}
		
	}
#endif
	return 0;
}
#endif

/* 
 * Initialize name of editor program (i.e. there is none) 
 */

void edit_init(void)
{
	editor = NULL;
}

/* Same as above (why complicate ?) */

#if !TEXT_CFG_IN
void edit_default(void)
{
	edit_set(NULL);
}
#endif

