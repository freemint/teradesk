/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren.
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

#include <np_aes.h>			/* HR 151102: modern */
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <tos.h>
#include <vdi.h>
#include <boolean.h>
#include <library.h>
#include <mint.h>
#include <system.h>
#include <xdialog.h>
#include <xscncode.h>

#include "batch.h"
#include "desk.h"
#include "environm.h"
#include "error.h"
#include "events.h"
#include "font.h"
#include "open.h"
#include "resource.h"
#include "version.h"
#include "xfilesys.h"
#include "dir.h"
#include "edit.h"
#include "file.h"
#include "filetype.h"
#include "icon.h"
#include "prgtype.h"
#include "screen.h"
#include "window.h"
#include "applik.h"
#include "icontype.h"
#include "vaproto.h"

#define RSRCNAME	"desktop.rsc"

#define EVNT_FLAGS	(MU_MESAG|MU_BUTTON|MU_KEYBD)

typedef struct
{
	int scancode;
	int title;
	int item;
} KEY;

static KEY keys[] =
{
	HELP, TDESK, MINFO,			/* Info */
	CTL_O, TLFILE, MOPEN,		/* Open */
	CTL_S, TLFILE, MSHOWINF,	/* Show info */
	CTL_F, TLFILE, MNEWDIR,		/* New folder */
	CTL_DEL,TLFILE, MDELETE,	/* HR 151102: Delete */
	CTL_U, TLFILE, MCLOSE,		/* Close */
	CTL_C, TLFILE, MCLOSEW,		/* Close window */
	CTL_A, TLFILE, MSELALL,		/* Select all */
	CTL_T, TVIEW, MSETMASK,		/* Set filetype */
	CTL_W, TLFILE, MCYCLE,		/* Cycle windows */
	CTL_Q, TLFILE, MQUIT,		/* Quit */
	CTL_L, TOPTIONS, MAPPLIK,	/* Install applikation */
	CTL_I, TOPTIONS, MIDSKICN,	/* Install icon */
	CTL_D, TOPTIONS, MCHNGICN,	/* Change icon */
	CTL_R, TOPTIONS, MREMICON,	/* Remove icons */
	CTL_P, TOPTIONS, MOPTIONS,	/* Set preferences */
	CTL_M, TOPTIONS, MPRGOPT,	/* Program options */
	CTL_K, TOPTIONS, MSAVESET,	/* Save settings */
	CTL_G, TVIEW, MSNAME,		/* Sort by name */
	CTL_H, TVIEW, MSEXT,		/* Sort by ext. */
	CTL_J, TVIEW, MSDATE,		/* Sort by date */
	CTL_N, TVIEW, MSSIZE,		/* Sort by size */
	CTL_V, TVIEW, MSUNSORT,		/* Unsorted */
	CTL_X, TVIEW, MHIDDEN,		/* Hidden files */
	CTL_Y, TVIEW, MSYSTEM		/* System files */
};

#define NKEYS	(sizeof(keys) / sizeof(KEY))

Options options;
int ap_id;
char *optname;
int vdi_handle, ncolors, max_w, max_h, nfonts;
SCRINFO screen_info;
FONT def_font;
static int menu_items[] =
{MINFO, TDESK, TLFILE, TVIEW, TOPTIONS};

#if _MINT_
boolean mint = FALSE, magx = FALSE;		/* HR 151102 */
#endif

boolean quit = FALSE;

char *global_memory;


char msg_resnfnd[] = "[1][Unable to find resource file.|"
					 "Resource file niet gevonden.|"
					 "Impossible de trouver le|fichier resource.|"
					 "Resource Datei nicht gefunden.][ OK ]";

int hndlmessage(int *message);

/*
 * Execute the desktop.bat file.
 *
 * Result : FALSE if there is no error, TRUE if there is an error.
 */

static boolean exec_deskbat(void)
{
	char *batname;
	int error;

	/* Remove the ARGV variable from the environment. */

	clr_argv();

	/* Initialize the name of the configuration file. */

	if ((optname = strdup("desktop.cfg")) == NULL)
		error = ENSMEM;
	else
	{
		/* Find the desktop.bat file. */

		if ((batname = xshel_find("desktop.bat", &error)) != NULL)
		{
			/* Execute it. */

			exec_bat(batname);
			free(batname);
		}
	}

	if ((error != 0) && (error != EFILNF))
	{
		xform_error(error);
		return TRUE;
	}
	else
		return FALSE;
}

