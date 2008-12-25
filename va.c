/*
 * Teradesk. Copyright (c) 1997 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2008  Dj. Vukovic
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
#include <library.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "font.h"
#include "xfilesys.h"
#include "config.h"
#include "window.h"
#include "open.h"
#include "copy.h"
#include "dir.h"
#include "showinfo.h"
#include "file.h"
#include "lists.h"
#include "slider.h"
#include "icon.h"
#include "va.h"


static void va_redraw(WINDOW *w, RECT *r);
static void va_iconify(WINDOW *w, RECT *r);
static void va_uniconify(WINDOW *w, RECT *r);
static void va_fulled(WINDOW *w, int dummy);
static void copy_avstat( AVSTAT *t, AVSTAT *s);
static void rem_avstat(AVSTAT **list, AVSTAT *t);
void load_settings(char *newinfname);
char *app_find_name(char *path, boolean full);

static WD_FUNC aw_functions =
{
	0L,						/* handle key */	
	0L,						/* handle button */
	va_redraw,				/* redraw; was wd_type_redraw */
	wd_type_topped,			/* topped */
	wd_type_bottomed,		/* bottomed */
	wd_type_topped,			/* newtop */
	wd_type_close,			/* closed */
	va_fulled,				/* fulled */
	xw_nop2,				/* arrowed */
	0L,						/* hslider */
	0L,						/* vslider */
	0L,						/* sized */
	0L,						/* moved */
	0L, 					/* hndlmenu */
	0L,						/* top */
	va_iconify,				/* iconify */
	va_uniconify			/* uniconify */
};

boolean 
	va_reply = FALSE;		/* true if AV-protocol handshake is in progress */

int
	av_current;				/* ap_id of the currently messaging AV-client */

extern XDFONT 
	dir_font;				/* data for the font used in directories */

AVTYPE 
	avwork,					/* work area for editing AV-clients data */
	*avclients;				/* List of signed-on AV-clients */

static AVSTAT
	avswork,				/* work area for client statuses */
	*avstatus;				/* for logging status of AV-clients */

AVSETW
	avsetw;					/* size for the next window */

const char
	*thisapp = "DESKTOP ";	/* AV-protocol name of this application */

/* These are VA_THAT_IZIT answers which correspond to ITMTYPEs */

#if _MORE_AV

static const int answertypes[]=
{
	VA_OB_UNKNOWN,	/* unused-unknown */
	VA_OB_DRIVE,	/* disk volume */
	VA_OB_SHREDDER,	/* was TRASHCAN first */
	VA_OB_UNKNOWN, 	/* printer- funny it is not provided in VA */
	VA_OB_FOLDER,	/* folder */
	VA_OB_FILE, 	/* program file */
	VA_OB_FILE,		/* any file */
	VA_OB_FOLDER,	/* parent directory */
	VA_OB_FILE,		/* symbolic link */
	VA_OB_FILE		/* network object */
};

#endif

#define SINGLE_Q 39 /* single quote character */

/*
 * Redraw AV-client's wndow
 */

static void va_redraw(WINDOW *w, RECT *r)
{
	xw_send_redraw(w, WM_REDRAW, r);
}


/*
 * Full the client's window
 */

static void va_fulled(WINDOW *w, int dummy)
{
	xw_send(w, WM_FULLED);
}


/*
 * Iconify the client's window
 */

static void va_iconify(WINDOW *w, RECT *r)
{
	xw_send_rect(w, WM_ICONIFY, w->xw_ap_id, r);
	w->xw_xflags |= XWF_ICN;
}


/*
 * Uniconify the client's window
 */

static void va_uniconify(WINDOW *w, RECT *r)
{
	wd_type_topped(w);
	xw_send_rect(w, WM_UNICONIFY, w->xw_ap_id, r);
	w->xw_xflags &= ~XWF_ICN;
}


/*
 * Clear the 'answer' buffer and set word [1] to TeraDesk's ap_id
 */

static void va_clranswer(int *va_answer)
{
	memclr(va_answer, (size_t)16); 
	va_answer[1] = ap_id;
}


/*
 * Initialize structures for using the AV-protocol.
 * Should be called before initialization of windows.
 */

void va_init(void)
{
	avclients = NULL;		/* list of AV-protocol clients */
	avstatus = NULL;		/* list of clients' statuses */
	avsetw.flag = FALSE;	/* don't set next window size */
	va_reply = FALSE;		/* a reply to a client is not in progress */
}


