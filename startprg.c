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


#include <library.h>
#include <xdialog.h>

#include "resource.h"
#include "desk.h"
#include "environm.h"
#include "error.h"
#include "events.h"
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
#include "dir.h"


typedef struct
{
	LNAME name;
	COMMAND cml;
	char *envp;
	_WORD appl_type;
	bool new;
} PRG_INFO;

static _WORD _CDECL (*old_critic) (_WORD error);

static PRG_INFO pinfo;

bool fargv = FALSE;


/* 
 * Display a title with the program name in the top of the screen. 
 */

static void set_title(const char *title)
{
	OBJECT dsktitle;
	TEDINFO ttd;

	init_obj(&dsktitle, G_BOXTEXT);
	dsktitle.ob_flags = OF_LASTOB;

	dsktitle.ob_spec.tedinfo = &ttd;
	dsktitle.ob_x = 0;
	dsktitle.ob_y = 0;
	dsktitle.ob_width = xd_desk.g_w;
	dsktitle.ob_height = xd_fnt_h + 2;

	ttd.te_ptext = (char *) (long) title;
	ttd.te_font = 3;
	ttd.te_just = 2;
	ttd.te_color = 0x1F0;
	ttd.te_thickness = 0;
	ttd.te_txtlen = xd_desk.g_w / xd_fnt_w;

	draw_tree(&dsktitle, (GRECT *) & dsktitle.ob_x);
}


/*
 * Trick to restore the mouse after some programs, which hide it. 
 * Does not work always.
 */

void clean_up(void)
{
	v_hide_c(vdi_handle);
	v_show_c(vdi_handle, 0);
}


static _WORD _CDECL new_critic(_WORD error /*,_WORD drive */ )
{
	return error;
}


static void install_critic(void)
{
	old_critic = (_WORD _CDECL (*)(_WORD error)) Setexc(0x101, (void (*)()) new_critic);
}


static void remove_critic(void)
{
	(void) Setexc(0x101, (void (*)()) old_critic);
}


/* 
 * Close and delete all windows before starting a program.
 * Windows need not be TeraDesk's only ?
 * This is relevant only for single-tos. 
 */

static void close_windows(void)
{
	_WORD handle;

	if (aes_version >= 0x140)
	{
		wind_new();
	} else
	{
		wind_get_int(0, WF_TOP, &handle);

		while (handle > 0)
		{
			wind_close(handle);
			wind_delete(handle);
			wind_get_int(0, WF_TOP, &handle);
		}
	}
}


/*
 * There seems to be a problem with TOS 2.06 swallowing the first mouse click
 * after a program is executed by clicking on a desktop icon, when there are
 * no windows open. It is mentioned also in the manual for the Thing desktop
 * (Thing seems to suffer from the same problem).
 * Code below seems to cure that: a mouse click is simulated by appl_tplay().
 * Note1: sim_click() is also used when starting a program, see above.
 * Note2: code below seems to be a working compromise fit both for
 * the unpatched and the patched TOS 2.06.
 * Note 3: TOS 3.06 being essentially the same as 2.06, this should
 * apply to it, too ???
 */

static void sim_click(void)
{
	if ((tos_version == 0x206 || tos_version == 0x306)
#if _MINT_
		&& !(mint || geneva)
#endif
		)
	{
		XDEVENT events;
		_WORD p[8] = { 0, 1, 1, 1, 0, 1, 0, 0 };

		if (!wd_dirortext(xw_top()))
			appl_tplay((void *) (&p), 2, 4);

		xd_clrevents(&events);
		events.ev_mflags = MU_BUTTON | MU_TIMER;
		events.ev_mbclicks = 0x102;
		events.ev_mbmask = 3;
		events.ev_mtlocount = 100;

		xe_xmulti(&events);
	}
}


/*
 * Execute a singletask program with a command line. Return error code.
 * Note: appl_type here is 0 or 1 only; 0 for TOS programs, 1 for GEM ones
 */