static void info(void)
{
	rsc_ltoftext(infobox, INFOFMEM, (long) x_alloc(-1L));
	xd_dialog(infobox, 0);
}

/********************************************************************
 *																	*
 * Hulpfunkties voor dialoogboxen.									*
 * HR 151102L strip_name, cramped_name courtesy XaAES				*
 *																	*
 ********************************************************************/

/* HR 220401: strip leading and trailing spaces. */

static
void strip_name(char *to, char *fro)
{
	char *last = fro + strlen(fro) - 1;
	while (*fro && *fro == ' ')  fro++;
	if (*fro)
	{
		while (*last == ' ')
			last--;
		while (*fro && fro != last + 1)
			*to++ = *fro++;
	}
	*to = 0;
}

/* should become c:\s...ng\foo.bar */

void cramped_name(char *s, char *t, int w)
{
	char *q=s,*p=t, tus[256];
	int l, d, h;

	l = strlen(q);
	d = l - w;

	if (d > 0)
	{
		strip_name(tus, s);
		q = tus;
		l = strlen(tus);
		d = l - w;
	}

	if (d <= 0)
		strcpy(t,s);
	else
	{
		if (w < 12)		/* 8.3 */
			strcpy(t, q+d); /* only the last ch's */
		else
		{
			h = (w-3)/2;
			strncpy(p,q,h);
			p+=h;
			*p++='.';
			*p++='.';
			*p++='.';
			strcpy(p,q+l-h);
		}
	}
}

void digit(char *s, int x)
{
	x = x % 100;
	s[0] = x / 10 + '0';
	s[1] = x % 10 + '0';
}

/* This_is_a_thirty_two__bytes_name */

void cv_fntoform(char *dest, const char *source, int l)		/* HR 271102 l */
{
#if _MINT_
		cramped_name(source, dest, l);			/* HR 151102 */
#else
	{
		int s = 0, d = 0;
	
		while ((source[s] != 0) && (source[s] != '.'))
			dest[d++] = source[s++];
		if (source[s] == 0)
			dest[d++] = 0;
		else
		{
			while (d < 8)
				dest[d++] = ' ';
			s++;
			while (source[s] != 0)
				dest[d++] = source[s++];
			dest[d++] = 0;
		}
	}
#endif
}

void cv_formtofn(char *dest, const char *source)
{
#if _MINT_		/* HR 151102 */
		strcpy(dest,source);
#else
	{
		int s = 0, d = 0;
	
		while ((source[s] != 0) && (s < 8))
			if (source[s] != ' ')
				dest[d++] = source[s++];
			else
				s++;
		if (source[s] == 0)
			dest[d++] = 0;
		else
		{
			dest[d++] = '.';
			while (source[s] != 0)
				if (source[s] != ' ')
					dest[d++] = source[s++];
				else
					s++;
			dest[d++] = 0;
		}
	}
#endif
}

void set_opt(OBJECT *tree, int opt, int button)
{
	if (options.cprefs & opt)
		tree[button].ob_state |= SELECTED;
	else
		tree[button].ob_state &= ~SELECTED;		/* HR 151102 ! -> ~ */
}

static void set_dialmode(void)
{
	xd_setdialmode(options.dial_mode, hndlmessage, menu, (int) (sizeof(menu_items) / sizeof(int)), menu_items);
}

static void setpreferences(void)
{
	int button, prefs = options.cprefs & 0xFE38;

	if (setprefs->ob_width > max_w)
	{
		alert_printf(1, MDIALTBG);
		return;
	}

	set_opt(setprefs, CF_COPY, CCOPY);
	set_opt(setprefs, CF_DEL, CDEL);
	set_opt(setprefs, CF_OVERW, COVERW);
	set_opt(setprefs, SAVE_COLORS, SVCOLORS);

	xd_set_rbutton(setprefs, OPTPAR2, (options.cprefs & DIALPOS_MODE) ? DMOUSE : DCENTER);
	xd_set_rbutton(setprefs, OPTPAR1, DNORMAL + options.dial_mode);

	itoa(options.tabsize, tabsize, 10);
	itoa(options.bufsize, copybuffer, 10);

	button = xd_dialog(setprefs, TABSIZE);

	if (button == OPTOK)
	{
		int posmode = XD_CENTERED;

		prefs |= ((setprefs[CCOPY].ob_state & SELECTED) != 0) ? CF_COPY : 0;
		prefs |= ((setprefs[CDEL].ob_state & SELECTED) != 0) ? CF_DEL : 0;
		prefs |= ((setprefs[COVERW].ob_state & SELECTED) != 0) ? CF_OVERW : 0;
		prefs |= ((setprefs[SVCOLORS].ob_state & SELECTED) != 0) ? SAVE_COLORS : 0;

		if (xd_get_rbutton(setprefs, OPTPAR2) == DMOUSE)
		{
			prefs |= DIALPOS_MODE;
			posmode = XD_MOUSE;
		}

		options.cprefs = prefs;

		options.dial_mode = xd_get_rbutton(setprefs, OPTPAR1) - DNORMAL;

		if ((options.bufsize = atoi(copybuffer)) < 1)
			options.bufsize = 1;

		if ((options.tabsize = atoi(tabsize)) < 1)
			options.tabsize = 1;

		set_dialmode();
		xd_setposmode(posmode);
	}
}

