/*
 * Teradesk. Copyright (c) 1997, 2002  W. Klaren,
 *                         2002, 2003  H. Robbers,
 *                         2003, 2004  Dj. Vukovic
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
#include <boolean.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "font.h"
#include "xfilesys.h"
#include "config.h"
#include "window.h"
#include "open.h"
#include "dir.h"
#include "showinfo.h"
#include "file.h"
#include "copy.h"
#include "lists.h"
#include "slider.h"
#include "icon.h"
#include "va.h"
#include "desktop.h"


static WD_FUNC aw_functions =
{
	0L,						/* key */	
	0L,						/* button */
	wd_type_redraw,			/* redraw */
	wd_type_topped,			/* topped */
	wd_type_topped,			/* newtop */
	va_close,				/* closed */
	0L,						/* fulled */
	0L,						/* arrowed */
	0L,						/* hslider */
	0L,						/* vslider */
	0L,						/* sized */
	0L,						/* moved */
	0L, 					/* hndlmenu placeholder */
	0L,						/* top */
	0L,						/* iconify */
	0L						/* uniconify */
};

boolean va_reply = FALSE;	/* true if AV-protocol handshake is in progress */
int av_current;				/* ap_id of the currently messaging AV-client */

extern FONT dir_font;

static int answer[8];		/* buffer for answers to AV-clients */


AVTYPE 
	avwork,		/* work area for editing AV-clients data */
	*avclients;	/* List of signed-on AV-clients */

AVSTAT
	avswork,	/* work area for client statuses */
	*avstatus;	/* for logging status of AV clients */

AVSETW
	avsetw;		/* size for the next window */


static void copy_avstat( AVSTAT *t, AVSTAT *s);
static void rem_avstat(AVSTAT **list, AVSTAT *t);



/*
 * Close a window of an av-client. In xw_close a WM_CLOSE is sent
 * to the client.
 */

void va_close(WINDOW *w)
{
	xw_closedelete(w);
}


/* 
 * Delete all pseudowindows structures of AV-clients
 * or all windows belonging to a single client, if ap_id >= 0
 * (because in single-tos all acc windows are closed anyway
 * when a program is started).
 */

void va_delall(int ap_id)
{
	WINDOW *w = xw_last();

	while(w)
	{
		if ( w->xw_type == ACC_WIND && (ap_id < 0 || w->xw_ap_id == ap_id) )
			xw_closedelete(w);
		w = w->xw_prev;
	}
}


/*
 * Copy data for one AV-client into
 */

static void copy_avtype (AVTYPE *t, AVTYPE *s)
{
	*t = *s;
}


/*
 * Find a signed-on AV-client identified by its ap_id
 */

AVTYPE *va_findclient(int ap_id)
{
	AVTYPE *f = avclients;
	while(f)
	{
		if ( f->ap_id == ap_id )
			return f;
		f = f->next;
	}
	return NULL;
}


/*
 * Use these AV-client-list-specific functions to manipulate lists: 
 */

#pragma warn -sus
static LS_FUNC avlist_func =	/* for the list of clients */
{
	copy_avtype,
	rem,
	NULL,
	find_lsitem, /* find an item specified by name or position */
	NULL
};

static LS_FUNC avslist_func =	/* for the list of status srings */
{
	copy_avstat,
	rem_avstat,
	NULL,
	find_lsitem, /* find an item specified by name or position */
	NULL
};

#pragma warn .sus


/*
 * Check if the application is already running, has signed as
 * an av-client, and supports VA_START.
 * If true, then use the VA_START command.
 * Note: accessories are an exception: they can be sent VA_START
 * even if they had not signed on (this is needed at least for 
 * ST-Guide, and maybe some other as well) 
 * Note 2: some other apss also understand VA_START but do not sign-on
 * to the server. Pity.
 *
 * Parameters:
 *
 * program	- name of the program.
 * cmdline	- commandline.
 *
 * Result: TRUE if started using the VA_START.
 */

