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
#include "icon.h"
#include "screen.h"
#include "startprg.h"
#include "font.h"
#include "config.h"
#include "window.h"


#define MAXTPATH	128

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
 * Close and delete all windows before starting a program 
 */

static void close_windows(void)
{
	int handle;

	if ( /* tos1_4() */ aes_version >= 0x140 )
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
 * Execute ... ... 
 */

static int exec_com(const char *name, COMMAND *cml, const char *envp, int appl_type)
{
	int error, *colors, stdout_handle, ostderr_handle;

	/* If save color option is set, save the current colors. */

	if (   (options.cprefs & SAVE_COLORS)
	    && ((colors = get_colors()) == NULL))
		return ENSMEM;

	/* If use file handle 2 as standard error option is set, redirect
	   file handle 2 to file handle 1. */

	if (options.cprefs & TOS_STDERR)
	{
		if ((stdout_handle = Fdup(1)) < 0)	/* Get a copy of stdout for Fforce. */
		{
			free(colors);
			return stdout_handle;
		}

		if ((ostderr_handle = Fdup(2)) < 0)	/* Duplicate old stderr. */
		{
			Fclose(stdout_handle);
			free(colors);
			return ostderr_handle;
		}

		Fforce(2,stdout_handle);
	}

	graf_mouse(HOURGLASS, NULL);

	/* 
	 * Save current position of windows into a buffer, then close all windows;
     */

	if ( wd_tmpcls() )
	{
		/* If windows successfully closed, remove menu bar */


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
			shel_write(SHW_EXEC, appl_type, 0, (char *)pinfo.name, (char *)&pinfo.cml);
			graf_mouse(HOURGLASS, NULL);
			wind_set(0, WF_NEWDESK, NULL, 0);

			/* Show program name as title on the screen */

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
					if (strlen(h) <= MAXTPATH)
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
 * Build an environment with an ARGV variable.
 * Result: pointer to environment or NULL if an error occured.
 */

const char *make_env(const char *program, const char *cmdline)
{
	long envl, argvl;
	char h, *d, *envp;
	const char *s;
	boolean q = FALSE;

	envl = envlen();
	argvl = envl + strlen(cmdline) + sizeof("ARGV=") + strlen(program) + 3;

	if ((envp = malloc(argvl)) != NULL)
	{
		d = envp;

		/* Copy the current environment. */

		s = _BasPag->p_env;
		do
		{
			while (*s)
				*d++ = *s++;
			*d++ = *s++;
		}
		while (*s);

		/* Add ARGV variable. */

		s = "ARGV=";
		while (*s)
			*d++ = *s++;
		*d++ = 0;

		/* Add program name. */

		s = program;
		while (*s)
			*d++ = *s++;
		*d++ = 0;

		/* Add command line. */

		s = cmdline;
		while ((h = *s++) != 0)
		{
			if ((h == ' ') && (q == FALSE))
			{
				*d++ = 0;
				while (*s == ' ')
					s++;
			}
			else if (h == '"')
			{
				if (*s == '"')
					*d++ = *s++;
				else
					q = (q) ? FALSE : TRUE;
			}
			else
				*d++ = h;
		}

		*d++ = 0;
		*d = 0;
	}
	else
		xform_error(ENSMEM);

	return envp;
}


/*
 * Description: Start a program. If 'path' is not NULL, 'path' is
 * used as current directory. If 'path' is NULL, the directory of
 * the program is used as current directory.
 *
 * Parameters:
 *
 * fname		- path+filename of program
 * cl			- command line (first byte is length)
 * path			- current directory
 * prg			- application type
 * argv			- use argv protocol
 * single		- runs in single mode in Magic 
 * limmem		- memory limit for program 
 * kstate		- state of SHIFT, CONTROL and ALTERNATE keys
 */

void start_prg(const char *fname, const char *cmdline,
			   const char *path, ApplType prg, boolean argv,
			   boolean single, long limmem,
			   int kstate)
{
	int appl_type;

	/* Determine program type */

	appl_type = ((prg == PGEM) || (prg == PGTP)) ? 1 : 0; 


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

	if (   ( single && (magx!=0) )
	    || (   _GemParBlk.glob.count != -1
	        && magx == 0
	       )
	   )
#endif
	{
		/* 
		 * AES is not multitasking, use Pexec() to start program,
		 * (or else start a background program in mint)
		 */

		COMMAND cl;
		char *prgpath, *olddir;
		const char *envp;
		int error = 0; 

#if _MINT_
		boolean background = (kstate & 4) ? TRUE : FALSE;
#endif

		/* Make a copy of cmdline. */

		memset(cl.command_tail, 0, sizeof(cl.command_tail));
		strsncpy(cl.command_tail, cmdline + 1, 126);
		cl.length = *cmdline;

		if (prg == PACC)	/* Dont start acc in single tos */
		{
			alert_iprint(MCANTACC); 
			return;
		}

#if _MINT_
		if (mint)		
			if ((appl_type == 1) && (background == TRUE))
			{
				alert_iprint(MGEMBACK); 
				return;
			}
#endif

		/* Build an environment if needed */

		if (argv)
		{
			if ((envp = make_env(fname, cmdline + 1)) == NULL)
				return;
			if (cl.length != 0)
				cl.length = 127;
		}
		else
			envp = NULL;

		/* 
		 * Do something only if filename of program is specified;
		 * (fn_get_path extracts path from full filename and
		 * allocates a string for this path
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
						error = (int) x_exec(100, fname, &cl, envp); /* just Pexec */
					else
#endif
						error = exec_com(fname, &cl, envp, appl_type);
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

		/* 
		 * There seems to be a problem with TOS 2.06 swallowing the 
		 * first mouse click after executing a program by clicking on
		 * a desktop icon, and there are no windows open.
		 * This below seems to cure that, at least temporarily
		 */
 
		if ( (tos_version == 0x206)  
#if _MINT_
		             && !(mint || geneva) 
#endif
		   ) 
		{
			int p[8] = {0,1, 1,1, 0,1, 0,0};

			appl_tplay( (void *)(&p), 1, 4 );
			appl_tplay( (void *)(&p[4]), 1, 4 );
		}

		/* Deallocate environment created for this program */

		if (envp)
			free(envp);

		if (error < 0)
			xform_error(error);
	}
#if _MINT_
	else
	{
		/* AES is multitasking, use shel_write() to start program. */

		if (prg == PACC)

			/* Start an accessory */

			shel_write(SHW_EXEC_ACC, 0, SHW_PARALLEL, (char *)fname, "\0"); 
		else
		{
			/* Start other types of applications */

			int mode;
			void *p[5];
			char prgpath[256], *h;

			/* Start gem/tos program (0x1), but in extended mode ( | 0x400) */

			mode = SHW_XMDDEFDIR | SHW_EXEC; /* always use SHW_EXEC */

			if ( limmem > 0L )	
				mode |= SHW_XMDLIMIT;

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

			if (argv)
				*(char *)cmdline = 127;
	
			p[0] = fname;	/* pointer to name */
			(long)p[1] = limmem;	/* value for Psetlimit() DjV 050 020703 */
			p[2] = NULL;	/* value for Prenice()   */
			p[3] = prgpath;	/* pointer to default directory */
			p[4] = NULL;
	
			/* SHW_PARALLEL(100) for MagiC */

			shel_write(mode, appl_type, magx ? SHW_PARALLEL : (int)argv,
			                 (char *)p, (char *)cmdline);
		}
	}
#endif
}