static _WORD exec_com(const char *name, COMMAND *cml, char *envp, _WORD appl_type)
{
	_WORD dummy;
	_WORD error;
	_WORD *colours = NULL;
	_WORD stdout_handle = 0;
	_WORD ostderr_handle = 0;

	/* If 'save colour' option is set, save the current colours. */

	if ((options.vprefs & SAVE_COLOURS) && ((colours = get_colours()) == NULL))
		return ENOMEM;

	/* 
	 * If use file handle 2 as standard error option is set, 
	 * redirect file handle 2 to file handle 1. 
	 */

	if (options.xprefs & TOS_STDERR)
	{
		if ((stdout_handle = (_WORD) Fdup(1)) < 0)	/* Get a copy of stdout for Fforce. */
		{
			free(colours);
			return stdout_handle;
		}

		if ((ostderr_handle = (_WORD) Fdup(2)) < 0)	/* Duplicate old stderr. */
		{
			Fclose(stdout_handle);
			free(colours);
			return ostderr_handle;
		}

		Fforce(2, stdout_handle);
	}

	hourglass_mouse();

	/* Save current position of windows into a buffer, then close all windows; */

	if (wd_tmpcls())
	{
		/* If windows were successfully closed, remove menu bar */

		menu_bar(menu, 0);
		strcpy(pinfo.name, name);
		pinfo.cml = *cml;
		pinfo.envp = envp;
		pinfo.appl_type = appl_type;
		pinfo.new = TRUE;

		while (pinfo.new)
		{
			_WORD aptype;

			aptype = pinfo.appl_type;
			pinfo.new = FALSE;

			/* 
			 * Launch a GEM or TOS application, type being based on 
			 * parameter #2.If it is (0), the application will be launched as
			 * a TOS application, otherwise if it is (1), the 
			 * application will be launched as a GEM application. 
			 * The extended bits in mode are only supported by AES 
			 * versions of at least 4.0. Parent applications which 
			 * launch children using this mode are supposed to be 
			 * suspended under MultiTOS.
			 * Note: errors returned by shel_write() are not handled!
			 */

			shel_write(SHW_EXEC, aptype, 0, pinfo.name, (char *) &pinfo.cml);

			hourglass_mouse();
			wind_set_ptr_int(0, WF_NEWDESK, NULL, 0);

			/* Show the name of the launched program as title on the screen */

			set_title(fn_get_name(pinfo.name));

			/* Why is this ? */

#if 0									/* why was this? */
			appl_exit();
			appl_init();
#endif

			/* Close and delete any remaining windows */

			close_windows();

			/* 
			 * if this is to be a tos program, turn mouse off, etc.
			 * otherwise redraw desktop space
			 */

			if (aptype == 0)
			{
				/* TOS app; remove mouse, set new exception vector */
				xd_mouse_off();
				v_enter_cur(vdi_handle);
				install_critic();
			} else
			{
				/* GEM app; redraw desktop space */
				dsk_draw();
			}

			/* 
			 * At least one application (i.e. Megapaint 6) stops at this
			 * moment and waits for a mouse button click, for reasons unclear.
			 * Most probably this is related to a bug in TOS 2.06;
			 * This problem seems to be fixed by simulating a click here.
			 * As far as I can see there are no bad effects on other apps?
			 * Note: in single-tos, all windows will by this time have been
			 * closed, so the criteria in sim_click for simulating a click
			 * will always be satisfied.
			 */

			sim_click();

			/* Start a program using Pexec */

			error = (_WORD) x_exec(0, pinfo.name, &pinfo.cml, pinfo.envp);

			if (aptype == 0)
			{
				/* This was a TOS program */

				remove_critic();

				if ((options.xprefs & TOS_KEY) != 0)
				{
					_WORD d,
					 mbs;

					v_curtext(vdi_handle, get_freestring(MKEYCONT));
					while (Bconstat(2))
						Bconin(2);
					while (!Bconstat(2) && (vq_mouse(vdi_handle, &mbs, &d, &d), mbs == 0)) ;
				}

				v_exit_cur(vdi_handle);
				xd_mouse_on();
			} else
			{
				/* This was a GEM program */

				close_windows();
			}

			{
				VLNAME cmd;				/* maybe LNAME is enough ? */
				char tail[128];

				shel_read(cmd, tail);

				if (strcmp(cmd, pinfo.name) && cmd[0])
				{
					COMMAND *ptail = (COMMAND *) tail;

					strcpy(pinfo.name, cmd);
					pinfo.cml = *ptail;
					pinfo.envp = NULL;
					pinfo.appl_type = 1;	/* Moet verbeterd worden. */
					pinfo.new = TRUE;
				}
			}

			if (pinfo.new)
			{
				char *h;

				if ((h = x_fullname(pinfo.name, &error)) != NULL)
				{
					if (strlen(h) < sizeof(pinfo.name))
					{
						strcpy(pinfo.name, h);
					} else
					{
						error = ENAMETOOLONG;
						pinfo.new = FALSE;
					}

					free(h);
				} else
				{
					pinfo.new = FALSE;
				}
			}

			/* Try to restore a visible mouse if possible */

			clean_up();

		}								/* do */

		shel_write(SHW_NOEXEC, 1, 0, "", "\0\0");

		/* Draw the desktop and menu bar, then reopen all windows */

		regen_desktop(desktop);
		wd_reopen();
	}

	/* 
	 * Clear keyboard buffer just in case there is something left there; 
	 * then set mouse pointer to arrow shape 
	 */
	while (key_state(&dummy, FALSE) > 0)	/* instead of clr_key_buf() */
	{
	}
	
	arrow_mouse();

	/* Restore handle 2 to old handle. */

	if (options.xprefs & TOS_STDERR)
	{
		Fforce(2, ostderr_handle);
		Fclose(ostderr_handle);
		Fclose(stdout_handle);
	}

	/* Restore old colours. */

	if (colours)
	{
		set_colours(colours);
		free(colours);
	}

	return error;
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

void start_prg(const char *fname,		/* path+filename of program */
			   char *cmdl,				/* command line (first byte is length) */
			   const char *path,		/* default directory for this program */
			   ApplType prg,			/* application type */
			   bool argv,				/* if true, use argv protocol */
			   bool single,				/* run in single mode when applicable */
			   bool back,				/* run in background when applicable */
			   long limmem,				/* memory limit for program */
			   char *localenv,			/* local environment string for the program */
			   _WORD kstate				/* state of SHIFT, CONTROL and ALTERNATE keys */
	)
{
	_WORD error = 0;
	_WORD appl_type;					/* 0 (TOS) or 1 (GEM) */

	size_t envl = 0;					/* length of environment string */
	size_t newenvl = 0;					/* length of environment string */

	char *pp;							/* temporary allocation */
	char *olddir = NULL;				/* previous default directory */
	char *buildenv = NULL;				/* this env. will be passed */
	char *tmpenv = NULL;				/* points to temp. env. string */

	VLNAME prgpath;						/* Default directory for this program */

	bool stask = TRUE;					/* TRUE if singletasking mode */
	bool doenv = FALSE;					/* TRUE to process local environment */
	bool doargv = FALSE;					/* TRUE to process ARGV */
	bool background = back;				/* run in background */
	bool catargv;						/* TRUE to append ARGV to local environmen */

#if !_MINT_
	(void) limmem;
	(void) single;
#endif

	/* Determine program type */

	appl_type = ((prg == PGEM) || (prg == PGTP)) ? 1 : 0;

	/* Background program is activated if [Control] is pressed */

	if (kstate & K_CTRL)
		background = TRUE;

	/* 
	 * Prepare local environment string if one is specified - 
	 * insert zeros instead of blanks between arguments
	 * Note: localenv is never passed if empty, it is NULL then 
	 */

	if (localenv)
	{
		doenv = TRUE;
		tmpenv = make_argv_env(NULL, localenv, &envl);
		envl--;
	}

	/* Should ARGV string be appended to the local environment? */

	catargv = argv;						/* but it may be modified later */

	/* 
	 * Also, slightly different behaviour is needed in some cases.
	 * See the explanation further below about adding ARGV 
	 * to local environment
	 */

#if 0									/* it seems better to always add ARGV to the environment */

#if _MINT_
	catargv = catargv && (fargv || magx || (mint /* && !magx */  && !geneva && doenv) || !(mint || geneva));
#endif

#endif

	/* 
	 * Create a new environment string, by concatenating the
	 * existing environment with the one created above.
	 * This is needed if local environment is to be passed, or
	 * else an ARGV is to be explicitely passed.
	 * tmpenv is added before the old environment.
	 * new_env() routine will always create an environment string, 
	 * even if it contains only the trailing zeros
	 */

	if (doenv || catargv)
	{
		buildenv = new_env(tmpenv, envl, 1, &newenvl);
		if (!(buildenv))
			goto errexit;
	}

	free(tmpenv);
	tmpenv = NULL;

	/* 
	 * Explicitely add ARGV to a local evironment string because it seems that 
	 * the mechanism in shel_write does not work equally for all AESses and 
	 * program types. ARGV has to be added in this way to the environment in 
	 * single-tos, and in AES4.1, NaAES or XaAES; for these multitasking AESses
	 * this has to be done ONLY if both the local environment and the ARGV
	 * are present (otherwise, shel_write works fine). This problem does not
	 * appear in Magic or Geneva.
	 */

	if (argv)
	{
		doargv = TRUE;

#if _MINT_
		if (catargv)
#endif
		{
			/* Append ARGV to the local environment string */

			char *envp = buildenv;

			doenv = TRUE;
			tmpenv = make_argv_env(fname, cmdl + 1, &envl);
			buildenv = malloc_chk(newenvl + envl + 2L);	/* +2L probably not needed here */

			if (!(tmpenv && buildenv))
				goto errexit;

			/* Concatenate the strings. Append ARGV string at trailing end */

			memcpy(buildenv, envp, newenvl);
			memcpy(&buildenv[newenvl - 1L], tmpenv, envl);
			free(tmpenv);
			free(envp);
			tmpenv = NULL;
		}
	}

	/* In what manner will this program be started?
	 * This is experimental and most probably is a bad thing to do, but better
	 * than nothing: until starting of a non-multitsking app is coded properly (if ever), 
	 * launch it as if it were a single-tasking system. This applies to Magic only
	 * (Geneva takes care of the issue by itself), and, in fact, seems to work
	 * (more-less). At least programs like DynaCADD and old 1stWord are running
	 * properly with it, probably mostly because of changing the default directory.
	 * However, memory limit for such application is not applicable, then.
	 */

#if _MINT_
	stask = (single && magx) || (_AESnumapps != -1 && !magx);
#endif

	/* 
	 * Find the default directory for this program.
	 * This path will have the trailing backslash only if it is
	 * to a root directory. Then, if it is not single-TOS, add the
	 * backslash if it is not there. 
	 */

	if ((pp = fn_get_path((path) ? path : fname)) != NULL)
	{
		/* A backslash may have to be concatenated, so copy the string */

		strsncpy(prgpath, pp, sizeof(VLNAME) - 1);
		free(pp);

		/* Now concatenate the backslash. Must do so for Geneva at least */

		if (!stask)
		{
			pp = strrchr(prgpath, '\\');

			if (!pp || (pp && pp[1] != '\0'))
				strcat(prgpath, bslash);
		}

		if (stask
#if _MINT_
			|| magx
#endif
			)
		{
			hourglass_mouse();
			olddir = x_getpath(0, &error);

			if (olddir)
				error = chdir(prgpath);

			if (error < 0)
			{
				arrow_mouse();
				xform_error(error);
				goto errexit;
			}
		}

		/* See note above about singletasking ! */

		if (stask || background)
		{
			/* 
			 * AES is not multitasking, use Pexec() to start program,
			 * (or else start a background program in mint)
			 */
			COMMAND cl;

			if ((appl_type == 1) && background)
				alert_iprint(MGEMBACK);	/* GEM programs can not be started in background */
			else
			{
				if (prg == PACC)
					alert_iprint(MCANTACC);	/* Don't start acc in single tos */
				else
				{
					/* Make a copy of cmdl. Never more than 125 characters */

					memclr(cl.command_tail, sizeof(cl.command_tail));
					strsncpy(cl.command_tail, cmdl + 1, 126);	/* "126" includes zero termination byte */
					cl.length = *cmdl;	/* value in the first byte */

					if (doargv)
						cl.length = 127;	/* signalizes that ARGV is used */

					if (background)
					{
#if _MINT_
						error = (_WORD) x_exec((mint) ? 100 : 0, fname, &cl, buildenv);	/* just Pexec */
#else
						error = (_WORD) x_exec(0, fname, &cl, buildenv);	/* just Pexec */
#endif
					} else
						error = exec_com(fname, &cl, buildenv, appl_type);

					/* Fix a TOS bug */

					sim_click();

					/* Report errors */

					xform_error(error);	/* Will ignore error >= 0 */
				}
			}
		}
#if _MINT_
		else
		{
			_WORD wiscr = SHW_PARALLEL;

			/* AES is multitasking, use shel_write() to start program. */

			if (prg == PACC)
			{
				/* Start an accessory. Note: returns 0 if error. */
				error = shel_write(SHW_EXEC_ACC, 0, wiscr, fname, empty);
			} else
			{
				/* Start other types of applications */
				_WORD mode;				/* launch mode */
				void *p[5];				/* parameters for the extended call */

				/* Start gem/tos program (0x1), but in extended mode ( | 0x400 ) */

				mode = SHW_XMDDEFDIR | SHW_EXEC;	/* always use SHW_EXEC */

				/* Notify if memory limit is defined */

				if (limmem > 0L)
					mode |= SHW_XMDLIMIT;

				/* Notify that an environment is passed */

				if (doenv)
					mode |= SHW_XMDENV;

				/* 
				 * Set pointers for extended mode. It seems that Magic will ignore
				 * the pointer for the default directory
				 */

				p[0] = (void *) (long) fname;	/* pointer to name                */
				p[1] = (void *) limmem;	/* value for Psetlimit()          */
				p[2] = NULL;			/* value for Prenice()            */
				p[3] = prgpath;			/* pointer to default directory   */
				p[4] = buildenv;		/* pointer to environment string  */

				/* Magic specifies the use of ARGV differenlty than other AESes */

				if (magx)
				{
					if (doargv)
					{
						if (catargv)
							cmdl[0] = 127;	/* Command will be passed through ARGV by TeraDesk */
						else
							cmdl[0] = 255;	/* Magic will do ARGV only if needeed */
					}
				} else
				{
					if (doargv && !catargv)
					{
						cmdl[0] = 127;	/* this signals the use of ARGV */
						wiscr = 1;		/* the same */
					} else
						wiscr = 0;		/* ARGV is provided in other way, if needed */
				}

				/* Start a program using shel_write */

				error = shel_write(mode, appl_type, wiscr, (char *) p, (char *) cmdl);
			}

			/* 
			 * Note: shel_write() returns an application_id;
			 * If it is 0, an error was encountered.
			 * Unfortunately, this does not work in all AESes;
			 * some happily return ap_id of a failed startup.
			 */

			if (error <= 0)
				alert_iprint(MFAILPRG);
		}
#endif
	}

	/* Program may jump directly here in case of error */

  errexit:;

	if (olddir)
	{
		hourglass_mouse();
		chdir(olddir);
		free(olddir);
		arrow_mouse();
	}

	free(tmpenv);
	free(buildenv);
}