int va_start_prg(const char *program, ApplType type, const char *cmdline)
{
	char prgname[9], *ptr;
	int i, dest_ap_id;

	/*
	 * Check if globally available buffer is large enough 
	 * for the command line.
	 */

	if (strlen(cmdline) > (GLOBAL_MEM_SIZE - 1))
		return FALSE;

	/*
	 * Copy the name of the program (without path) to 'prgname' and
	 * append spaces to make the total length eight characters.
	 */

	if ((ptr = strrchr(program, '\\')) == NULL)
		return FALSE;

	ptr++;
	i = 0;

	/* Copy not more than first eight characters od program name */
 
	while (*ptr && (*ptr != '.') && (i < 8))
		prgname[i++] = *ptr++;

	/* Fill with blanks up to eighth character */

	while (i < 8)
		prgname[i++] = ' ';

	prgname[i] = 0;

	/* 
	 * Has this application signed on as an AV-client 
	 * that supports VA_START ? 
	 * (or, is it maybe an accessory? If neither, return FALSE)
	 * (Some?) accessories (ST-Guide, at least) sign-on only when
	 * they open a window, so it is necessary to be able to send
	 * them a VA_START even if they are not signed on.
	 * BUT: it seems that applications generally do NOT sign-on as
	 * AV-protocol clients, so this severely restricts the behaviour
	 * of the desktop vs applications. Maybe better to test each
	 * time if an application is still running.
	 */

/*
	theclient = (AVTYPE *)find_lsitem((LSTYPE **)&avclients, prgname, &i ); 

	if (type != PACC && (!theclient || (theclient->avcap3 & VV_START) != 0) )
		return FALSE; /* this is not a VA_START capable client */
*/

	/*
	 * Check if the application is still running.
	 * Something seems to be wrong here!!!!!
	 * If CAB is started, and then EVEREST from CAB
	 * (i.e. to view html source) and then EVEREST and CAB exited;
	 * next time CAB can not be started: check below
	 * returns dest_ap_id = 0, as if it is already running.
	 * Maybe avoid it so that a check is made if destination
	 * is the same as the current 
	 */

	dest_ap_id = appl_find(prgname);

	if (dest_ap_id >= 0)
	{
		if ( ap_id == dest_ap_id ) /* should this fix it ? */
			return FALSE;

		strcpy(global_memory, cmdline);

		answer[0] = VA_START;
		answer[1] = ap_id;
		answer[2] = 0;
		*(char **)(answer + 3) = global_memory;
		answer[5] = 0;
		answer[6] = 0;
		answer[7] = 0;

		appl_write(dest_ap_id, 16, answer);

		return TRUE;
	}
	else
		return FALSE;
}


#if _MORE_AV
/*
 * Send all registered clients a message- if they support (and need) it.
 * This routine must not be used if another handshake is in progress.
 */

static void va_send_all(int cap, int *message)
{
	AVTYPE *f = avclients;

	while(f)
	{
		if ( (f->avcap3 & cap) == cap )
			appl_write(f->ap_id, 16, message);

		f = f->next;
	}
}
#endif


/*
 * Report a font or a font change to an av-client
 * Currently, this always returns TRUE, even if failed
 */

boolean va_fontreply(int messid, int dest_ap_id)
{
	answer[0] = messid;					/* message id */
	answer[1] = ap_id;
	answer[2] = 0;
	answer[3] = dir_font.id;			/* id */
	answer[4] = dir_font.size;			/* size */
	answer[5] = 0;
	answer[6] = 0;
	answer[7] = 0;

#if _MORE_AV

	if ( messid == VA_FONTCHANGED )
	{
		answer[5] = dir_font.id;		/* simulate console font */
		answer[6] = dir_font.size;		/* and size */

		va_send_all(VV_FONTASKED | VV_FONTCHANGED, answer);
	}
	else		
#endif
		appl_write(dest_ap_id, 16, answer);

	return TRUE;
}


/*
 * Add a name to a reply string for an AV-client. If the name contains
 * spaces, it will be quoted with single quotes ('). 
 * Currently, names conatining quotes are not supported. 
 * This routine is also used to create a list of names to be sent
 * to an application using the Drag & drop protocol
 */