static void opt_default(void)
{
	options.version = CFG_VERSION;
	options.magic = MAGIC;
	options.cprefs = CF_COPY | CF_DEL | CF_OVERW;
	options.sort = 0;
	options.attribs = 0;
	options.tabsize = 8;
	options.mode = 0;
	options.bufsize = 512;
	options.dial_mode = XD_NORMAL;
	options.resvd1 = 0;
	options.resvd2 = 0;
}

static void load_options(void)
{
	XFILE *file;
	Options tmp;
	int error = 0;
	long n;

	if ((file = x_fopen(optname, O_DENYW | O_RDONLY, &error)) != NULL)
	{
		if ((n = x_fread(file, &tmp, sizeof(Options))) == sizeof(Options))
		{
			if ((tmp.version >= 0x119) && (tmp.version <= CFG_VERSION) && (tmp.magic == MAGIC))
			{
				options = tmp;
				wd_deselect_all();
				wd_default();

				if (tmp.cprefs & SAVE_COLORS)
					error = load_colors(file);

				if (error == 0)
					if ((error = dsk_load(file)) == 0)
						if ((error = ft_load(file)) == 0)
							if ((error = icnt_load(file)) == 0)
								if ((error = app_load(file)) == 0)
									if ((error = prg_load(file)) == 0)
										error = wd_load(file);
			}
			else
			{
				alert_printf(1, MVALIDCF);
				x_fclose(file);
				return;
			}
		}
		else
		{
			error = (n < 0) ? (int) n : EEOF;
			hndl_error(MLOADCFG, error);
			x_fclose(file);
			return;
		}
		x_fclose(file);
	}

	if (error != 0)
	{
		hndl_error(MLOADCFG, error);

		opt_default();
		dsk_default();
		ft_default();
		icnt_default();
		app_default();
		prg_default();
		wd_default();
	}

	if (options.version < 0x0130)
		options.dial_mode = (options.cprefs & 0x80) ? XD_BUFFERED : XD_NORMAL;

	xd_setposmode((options.cprefs & DIALPOS_MODE) ? XD_MOUSE : XD_CENTERED);
	set_dialmode();

	options.version = CFG_VERSION;
}

static void save_options(void)
{
	XFILE *file;
	long n;
	int error = 0, h;

	graf_mouse(HOURGLASS, NULL);

	if ((file = x_fopen(optname, O_DENYRW | O_WRONLY, &error)) != NULL)
	{
		if ((n = x_fwrite(file, &options, sizeof(Options))) == sizeof(Options))
		{
			if (options.cprefs & SAVE_COLORS)
				error = save_colors(file);

			if (error == 0)
				if ((error = dsk_save(file)) == 0)
					if ((error = ft_save(file)) == 0)
						if ((error = icnt_save(file)) == 0)
							if ((error = app_save(file)) == 0)
								if ((error = prg_save(file)) == 0)
									error = wd_save(file);
		}
		else
			error = (int) n;

		if (((h = x_fclose(file)) < 0) && (error == 0))
			error = h;
	}

	graf_mouse(ARROW, NULL);

	if (error != 0)
		hndl_error(MSAVECFG, error);

	wd_set_update(WD_UPD_COPIED, optname, NULL);
	wd_do_update();
}

static void save_options_as(void)
{
	char *newname;

	if ((newname = locate(optname, L_SAVECFG)) != NULL)
	{
		free(optname);
		optname = newname;
		save_options();
	}
}

