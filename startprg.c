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
#include <tos.h>
#include <vdi.h>
#include <library.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "environm.h"
#include "error.h"
#include "events.h"
#include "resource.h"
#include "xfilesys.h"
#include "file.h"
#include "lists.h" 
#include "slider.h"
#include "font.h"
#include "config.h"
#include "window.h"
#include "icon.h"
#include "screen.h"
#include "startprg.h"
#include "va.h"

typedef struct
{
	const char *name;
	LNAME path;
	COMMAND cml;
	const char *envp;
	int appl_type;
	boolean new;
} PRG_INFO;

int cdecl(*old_critic) (int error);
PRG_INFO pinfo;

extern int tos_version, aes_version;

void sim_click(void);


/* 
 * Display a title with the program name in the top of the screen. 
 */

static void set_title(char *title)
{
	OBJECT dsktitle;
	TEDINFO ttd;

	dsktitle.ob_next = -1;
	dsktitle.ob_head = -1;
	dsktitle.ob_tail = -1;
	dsktitle.ob_type = G_BOXTEXT;
	dsktitle.ob_flags = LASTOB;
	dsktitle.ob_state = 0;
	dsktitle.ob_spec.tedinfo = &ttd;
	dsktitle.r.x = 0;
	dsktitle.r.y = 0;
	dsktitle.r.w = screen_info.dsk.w;
	dsktitle.r.h = screen_info.fnt_h + 2;

	ttd.te_ptext = title;
	ttd.te_font = 3;
	ttd.te_just = 2;
	ttd.te_color = 0x1F0;
	ttd.te_thickness = 0;
	ttd.te_txtlen = screen_info.dsk.w / screen_info.fnt_w;
	objc_draw(&dsktitle, ROOT, MAX_DEPTH, dsktitle.r.x, dsktitle.r.y, dsktitle.r.w, dsktitle.r.h);
}


/*
 * Trick to restore the mouse after some programs, which hide the
 * mouse. Does not work always.
 */

static void clean_up(void)
{
	v_hide_c(vdi_handle);
	v_show_c(vdi_handle, 0);
}


static int cdecl new_critic(int error /*,int drive*/ )
{
	return error;
}


static void install_critic(void)
{
	old_critic = (int cdecl(*)(int error)) Setexc(0x101, (void (*)()) new_critic);
}


static void remove_critic(void)
{
	Setexc(0x101, (void (*)()) old_critic);
}


/* 
 * Copy a command line 
 */

static void copy_cmd(COMMAND *d, COMMAND *s)
{
	int i, m;
	char *s1, *s2;

	d->length = s->length;

	m = (int) (sizeof(d->command_tail) / sizeof(char));

	s1 = s->command_tail;
	s2 = d->command_tail;

	for (i = 0; i < m; i++)
		*s2++ = *s1++;
}


/* 
 * Close and delete all windows before starting a program.
 * This is relevant only for single-tos. This routine will also
 * close all pseudowindows of signed-on av-clients 
 */

static void close_windows(void)
{
	int handle;

	va_delall(-1);

	if ( aes_version >= 0x140 )
		wind_new();
	else
	{
		wind_get(0, WF_TOP, &handle);

		while (handle > 0)
		{
			wind_close(handle);
			wind_delete(handle);
			wind_get(0, WF_TOP, &handle);
		}
	}
}


/* 
 * Execute a singletask program with a command line
 */