boolean va_add_name(int type, const char *name )
{
	long 
		i,								/* character index */
		l = strlen(name),				/* name length */
		g = strlen(global_memory);		/* cumulative string length */

	boolean
		q = FALSE;		/* True if name is to be quoted */



	if ( g + l + 4 >= GLOBAL_MEM_SIZE )
	{
		alert_iprint(TFNTLNG);		
		return FALSE;
	}	
	else
	{
		/* Add a blank before the name, but not before the first one */

		if ( *global_memory != 0 )
			strcat(global_memory, " ");

		/* Need to quote ? */

		if ( strchr(name, ' ') )
		{
			strcat(global_memory,"'");
			q = TRUE;
		}

		/* 
		 * Concatenate the name. If names containing quotes are to be
		 * supported some day, this will have to be done differently
		 */

		strcat(global_memory, name);

		/* Add a trailing backslash if needed */

		if ( type == (int)ITM_FOLDER )
			strcat(global_memory, "\\");

		/* Unquote ? (q is reset on the next call of this routine) */

		if (q)
			strcat(global_memory,"'");
	}
	return TRUE;
}


#if _MORE_AV

/*
 * Send path to be updated to registered AV clients
 */

boolean va_pathupdate( const char *path )
{
	int answer[8];

	if ( !va_reply )
	{
		answer[0] = VA_PATH_UPDATE;			/* message id */
		answer[1] = ap_id;
		answer[2] = 0;
		*(char **)(answer + 3) = global_memory;
		answer[5] = 0;
		answer[6] = 0;
		answer[7] = 0;

		*global_memory = 0;
		va_add_name( isroot(path)? ITM_DRIVE : ITM_FOLDER, path );

		va_send_all( VV_PATH_UPDATE, answer );
	}

	return TRUE;
}
#endif



/*
 * Drop items onto accwindow using AV/VA protocol.
 * Note: client window has to be signed-on for this to work
 */

boolean va_accdrop(WINDOW *dw, WINDOW *sw, int *list, int n, int kstate, int x, int y)
{
	AVTYPE *client;
	ITMTYPE itype;
	char *thename;
	int i;

	/* Find the data for the client which created this window */

	client = va_findclient(dw->xw_ap_id);

	if ( client )
	{
		/* Client found; add each name into a global memory string */

		*global_memory = 0;

		for ( i = 0; i < n; i++ )
		{
			if (
					((itype = itm_type(sw, list[i])) == ITM_NOTUSED ) ||
					((thename = itm_fullname(sw, list[i])) == NULL )  ||
					(!va_add_name(itype, thename))
				)
				{
					free(thename);
					return FALSE;
				} 

			free(thename);
		}

		/* Create a message and send it */

		answer[0] = VA_DRAGACCWIND;
		answer[1] = ap_id;
		answer[3] = dw->xw_handle;
		answer[4] = x;
		answer[5] = y;
		*(char **)(answer + 6) = global_memory;

		appl_write(client->ap_id, 16, answer);

		client->flags |= AVCOPYING;
		return TRUE;
	}
	else
		return FALSE;
}


/*
 * Initialize structures for using the AV-protocol.
 * Should be used before initialization of windows.
 */

void va_init(void)
{
	avclients = NULL;		/* list of AV-protocol clients */
	avstatus = NULL;		/* list of clients' statuses */
	avsetw.flag = FALSE;
	va_reply = FALSE;		/* a reply to a client is not in progress */
}