/*
 * Find out if there are any AV-client windows "open"
 * Return index of the first accessory window 
 * This routine will also log any accessory windows open as TeraDesk windows
 */

WINDOW *va_accw(void)
{
	WINDOW *w = xw_first();

	while(w)
	{
		if ( w->xw_type == ACC_WIND )
		{
			/* A call to xw_top will log any acc window that is topped */

			xw_top(); 
			return w;
		}

		w = xw_next(w);
	}
	
	return NULL;
}


/* 
 * Delete all pseudowindows structures of AV-clients (if ap_id < 0)
 * or all windows belonging to a single client (if ap_id >= 0).
 * (because in single-tos all acc windows are closed anyway
 * when a program is started).
 * If force is FALSE, pseudowindows will be just closed (by sending
 * messages to relevant applicatons) and TeraDesk will rely on those
 * applications to send AV_ACCWINDCLOSED; if force is TRUE, 
 * the pseudo-windows structures in TeraDesk will be deleted 
 */

void va_delall(int ap_id, boolean force)
{
	WINDOW *prev, *w = xw_last();

	while(w)
	{
		prev = xw_prev(w);

		if ( w->xw_type == ACC_WIND && (ap_id < 0 || w->xw_ap_id == ap_id) )
		{
			xw_close(w);

			if(force)
				xw_delete(w);
		}

		w = prev;
	}
}


/*
 * Copy data for one AV-client into. Here, maybe there is no need
 * to preserve target's pointer to next ????
 */