static int exec_com(const char *name, COMMAND *cml, const char *envp, int appl_type)
{
	int error, *colors, stdout_handle, ostderr_handle;

	/* If save color option is set, save the current colors. */

	if (   (options.cprefs & SAVE_COLORS)
	    && ((colors = get_colors()) == NULL))
		return ENSMEM;

	/* 
	 * If use file handle 2 as standard error option is set, 
	 * redirect file handle 2 to file handle 1. 
	 */

	if (options.cprefs & TOS_STDERR)
	{
		if ((stdout_handle = (int)Fdup(1)) < 0)	/* Get a copy of stdout for Fforce. */
		{
			free(colors);
			return stdout_handle;
		}

		if ((ostderr_handle = (int)Fdup(2)) < 0)	/* Duplicate old stderr. */
		{
			Fclose(stdout_handle);
			free(colors);
			return ostderr_handle;
		}

		Fforce(2,stdout_handle);
	}

	graf_mouse(HOURGLASS, NULL);

	/* Save current position of windows into a buffer, then close all windows; */

	if ( wd_tmpcls() )
	{
		/* If windows were successfully closed, remove menu bar */

		menu_bar(menu, 0);

		pinfo.name = name;
		copy_cmd(&pinfo.cml, cml);
		pinfo.envp = envp;
		pinfo.appl_type = appl_type;
		pinfo.new = TRUE;

		while (pinfo.new == TRUE)
		{
			int appl_type;

			appl_type = pinfo.appl_type;
			pinfo.new = FALSE;

			/* 
			 * Launch a GEM or TOS application based on parameter #2.
			 * If it is (0), the application will be launched as
			 * a TOS application, otherwise if it is (1), the 
			 * application will be launched as a GEM application. 
             * The extended bits in mode are only supported by AES 
			 * versions of at least 4.0. Parent applications which 
			 * launch children using this mode are suspended
			 * under MultiTOS.
			 */

			shel_write(SHW_EXEC, appl_type, 0, (char *)pinfo.name, (char *)&pinfo.cml);

			graf_mouse(HOURGLASS, NULL);
			wind_set(0, WF_NEWDESK, NULL, 0);

			/* Show the name of the launched program as title on the screen */

			{
				LNAME pname;

				split_path(pinfo.path, pname, pinfo.name);
				set_title(pname);
			}

			appl_exit();
			appl_init();

			/* close and delete all windows */

			close_windows();

			/* 
			 * if this is to be a tos program, turn mouse off, etc.
			 * otherwise draw desktop
			 */

			if (appl_type == 0)
			{
				graf_mouse(M_OFF, NULL);
				v_enter_cur(vdi_handle);
				install_critic();
			}
			else
				dsk_draw();

			/* 
			 * At least one application (i.e. Megapaint 6) stops at this
			 * moment and waits for a mouse button click, for reasons unclear.
			 * This seems to be fixed by simulating a click here.
			 * As far as I can see there are no bad effects on other apps?
			 * Note: in single-tos, all windows will by this time have been
			 * closed, so the criteria in sim_click for simulating a click
			 * will always be satisfied.
			 */
		
			sim_click(); 

			/* Start a program using Pexec */

			error = (int) x_exec(0, pinfo.name, &pinfo.cml, pinfo.envp);

			if (appl_type == 0)
			{
				remove_critic();

				if ((options.cprefs & TOS_KEY) != 0)
				{
					int d, mbs;
					char *s;

					rsrc_gaddr(R_STRING, MKEYCONT, &s);

					v_curtext(vdi_handle, s);

					while (Bconstat(2))
						Bconin(2);

					while (!Bconstat(2) &&
						   (vq_mouse(vdi_handle, &mbs, &d, &d), mbs == 0));
				}

				v_exit_cur(vdi_handle);
				graf_mouse(M_ON, NULL);
			}
			else
				close_windows();

			{
				LNAME cmd;
				char tail[128];

				shel_read(cmd, tail);

				if (strcmp(cmd, pinfo.name) && cmd[0])
				{
					static LNAME name;

					pinfo.new = TRUE;
					strcpy(name, cmd);
					pinfo.name = name;
					copy_cmd(&pinfo.cml, (COMMAND *)tail);
					pinfo.envp = NULL;
					pinfo.appl_type = 1;	/* Moet verbeterd worden. */
				}
			}

			if (pinfo.new == TRUE)
			{
				char *h;

				if ((h = x_fullname(pinfo.name, &error)) != NULL)
				{
					if (strlen(h) <= PATH_MAX)
						strcpy((char *) pinfo.name, h);
					else
					{
						error = EPTHTL;
						pinfo.new = FALSE;
					}
					free(h);
				}
				else
					pinfo.new = FALSE;
			}

			/* Try to restore a visible mouse if possible */

			clean_up();
		}
			
		shel_write(SHW_NOEXEC, 1, 0, "", "\0\0");

		/* Draw the desktop and menu bar, then reopen all windows */

		regen_desktop(desktop);
		menu_bar(menu, 1);
		wd_reopen();
	}

	/* 
	 * Clear keyboard buffer just in case there is something left there; 
	 * then set mouse pointer to arrow shape 
	 */

	clr_key_buf();

	graf_mouse(ARROW, NULL);

	/* Restore handle 2 to old handle. */

	if (options.cprefs & TOS_STDERR)
	{
		Fforce(2,ostderr_handle);
		Fclose(ostderr_handle);
		Fclose(stdout_handle);
	}

	/* Restore old colors. */

	if (options.cprefs & SAVE_COLORS)
	{
		set_colors(colors);
		free(colors);
	}

	return error;
}