/*
 * Handle (most of) AV messages and FONT messages
 *
 * Parameters:
 *
 * message	- buffer with AES message.
 *
 * AV-protocol messages handled in these routines (request/reply):
 *
 * AV_PROTOKOLL/VA_PROTOSTATUS
 * AV_ASKCONFONT/VA_CONFONT
 * AV_ASKFILEFONT/VA_FILEFONT/VA_FONTCHANGED
 * VA_START/AV_STARTED 
 * VA_DRAGACCWIND
 * AV_ACCWINDOPEN
 * AV_ACCWINDCLOSED
 * AV_OPENWIND/VA_WINDOPEN
 * AV_XWIND/VA_XOPEN
 * AV_STARTPROG/VA_PROGSTART
 * AV_VIEW/VA_VIEWED
 * AV_PATH_UPDATE
 * AV_COPY_DRAGGED/VA_COPY_COMPLETE
 * AV_DRAG_ON_WINDOW/VA_DRAG_COMPLETE
 * AV_FILEINFO/VA_FILECHANGED
 * AV_DELFILE/VA_FILEDELETED
 * AV_COPYFILE/VA_FILECOPIED
 * AV_STATUS
 * AV_GETSTATUS/VA_SETSTATUS
 * AV_SETWINDPOS
 * AV_SENDKEY
 * VA_PATH_UPDATE
 * AV_EXIT
 *
 * Unsupported:
 *
 * AV_OPENCONSOLE/VA_CONSOLEOPEN
 * AV_ASKOBJECT/VA_OBJECT
 *
 * FONT protocol messages supported:
 * 
 * FONT_SELECT/FONT_CHANGED  (FONT protocol, see also font.c)
 *
 */