static void copy_avtype (AVTYPE *t, AVTYPE *s)
{
	AVTYPE *next = t->next;
	*t = *s;
	t->next = next;
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
 * Remove an AV-protocol client from the list. Close all its windows
 */

void rem_avtype(AVTYPE **list, AVTYPE *item)
{
	xw_dosend = 0;
	va_delall(item->ap_id, TRUE);
	lsrem((LSTYPE **)list, (LSTYPE *)item);
	xw_dosend = 1;
}


/*
 * Use these AV-client-list-specific functions to manipulate lists: 
 */

#pragma warn -sus
static LS_FUNC avlist_func =	/* for the list of clients */
{
	copy_avtype,
	rem_avtype,
	NULL,
	find_lsitem, /* find an item specified by name or position */
	NULL
};

static LS_FUNC avslist_func =	/* for the list of status strings */
{
	copy_avstat,
	rem_avstat,
	NULL,
	find_lsitem, /* find an item specified by name or position */
	NULL
};

#pragma warn .sus


/*
 * Check for existing AV-clients.
 * Maybe some of the signed-on AV-clients has crashed or illegally exited
 * without signing-off, and so its windows and data generally have to be
 * removed. In such cases, messages should not be sent to those clients,
 * because they do not in fact exist anymore.
 * Run this function at some convenient moments, such as program startup
 */

void va_checkclient(void)
{
	AVTYPE *f = avclients, *next;

	while(f)
	{
		next = f->next; /* f is about to be destroyed, so save f->next */

		if (appl_find(f->name) < 0)
			rem_avtype(&avclients, f);

		f = next;
	}
}


/*
 * Check if the application specified is already running, 
 * has signed as an av-client, and supports VA_START.
 * If true, then use the VA_START message to pass the command.
 * Note: accessories are maybe an exception?: they can be sent VA_START
 * even if they had not signed on.
 * Note 2: some other aps also understand VA_START but do not sign-on
 * to the server. Pity. So the above idea is impractical. See below.
 *
 * Parameters:
 *
 * program	- name of the program.
 * cmdl	- commandline.
 *
 * If flag 'onfile' is set to TRUE elsewhere, this routine will not ask
 * about starting another instance of an already running application
 *
 * Result: TRUE if command has been passed using VA_START.
 */

int va_start_prg(const char *program, ApplType type, const char *cmdl)
{
	char 
		*fb,			/* not used here */
		prgname[14];	/* 8-chars-long name of the program. Must be 14 bytes long here */

	int 
		va_answer[8],	/* local answer-message buffer */
		dest_ap_id;		/* ap_id of the application parameters are sent to */


	/* 
	 * Use this opportunity to check for existing AV-clients.
	 * As programs are not started very often, probably there will
	 * not be any noticeable penalty in speed here. 
	 */

	va_checkclient();

	/*
	 * Check if globally available buffer is large enough 
	 * for the command line.
	 */

	if (strlen(cmdl) < global_mem_size)
	{
		/* 
		 * Now copy not more than first eight characters of program name (no path) 
		 * (should it be converted to uppercase?)
		 * YES- appl_find() in Magic is case sensitive.
		 */
 
		cv_tos_fn2form(prgname, fn_get_name(program));
		strupr(prgname);

		prgname[8] = 0;

		/* 
		 * Has this application signed on as an AV-client?
		 * that supports VA_START ? 
		 * (or, is it maybe an accessory? If neither, return FALSE)
		 * Some accessories appear to sign-on only when
		 * they open a window, so it is necessary to be able to send
		 * them a VA_START even if they have not signed on.
		 * BUT: it seems that applications generally do NOT sign-on as
		 * AV-protocol clients, so this severely restricts the behaviour
		 * of the desktop vs applications. Maybe better to test each
		 * time if an application is still running?
		 * Note: this function may be called even if no AV-protocol clients
		 * had previously signed-on. 
		 */

/* disabled for the time being

		theclient = (AVTYPE *)find_lsitem((LSTYPE **)&avclients, prgname, &i );
 
		if (type != PACC && (!theclient || (theclient->avcap3 & VV_START) != 0) )
			return FALSE; /* this is not a VA_START capable client */
*/

		/* Check if the application with this name is still/already running */

		dest_ap_id = appl_find(prgname);

		if (dest_ap_id >= 0)
		{
			/* 
			 * Yes, this applicaton already runs. Should it be
			 * started again? If this is a certain kind of special app,
			 * just ignore it if it already runs, but behave as if it
			 * has been started
			 */

			if(onone)
				return TRUE;

			/*			
			 * Consider again: should the application be started anew?
			 * Do not ask this if this application is to be used to open
			 * a file- in that case always assume that the parameters will be
			 * passed to the running application)
			 */

			if ( onfile || (alert_query(MDUPAPP) != 1) )
			{
				/*
				 * Something seems to be wrong here!!!!!
				 * If CAB is started, and then EVEREST from CAB
				 * (i.e. to view html source) and then EVEREST and CAB exited;
				 * next time CAB can not be started: check below
				 * returns dest_ap_id = 0, as if it is already running.
				 * Maybe avoid it so that a check is made if destination
				 * is the same as the current app.  
				 * However, avoid confusing CAB with TeraDesk itself.
				 */

				if ( ap_id == dest_ap_id && strcmp(prgname, thisapp) != 0 ) /* should this fix CAB ? */
					return FALSE;

				/* Double quotes must be converted to single quotes */

				strcpyrq(global_memory, (char *)cmdl, SINGLE_Q, &fb);
				va_clranswer(va_answer);

				va_answer[0] = VA_START;
				*(char **)(va_answer + 3) = global_memory;

				appl_write(dest_ap_id, 16, va_answer);	

				return TRUE;
			}
		}
	}

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
	int va_answer[8];

	va_clranswer(va_answer);

	/* Note: font colour and effects are not sent in these replies */

	va_answer[0] = messid;			/* message id */
	va_answer[3] = dir_font.id;		/* directory font id */
	va_answer[4] = dir_font.size;	/* font size */

#if _MORE_AV
	if ( messid == VA_FONTCHANGED )
	{
		va_answer[5] = dir_font.id;		/* simulate console font */
		va_answer[6] = dir_font.size;	/* and size */

		va_send_all(VV_FONTASKED | VV_FONTCHANGED, va_answer);
	}
	else		
#endif
		appl_write(dest_ap_id, 16, va_answer);

	return TRUE;
}


/*
 * Add a name to a reply string for an AV-client. If the name contains
 * spaces, it will be quoted with single quotes ('). If the name contais
 * the single-quote character, it will be doubled.
 * This routine is also used to create a list of names to be sent
 * to an application using the Drag & drop protocol
 */

boolean va_add_name(int type, const char *name)
{
	long 
		g = strlen(global_memory);		/* cumulative string length */


	/* 
	 * Check for available space in the global buffer:
	 * Must fit: existing string, the name (maybe quoted), a blank 
	 * a backslash and a terminating 0. These 3 bytes are accounted
	 * for in strlenq.
	 */

	if ( g + strlenq(name) > global_mem_size )
	{
		alert_iprint(TFNTLNG);		
		return FALSE;
	}	
	else
	{
		char 
			*pd;

		/* Add a blank before the name, but not before the first one */

		if ( *global_memory != 0 )
			strcat(global_memory, " ");

		pd = global_memory + strlen(global_memory);

		/* Add the name, quoting it if necessary */

		pd = strcpyq(pd, name, SINGLE_Q);

		/* Add a trailing backslash to folder names */

		if(*name && (type == ITM_FOLDER || type == ITM_PREVDIR))
		{
			if(*(pd - 1) == SINGLE_Q)
			{
				*(pd - 1) = '\\';
				*pd++ = SINGLE_Q;
			}
			else
				*pd++ = '\\';

			*pd = '\0';
		}
	}

	return TRUE;
}


#if _MORE_AV

/*
 * Send window path to be updated to registered AV clients
 *(can global 'answer' be used here? va_pathupdate happens
 * unprovoked by clients, maybe during a handshake).
 * If composing of another VA_ mesage is in progress,
 * messages will not be sent; instead. windows will be marked
 * for sending the message.
 * Return TRUE if messages sent.
 * Take care to use this routine only for directory windows.
 */

boolean va_pathupdate( WINDOW *w )
{
	if ( !va_reply )
	{
		int va_answer[8];

		va_clranswer(va_answer);

		va_answer[0] = VA_PATH_UPDATE;			/* message id */
		*(char **)(va_answer + 3) = global_memory;
		*global_memory = 0; /* so that va_add_name() works properly */

		va_add_name( isroot(((DIR_WINDOW *)w)->path) ? ITM_DRIVE : ITM_FOLDER, ((DIR_WINDOW *)w)->path );
		va_send_all( VV_PATH_UPDATE, va_answer );

		((DIR_WINDOW *)w)->va_refresh = FALSE;

		return TRUE;
	}

	((DIR_WINDOW *)w)->va_refresh = TRUE;
	return FALSE;
}
#endif



/*
 * Drop items onto an accwindow using AV/VA protocol.
 * Note 1: client window has to be signed-on for this to work
 * Note 2: parameter 'list' is locally modified
 */

boolean va_accdrop(WINDOW *dw, WINDOW *sw, int *list, int n, int kstate, int x, int y)
{
	AVTYPE *client;
	ITMTYPE itype;
	char *thename;
	int i;

	/* 
	 * Find the data for the client which created this window.
	 * If the window exists, it is assumed that the client is still alive
	 * and there is no need to check it.
	 */

	client = va_findclient(dw->xw_ap_id);

	if ( client )
	{
		int va_answer[8];

		/* Client found; add each name from the list into the global memory */

		*global_memory = 0; /* clear previous */

		for ( i = 0; i < n; i++ )
		{
			thename = NULL; /* see further below */

			if 
			(
				((itype = itm_type(sw, *list)) == ITM_NOTUSED ) ||
				((thename = itm_fullname(sw, *list)) == NULL )  ||
				(!va_add_name(itype, thename))
			)
			{
				free(thename);
				return FALSE;
			} 

			/* 
			 * "thename" must be NULL for the next loop, otherwise, if an
			 * unused item type occurs, an unallocated block will be
			 * freed above
			 */

			free(thename);

			list++;
		}

		/* Create a message and send it */

		va_clranswer(va_answer);
		va_answer[0] = VA_DRAGACCWIND;
		va_answer[3] = dw->xw_handle;
		va_answer[4] = x;
		va_answer[5] = y;
		*(char **)(va_answer + 6) = global_memory;

		appl_write(client->ap_id, 16, va_answer);

		client->flags |= AVCOPYING;

		return TRUE;
	}

	return FALSE;
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
		*mask = NULL,
		*pp3,			/* location in the message as a string pointer */
		*mp5,			/* same */
		*pp6;			/* same */

	int 
#if _MINT_
		j,
#endif
		error,
		answer[8],			/* answer message will be composed here */
		m3 = message[3],	/* save some bytes in program size */
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


	/* Clear the answer message buffer; then set TeraDesk's ap_id where it will be */

	va_clranswer(answer);

	/* Find data for the client if it exists */

	av_current = message[1];					/* who sent the message (its ap_id) */
	theclient = va_findclient(av_current);		/* are there any data for it */

	/* 
	 * Ignore unknown clients, except if they are signing-on, 
	 * or ask for a font, or inf-file is sent to TeraDesk
	 */

	if( !(message[0] == AV_PROTOKOLL || message[0] == FONT_SELECT || (message[0] == VA_START && av_current == ap_id) || theclient) )
		return;

	/* 
	 * Some locations in the message may point to strings and
	 * these pointers are used in several functions, so determine
	 * them here for all concerned
	 */

	pp3 = *(char **)(message + 3);
	mp5 = *(char **)(message + 5);
	pp6 = *(char **)(message + 6);

	switch(message[0])
	{
		case AV_PROTOKOLL:
		{
	
			/*
			 * Client signing on.
			 * Mostly ignore the features send by the (sender) client.
			 * Return the server-supported features.
			 * Maybe the name should be converted to uppercase here ? 
			 * Hopefully not- AV protocol requires that names be sent in uppercase.
			 */
	
			strcpy(avwork.name, pp6);
	
			avwork.ap_id = appl_find( (const char *)avwork.name );
			avwork.avcap3 = m3; /* notify client-supported features */
			avwork.flags = 0;
	
			/* 
			 * Add the client to the list- but it seems that some clients 
			 * (e.g. ST-GUIDE) may sign-on more than once without signing off. 
			 * Avoid this.
			 */
	
			if (!theclient && av_current != ap_id)
			{
				if (!lsadd_end((LSTYPE **)&avclients, sizeof(AVTYPE), (LSTYPE *)(&avwork), copy_avtype ))
					reply = FALSE; /* can't add client */
			}
	
			if (reply) 
			{
				strcpy(global_memory, thisapp); /* must be exactly 8 characters long */
	
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
		}
		case AV_EXIT:
		{
			/* 
			 * Note: do not send to an AV client the instructions to close its
			 * windows. It is supposed that now it will do that by itself.
			 * Therefore xw_dosend = 0 temporarily
			 */
	
			rem_avtype(&avclients, va_findclient(av_current));
			reply = FALSE;
			break;
		}
		case AV_ASKCONFONT:
		{
			/* 
			 * Return the id and size of the console window font.
			 * Reply to this is currently the same as for directory window font 
			 */
		}
		case AV_ASKFILEFONT:
		{
			/* 
			 * Return the id and size of the currently selected directory font. 
			 * Message is composed and sent in va_fontreply(); there is no need
			 * for an additional reply
			 */
	
			theclient->avcap3 |= VV_FONTASKED;
	
			va_fontreply( (message[0] == AV_ASKCONFONT) ? VA_CONFONT : VA_FILEFONT, message[1]); 
			reply = FALSE;
			break;
		}
		case AV_ACCWINDOPEN:
		{
			/* 
			 * Create an av-client pseudowindow; 
			 * use "flags" parameter to pass the window handle
			 * which the client has supplied in message[3]
			 */
	
			aw = xw_create(ACC_WIND, &aw_functions, m3, NULL, sizeof(ACC_WINDOW), NULL, &error );
			aw->xw_xflags |= XWF_OPN;
			aw->xw_ap_id = av_current;
	
			reply = FALSE;
			break;
		}
		case AV_ACCWINDCLOSED:
		{
			/* 
			 * Client has closed a window, identified by its handle.
			 * Now delete the data structire for this window
			 */
	
			xw_delete(xw_hfind(m3));
	
			reply = FALSE;
			break;
		}
		case AV_COPY_DRAGGED:
		{
			/* Confirmation of copying to an acc window */
	
			if ( theclient->flags & AVCOPYING )
			{
				answer[0] = VA_COPY_COMPLETE;
				answer[3] = 1;
				theclient->flags &= ~AVCOPYING;
			}
			else
				reply = FALSE;
	
			break;
		}
	#if _MORE_AV
		case AV_XWIND:
		{
			/* 
			 * Open a directory window with additional features.
			 * Action is currently similar to that for just opening a window.
			 * It is not entirely clear whether this message is 
			 * correctly supported 
			 */
	
			answer[0] = VA_XOPEN;
			goto getwpath;
		}
		case AV_OPENWIND:
		{
			/* Open a directory window. (name lengths are checked in dir_add_window() ) */
	
			answer[0] = VA_WINDOPEN;
	
			getwpath:;
	
			path = strdup(pp3); /* path must be kept */
	
			if ( path )
			{
				dir_trim_slash(path);
				stat = 1;
	
				/* If an existing window can not be topped, open a new one */
	
				if ( !( message[0] == AV_XWIND && (message[7] & 0x01) && dir_do_path(path, DO_PATH_TOP)) )
				{
					mask = strdup(mp5);	/* mask must be kept too */
	
					/* 
					 * In case of an error (e.g. too long string), path and mask are 
					 * deallocated in dir_add_window()
					 */
	
					if( mask )
						stat = (int)dir_add_window(path, mask, NULL);
					else
					{
						free(path);
						stat = 0;
					}
				}
				else
				{
					/* This was AV_XWIND; path was just used for comparison */
					free(path);
				}
			}
			else
				stat = 0;
	
			answer[3] = stat;	/* status */
			break;
		}
	#endif
		case VA_START:
		{
			/* 
			 * TeraDesk can understand about inf files being sent to it by itself.
			 * There is no point in sending the reply message back
			 * because AV_STARTED is ignored anyway.
			 * Name of the file must be kept. 
			 * This command is ignored if it does not come from TeraDesk?
			 */
			 
			reply = FALSE;
	
			if(av_current == ap_id)
				load_settings(strdup(pp3)); 
	
			break;
		}
		case AV_STARTPROG:
		{
			/* 
			 * Start a program with possibly a command line. Instead of a
			 * program name, a filename may be passed, and an application
			 * should be found for it
			 */
	
			mask = strdup(mp5);
			answer[0] = VA_PROGSTART;
			answer[7] = message[7];
			goto openit;
		}
		case AV_VIEW:
		{
			/* 
			 * Activate a viewer for one file. Currently, TeraDesk does not
			 * differentiate between a viewer and a processing program;
			 * so, behaviour for AV_VIEW and AV_STARTPROG is essentially
			 * the same.
			 * If the application is already running, parameters will be
			 * passed to it.
			 * Parameter "mask" may contain the command line
			 */
	
			answer[0] = VA_VIEWED;
	
			openit:;
	
			onfile = TRUE;
			path = strdup(pp3); 
	
			if ( path )
			{
				/* 
				 * Internet access programs may send very longs strings. 
				 * Some special provisions for those. Find which application
				 * is to be used, and use the recieved string as a command.
				 */
	
				if(!mask && x_netob(path) && strlen(path) >= sizeof(VLNAME))
				{
					char *app = app_find_name(path, TRUE);
	
					if(app)
					{
						mask = path;
						path = strdup(app);
					}
				}
	
				/* Now open the item */
	
				stat = item_open( NULL, 0, 0, path, mask );
			}
			else
				stat = 0;
	
			free(path);
			free(mask);
	
			answer[3] = stat;
			break;
		}
	#if _MORE_AV
	
		case AV_SETWINDPOS:
		{
			avsetw.flag = TRUE;
			avsetw.size = *( (RECT *)(&message[3]) ); /* shorter */
			reply = FALSE;
			break;
		}
		case AV_PATH_UPDATE:
		{
			/* 
			 * Update a directory window. Probably there is no need to check path
			 * because that string is just compared to existing paths
			 */
	
			path = strdup(pp3); /*duplicate because it will be modified */
	
			if ( path && !x_netob(path) )
			{
				dir_trim_slash(path);
				dir_do_path(path, DO_PATH_UPDATE);
			}
	
			free(path);
	
			reply = FALSE; /* no reply to this message */
			break;
		}
		case AV_STATUS: 
		{
			/* Note: "path" has to be kept; it contains the status string */
	
			path = strdup(pp3);
	
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
						/* string changed; replace pointer to status string */
						free(thestatus->stat);
						thestatus->stat = path;
					}
				}
				else	
				{
					/* add this status string to the pool */
					strcpy(avswork.name, theclient->name);
					avswork.stat = path;
	
					if (!lsadd_end((LSTYPE **)&avstatus, sizeof(AVSTAT), (LSTYPE *)(&avswork), copy_avstat ))
						free(path);
				}
			}
	
			reply = FALSE;
			break;
		}	
		case AV_GETSTATUS:
		{
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
	
			break;
		}
		case AV_WHAT_IZIT:
		{
			int item, wind_ap_id;
	
			*global_memory = 0; /* clear any old strings */
	
			/* Find the owner of the window (can't be always done in single-tos) */
	
			wind_get( wind_find(m3, message[4]), WF_OWNER, &wind_ap_id);
	
			/* Note: it is not clear what should be returned in answer[3] */
	
			answer[3] = wind_ap_id;
	
			if ( wind_ap_id != ap_id ) /* this is not TeraDesk's window */
				answer[4] = VA_OB_WINDOW;
			else
			{
				answer[4] = VA_OB_UNKNOWN;
	
				if ( (aw = xw_find(m3, message[4])) != NULL )
				{
					/* Yes, this is TeraDesk's window */
	
					if ( xw_type(aw) == TEXT_WIND )
					{
						/* this is a text window  and might as well belong to another app */
						answer[4] = VA_OB_WINDOW;
					}
					else if ( (item = itm_find(aw, m3, message[4])) >= 0 )
					{
						/* An item can be located in a desktop or directory window */
	
						ITMTYPE itype = itm_type(aw, item);
	
						if ( itype >= ITM_NOTUSED && itype <= ITM_NETOB )
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
									*(char **)(answer + 5) = global_memory;
									free(path);
								}
							}			/* A file or a volume; has path ? */
						}			/* Recognized item type ? */
					}			/* Not a text window ? */
					else if(xw_type(aw) == DIR_WIND)
					{
						/* this is a directory window background */
						answer[4] = VA_OB_FOLDER;
						va_add_name(ITM_FOLDER, ((DIR_WINDOW *)aw)->path);
						*(char **)(answer + 5) = global_memory;
					}
				}			/* Window found ? */
			}			/* TeraDesk's window ? */
	
			answer[0] = VA_THAT_IZIT;			
			break;
		} /* what is it ? */
	
		case AV_DRAG_ON_WINDOW:
		{
			/* Drag one or several files to the path of a window */
	
			answer[0] = VA_DRAG_COMPLETE;
			path = strdup(pp6);	/* source; duplicate because it will be modified */
			goto processname;
		}
		case AV_COPYFILE:
		{
			/* Copy one or several files to a path */
	
			answer[0] = VA_FILECOPIED;
			mask = strdup(mp5);	/* destination */
			goto getpath;
		}
		case AV_DELFILE:
		{
			answer[0] = VA_FILEDELETED;
			goto getpath;
		}
		case AV_FILEINFO:
		{
			/* Delete one or several files or return information about it/them */
	
			answer[0] = VA_FILECHANGED;
	
			getpath:;
	
			path = strdup(pp3);	/* source; duplicate because it will be modified */
	
			processname:;
	
			if ( path )
			{
				char 
					*p = path,		/* current position in the string */
					*wpath = NULL,	/* simulated window path */
					*cs, *cq, 		/* position of the next " " and "'" */
					*pp = NULL;		/* position of the next name */
	
				boolean 
					q = FALSE;		/* true if name is quoted */
	
				ITMTYPE 
					itype;			/* type of the item */
	
				DIR_WINDOW 
					ww;				/* structure for the simulated window */
	
				int 
					list = 0;		/* simulated selected item */
	
				*global_memory = 0;	/* clear the string in the buffer */
				stat = 1;			/* all is well for the time being */
	
				/* In this loop, items in the message are processed one by one */
	
				while(stat && p && *p)
				{
					/* Attempt to extract next item name (possibly quoted) */
	
					if (*p == SINGLE_Q && *(p + 1) != SINGLE_Q) /* single quote, but not doubled */
					{
						p++;		/* move to the character after the quote */
						q = TRUE;	/* quoting has been started */
					}
	
					strip_name(p, p);		/* strip leading/trailing blanks */
					cq = p;
	
					while((cq = strchr(cq, SINGLE_Q)) != NULL && *(cq + 1) == SINGLE_Q)
					{
						cq += 2;		
					}
	
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
	
					dir_trim_slash(p);
					itype = diritem_type(p);
	
					if ( itype != ITM_NOTUSED && itype != ITM_NETOB )
					{
						wpath = fn_get_path(p);
	
						dir_simw(&ww, wpath, fn_get_name(p), itype);
	
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
							{
								stat = 
								(int)itm_move
								( 
									(WINDOW *)&ww, 
									0, 
									m3,
									message[4],
									message[5]
								);	
								break;
							}
							case AV_COPYFILE:
							{
								/* 
								 * Note: this is still not fully compliant to the
								 * AV-protocol: links can not be created
								 */
								int old_prefs = options.cprefs;
								options.cprefs = (message[7] & 0x0004) ? old_prefs : (old_prefs & ~CF_OVERW);
								rename_files = (message[7] & 0x0002) ? TRUE : FALSE;
								stat = (mask) ?
								(int)itmlist_op
								(
									(WINDOW *)&ww, 
									1, 
									&list, 
									mask, 
									( message[7] & 0x0001) ? CMD_MOVE : CMD_COPY
								) : 0;
								options.cprefs = old_prefs;
								break;
							}
							case AV_DELFILE:
							{
								stat = 
								(int)itmlist_wop
								(
									(WINDOW *)&ww, 
									1, 
									&list, 
									CMD_DELETE
								);
								break;
							}
							case AV_FILEINFO:
							{
								*(char **)(answer + 6) = global_memory;	
								item_showinfo((WINDOW *)&ww, 1, &list, FALSE);
								stat = 1; /* but it is not always so! */
							}
						}
	
						wd_noselection();
						free(wpath);
					}
					else
						stat = 0;
	
					p = pp;
				} /* while */
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
		}
	#endif

	#if _FONT_SEL
	
		case FONT_SELECT:
		{
	
			/* 
			 * Select a font for the client (FONT protocol)
			 * message[4] = current font id of the client 
			 */
	
			fnt_mdialog(message[1], m3, message[4], message[5],
						message[6], message[7], 1);
			reply = FALSE;
			break;
		}
	#endif
	
		default:
		{
			/* 
			 * Beside the unsupported messages, this also handles
			 * those (mostly acknowledge) messages which do not require 
			 * any action
			 */
			reply = FALSE;
		}
	}

	if ( reply )
		appl_write(av_current, 16, answer);