/* 
 * There seems to be a problem with TOS 2.06 swallowing the 
 * first mouse click after executing a program by clicking on
 * a desktop icon, and there are no windows open.
 * It is mentioned also in the manual for the Thing desktop
 * (Thing seems to suffer from the same problem).
 * Code below seems to cure that:
 * a mouse click is simulated (after executing a program) by appl_tplay.
 * Note1: sim_click() is also used when starting a program, see above.
 * Note2: code below seems to be a woring compromise fit both for
 * the unpatched and the patched TOS 2.06.
 * Note 3: TOS 3.06 being essentially the same as 2.06, this should
 * apply to it, too ???
 */

void sim_click(void)
{
 
	if ( (tos_version == 0x206 || tos_version == 0x306 )  
#if _MINT_
	             && !(mint || geneva) 
#endif
	   ) 
	{
		WINDOW *tw;
		XDEVENT events;

		int twt = 0, p[8] = {0,1, 1,1, 0,1, 0,0};

		tw = xw_top();

		if ( tw )
			twt  = xw_type(tw);

		if ( twt != DIR_WIND && twt != TEXT_WIND )
		{
			appl_tplay( (void *)(&p), 2, 4 );
		}

		events.ev_mflags = MU_BUTTON | MU_TIMER;
		events.ev_mbclicks = 0x102;
		events.ev_mbmask = 3;
		events.ev_mbstate = 0;
		events.ev_mm1flags = 0;
		events.ev_mm2flags = 0;
		events.ev_mtlocount = 100;
		events.ev_mthicount = 0;

		xe_xmulti(&events); 
	}
}


/*
 * Start a program. 
 * If 'path' is not NULL, it is used as current directory. 
 * If 'path' is NULL, the directory of the program is used as  
 * current directory.
 * Note1: local evironment string argument does NOT include ARGV;
 * ARGV can be added (if needed) in this routine.
 * Note2: local environent is never passed if it is an empty string.
 *
 * This routine is called only once, in app_exec() 
 */