void handle_av_protocol(const int *message)
{
	char 
		*path = NULL, 
		*mask = NULL;

	int 
		j,
		error,
		stat;

	boolean 
		reply = TRUE;

	AVTYPE
/* not used for the time being, see below
		*oldclient,
*/
		*theclient;

#if _MORE_AV

	AVSTAT
		*thestatus;

#endif

	WINDOW 
		*aw;

	/* Clear the answer block; then set TeraDesk's ap_id where it will be */

	memset( answer, 0, sizeof(answer) );
	answer[1] = ap_id;

	/* Find data for the client if it exists */

	av_current = message[1];					/* who sent the message (its ap_id) */
	theclient = va_findclient(av_current);		/* are there any data for it */

	/* Ignore unknown clients, except if they are signing-on */

	if( message[0] != AV_PROTOKOLL && !theclient )
		return;

	switch(message[0])
	{
	/* AV protocol */

	case AV_PROTOKOLL:
		/*
		 * Client signing on.
		 * Mostly ignore the features send by the (sender) client.
		 * Return the server-supported features.
		 */

		strcpy(avwork.name, (*(char **)(message + 6)) );
		avwork.ap_id = appl_find( (const char *)avwork.name );
		avwork.avcap3 = message[3]; /* notify client-supported features */
		avwork.flags = 0;


/* It is probably better NOT to do this

		/*
		 * Check if an av-client with this name already exists.
		 * If it does, delete it and its (pseudo)windows.
		 * This is because a client might have terminated 
		 * without notifying of its exit.
		 * Note: this is not very good!!! it should be checked
		 * if the client was still running.
		 */		

		if ( (oldclient = (AVTYPE *)find_lsitem( (LSTYPE **)(&avclients), avwork.name, &j)) != NULL )
		{
			va_delall(oldclient->ap_id);
			rem( (LSTYPE **)(&avclients), (LSTYPE *)oldclient);
		}
*/

		if (!lsadd((LSTYPE **)&avclients, sizeof(AVTYPE), (LSTYPE *)(&avwork), 32700, copy_avtype ))
			reply = FALSE;
		else 
		{
			strcpy(global_memory, "DESKTOP "); /* must be exactly 8 characters long */

			answer[0] = VA_PROTOSTATUS;
			answer[3] = AA_SENDKEY |
						AA_ASKFILEFONT |
						AA_ASKCONFONT |
						AA_COPY_DRAGGED |
						AA_STARTPROG |
						AA_ACCWIND | 
						AA_EXIT |
#if _MORE_AV

						AA_SRV_QUOTING |

						AA_STATUS |
						AA_XWIND |
						AA_OPENWIND |
						AA_DRAG_ON_WINDOW | /* also for AV_WHAT_IZIT and AV_PATH_UPDATE */
						AA_FILE| /* for FILEINFO */
						AA_FONTCHANGED |
#endif
						AA_STARTED;

			answer[4] =	
#if _MORE_AV
						AA_COPY |
						AA_DELETE |
						AA_SETWINDPOS | 
#endif
						AA_VIEW ;


			*(char **)(answer + 6) = global_memory;

		}
		break;

	case AV_EXIT:

		va_delall(av_current); /* delete all client's windows, just in case */
		rem( (LSTYPE **)&avclients, (LSTYPE *)va_findclient(av_current));
		reply = FALSE;
		break;

	case AV_ASKCONFONT:

		/* 
		 * Return the id and size of the console window font.
		 * Reply to this is currently the same as for directory window 
		 */

	case AV_ASKFILEFONT:

		/* Return the id and size of the currently selected directory font. */

		theclient->avcap3 |= VV_FONTASKED;

		va_fontreply( (message[0] == AV_ASKCONFONT) ? VA_CONFONT : VA_FILEFONT, message[1]); 
		reply = FALSE;

		break;

	case AV_ACCWINDOPEN:

		/* 
		 * Create an av-client pseudowindow; 
		 * use "flags" parameter to pass window handle
		 * which the client supplied in message [3]
		 */
		aw = xw_create(ACC_WIND, &aw_functions, message[3], NULL, sizeof(ACC_WINDOW), NULL, &error );
		aw->xw_opened = TRUE;
		aw->xw_ap_id = av_current;
		reply = FALSE;
		break;

	case AV_ACCWINDCLOSED:

		/* Client has closed a window, identified by its handle */

		aw = xw_first();
		while ( aw )
		{
			if ( aw->xw_handle == message[3] )
				xw_delete(aw);
			aw = aw->xw_next;
		}
		reply = FALSE;
		break;

	case AV_COPY_DRAGGED:

		/* Confirmation of copying to an acc window */

		if ( theclient->flags & AVCOPYING )
		{
			answer[0] = VA_COPY_COMPLETE;
			answer[3] = 1;
			theclient->flags &= ~AVCOPYING;
		}

		break;

#if _MORE_AV
	case AV_XWIND:

		/* 
		 * Open a directory window with additional features
		 * Action is currently the similar as for just opening a window;
		 * Ignored features: wildcard object selection.
		 * It is not entirely clear whether this message is 
		 * correctly supported 
		 */

	case AV_OPENWIND:

		/* Open a directory window (path must be kept) */

		path = strdup(*(char **)(message + 3));

		if ( path && *path )
		{
			dir_trim_slash(path);
			stat = 1;

			/* If an existing window can not be topped, open a new one */

			if ( !( message[0] == AV_XWIND && (message[7] & 0x01) && dir_do_path(path, DO_PATH_TOP)) )
			{
				mask = strdup(*(char **)(message + 5));
				stat = ( (path != NULL) && (mask != NULL) );

				if ( stat )
					stat = (int)dir_add_window(path, mask, NULL);
			}
		}
		else
			stat = 0;

		answer[0] = ( message[0] == AV_OPENWIND) ? VA_WINDOPEN : VA_XOPEN;
		answer[3] = stat;				/* status */
		break;

#endif

	case AV_STARTPROG:

		/* Start a program with possibly a command line */

		mask = strdup(*(char **)(message + 5)); 
		answer[7] = message[7];

	case AV_VIEW:

		/* 
		 * Activate a viewer for the file. Currently, TeraDesk does not
		 * differentiate between a viewer and a processing program;
		 * so, behaviour for AV_VIEW and AV_STARTPROG is essentially
		 * the same
		 */

		path = strdup(*(char **)(message + 3));

		if ( path && *path )
			stat = item_open( NULL, 0, 0, path, mask );
		else
			stat = 0;

		answer[0] = (message[0] == AV_STARTPROG) ? VA_PROGSTART : VA_VIEWED;
		answer[3] = stat;
		free(path);
		free(mask);
		break;

#if _MORE_AV

	case AV_SETWINDPOS:

		avsetw.flag = TRUE;
		avsetw.size.x = message[3];
		avsetw.size.y = message[4];
		avsetw.size.w = message[5];
		avsetw.size.h = message[6];
		reply = FALSE;
		break;

	case AV_PATH_UPDATE:

		/* Update a dir window */

		path = strdup(*(char **)(message + 3));
		if ( path && *path )
		{
			dir_trim_slash(path);
			dir_do_path(path, DO_PATH_UPDATE);
		}

		free(path);
		reply = FALSE; /* no reply to this message */
		break;

	case AV_STATUS: 

		/* Note: "path" has to be kept; it contains the status string */

		path = strdup(*(char **)(message + 3));

		thestatus = (AVSTAT *)find_lsitem((LSTYPE **)&avstatus, theclient->name, &j);

		if ( path )
		{
			/* A status string is supplied by the client */

			if ( thestatus )
			{
				/* Yes, there is status string for this client */

				if ( strcmp(thestatus->stat, path) == 0 )
					/* string is the same, no need to keep "path" */
					free(path);
				else
				{
					/* sting changed; replace pointer to status string */
					free(thestatus->stat);
					thestatus->stat = path;
				}
			}
			else	
			{
				/* add this status string to the pool */
				strcpy(avswork.name, theclient->name);
				avswork.stat = path;

				if (!lsadd((LSTYPE **)&avstatus, sizeof(AVSTAT), (LSTYPE *)(&avswork), 32700, copy_avstat ))
					free(path);
			}
		}

		reply = FALSE;
		break;
		
	case AV_GETSTATUS:

		answer[0] = VA_SETSTATUS;

		/* 
		 * Note: if status is not available, NULL is supposed to be
		 * returned as a pointer to the string. Maybe it would be better
		 * to supply an empty string ???
		 */

		thestatus = (AVSTAT *)find_lsitem((LSTYPE **)&avstatus, theclient->name, &j);
		if ( thestatus && thestatus->stat )
		{
			strcpy(global_memory, thestatus->stat);
			*(char **)(answer + 3) = global_memory;
		}
		else
			*(char **)(answer + 3) = NULL;

		break;

	case AV_WHAT_IZIT:
	{
		int item, wind_ap_id;
		ITMTYPE itype = ITM_NOTUSED;

		/* These are VA_THAT_IZIT answers which correspond to ITMTYPEs */

		int answertypes[]=
		{
			VA_OB_UNKNOWN,	/* unused-unknown */
			VA_OB_DRIVE,	/* disk volume */
			VA_OB_SHREDDER,	/* was TRASHCAN first */
			VA_OB_UNKNOWN, 	/* printer- funny it is not provided in VA */
			VA_OB_FOLDER,	/* folder */
			VA_OB_FILE, 	/* program file */
			VA_OB_FILE,		/* any file */
			VA_OB_FOLDER,	/* parent directory */
			VA_OB_FILE		/* symbolic link */
		};

		*global_memory = 0; /* clear any old strings */

		answer[4] = VA_OB_UNKNOWN;

		/* Find the owner of the window (can't be always done in single-tos) */

		wind_get( wind_find(message[3], message[4]), WF_OWNER, &wind_ap_id);

		if ( wind_ap_id != ap_id ) /* this is not teradesk's window */
			answer[4] = VA_OB_WINDOW;
		else
			if ( (aw = xw_find(message[3], message[4])) != NULL )
			{
				/* Yes, this is TeraDesk's window */

				if ( (item = itm_find(aw, message[3], message[4])) >= 0 )
				{
					/* An item can be located */

					itype = itm_type(aw, item);
					if ( itype >= ITM_NOTUSED && itype <= ITM_LINK )
					{
						answer[4] = answertypes[itype];

						/* 
						 * Fortunately, single item names are always shorter
						 * than global memory buffer. No need to check size. 
						 */
						if (  isfile(itype) || itype == ITM_DRIVE )
						{
							/* Should fullname or just name be used below ? */

							if ( (path = itm_fullname(aw, item)) != NULL)
							{
								va_add_name(itype, path);
								*(char **)(answer + 3) = global_memory;
								free(path);
							}
						}
					}
				}
		}
		answer[0] = VA_THAT_IZIT;			
	}

		break;

	case AV_DRAG_ON_WINDOW:
		path = strdup(*(char **)(message + 6));	/* source */
		goto processname;
	case AV_COPYFILE:
		mask = strdup(*(char **)(message + 5)); /* destination */
	case AV_DELFILE:
	case AV_FILEINFO:
		path = strdup(*(char **)(message + 3)); /* source */

		processname:;

		if ( path && *path )
		{
			dir_trim_slash(path);
			*global_memory = 0;

			{
				char 
					*p = path,		/* current position in the string */
					*wpath = NULL,	/* simulated window path */
					*cs, *cq, 		/* position of the next " " and "'" */
					*pp = NULL;		/* position of the next name */

				boolean q = FALSE;	/* true if name quoted */

				ITMTYPE itype;		/* type of the item */
				DIR_WINDOW *ww;		/* pointer to the simulated window */
				int list = 0;		/* simulated selected item */

				stat = 1;

				/* In this loop, items in the message are processed one by one */

				while(stat && p && *p)
				{
					if (*p == 39) 	/* single quote */
					{
						p++;		/* character after the quote */
						q = TRUE;
					}

					strip_name(p, p);

					cq = strchr(p, 39);  	/* next quote (unquote) */

					if ( q && cq )			/* quoted and unquote exists ? */
						*cq++ = 0;			/* terminate string at quote */
					else 
						cq = p;

					cs = strchr(cq, ' ');	/* space after the quote */

					pp = NULL;

					if ( cs )				/* there is a next space */
					{
						pp = cs + 1L; 		/* character after the space */
						*cs = 0;
					}

					/* 
					 * Try to determine the type of the item.
					 * This is done by examining the name and by
					 * analyzing the object attributes (i.e. data
					 * from the disk are being read here)- if the
					 * object does not exist an error alert will be
					 * displayed and ITM_NOTUSED returned.
					 */

					itype = diritem_type(p);

					if ( itype != ITM_NOTUSED )
					{
						wpath = fn_get_path(p);

						dir_simw(&ww, wpath, fn_get_name(p), itype, (size_t)1, 0);

						/* 
						 * Some routines will perform differently when
						 * working as a response to a VA-protocol command.
						 * This is set through va_reply.
						 * Note: these four messages may also provoke
						 * a VA_PATH_UPDATE response	
						 */

						va_reply = TRUE;

						switch(message[0])
						{
						case AV_DRAG_ON_WINDOW:
							answer[0] = VA_DRAG_COMPLETE;
							stat = (int)itm_move( (WINDOW *)ww, 0, message[3], message[4], message[5]);	
							break;
						case AV_COPYFILE:
						{
							/* 
							 * Note: this is not fully compliant to the
							 * protocol: links can not be created
							 */
							int old_prefs = options.cprefs;
							options.cprefs = (message[7] & 4) ? old_prefs : (old_prefs & ~CF_OVERW);
							answer[0] = VA_FILECOPIED;
							rename_files = (message[7] & 2) ? TRUE : FALSE;
							stat = (int)itmlist_op((WINDOW *)ww, 1, &list, mask,( message[7] & 1) ? CMD_MOVE : CMD_COPY);
							options.cprefs = old_prefs;
							break;
						}
						case AV_DELFILE:
							answer[0] = VA_FILEDELETED;
							stat = (int)itmlist_op((WINDOW *)ww, 1, &list, NULL, CMD_DELETE);
							break;
						case AV_FILEINFO:
							answer[0] = VA_FILECHANGED;
							item_showinfo((WINDOW *)ww, 1, &list, FALSE);
							*(char **)(answer + 6) = global_memory;	
							stat = 1; /* but it is not always so! */
							break;
						}
						free(wpath);
					}
					else
						stat = 0;

					p = pp;

				} /* while */
			}

		}
		else
			stat = 0;

		answer[3] = stat;

		va_reply = FALSE;				

		if ( message[0] == AV_FILEINFO )
			closeinfo(); 

		strip_name(global_memory, (const char *)global_memory);

		free(path);
		free(mask);
		break;
#endif

#if _FONT_SEL

	case FONT_SELECT:

		/* Select a font for the client (FONT protocol) */

		fnt_mdialog(message[1], message[3], message[4], message[5],
					message[6], message[7], 1);
		reply = FALSE;
		break;
#endif

	default:
		/* 
		 * Beside the unsupported messages, this also handles
		 * those (mostly acknowledge) messages which do not need 
		 * any action
		 */
		reply = FALSE;
		break;
	}

	if ( reply )
		appl_write(av_current, 16, answer);

	answer[0] = 0;
}