#if _MORE_AV

	/* Send VA_PATH_UPDATE if any is due */

	aw = xw_first();

	while(aw)
	{
		if((xw_type(aw) == DIR_WIND) && ((DIR_WINDOW *)aw)->va_refresh)
			va_pathupdate(aw);

		aw = xw_next(aw);
	}

#endif

}


/*
 * Remove all AV-clients status data from the list
 */

void rem_all_avstat(void)
{
	lsrem_all( (LSTYPE **)(&avstatus), rem_avstat );
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


/*
 * Close a window of an av-client. In xw_close() a WM_CLOSE is sent
 * to the client.
 */

void va_close(WINDOW *w)
{
/*
	xw_closedelete(w); /* xw_close(w) then xw_delete(w) */
*/
	xw_close(w);
}

#endif


/*
 * Structures for saving and loading AV-protocol client status.
 * Any status string should not be longer than 255 characters.
 */

typedef struct
{
	SNAME name;
	VLNAME stat;
} SINFO;


static SINFO this;

CfgEntry stat_table[] =
{
	{CFG_HDR, "status" },
	{CFG_BEG},
	{CFG_S,   "name",  this.name	},
	{CFG_S,   "stat",  this.stat	},
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
	lsrem((LSTYPE **)list, (LSTYPE *)t);		/* the list entry */
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
			*error = CfgSave(file, stat_table, lvl, CFGEMP); 
	
			a = a->next;
		}
	}
	else
	{
		memclr(&avswork, sizeof(avswork));

		*error = CfgLoad(file, stat_table, MAX_CFGLINE, lvl); 

		if ( *error == 0 )
		{
 			if ( this.name[0] == 0 )
				*error = EFRVAL;
			else
			{
				char *stat = malloc(strlen(this.stat) + 1L);

				if (stat)
				{
					strcpy(stat, this.stat);
					strcpy(avswork.name, this.name);
					avswork.stat = stat;

					if ( lsadd_end
						 ( 
				             (LSTYPE **)&avstatus, 
				              sizeof(avswork), 
				              (LSTYPE *)&avswork, 
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
	{CFG_HDR,  "avstats" },
	{CFG_BEG},
	{CFG_NEST, "status", one_avstat  },		/* Repeating group */
	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Handle saving/loading of all AV-client status data
 */

CfgNest va_config
{ 
	*error = handle_cfg(file, va_table, lvl, CFGEMP, io, rem_all_avstat, vastat_default);
}