void start_prg
(
	const char *fname,		/* path+filename of program */ 
	const char *cmdline,	/* command line (first byte is length) */
	const char *path,		/* default directory for this program */
	ApplType prg,			/* application type */
	boolean argv,			/* if true, use argv protocol */
	boolean single, 		/* runs in single mode in Magic */
	long limmem, 			/* memory limit for program */
	char *localenv,			/* local environment string for the program */
	int kstate				/* state of SHIFT, CONTROL and ALTERNATE keys */
)
{
	int 
		appl_type;

	size_t 
		envl = 0,							/* length of environment string */ 
		newenvl = 0;						/* length of environment string */

	char 
		*buildenv = NULL,					/* this env will be passed */
		*tmpenv = NULL;

	boolean 
		doenv = FALSE,						/* process local environment */
		doargv = FALSE,						/* process ARGV */
		catargv = (argv) ? TRUE : FALSE;	/* append ARGV to local environmen */


	/* Determine program type */

	appl_type = ((prg == PGEM) || (prg == PGTP)) ? 1 : 0; 

	/* 
	 * Prepare local environment string if one is specified - 
	 * insert zeros instead of blanks between arguments 
	 */

	if ( localenv /* && *localenv not needed, never passed if empty */  )
	{
		doenv = TRUE;
		tmpenv = make_argv_env(NULL, localenv, &envl);
		envl--;
	}

	/* 
	 * Should ARGV string be appended to the local environment? 
	 */
	
#if _MINT_

	catargv = argv && ( (mint && !magx && !geneva && doenv) || !(mint || geneva) ); 

#endif

	/* 
	 * Create a new environment string, by concatenating the
	 * existing environment with the one created above.
	 * tmpenv is added before the old environment.
	 * new_env() routine will always create an environment string, 
	 * even if it contains only the trailing zeros
	 */

	if ( doenv || catargv )
	{
		buildenv = new_env( tmpenv, envl, 1, &newenvl );
		if ( !(buildenv) )
		{
			xform_error(ENSMEM);
			goto errexit;
		};
	}

	free(tmpenv);

	/* 
	 * Explicitely add ARGV to a local evironment string because it seems that 
	 * the mechanism in shel_write does not work equally for all AESses and 
	 * program types. ARGV has to be added in this way to the environment in 
	 * single-tos, and in AES4.1, NaAES or XaAES; for these multitasking AESses
	 * this has to be done ONLY if both the local environment and the ARGV
	 * are present (otherwise, shel_write works fine). This problem does not
	 * appear in Magic or Geneva.
	 */

	if ( argv ) 
	{
		doargv = TRUE;

#if _MINT_
		if ( catargv )
#endif
		{

			/* Append ARGV to the local environment string */

			const char *envp = buildenv; 

			doenv = TRUE;

			tmpenv = make_argv_env(fname, cmdline + 1, &envl);

			buildenv = malloc(newenvl + envl + 2L);

			if ( !(tmpenv && buildenv) )
			{
				xform_error(ENSMEM);
				goto errexit;
			};

			/* Concatenate the strings. Append ARGV string at trailing end */

			memcpy( buildenv, envp, newenvl );
			memcpy(&buildenv[newenvl - 1L], tmpenv, envl );
			free(tmpenv);
			free(envp);
		}
	}

#if _MINT_ 

/*
 * This is experimental and most probably is a bad thing to do, but better
 * than nothing: until starting of a non-multitsking app is coded properly (if ever), 
 * launch it as if it were a single-tasking system. This applies to Magic only
 * (geneva takes care of the issue by itself), and, in fact, seems to work
 * (more-less). At least programs like DynaCADD and old 1stWord are running
 * properly with it, probably mostly because of changing the default directory.
 * However, memory limit for such application is not applicatble, then.
 */

	if (   ( single && magx )
	    || ( _GemParBlk.glob.count != -1 && !magx )
	   )
#endif
	{
		/* 
		 * AES is not multitasking, use Pexec() to start program,
		 * (or else start a background program in mint)
		 */

		COMMAND cl;
		char *prgpath, *olddir;
		int error = 0; 

		/* Mint background program is activated if [Alt] (?) is pressed */

#if _MINT_
		boolean background = (kstate & 4) ? TRUE : FALSE;
#endif

		/* Make a copy of cmdline. Never more than 125 characters */

		memset(cl.command_tail, 0, sizeof(cl.command_tail));
		strsncpy(cl.command_tail, cmdline + 1, 126); /* "126" includes zero termination byte */
		cl.length = *cmdline; /* value in the first byte */

		if (prg == PACC)	/* Dont start acc in single tos */
		{
			alert_iprint(MCANTACC); 
			goto errexit;
		}

#if _MINT_
		if (mint)		
			if ((appl_type == 1) && (background == TRUE))
			{
				alert_iprint(MGEMBACK); 
				goto errexit;
			}
#endif

		if ( doargv )
			cl.length = 127; /* this is for ARGV */

		/* 
		 * Do something only if filename of program is specified;
		 * (fn_get_path extracts path from full filename and
		 * allocates a string for this path)
		 */

		if ((prgpath = fn_get_path(fname)) != NULL)
		{
			/* 
			 * Find and remember current default directory, 
			 * if OK, then change to one specified for this program
			 */

			if ((olddir = getdir(&error)) != NULL)
			{
				/* 
				 * If path is not given, directory of the program
				 * will be set as current directory;
				 * otherwise "path" will be set as current directory
				 */

				graf_mouse(HOURGLASS, NULL); 
				error = chdir((path == NULL) ? prgpath : path);
				graf_mouse(ARROW, NULL);

				if (error == 0)
				{
					/*
					 * All is well so far; now start a background program
					 * in mint, or else start a single-task program	
					 */

#if _MINT_
					if ( mint && background )	

						error = (int) x_exec(100, fname, &cl, buildenv); /* just Pexec */
					else
#endif
						error = exec_com(fname, &cl, buildenv, appl_type);
				}

				/* Return to old default directory */

				graf_mouse(HOURGLASS, NULL);
				chdir(olddir);
				graf_mouse(ARROW, NULL);
				free(olddir);
			}

			/* Free string space which was allocated for program path */

			free(prgpath);
		}

		/* Fix a TOS bug */

		sim_click();
 
		if (error < 0)
			xform_error(error);
	}
#if _MINT_
	else
	{

		int wiscr = SHW_PARALLEL;

		/* AES is multitasking, use shel_write() to start program. */

		if (prg == PACC)

			/* Start an accessory */

			shel_write(SHW_EXEC_ACC, 0, wiscr, (char *)fname, "\0"); 
		else
		{
			/* Start other types of applications */

			int mode;
			void *p[5];
			char *h;
			VLNAME prgpath;

			/* Start gem/tos program (0x1), but in extended mode ( | 0x400 ) */

			mode = SHW_XMDDEFDIR | SHW_EXEC; /* always use SHW_EXEC */

			/* Notify if memory limit is defined */

			if ( limmem > 0L )	
				mode |= SHW_XMDLIMIT;

			/* Notify that an environment is passed */

			if ( doenv ) 
				mode |= SHW_XMDENV;

			/* Use appropriate path */

			if (path == NULL)
			{
				strcpy(prgpath, fname);
				if ((h = strrchr(prgpath, '\\')) != NULL)
					h[1] = 0;
			}
			else
			{
				strcpy(prgpath, path);
				strcat(prgpath,"\\");		/* Necessary for Geneva. */
			}
	
			/* Set pointers for extended mode */

			p[0] = fname;			/* pointer to name                */
			(long)p[1] = limmem;	/* value for Psetlimit()          */
			p[2] = NULL;			/* value for Prenice()            */
			p[3] = prgpath;			/* pointer to default directory   */
			p[4] = (doenv) ? buildenv : NULL;
	
			if ( magx )
				(char)cmdline[0] = 255;  /* So Magic will provide ARGV if needed */
			else
			{
				if ( doargv && !catargv )
				{
					(char)cmdline[0] = 127; /* this marks use of ARGV */
					wiscr = 1;
				}
				else
					/* ARGV is provided in other way, if needed */
					wiscr = 0;
			}

			/* Start a program using shel_write */

			shel_write(mode, appl_type, wiscr, (char *)p, (char *)cmdline);
		}
	}
#endif

	errexit:;

	free(tmpenv);
	free(buildenv);
}