/*
 * Remove all AV-clients status data from the list
 */

void rem_all_avstat(void)
{
	rem_all( (LSTYPE **)(&avstatus), rem_avstat );
}


#if !__USE_MACROS
/*
 * Initialize AV-clients status data.
 * This routine is not really necessary; it exists 
 * just for the consistency of style.
 * It can be substituted with a macro.
 */

void vastat_default(void)
{
	rem_all_avstat();
}

#endif

/*
 * Structures for saving and loading AV-protocol client status
 */

typedef struct
{
	SNAME name;
	VLNAME stat;
} SINFO;


static SINFO this;

CfgEntry stat_table[] =
{
	{CFG_HDR, 0, "status" },
	{CFG_BEG},
	{CFG_S,   0, "name",  this.name	},
	{CFG_S,   0, "stat",  this.stat	},
	{CFG_END},
	{CFG_LAST}
};


/*
 * Copy AV-clients status data into the list
 * Note: for the status string, only the pointer is copied,
 * the string itself is not duplicated.
 */

static void copy_avstat(AVSTAT *t, AVSTAT *s)
{
	strcpy(t->name, s->name);
	t->stat = s->stat;
}


/*
 * Remove one AV-client's status data from the list
 */

static void rem_avstat(AVSTAT **list, AVSTAT *t)
{
	free(t->stat);							/* the string itself */
	rem((LSTYPE **)list, (LSTYPE *)t);		/* the list entry */
}