static void load_settings(void)
{
	char *newname;

	if ((newname = locate(optname, L_LOADCFG)) != NULL)
	{
		free(optname);
		optname = newname;
		load_options();
	}
}

/*
 * Initiation function.
 *
 * Result: FALSE if no error, TRUE if error.
 */

static boolean init(void)
{
	int error;
	char *fullname;

	xw_get(NULL, WF_WORKXYWH, (GRECT *) &screen_info.dsk_x);

	if ((fullname = xshel_find(optname, &error)) != NULL)
	{
		free(optname);
		optname = fullname;
	}
	else
	{
		if ((error == EFILNF) && ((fullname = x_fullname(optname, &error)) != NULL))
		{
			free(optname);
			optname = fullname;
		}
		if (error != 0)
		{
			xform_error(error);
			return TRUE;
		}
	}

	if (dsk_init() == TRUE)
		return TRUE;

	ft_init();
	icnt_init();
	app_init();
	prg_init();
	wd_init();

	menu_bar(menu, 1);

	x_setpath("\\");

	load_options();

	return FALSE;
}

static void init_vdi(void)
{
	int dummy, work_out[58], pix_height;

	screen_info.phy_handle = graf_handle(&screen_info.fnt_w, &screen_info.fnt_h, &dummy, &dummy);

	vq_extnd(vdi_handle, 0, work_out);

	max_w = work_out[0] + 1;
	max_h = work_out[1] + 1;
	ncolors = work_out[13];
	pix_height = work_out[4];
	vqt_attributes(vdi_handle, work_out);
	fnt_setfont(1, (int) (((long) work_out[7] * (long) pix_height * 72L + 12700L) / 25400L), &def_font);

	screen_info.vdi_handle = vdi_handle;
}

static int alloc_global_memory(void)
{
#if _MINT_
	if (magx || mint)
		global_memory = Mxalloc(GLOBAL_MEM_SIZE, 0x43);
	else
#endif
		global_memory = Malloc(GLOBAL_MEM_SIZE);

	return (global_memory) ? 0 : ENSMEM;
}

static void hndlmenu(int title, int item, int kstate)
{
	if ((menu[item].ob_state & DISABLED) == 0)
	{
		switch (item)
		{
		case MINFO:
			info();
			break;
		case MQUIT:
			menu_tnormal(menu, title, 1);
			quit = TRUE;
			break;
		case MOPTIONS:
			setpreferences();
			break;
		case MPRGOPT:
			prg_setprefs();
			break;
		case MSAVESET:
			save_options();
			break;
		case MLOADOPT:
			load_settings();
			break;
		case MSAVEAS:
			save_options_as();
			break;
		case MAPPLIK:
			app_install();
			break;
		case MIDSKICN:
			dsk_insticon();
			break;
		case MIWDICN:
			icnt_settypes();
			break;
		case MCHNGICN:
			dsk_chngicon();
			break;
		case MREMICON:
			dsk_remicon();
			break;
		case MEDITOR:
			set_editor();
			break;
		case MWDOPT:
			dsk_options();
			break;
		default:
			wd_hndlmenu(item, kstate);
			break;
		}
	}
	menu_tnormal(menu, title, 1);
}

static void hndlkey(int key, int kstate)
{
	int i = 0, k;
	APPLINFO *appl;

	k = key & ~XD_CTRL;

	if ((((unsigned int) k >= 0x803B) && ((unsigned int) k <= 0x8044)) ||
		(((unsigned int) k >= 0x8154) && ((unsigned int) k <= 0x815D)))
	{
		k &= 0xFF;
		k = (k >= 0x54) ? (k - 0x54 + 11) : (k - 0x3B + 1);

		if ((appl = find_fkey(k)) != NULL)
			app_exec(NULL, appl, NULL, NULL, 0, kstate, FALSE);
	}
	else
	{
		k = key & ~XD_ALT;

		while ((keys[i].scancode != k) && (i < (NKEYS - 1)))
			i++;

		if (keys[i].scancode == k)
		{
			menu_tnormal(menu, keys[i].title, 0);
			hndlmenu(keys[i].title, keys[i].item, kstate);
		}
		else
		{
			i = 0;
			if ((key >= ALT_A) && (key <= ALT_Z))
			{
				i = key - (XD_ALT | 'A');
				if (check_drive(i))
				{
					char *path;

					if ((path = strdup("A:\\")) != NULL)
					{
						path[0] = (char) i + 'A';
						dir_add_window(path);
					}
					else
						xform_error(ENSMEM);
				}
			}
		}
	}
}