/*
 * Handle saving/loading status data for one AV-client
 */

static CfgNest one_avstat
{
	*error = 0;

	if ( io == CFG_SAVE )
	{
		AVSTAT *a = avstatus;

		while ( (*error == 0) && a)
		{
			strcpy(this.name, a->name);
			strcpy(this.stat, a->stat);
			*error = CfgSave(file, stat_table, lvl + 1, CFGEMP); 
	
			a = a->next;
		}
	}
	else
	{
		memset(&avswork, 0, sizeof(avswork));

		*error = CfgLoad(file, stat_table, MAX_CFGLINE - 1, lvl + 1); 

		if ( *error == 0 )				/* got one ? */
		{
 			if ( this.name[0] == 0 )
				*error = EFRVAL;
			else
			{
				char *stat = malloc(strlen(this.stat) + 1);

				if (stat)
				{
					strcpy(stat, this.stat);
					strcpy(avswork.name, this.name);
					avswork.stat = stat;

					if ( lsadd( 
				             (LSTYPE **)&avstatus, 
				              sizeof(avswork), 
				              (LSTYPE *)&avswork, 
				              32000, 
				              copy_avstat
				           ) == NULL 
				   		)
					{
						free(stat);
						*error = ENOMSG;
					}
				}
				else
					*error = ENSMEM;
			}
		}
	}
}


static CfgEntry va_table[] =
{
	{CFG_HDR, 0, "avstats" },
	{CFG_BEG},
	{CFG_NEST,0, "status", one_avstat  },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Handle saving/loading of all AV-client status data
 */

CfgNest va_config
{ 
	*error = handle_cfg(file, va_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, rem_all_avstat, vastat_default);
}