#define AP_TERM		50
#define AP_TFAIL	51
#define SH_WDRAW	72

int _hndlmessage(int *message, boolean allow_exit)
{
	if (((message[0] >= AV_PROTOKOLL) && (message[0] <= AV_LAST)) ||
		((message[0] >= FONT_CHANGED) && (message[0] <= FONT_ACK)))
		handle_av_protocol(message);
	else
	{
		switch (message[0])
		{
		case AP_TERM:
			if (allow_exit)
				quit = TRUE;
			else
			{
				static int ap_tfail[8] = { AP_TFAIL, 0 };

				shel_write(10, 0, 0, (char *) ap_tfail, NULL);
			}
			break;

		case SH_WDRAW:
			wd_update_drv(message[3]);
			break;
		}
	}

	return 0;
}

int hndlmessage(int *message)
{
	return _hndlmessage(message, FALSE);
}

static void evntloop(void)
{
	int event;
	XDEVENT events;

	events.ev_mflags = EVNT_FLAGS;
	events.ev_mbclicks = 0x102;			/* HR 151102: right button is double click */
	events.ev_mbmask = 3;
	events.ev_mbstate = 0;
	events.ev_mm1flags = 0;
	events.ev_mm2flags = 0;
	events.ev_mtlocount = 0;
	events.ev_mthicount = 0;

	while (!quit)
	{
		event = xe_xmulti(&events);

		clr_key_buf();		/*	HR 151102: This imposed a unsolved problem with N.Aes 1.2 (lockup of teradesk after live moving) */
/* It is not a essential function. */

		if (event & MU_MESAG)
		{
			if (events.ev_mmgpbuf[0] == MN_SELECTED)
				hndlmenu(events.ev_mmgpbuf[3], events.ev_mmgpbuf[4], events.ev_mmokstate);
			else
				_hndlmessage(events.ev_mmgpbuf, TRUE);
		}

		if (event & MU_KEYBD)
			hndlkey(events.xd_keycode, events.ev_mmokstate);
	}
}

#if _MINT_
int have_ssystem;
#endif

int main(void)
{
	int error;

#if _MINT_				/* HR 151102 */
	have_ssystem = Ssystem(-1, 0, 0) == 0;		/* HR 151102: use Ssystem where possible */

	mint = (find_cookie('MiNT') == -1) ? FALSE : TRUE;
	magx = (find_cookie('MagX') == -1) ? FALSE : TRUE;	/* HR 151102 */
	mint |= magx;			/* Quick & dirty */

	if (mint)
	{
		Psigsetmask(0x7FFFE14EL);
		Pdomain(1);
	}
#endif

	x_init();

	if ((ap_id = appl_init()) < 0)
		return -1;

	if  (_GemParBlk.glob.version >= 0x400)
	{
		shel_write(9, 1, 0, NULL, NULL);
		menu_register(ap_id, "  Tera Desktop");
	}

	if (rsrc_load(RSRCNAME) == 0)
		form_alert(1, msg_resnfnd);
	else
	{
		if ((error = init_xdialog(&vdi_handle, malloc, free,
								  "Tera Desktop", 1, &nfonts)) < 0)
			xform_error(error);
		else
		{
			init_vdi();
			rsc_init();

			if (((max_w / screen_info.fnt_w) < 40) || ((max_h / screen_info.fnt_h) < 25))
				alert_printf(1, MRESTLOW);
			else
			{
				if ((error = alloc_global_memory()) == 0)
				{
					if (exec_deskbat() == FALSE)
					{
						if (load_icons() == FALSE)
						{
							if (init() == FALSE)
							{
								graf_mouse(ARROW, NULL);
								evntloop();

								wd_del_all();
								menu_bar(menu, 0);
								xw_close_desk();
							}

							free_icons();		/* HR 151102 */

							wind_set(0, WF_NEWDESK, NULL, 0);
							dsk_draw();
						}
					}

					Mfree(global_memory);
				}
				else
					xform_error(error);
			}

			if (vq_gdos() != 0)
				vst_unload_fonts(vdi_handle, 0);
			exit_xdialog();
		}

		rsrc_free();
	}

	appl_exit();

	return 0;
}
