/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren. 
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


#if _LOGFILE
#include <stdio.h>
#endif

#include <np_aes.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <vdi.h>
#include <library.h>
#include <mint.h>
#include <system.h>
#include <xdialog.h>
#include <xscncode.h>

#include "resource.h"
#include "desk.h"
#include "environm.h"
#include "error.h"
#include "events.h"
#include "font.h"
#include "open.h"
#include "version.h"
#include "xfilesys.h"
#include "config.h"
#include "window.h"
#include "dir.h"
#include "file.h"
#include "lists.h"
#include "slider.h"
#include "filetype.h"
#include "icon.h"
#include "prgtype.h"
#include "screen.h"
#include "icontype.h"
#include "applik.h"
#include "va.h"
#include "video.h"

#define RSRCNAME	"desktop.rsc" /* Name of te program's resource file */

#define EVNT_FLAGS	(MU_MESAG | MU_BUTTON | MU_KEYBD) /* For the main loop */

#define MAX_PLINE 132 	/* maximum printer line length */
#define MIN_PLINE 32	/* minimum printer line length */
#define DEF_PLINE 80	/* default printer line length */

extern boolean
	fargv;				/* flag to force use of ARGV whenever possible */

extern char 
	*xd_cancelstring;	/* from xdialog; possible 'Cancel' texts */

Options 
	options;			/* configuration options */

XDFONT 
	def_font;		/* Data for the default (system) font */

char
	*teraenv,		/* pointer to value of TERAENV environment variable */ 
	*fsdefext,		/* default filename extension in current OS      */
	*infname,		/* name of V3 configuration file (teradesk.inf)  */ 
	*palname,		/* name of V3 colour palette file (teradesk.pal) */
	*global_memory;	/* Globally available buffer for passing params  */

static char
	*definfname;	/* name of the initial configuration file at startup */

static const char
	*infide = "TeraDesk-inf"; 	/* File identifier header */

const char
	*empty = "\0",		/* often used string */
	*bslash = "\\",		/* often used string */
	*adrive = "A:\\",	/* often used string */
	*prevdir = "..";	/* often used string */

long
	global_mem_size;/* size of global_memory */ 

int 
	tos_version,	/* detected version of TOS; interpret in hex format */
	aes_version,	/* detected version of AES; interpret in hex format */
	ap_id,			/* application id. of this program (TeraDesk) */
	vdi_handle, 	/* current VDI station handle   */
	nfonts;			/* number of available fonts    */

static int
	shutopt = 0;	/* shutdown type: system halt/poweroff */

/* Names of menu boxes in the main menu */

static const int 
	menu_items[] = {MINFO, TDESK, TLFILE, TVIEW, TWINDOW, TOPTIONS};

#if _MINT_

int 
	have_ssystem = 0;

boolean			/* No need to define values, always set when starting main() */
	mint, 		/* True if Mint  is present  */
	magx,		/* True if MagiC is present  */	
	naes,		/* True if N.AES is present  */
	geneva;		/* True if Geneva is present */

#endif

boolean
	shutdown = FALSE,	/* true if system shutdown is required  */
	startup = TRUE;		/* true if desktop is starting */

static boolean
	ct60,				/* True if this is a CT60 machine       */
	chrez = FALSE, 		/* true if resolution should be changed */
	quit = FALSE,		/* true if teradesk should finish       */ 
    reboot = FALSE,		/* true if a reboot should happen       */
	shutting = FALSE;	/* true if started shutting down        */

boolean
	onekey_shorts;	/* true if any single-key menu shortcuts are defined */

#if _LOGFILE
FILE
	*logfile = NULL;
char
	*logname;
#endif




/*
 * Below is supposed to be the only text embedded in the code:
 * information (in several languages) that a resource file 
 * can not be found. It is shown in an alert box.
 */

static const char 
	msg_resnfnd[] =  "[1][Can not find the resource file.|"
                         "Resource Datei nicht gefunden.|"
					     "Resource file niet gevonden.|"
					     "Impossible de trouver le|fichier resource.][ OK ]";

static XDEVENT 
	loopevents;		/* events awaited for in the main loop */


void Shutdown(long mode);

static CfgNest				/* Elsewhere defined configuration routines */ 
	opt_config, 
	short_config; 

CfgNest
	dsk_config;

/* Root level of configuration data */

static const CfgEntry Config_table[] =
{
	{CFG_NEST, "options",	 opt_config	 },
	{CFG_NEST, "shortcuts",	 short_config},
	{CFG_NEST, "filetypes",	 ft_config	 },
	{CFG_NEST, "apptypes",    prg_config }, /* must be before icons and apps */
	{CFG_NEST, "icontypes",	 icnt_config },
	{CFG_NEST, "deskicons",	 dsk_config	 },
	{CFG_NEST, "applications",app_config },
	{CFG_NEST, "windows",	 wd_config	 },
	{CFG_NEST, "avstats",	 va_config   },
	{CFG_FINAL}, /* file completness check */
	{CFG_LAST}
};


/*
 * Configuration table for desktop options.
 * Take care to keep "infv" first in the list.
 * Bit flags are written in hex format for easier 
 * manipulation by humans.
 */

CfgEntry Options_table[] =
{
	{CFG_HDR, "options" },
	{CFG_BEG},
	/* file version */
	{CFG_X, "infv", &options.version	}, 		/* file version */
	/* desktop preferences */
	{CFG_X, "save", &options.sexit		},		/* what to save at exit */
	{CFG_X, "dial", &options.dial_mode	},		/* bit flags !!! dialog mode and position */
	{CFG_X, "xpre", &options.xprefs		}, 		/* bit flags !!! diverse prefs */
	/* Copy preferences */
	{CFG_X, "pref", &options.cprefs		}, 		/* bit flags !!! copy prefs */
	/* sizes of diverse items */
	{CFG_D, "buff", &options.bufsize	}, 		/* copy buffer size */
	{CFG_L, "maxd", &options.max_dir	},		/* initial dir size */
	{CFG_D, "plin", &options.plinelen	},		/* printer line length */
	{CFG_D, "tabs", &options.tabsize	}, 		/* tab size    */
	{CFG_D, "cwin", &options.cwin	    }, 		/* compare match size */
	/* settings of the View menu */
	{CFG_D, "mode", &options.mode		}, 		/* text/icon mode */
	{CFG_D, "sort", &options.sort		}, 		/* sorting key */
	{CFG_D, "aarr", &options.aarr		}, 		/* auto arrange */
	{CFG_X, "attr", &options.attribs	},		/* Bit flags !!! global attributes to show */
	{CFG_X, "flds", &options.fields		},		/* Bit flags !!! dir. fields to show  */
	/* video options */
	{CFG_X, "vidp", &options.vprefs	}, 			/* Bit flags ! */
	{CFG_D, "vres", &options.vrez	},			/* video resolution  (currently unused) */
	/* patterns and colours */
	{CFG_D, "dpat", &options.dsk_pattern},		/* desk pattern */
	{CFG_D, "dcol", &options.dsk_colour	},		/* desk colour  */
	{CFG_D, "wpat", &options.win_pattern},		/* window pattern */
	{CFG_D, "wcol", &options.win_colour	},		/* window colour  */

	{CFG_ENDG},
	{CFG_LAST}
};


/*
 * Configuration table for menu shortcuts. If the main menu of 
 * TeraDesk is changed,  this table should always be updated to 
 * match the actual state.
 */


CfgEntry Shortcut_table[] =
{
	{CFG_HDR, "shortcuts" },
	{CFG_BEG},

	/* File menu */
	{CFG_X, "open", &options.kbshort[MOPEN		- MFIRST]	},
	{CFG_X, "show", &options.kbshort[MSHOWINF	- MFIRST]	},
	{CFG_X, "newd", &options.kbshort[MNEWDIR	- MFIRST]	},
	{CFG_X, "comp", &options.kbshort[MCOMPARE	- MFIRST]	},
	{CFG_X, "srch", &options.kbshort[MSEARCH	- MFIRST]	},
	{CFG_X, "prin", &options.kbshort[MPRINT		- MFIRST]	},
	{CFG_X, "dele", &options.kbshort[MDELETE	- MFIRST]	},
	{CFG_X, "sela", &options.kbshort[MSELALL	- MFIRST]	},
#if MFCOPY
	{CFG_X, "copy", &options.kbshort[MFCOPY		- MFIRST]	},
	{CFG_X, "form", &options.kbshort[MFFORMAT	- MFIRST]	},
#else
	{CFG_X | CFG_INHIB, "copy", &inhibit					},
	{CFG_X | CFG_INHIB, "form", &inhibit					},
#endif
	{CFG_X, "quit", &options.kbshort[MQUIT		- MFIRST]	},

	/* View menu */
#ifdef MSHOWTXT
	{CFG_X, "shtx", &options.kbshort[MSHOWTXT	- MFIRST]	},
#endif
	{CFG_X, "shic", &options.kbshort[MSHOWICN	- MFIRST]	},
	{CFG_X, "sarr", &options.kbshort[MAARNG		- MFIRST]	},
	{CFG_X, "snam", &options.kbshort[MSNAME		- MFIRST]	},
	{CFG_X, "sext", &options.kbshort[MSEXT		- MFIRST]	},
	{CFG_X, "sdat", &options.kbshort[MSDATE		- MFIRST]	},
	{CFG_X, "ssiz", &options.kbshort[MSSIZE		- MFIRST]	},
	{CFG_X, "suns", &options.kbshort[MSUNSORT	- MFIRST]	},
	{CFG_X, "revo", &options.kbshort[MREVS		- MFIRST]	},
	{CFG_X, "asiz", &options.kbshort[MSHSIZ		- MFIRST]	},
	{CFG_X, "adat", &options.kbshort[MSHDAT		- MFIRST]	},
	{CFG_X, "atim", &options.kbshort[MSHTIM		- MFIRST]	},
	{CFG_X, "aatt", &options.kbshort[MSHATT		- MFIRST]	},
#if _MINT_
	{CFG_X, "aown", &options.kbshort[MSHOWN		- MFIRST]	},
#endif
	{CFG_X, "smsk", &options.kbshort[MSETMASK	- MFIRST]	},

	/* Window menu */

	{CFG_X, "wico", &options.kbshort[MICONIF	- MFIRST]	},
	{CFG_X, "wful", &options.kbshort[MFULL		- MFIRST]	},
	{CFG_X, "clos", &options.kbshort[MCLOSE		- MFIRST]	},
	{CFG_X, "wclo", &options.kbshort[MCLOSEW	- MFIRST]	},
	{CFG_X, "cloa", &options.kbshort[MCLOSALL	- MFIRST]	},
	{CFG_X, "wdup", &options.kbshort[MDUPLIC	- MFIRST]	},
	{CFG_X, "cycl", &options.kbshort[MCYCLE		- MFIRST]	},

	/* Options menu */
	{CFG_X, "appl", &options.kbshort[MAPPLIK	- MFIRST]	},
	{CFG_X, "ptyp", &options.kbshort[MPRGOPT	- MFIRST]	},
	{CFG_X, "dski", &options.kbshort[MIDSKICN	- MFIRST]	},
	{CFG_X, "wini", &options.kbshort[MIWDICN	- MFIRST]	},
	{CFG_X, "pref", &options.kbshort[MOPTIONS	- MFIRST]	},
	{CFG_X, "copt", &options.kbshort[MCOPTS		- MFIRST]	},
	{CFG_X, "wopt", &options.kbshort[MWDOPT		- MFIRST]	},
	{CFG_X, "vopt", &options.kbshort[MVOPTS		- MFIRST]	},
	{CFG_X, "ldop", &options.kbshort[MLOADOPT	- MFIRST]	},
	{CFG_X, "svop", &options.kbshort[MSAVESET	- MFIRST]	},
	{CFG_X, "svas", &options.kbshort[MSAVEAS	- MFIRST]	},

	{CFG_ENDG},
	{CFG_LAST}
};


/* 
 * Try to allocate some memory and check success. Display an alert if failed.
 * There will generally be some loss in speed, so use with discretion.
 */

void *malloc_chk(size_t size)
{
	void *address = malloc(size);

	if (address == NULL)
		xform_error(ENSMEM);

	return address;
}


/*
 * Display a dialog, but check for errors when opening and display an alert.
 * This routine ends only when the dialog is closed.
 */

int chk_xd_dialog(OBJECT *tree, int start)
{
	int error = xd_dialog(tree, start);

	xform_error(error);
	return error;
}


/*
 * Open a dialog, but check for errors and display an alert.
 * In some dialogs it is not convenient to use this routine :(
 */

int chk_xd_open(OBJECT *tree, XDINFO *info)
{
	int error;

	arrow_mouse();
	error = xd_open(tree, info);
	xform_error(error);
	return error;
}


/* 
 * Inquire about the size of the largest block of free memory. 
 * At least TOS 2.06 is needed in order to inquire about TT/Alt memory.
 * Note that MagiC reports as TOS 2.00 only but it is assumed that
 * Mint and Magic can always handle Alt/TT-RAM
 */

static void get_freemem(long *stsize, long *ttsize)
{
#if _MINT_
	if ( mint || (tos_version >= 0x206))
#else
	if (tos_version >= 0x206)
#endif
	{
		*stsize = (long)Mxalloc( -1L, 0 );	
		*ttsize = (long)Mxalloc( -1L, 1 );
	}
	else
	{
		*stsize = (long)Malloc( -1L );
		*ttsize = 0L;
	}
}

/*
 * Show information on current versions of TeraDesk, TOS, AES...
 * Note: constant part of info for this dialog is handled in resource.c 
 * This involves setting TeraDesk name and version, etc.
 */

static void info(void)
{
	long stsize, ttsize; 			/* sizes of ST-RAM and TT/ALT-RAM */

	/* Inquire about the size of the largest free memory block */

	get_freemem(&stsize, &ttsize);

	/* 
	 * Display currently available memory. 
	 * Maximum correctly displayed size is 2GB 
	 */

	rsc_ltoftext(infobox, INFSTMEM, stsize );
	rsc_ltoftext(infobox, INFTTMEM, ttsize ); 

	chk_xd_dialog(infobox, 0);
}


/*
 * If HELP is pressed, display consecutive boxes of text in a dialog,
 * or, if <Shift><Help> is pressed, try to call the .HYP file viewer
 * (usually this is ST-GUIDE program or accessory).
 * Currently, there is no notification if the call to the viewer
 * is successful.
 */

static void showhelp (unsigned int key) 		
{		
	static const char 
		hbox[] = {HLPBOX1, HLPBOX2, HLPBOX3, HLPBOX4};


	hyppage = (char *)empty;

	if ( key & XD_SHIFT )
	{
		opn_hyphelp(); 
	}
	else
	{
		XDINFO 
			info;

		int 
			i = 0, 
			button;

		obj_unhide(helpno1[HELPOK]);
		obj_unhide(helpno1[HLPBOX1]);

		if (chk_xd_open(helpno1, &info) >=0)
		{
			do
			{
				button = xd_form_do(&info, ROOT);
				xd_buttnorm(&info, button); 
				obj_hide(helpno1[hbox[i]]);

				/* At i == 3 one must and can exit this way only */	

				if ( button == HELPCANC )
					break;

				i++;

				obj_unhide( helpno1[hbox[i]] );

				if ( i == 3 )
					obj_hide(helpno1[HELPOK]);

				xd_drawdeep(&info, ROOT);
			}
			while(TRUE);

			xd_close(&info);
		} /* open ? */
	}
}


/* 
 * Generalized set_opt() to change display of any options button from bitflags
 * In fact it can set option buttons from boolean variables as well
 * (then set "opt" to 1, and "button" is a boolean variable)
 */

void set_opt( OBJECT *tree, int flags, int opt, int button)
{
	if ( flags & opt )
		obj_select(tree[button]);
	else
		obj_deselect(tree[button]);		
}


/* 
 * Inverse function to the above- set bit flag from button id. 
 */

void get_opt( OBJECT *tree, int *flags, int opt, int button)
{
	if ( tree[button].ob_state & SELECTED )
		*flags |= opt;
	else
		*flags &= ~opt;
}


/* 
 * Set dialogs to a specific display mode, without too many arguments 
 */

static void set_dialmode(void)
{
	xd_setdialmode( (options.dial_mode & DIAL_MODE), hndlmessage, menu, (int) (sizeof(menu_items) / sizeof(int)), menu_items);
}


/* See also desk.h */

/*
 * A routine for displaying a keyboard shortcut in a human-readable form; 
 * <DEL>, <BS>, <TAB> and <SP> are displayed as "DEL", "BS", "TAB" and "SP",
 * other single characters are represented by that character;
 * "control" is represented by "^";
 * Uppercase or ctrl-uppercase are always assumed;
 * resultant string is never more than 4 characters long;
 * XD_CTRL is used for convenience; no need to define a new flag
 */

static void disp_short
(
	char *string,	/* resultant string */ 
	int kbshort,	/* keyboard shortcut to be displayed */ 
	int left		/* left-justify and 0-terminate string if true */
)
{
	char
		k = (char)(kbshort & 0x00FF),
		*strp = &string[3];

	switch (k)
	{
		case BACKSPC:
		{
			*strp-- = 'S';
			*strp-- = 'B';
			break;
		}
		case TAB:
		{ 
			*strp-- = 'B';
			*strp-- = 'A';
			*strp-- = 'T';
			break;
		}
		case SPACE:
		{ 
			*strp-- = 'P';
			*strp-- = 'S';
			break;
		}
		case DELETE:
		{ 
			*strp-- = 'L';
			*strp-- = 'E';
			*strp-- = 'D';
			break;
		}
		default:				/* Other  */
		{
			if ( kbshort != 0 ) 
				*strp-- = k;
		} 	
	}
	
	if ( kbshort & XD_CTRL )	/* Preceded by ^ ? */
		*strp-- = '^';		

	while(strp >= string)
		*strp-- = ' ';
	
	if (left)		/* if needed- left justify, terminate with 0 */
	{
		string[4] = 0;
		strip_name( string, string );
	}
}


/* 
 * A routine for inserting keyboard menu shortcuts into menu texts;
 * it also marks where the domain of a new menu title begins;
 * for this, bit-flag XD_ALT is used (just convenient, no need to define
 * something else)
 */

static void ins_shorts(void) 
{
	int 
		menui, 	/* menu item counter */
		lm;		/* length of string in menu item */ 

	char
		*where; /* address of location of shortcut in a menu string */

	onekey_shorts = FALSE;

	for ( menui = MFIRST; menui <= MLAST; menui++ ) 		/* for each menu item */
	{
		if ( menu[menui].ob_type == G_STRING )		 		/* which is a string... */
		{
			if ( menu[menui].ob_spec.free_string[1] == ' ') /* and not a separator line */
			{
				int shortcut = options.kbshort[menui - MFIRST];

				if(shortcut == BACKSPC || (shortcut >= ' ' && shortcut <= '~') )
					onekey_shorts = TRUE;

				lm = (int)strlen(menu[menui].ob_spec.free_string); /* includes trailing spaces */
				where = menu[menui].ob_spec.free_string + (long)(lm - 5);

				disp_short( where, shortcut, FALSE);

			} 		/* ' ' ? 			*/
		}			/* string ? 		*/
		else
			options.kbshort[menui - MFIRST] = XD_ALT; /* under new title from now on */
	}				/* for... 			*/
}


/* 
 * Handle the "Preferences" dialog for setting some 
 * TeraDesk configuration aspects 
 */

static void setpreferences(void)
{
	XDINFO 
		prefinfo;			/* dialog handling structure */
 
	static int 				/* Static so as to remember position */
		menui = MFIRST;		/* .rsc index of currently displayed menu item */

	int 
		button = OPTMNEXT,	/* current button index */
		mi,					/* menui - MFIRST */
		redraw = TRUE,		/* true if to redraw menu item and key def */
		lm,					/* length of text field in current menu item */
		lf,					/* length of form for menu item text */
		i, j;				/* counters */

	unsigned int 
		*tmpmi,
		tmp[NITEM + 2];		/* temporary kbd shortcuts (until OK'd) */

	boolean
		shok;				/* TRUE if a shortcut is acceptable */

	char
		aux[5];				/* temp. buffer for string manipulation */


	/*  Set state of radio buttons and checkbox button(s) */

	xd_set_rbutton(setprefs, OPTPAR2, (options.dial_mode & DIALPOS_MODE) ? DMOUSE : DCENTER);
	xd_set_rbutton(setprefs, OPTPAR1, DBUFFERD + (options.dial_mode & DIAL_MODE) - 1);
	set_opt( setprefs, options.sexit, SAVE_CFG, SEXIT);

	lf = (int)strlen(setprefs[OPTMTEXT].ob_spec.free_string); /* get length of space for displaying menu items */

	/* Copy shortcuts to temporary storage */

	memcpy( &tmp[0], &options.kbshort[0], (NITEM + 1) * sizeof(int) );

	if(chk_xd_open(setprefs, &prefinfo) >= 0)	/* Open dialog; then loop until OK or Cancel */
	{
		/* 
		 * Handle setting of keyboard shortcuts; note: because of limitations in
		 * keyboard scancodes, distinction of some key combinations is impossible
		 * Because of that, e.g. ^ESC, ^BS and ^TAB are not permitted.
		 * Also, characters like SP and CR which are used for other purposes
		 * are not permitted as shortcuts.
		 */

		while ( button != OPTOK && button != OPTCANC )
		{
			/* Display text of current menu item */

			mi = menui - MFIRST;
			tmpmi = &tmp[mi];

			if ( redraw )
			{
				lm = (int)strlen(menu[menui].ob_spec.free_string); /* How long? Assumed always to be lm > 5 */

				/* Copy menu text to dialog, remove shortcut text */

				strncpy 
				( 
					setprefs[OPTMTEXT].ob_spec.free_string,
					menu[menui].ob_spec.free_string + 1L,
					min(lm, lf) - 1
				);

				for ( i= min(lf, lm - 6); i < lf; i++ ) 
					setprefs[OPTMTEXT].ob_spec.free_string[i] = ' '; 

				/* Display defined shortcut and assigned key in ASCII form */

				disp_short( setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext, *tmpmi, TRUE );
        		xd_drawthis( &prefinfo, OPTMTEXT );
				xd_drawthis( &prefinfo, OPTKKEY );
				redraw = FALSE;
			}

			do 		/* again: */
			{
				shok = TRUE;

				button = xd_form_do ( &prefinfo, ROOT ); 

				/* Interpret shortcut from the dialog */

				strip_name( aux, setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext );
				strcpy ( setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext, aux );
			
				i = (int)strlen( aux ); 
				*tmpmi = 0;

				switch ( i )
				{
					case 0:						/* nothing defined */
					{
						break;
					}
					case 1:						/* single-character shortcut */
					{
						*tmpmi = aux[0] & 0x00FF;
						break;
					}
					case 2:						/* two-character ^something shortcut or BS */
					{
						if 
						(    
							(aux[0] == '^')  &&
						    ( 
								isupper((int)aux[1]) ||
								isdigit((int)aux[1]) ||
								( (aux[1] & 0x80) != 0 )
							)
						)
							*tmpmi = (aux[1] & 0x00FF) | XD_CTRL;
						else if (strcmp(aux, "BS") == 0)	/* BS */
							*tmpmi = BACKSPC;
						else
							*tmpmi = XD_CTRL;  /* illegal/ignored menu object */
						break;	
					}
					default:					/* longer shortcuts */
					{
						if ( aux[0] == '^' )
						{
							*tmpmi = XD_CTRL;
							aux[0] = ' ';
							strip_name( aux, aux );

							if ( strcmp( aux, "SP") == 0 )		/* ^SP */
								*tmpmi |= SPACE;
						}
						else
						{
							if ( strcmp( aux, "TAB") == 0 )		/* TAB */
								*tmpmi = TAB;
						}

						if ( strcmp( aux, "DEL" ) == 0 )		/* DEL & ^DEL */
							*tmpmi |= DELETE;

						if(*tmpmi == 0)
							*tmpmi = XD_CTRL;
					}
				}		/* switch */

				/* Now check for duplicate keys */

				if( button != OPTCANC && button != OPTKCLR )
				{
					/* Note: this tests tmp[mi] many times repeatedly, but who cares */

					for ( j = 0; j <= NITEM; j++ ) 
					{
						/* XD_ALT below in order to skip items related to menu boxes */
						if (   ( ( (*tmpmi & XD_SCANCODE) != 0 ) && ( (*tmpmi & 0xFF) < 128) )
					    	|| (   (*tmpmi & ~XD_ALT) != 0 && (mi != j) && *tmpmi == tmp[j] ) /* duplicate */
							|| *tmpmi == XD_CTRL				/* ^ only, illegal */
					       )
						{
							alert_printf ( 1, ADUPKEY, setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext );
							shok = FALSE;
							break;				
						}
					}
				}
			}
			while(!shok);

			/* 
			 * Only menu items which lay between MFIRST and MLAST are considered;
			 * if menu structure is changed, this interval should be redefined too;
			 * only those menu items with a space in the second char position
			 * are considered; other items are assumed not to be valid menu texts
			 * note: this code will crash in the (ridiculous) situation when the
			 * first or the last menu item is not a good text
			 */

			switch ( button )
			{
				case OPTMPREV:
				{
					while ( menui > MFIRST && menu[--menui].ob_type != G_STRING);
					if ( menu[menui].ob_spec.free_string[1] != ' ' ) 
						menui--;
					redraw = TRUE;
					break;
				}
				case OPTMNEXT:
				{
					while ( menui < MLAST && menu[++menui].ob_type != G_STRING);
					if ( menu[menui].ob_spec.free_string[1] != ' ' ) 
						menui++;
					redraw = TRUE;
					break;
				}
				case OPTKCLR:
				{
					memclr( &tmp[0], (NITEM + 2) * sizeof(int) );
					redraw = TRUE;
					break;
				}
				default:
				{
					break;
				}
			}
		} /* while... */

		/* Here we come only with OK or Cancel */

		xd_buttnorm( &prefinfo, button);
		xd_close(&prefinfo);

		if (button == OPTOK)
		{
			int posmode = XD_CENTERED;

			/* 
			 * Move kbd. shortcuts into permanent storage,
			 * then insert them into menu texts
			 */

			memcpy( &options.kbshort[0], &tmp[0], (NITEM + 1) * sizeof(int) );

			ins_shorts();

			/* Get and then set dialog display mode */

			options.dial_mode = xd_get_rbutton(setprefs, OPTPAR1) - DBUFFERD + 1;

			if (xd_get_rbutton(setprefs, OPTPAR2) == DMOUSE)
			{
				options.dial_mode |= DIALPOS_MODE;	
				posmode = XD_MOUSE;
			}
			else
				options.dial_mode &= ~DIALPOS_MODE;

			get_opt(setprefs, (int *)(&options.sexit), SAVE_CFG, SEXIT);
 
			set_dialmode();
			xd_setposmode(posmode);
		}
	}		/* open ? */
}


/* 
 * Handle the dialog for setting options related to copying and printing 
 */

static void copyprefs(void)
{
	char 
		*copybuffer = copyoptions[COPYBUF].ob_spec.tedinfo->te_ptext;	

	int 
		i, 
		button;

	static const int 
		bitflags[] = {CF_COPY, CF_OVERW, CF_DEL, CF_PRINT, CF_SHOWD, CF_KEEPS, P_HEADER};
 

	/* Set states of appropriate options buttons and copy buffer field */

	for ( i = CCOPY; i <= CPPHEAD; i++ )
		set_opt(copyoptions, options.cprefs, bitflags[i - CCOPY], i);

	itoa(options.bufsize, copybuffer, 10);
	itoa(options.plinelen, copyoptions[CPPLINE].ob_spec.tedinfo->te_ptext, 10 ); 

	/* The dialog itself */

	button = chk_xd_dialog(copyoptions, 0);

	/* If OK is selected... */

	if (button == COPTOK) 
	{	  
		/* Get new states of options buttons and new copy buffer size */

		for ( i = CCOPY; i <= CPPHEAD; i++ )
			get_opt( copyoptions, &options.cprefs, bitflags[i - CCOPY], i);

		if ((options.bufsize = atoi(copybuffer)) < 1)
			options.bufsize = 1;

		options.plinelen = atoi(copyoptions[CPPLINE].ob_spec.tedinfo->te_ptext);

		if ((options.plinelen < MIN_PLINE) || (options.plinelen > MAX_PLINE) )
			options.plinelen = DEF_PLINE;
	}
}


/*
 * Set default options to be in effect without a configuration file.
 * Also set desktop and window background.
 */

static void opt_default(void)
{
	get_set_video(0);			/* get current video mode for default */
	
	memclr( &options, sizeof(options) );
	options.version = CFG_VERSION;
	options.max_dir = 256;
	options.dial_mode = XD_BUFFERED;
	options.bufsize = 512;
	options.tabsize = 8;
#if _PREDEF
	options.cprefs = CF_COPY | CF_DEL | CF_OVERW | CF_PRINT | CF_TOUCH | CF_SHOWD;
	options.fields = WD_SHSIZ | WD_SHDAT | WD_SHTIM | WD_SHATT | WD_SHOWN;    
	options.plinelen = DEF_PLINE; 							
	options.attribs = FA_SUBDIR | FA_SYSTEM;
#endif
	options.aarr = 1;	

	/* 
	 * There is no need to set options.sort, .mode, .sexit, .dsk_pattern, 
	 * .dsk_colour, .win_pattern and .win_colour  because all of options
	 * is set to 0 in memclr() above
	 */									    
}


/* 
 * Define some default keyboard shortcuts ... 
 * those offered here have become a de-facto standard in other applications.
 * On the other hand, they are different from those which Atari has adopted
 * for the in-built TOS desktop.Personally, I would prefer some other ones,
 * e.g. "?" for "Info..." and "*" for "Select all"
 */

static void short_default(void)
{
	memclr( &options.kbshort[MOPEN - MFIRST], (size_t)(MSAVESET - MFIRST + 1) );

#if _PREDEF
	options.kbshort[MOPEN - MFIRST] =    XD_CTRL | 'O';		/* ^O, etc. */
	options.kbshort[MSHOWINF - MFIRST] = XD_CTRL | 'I';
	options.kbshort[MSEARCH - MFIRST] =  XD_CTRL | 'F';
	options.kbshort[MPRINT - MFIRST] =   XD_CTRL | 'P';
	options.kbshort[MSELALL - MFIRST] =  XD_CTRL | 'A';
	options.kbshort[MQUIT - MFIRST] =    XD_CTRL | 'Q';
	options.kbshort[MDELETE - MFIRST] =  XD_CTRL | DELETE;
	options.kbshort[MSAVESET - MFIRST] = XD_CTRL | 'S';
	options.kbshort[MCYCLE - MFIRST] =   XD_CTRL | 'W';
#endif

	ins_shorts();
}


/* 
 * Read configuration from the configuration file 
 */

static int load_options(void)
{
	int error = 0;

	autoloc_off();
	noicons = FALSE;			/* so that missing ones be reported */

	error = handle_cfgfile(infname, Config_table, infide, CFG_LOAD); 

	/* 
	 * If there was an error when loading options, load defaults.
	 */

	if (error != 0)
	{
		opt_default();		/* options */
		wd_default();		/* windows */
		dsk_default();		/* desktop */
		get_set_video(1);	/* set video */
		short_default();	/* kbd shortcuts */
		ft_default();		/* filetypes */
		prg_default();		/* programtypes */
		icnt_default();		/* icontypes */
		app_default();		/* applications */
		vastat_default();	/* AV status strings */
	}

	/* Set dialogs mode */

	xd_setposmode((options.dial_mode & DIALPOS_MODE) ? XD_MOUSE : XD_CENTERED);
	set_dialmode();

	return error;
}


/*
 * Configuration routine for basic desktop options
 */

static CfgNest opt_config
{
	if (io == CFG_SAVE)
	{
		/* Save options */

		options.version = CFG_VERSION;
		get_set_video(0);			/* get current video mode */

		*error = CfgSave(file, Options_table, lvl, CFGEMP); 
	}
	else
	{
		/* Initialize options structure to zero, then default, then load options */

		opt_default();

		*error = CfgLoad(file, Options_table, MAX_KEYLEN, lvl); 

		if ( *error >= 0 )
		{
			/* 
			 * Check some critical values against limits; if not checked and
			 * some illegal value happened to be in the file, these variables
			 * might crash the program or have other ugly consequences
			 */

			if ( options.plinelen < MIN_PLINE ) /* probably not entered in the dialog */
				options.plinelen = DEF_PLINE; 

			if 
			(   options.version < MIN_VERSION 
				|| options.version > CFG_VERSION 
				|| (options.sort & ~WD_REVSORT) > WD_NOSORT
				|| options.plinelen > MAX_PLINE
				|| options.max_dir < 32 
				|| (options.dial_mode & DIAL_MODE) > XD_WINDOW
			)
				*error = EFRVAL;

			if ( *error >= 0 )
			{

				/* Block possible junk from some fields in config file */

				options.attribs &= 0x0077;
				options.dsk_pattern = limpattern(options.dsk_pattern);
				options.win_pattern = limpattern(options.win_pattern);
/* currently not used
				options.vrez &= 0x0007;
*/
				/* Currently it makes no sense NOT to confirm touch */

				options.cprefs |= CF_TOUCH;
				options.cprefs &= ~(CF_CTIME | CF_CATTR);
#if PALETTES
				/* Load palette. Ignore errors */

				handle_colours(CFG_LOAD);
#endif
				/* Set video state but do not change resolution */

				get_set_video(1);

				/* If all is OK so far, start setting TeraDesk */

				set_dsk_background(options.dsk_pattern, options.dsk_colour);
			}
		}
	}
}


/*
 * Configure (load or save) keyboard shortcuts
 * Note: for loading, this is initialized to zero earlier in opt_config.
 */

static CfgNest short_config
{
	*error = handle_cfg(file, Shortcut_table, lvl, CFGEMP, io, NULL, short_default);

	if ( io == CFG_LOAD )
	{
		if ( *error == 0 )
			ins_shorts();
	}
}


/* 
 * Save configuration into default config file 
 */

static void save_options(const char *fname)
{
	hourglass_mouse();

	/* First save the palette (ignore errors) */
#if PALETTES
		handle_colours(CFG_SAVE);
#endif

	/* Then save the configuration */

	handle_cfgfile( (char *)fname, Config_table, infide, CFG_SAVE );
	
	/* Update destination window */

	wd_set_update(WD_UPD_COPIED, fname, NULL);
	wd_do_update();
	arrow_mouse();
}


/* 
 * Save configuration into an explicitely specified config file
 * (file selector is opened to specify the file)
 */

static void save_options_as(void)
{
	char *newinfname;

	if ((newinfname = locate(infname, L_SAVECFG)) != NULL)
	{
		free(infname);
		infname = newinfname;
		save_options(infname);
	}
}


/* 
 * Load configuration from an explicitely specified config file.
 * If this does not succeed, attempt to recover the state obtained
 * from a previously loaded configuration file (i.e. reload that file).
 */

void load_settings
(
	char *newinfname	/* Name of a configuration file */
)
{
	if (newinfname && *newinfname && !x_checkname(newinfname, empty))
	{
		char *oldinfname = infname;

		infname = newinfname;

		if(load_options() != 0 && oldinfname)
		{
			free(infname);
			infname = oldinfname;
			load_options();
		}
		else
			free(oldinfname);
	}
}


/*
 * Set complete specification for configuration file(s)
 */

boolean find_cfgfiles(char **cfgname)
{
	char
		*fullname;

	int
		error;

	if ((fullname = xshel_find(*cfgname, &error)) != NULL)
	{
		free(*cfgname);
		*cfgname = fullname;
	}
	else	/* fullname is NULL, so error must be nonzero */
	{
		if ((error == EFILNF) && ((fullname = x_fullname(*cfgname, &error)) != NULL))
		{
			free(*cfgname);
			*cfgname = fullname;
		}
	}

	xform_error(error);

	return (error >= 0) ? TRUE : FALSE;
}


/*
 * Initiation function (reading configuraton, defaults, etc.).
 * Result: TRUE if OK.
 * This function is called only once during program execution.
 */

static boolean init(void)
{
	/* Get screen work area */

	xw_getwork(NULL, &xd_desk);

	/* Find configuration files */

	if(find_cfgfiles(&infname) && find_cfgfiles(&palname))
	{
		/* Save the name of the initial configuration file */

		definfname = strdup(infname);

#if _LOGFILE
		logname  = strdup(infname);
		strcpy(strrchr(logname,'.'), ".log");
		logfile = fopen((const char *)logname, "w");

		if (logfile == NULL)
			printf("\n CAN'T OPEN LOGFILE");
		else
			fprintf(logfile, "\n LOG FILE OPENED; ap_id %i", ap_id);
#endif

		if (!dsk_init())
			return FALSE;

		/* Clear all definable items */

		ft_init();				/* filetype masks */
		icnt_init();			/* filetype icons */
		prg_init();				/* program types  */
		app_init();				/* installed apps */
		va_init();				/* AV-protocol    */
		wd_init();				/* windows        */

		hyppage = (char *)empty;

		menu_bar(menu, 1);

		/* 
	 	 * Force the name of the single-tasking program to DESKTOP 
	 	 * this call to menu_register() is documented only since AES4
	 	 * but in fact works in all Atari AESes.
	 	 * Must come after menu_bar()
	 	 */

#if _MINT_
		/* no need to do anything here ? */
#else
		menu_register(-1, thisapp);
#endif

		/* Set a default path. Ignore errors */

		x_setpath(bslash);

		/* Load the configuration file */

		load_options();

		/* Start applications which have been defined as autostart */

		app_specstart(AT_AUTO, NULL, NULL, 0, 0);
		startup = FALSE;

		return TRUE;
	}
	else
		return FALSE;
}


/* 
 * Initialize some stuff related to VDI
 */

static void init_vdi(void)
{
	int lwork_out[10];

	/* Note: graf_handle returns screen physical handle, but it is not needed */

	xd_screensize();
	vqt_attributes(vdi_handle, lwork_out);
	fnt_setfont(1, (int) (((long) lwork_out[7] * (long)xd_pix_height * 72L + 12700L) / 25400L), &def_font);
	def_font.effects = FE_NONE;
	def_font.colour = BLACK;
}


/*
 * Allocate a globally readable buffer for data exchange (e.g. AV-protocol)
 */

static int alloc_global_memory(void)
{
	long
		stsize,		/* size of available ST RAM */
		ttsize;		/* size of available TT RAM */

	/* 
	 * 0x02: Alt/TT-RAM or ST-RAM, prefer ST-RAM
	 * 0x03: Alt/TT-RAM or ST-RAM, prefer Alt/TT-RAM
	 * 0x40: globally readable
	 * Maybe better to use ST-RAM, if some other programs have
	 * problems knowing of Alt-RAM ???
	 */

	int
		mode = 0x03;

#if _MINT_
	if(mint)
		mode |= 0x40;
#endif

	/* 
	 * What is the size of the largest available memory block?
	 * allocate about 0.2% to 0.4%  of that for the global buffer 
	 * (or at least 1KB, and no more than 128KB)
	 */

	get_freemem(&stsize, &ttsize);

	global_mem_size = lmax(stsize, ttsize) / 512000L;

	if(global_mem_size > 16)
		global_mem_size *= 2;	/* double allocation size on larger systems */

	/* Always use at least 1KB, but never more than 128KB */

	global_mem_size = lminmax(1L, global_mem_size, 128L) * 1024L;

	/* Note: Mint or Magic can always handle Alt/TT-RAM, I suppose? */

#if _MINT_
	if ( mint || (tos_version >= 0x206))
#else
	if (tos_version >= 0x206)
#endif
		global_memory = Mxalloc(global_mem_size, mode);
	else	
		global_memory = Malloc(global_mem_size);

	return (global_memory) ? 0 : ENSMEM;
}


/* 
 * Handle TeraDesk's main menu 
 * (but not all of it, some items are handled in window.c) 
 */

static void hndlmenu(int title, int item, int kstate)
{
#if _LOGFILE
fprintf(logfile,"\n hndlmenu %i %i %i", title, item, kstate);
#endif

	if ((menu[item].ob_state & DISABLED) == 0)
	{
		hyppage = menu[item].ob_spec.free_string;

		switch (item)
		{
			case MINFO:
			{
				info();
				break;
			}
			case MQUIT:	
			{
				int qbutton;

				bell();
				wait(150);
				bell();
				qbutton = chk_xd_dialog(quitopt, ROOT);

				switch (qbutton)
				{
					case QUITBOOT:		/* reboot */
					{
						reboot = TRUE;
						shutopt = 2;
						shutdown = TRUE;
						goto toshut;
					}
					case QUITSHUT:		/* shutdown */
						shutdown = TRUE;
						if ( app_specstart(AT_SHUT, NULL, NULL, 0, 0) )
						{
							shutdown = FALSE;
							break;
						}
						toshut:;
					case QUITQUIT:		/* quit */      
					{
						quit = TRUE;
					}
					case QUITCANC:		/* cancel */
					{
						break;
					}
				}
				break;
			}				/* MQUIT ? */				
			case MOPTIONS:
			{
				setpreferences();
				break;
			}
			case MPRGOPT:
			{
				prg_setprefs();
				break;
			}
			case MSAVESET:
			{
				save_options(definfname);
				break;
			}
			case MSAVEAS:
			{
				save_options_as();
				break;
			}
			case MAPPLIK:
			{
				app_install(LS_APPL, &applikations);
				break;
			}
			case MCOPTS:
			{
				copyprefs();
				break;
			}
			case MWDOPT:
			{
				dsk_options();
				break;
			}	
			case MVOPTS:
			{
				if ( !app_specstart(AT_VIDE, NULL, NULL, 0, 0) )
				{
					chrez = voptions();

					if ( chrez ) 
						quit = TRUE;
				}

				break;
			}
			default:
			{
				wd_hndlmenu(item, kstate);	/* handle all the rest in window.c */
			}
		}
	}

	menu_tnormal(menu, title, 1);
}


/*
 * Convert keyboard scancodes into format in which
 * keyboard shortcuts are saved ( XD_CTRL | char_ascii_code )
 * this routine should prevent, somewhat better than earlier code,
 * unwanted (erroneous) recognition of weird key combinations
 * which are created by limitations in keyboard scan codes
 * (some key combinations can can not be differed from others)
 */

static int scansh ( int key, int kstate )
{
	int 
		a = touppc(key & 0xFF),
		h = key & 0x80; 		/* upper half of the 255-characters set */


	if( key & XD_SCANCODE)
		return -1;
	else if ((kstate & (K_CTRL | K_ALT)) == K_CTRL) /* Ctrl... or Shift-Ctrl... */
	{
		if ( a == SPACE )					/* ^SP           */
			a |= XD_CTRL;
		else if ( a == 0x1f ) 				/* ^DEL          */
			a |= ( XD_CTRL | 0x60 );
		else if (0x30 <= a && a < 0x40 )	/* ^1 to ^9      */
			a |= ( XD_CTRL | 0x30 );			
		else  if (!h) 						/* ^A ... ^\     */
			a |= ( XD_CTRL | 0x40 );
		else								/* Above 127     */
			a |= XD_CTRL;
	}
	else if ( (kstate > (K_RSHIFT | K_LSHIFT)) || key <= 0 ) 		/* everything but shift or plain */
		a = -1;								/* shortcut def. is never this value */

	return a;
}


/* 
 * Handle keyboard commands 
 */

static void hndlkey(int key, int kstate)
{
	APPLINFO
		*appl;		/* pointer to data for application assigned to F-key */

	int
		i = 0,		/* aux. counter */ 
		k,			/* key code with CTRL masked */
		title; 		/* rsc index of current menu title */

	unsigned int
		uk = (unsigned int)key;

#if _LOGFILE
fprintf(logfile, "\n hndlkey 0x%x 0x%x", key, kstate);
#endif

	/* [Help] key ? */

	if ( uk  == HELP || uk == SHIFT_HELP )
		showhelp(uk);

	if(uk == UNDO)
		wd_deselect_all();

	k = key & ~XD_CTRL; 

	if ((( uk >= 0x803B) && ( uk <= 0x8044)) ||	/* these are function keys codes  */
		(( uk >= 0x8154) && ( uk <= 0x815D)))	/* same for shifted function keys */
	{
		/* Function key ? */

		k &= 0xFF;
		k = (k >= 0x54) ? (k - 0x54 + 11) : (k - 0x3B + 1);

		/* Exec application assigned to a F-key */

		if ((appl = find_fkey(k)) != NULL)
			app_exec(NULL, appl, NULL, NULL, 0, kstate);
	}
	else
	{
		/* Find if this is defined as a menu shortcut */

		k = scansh( key, kstate );

		title = TFIRST;

		while (   (options.kbshort[i] != k) && (i <= ( MLAST - MFIRST)) )
		{
			if ( (options.kbshort[i] & XD_ALT) != 0 ) 
				title++;  

			i++;
		}

		/* Handle keyboard shortcuts or opening of root directories */

		if ( options.kbshort[i] == k )
		{
			/* A keyboard shortcut has been recognized */

			menu_tnormal(menu, title, 0 );
			hndlmenu( title, i + MFIRST, kstate );
		}
		else
		{
			/* A directory window is to be opened */

			if(dir_onalt(key, NULL))
				autoloc_off();
		}
	}		/* uk ? */
}


/*
 * Handle (some) AES messages: All AV-protocol & FONT protocol ones.
 * and also AP_TERM and SH_WDRAW. 
 * It should also handle drag & drop (receive) messages some day
 * Return -1 upon AP_TERM message, return 0 otherwise
 */

static int _hndlmessage(int *message, boolean allow_exit)
{
#if _LOGFILE
fprintf(logfile, "\n _hndlmessage 0x%x %i %i", message[0], message[1], allow_exit);
#endif

	if 
	(   
		( message[0] >= AV_PROTOKOLL && message[0] < VA_HIGH ) || 
		( message[0] >= FONT_CHANGED && message[0] <= FONT_ACK )
	)
		handle_av_protocol(message);
	else
	{
		/* Perhaps it would make sense to support SH_M_SPECIAL as well? */

		switch (message[0])
		{
			case AP_TERM:
			{
#if _LOGFILE
fprintf(logfile,"\n  AP_TERM");
#endif

				if (allow_exit)
				{
					/* This will cause an exit from TeraDesk's main loop */
					quit = TRUE;
				}
				else
				{
					/* Tell the AES that TeraDesk can't terminate just now */
					int ap_tfail[8] = {AP_TFAIL, 0, 0, 0};

					ap_tfail[1] = ap_id; /* ??? */
					shel_write(SHW_AESSEND, 0, 0, (char *) ap_tfail, NULL); /* send message to AES */
				}
				return -1;
			}
			case SH_WDRAW:
			{
				/* Windows will be updated */
				wd_update_drv(message[3]);
				break;

			}
/* currently not used

#if _MINT_
			case AP_DRAGDROP:
			{
				break;
			}
#endif

*/
		}
	}
 
	return 0;
}


/* 
 * Similar to the above, but with one argument only.
 * this is needed for some xdialog routines
 */

int hndlmessage(int *message)
{
	return _hndlmessage(message, FALSE);
}


/* 
 * Main event loop in the program 
 */

static void evntloop(void)
{
	int event;


	/*
	 * Set-up parameters for waiting on an event.
	 * Note: right mouse button has the effect of:
	 * [ left button doubleclick + right button pressed ]
	 * (also convenient for activating programs in a non-selected window)
	 */

	xd_clrevents(&loopevents);
	loopevents.ev_mflags = EVNT_FLAGS;
	loopevents.ev_mbclicks = 0x102;
	loopevents.ev_mbmask = 3;

	while (!quit)
	{
		/* 
		 * In order to properly execute in Single-TOS some items
		 * related to AV-protocol, window- and menu-updates,
		 * the loop has to be executed every once in a while-
		 * like every 500ms defined below. It looks as if this 
		 * is not needed in a multitasking environment.
		 * Do this only if there are open accessory (AV-client) windows.
		 */

		if
		(
/* it IS needed in multitasking, after all- but will it degrade speed?
#if _MINT_
			!mint &&
#endif
*/
			va_accw()
		)
		{
			loopevents.ev_mtlocount = 500;		/* 500ms */
			loopevents.ev_mflags |= MU_TIMER;	/* with timer events */
		}	
		else
		{
			loopevents.ev_mtlocount = 0;
			loopevents.ev_mflags &= ~MU_TIMER;	/* no timer events */
		}

		/*
		 * Enable/disable menu items depending on current context.
		 * Note: in order for this to work with AV-protcol clients
		 * in single-TOS, the loop has to be executed 
		 * every once in a while (like 500ms defined above)
		 */

		itm_set_menu(selection.w);

		/* 
		 * Wait for an event.
		 * Note: as some of the events are handled within xe_xmulti,
		 * after which the appropriate bitflags are reset,
		 * it is quite possible that the result returned by xe_xmulti
		 * be a "nonevent" i.e. with code 0
		 */

		event = xe_xmulti(&loopevents);

#if _LOGFILE
	fprintf(logfile,"\n loopevent 0x%x", event); 
#endif

		/* Process any recieved messages */

		if (event & MU_MESAG)
		{
			if (loopevents.ev_mmgpbuf[0] == MN_SELECTED)
				hndlmenu(loopevents.ev_mmgpbuf[3], loopevents.ev_mmgpbuf[4], loopevents.ev_mmokstate);	
			else
				_hndlmessage(loopevents.ev_mmgpbuf, TRUE);
		}

		/* Process keyboard events */

		if (event & MU_KEYBD)
			hndlkey(loopevents.xd_keycode, loopevents.ev_mmokstate);
	}
}


/*
 * If AP_TERM is received within 2 seconds after starting this routine
 * then set shutting = true and return TRUE. 
 * If another message is received within that time, wait 2 seconds more.
 * (no other action is performed for other messages).
 * Theoretically this may get into an endless loop if some application
 * starts sending messages endlessly
 */

boolean wait_to_quit(void)
{
	int event = 0;

	xd_clrevents(&loopevents);

	loopevents.ev_mflags = MU_TIMER | MU_MESAG;
	loopevents.ev_mtlocount = 2000; /* 2 seconds */

	do
	{
		event = xe_xmulti(&loopevents);

		if((event & MU_MESAG)  && (loopevents.ev_mmgpbuf[0] == AP_TERM))
		{
			shutting = TRUE;
			return TRUE;
		}

		if((event & MU_TIMER))
			return FALSE;
	}
	while(TRUE);
}


/*
 * A couple of routines used to put the computer into a coma.
 * Some endless loops are set.
 * This is probaly not a very bright idea, and, besides, 
 * it is possible that it is not implemented correctly.
 * These routines should only come into effect in single-TOS.
 * In multittasking environments, proper shutdown is used.
 */

void loopcpu(void)
{
	for(;;);
}


void lobo(void)
{
	if(ct60)
	{
		*((char *)0xFA800000L) = 1;				/* turn off CT60 */	
	}
	else
	{
		Setexc(0x070, (void (*)())loopcpu);		/* vert.blank */
		Setexc(0x070, (void (*)())loopcpu);		/* hor. blank */
		Setexc(0x114, (void (*)())loopcpu); 	/* 200 Hz */
	}

	loopcpu();
}


/* 
 * Main TeraDesk routine- the desktop program itself.
 * The main routine contains initialisation and shutdown stuff.
 * It calls evntloop() for all the work that has to be done
 * while the desktop is running. 
 */

int main(void)
{
	int error;	/* return code from diverse routines */

	/*
	 * Get the value of the environment variable TERAENV
	 * which may in the future set various aspects of TeraDesk.
	 * Options should be defined as uppercase letters or
	 * uppercase letters immediately followed by a number.
	 *
	 * Currently used:
	 * D = always draw userdef objects, regardless of AES capabiliites
	 * A = enforce use of ARGV protocol when passing commands to applications
	 */

	if ( (teraenv = getenv("TERAENV")) == NULL )
		teraenv = (char *)empty;

	if ( strchr(teraenv, 'D') )
		xd_own_xobjects(1);

	if( strchr(teraenv, 'A') )
		fargv = TRUE;

	/* 
	 * Find out and set some details depending on the type of OS.
	 * In most cases this is done by searching for certain cookies. 
	 * This is relevant only for the multitasking version.
	 * Ssystem() is available only since Mint 1.15.
	 */

	fsdefext = (char *)TOSDEFAULT_EXT;	/* Set "*.* as a default filename extension */

#if _MINT_

	have_ssystem = (Ssystem(-1, 0L, 0L) == 0);		/* use Ssystem where possible */

	/* 
	 * Attempt to divine from cookies the versions of TOS and AES.
	 * this can NOT detect: MyAES, XaAES and Atari AES 4.1, but is
	 * OK for Mint, Magic, Geneva  and N.AES.
	 */

	mint   = (find_cookie('MiNT') == -1) ? FALSE : TRUE;
	magx   = (find_cookie('MagX') == -1) ? FALSE : TRUE;
	geneva = (find_cookie('Gnva') == -1) ? FALSE : TRUE;
	naes   = (find_cookie('nAES') == -1) ? FALSE : TRUE;

	/* 
	 * In most cases behaviour of this program in Magic should be the same
	 * as in Mint. When Magic-particular action is required, existence of
	 * magx cookie should be checked.
	 */

	mint  |= magx;			/* Quick & dirty */

	if (mint)
	{
		Psigsetmask(0x7FFFE14EL);
		Pdomain(1);
		fsdefext = (char *)DEFAULT_EXT;	/* Set "*" as a default filename extension */
	}
#endif

	/* Find if files can be locked in this version of OS */

	x_init();

	/* 
	 * Register this app with GEM; get its application id.
	 * If this fails, there is no point in continuing
	 */

	if ((ap_id = appl_init()) < 0)
		return -1;

	/* 
	 * Get the version of the TOS and the AES.
	 * aes_version can not be determined earlier than appl_init()
	 */

	tos_version = get_tosversion();
	aes_version = get_aesversion();

	/* 
	 * Try to detect CT60 hardware: it can be switched off
	 * by a direct register access at shutdown
	 */

	ct60 = (find_cookie('CT60') == -1) ? FALSE : TRUE;

	/* 
	 * Load the dekstop.rsc resource file. Another required resource file
	 * (the icons file) is loded in load_icons() below (defined in ICONS.C).
	 * file desktop.rsc is language-dependent as it contains all texts
	 * displayed by Teradesk. Files icons.rsc and cicons.rsc only weakly
	 * language-dependent: the only texts they contain are icon names 
	 */

	if (rsrc_load(RSRCNAME) == 0)
	{
		/* Failed, probably file not found */
		form_alert(1, msg_resnfnd);
	}
	else
	{
		/* The resource file has been loaded. Initialize x-dialogs. */

		error = init_xdialog(&vdi_handle, malloc_chk, free, get_freestring(DWTITLE), 1, &nfonts);

		/* Proceed only if successful */

		if(error < 0)
			xform_error(error);
		else
		{
			/*
			 * Inform AES of TeraDesk's capabilities regarding messages 
			 * (should here be version 0x340, 0x399 or 0x400 ?).
			 * Also, put this appliction into the first menu.
			 */

			if ( aes_version >= 0x399 ) 
			{
				/* Inform AES that AP_TERM is understood ("1"= NM_APTERM) */

				shel_write(SHW_INFRECGN, 1, 0, NULL, NULL); 
				menu_register(ap_id, get_freestring(MENUREG));
			}

			/* Initialize things related to vdi. Set default font */

			init_vdi();

			/* Initialize the resource structure, fix some positions, etc. */

			rsc_init();

			/* Some details about xdialogs (possible words for 'Cancel') */

			xd_cancelstring = get_freestring(CANCTXT);

			/* 
			 * If screen resolution is too low (less than 40x25), can't continue.
			 * AES should supply a reasonable font size for this to work
			 */

			if (((xd_screen.w / xd_fnt_w) < 40) || ((xd_screen.h / xd_fnt_h) < 25))
				alert_abort(MRESTLOW);
			else
			{
				/* 
				 * Proceed only if global memory is allocated OK
				 * This will be used for the AV- and Drag & Drop protocols
				 */

				if ((error = alloc_global_memory()) == 0)
				{
					/* Remove the ARGV variable from the environment. */

					clr_argv();

					/* 
					 * Initialize the names of the configuration file(s).
					 * teradesk.inf = main configuration file
					 * teradesk.pal = colour palette file
					 * Note: in teradesk.inf it is specified whether
					 * teradesk.pal will be used at all.
					 * Proceed only of OK
					 */

					if 
					(
						((palname = strdup("teradesk.pal")) != NULL) &&
						((infname = strdup("teradesk.inf")) != NULL)
					)
					{
						/* Proceed if icons are loaded from icons resource file */

						if ( load_icons() )
						{
							/* Proceed if defaults set, configuration loaded, etc. */

							if ( init() )
							{
								arrow_mouse();	

								/* 
								 * Main event loop of this Desktop. 
								 * All of the work in TeraDesk happens in here
								 */

								evntloop();

								/* 
								 * Start quitting / shutting down 
								 * (remove AV-windows before saving config)
								 */

								va_checkclient();		/* remove nonexistent clients */
								va_delall(-1, TRUE);	/* remove remaining pseudowindows  */

								if ((options.sexit & SAVE_CFG) != 0)	/* save config */
									save_options(definfname);

								wd_del_all();		/* remove all windows        */
								menu_bar(menu, 0);	/* remove menu bar       */
							}

							free_icons();
							regen_desktop(NULL);

							/* This is a cosmetic clearing of the screen at the end */

							if
							( 
								shutdown
#if _MINT_ 
									&& (magx || !mint)
#endif
							)
							{
								xd_mouse_off();
								clr_object(&xd_screen, BLACK, 4);
							} 
						}
					}

					/* Release global memory buffer */

					Mfree(global_memory); 
				}
				else
					xform_error(error);
			}

			/* Unload loaded fonts */

			if (vq_gdos() != 0)
				vst_unload_fonts(vdi_handle, 0);

			/* Close all xdialog related stuff */

			exit_xdialog();
		}

		/* Deallocate resource structures */

		rsrc_free();
	}

#if _LOGFILE
		if(logfile)
		{
			fprintf(logfile,"\n CLOSE LOG FILE \n");
			fclose(logfile);
		}
#endif

	/* 
	 * The following segment handles final system shutdown and resolution change
	 * If a resolution change is required, shutdown is (supposed to be)
	 * performed first (but it did not work as planned... see below).
	 * If only shutdown is required, the system will reset at the end.
	 * Note that if an external application is set to perform shutdown,
	 * all this will -not- be executed: upon receiving AP_TERM, TeraDesk
	 * will just quit.
	 */ 

	if ( chrez || shutdown )	/* If change resolution or shutdown ... */
	{
		if ( chrez )
		{
			/* 
			 * Change the screen resolution if needed;
			 * A call to shel_write() to change resolution
			 * in get_set_video() also closes all applications
			 */
			get_set_video(2); /* contains shel_write(SHW_RESCHNG,...) */
		}
		else
		{
			/* 
			 * Tell all GEM applications which would understand it to end.
			 * This was supposed to be done also before resolution change,
			 * but it was found out that Magic and XaAES do not react to
			 * it properly: Magic just restarts the desktop and XaAES goes
			 * to shutdown. Therefore it is not done now for resolution change.
			 * After shel_write() TeraDesk waits for a termination message
			 * in case an AES decides to complete the shutdown by itself
			 * (AFAIK only MyAES does that). If the message arrives, TeraDesk 
			 * just quits. Otherwise it attempts to shut down the system.
			 *
			 * THERE SEEMS TO BE SOME CONFUSION IN THE DOCS AS TO WHICH
			 * PARAMETER TO shel_write() CONTROLS SHUTDOWN TYPE!
			 */
#if _MINT_
			int ignor = 0;

			quit = shel_write( SHW_SHUTDOWN, 2, 0, (naes) ? (void *)&ignor : NULL, NULL ); 	/* complete shutdown */
#else
			quit = shel_write( SHW_SHUTDOWN, 2, 0, NULL, NULL ); 	/* complete shutdown */
#endif
			wait_to_quit();	/* wait 2 seconds for a termination message */
		}

		wait(500);

		/* 
		 * If a shutdown was initiated but no termination message was received,
		 * and also if this is not a resolution change, TeraDesk will attempt
		 * to kill the system on its own. 
		 */

		if(!shutting && !chrez)		
		{
			long (*rv)();		/* reset vector */

#if _MINT_
			/* shutopt: 0 = halt/poweroff,  1 = reset,  2 = coldreset */ 

			Shutdown((long)shutopt);
#endif			
			wait(5000);

			/* 
			 * If execution of the program comes to this point, it means
			 * that Shutdown() was not known of on the system, or that it
			 * has failed. In any case, now attempt to stop or reset the
			 * system by brute force. This is supposed to happen only 
			 * without mint. Case of Magic is unclear (?).
			 */

			/* Get into supervisor mode; old stack will not be needed again */

			Super(0L);

			if(reboot)
			{
				/* This is supposed to be a cold reset */

				(long)rv = *((long *)os_start + 4);	/* pointer to reset handler */
				memval = 0L;	/* corrupt some variables for a reset */				
				memval2 = 0L;	/* same... */
				resvector = 0L;	/* same... */
				resvalid = 0L;	/* same... */
			}
			else
			{
				/* 
				 * An endless loops routine is used to 'stop' the cpu
				 * in an attempt to completely freeze the computer.
				 * This is admittedly a very stupid way to handle the
				 * problem- it actually lobotomizes the computer.
				 * btw. CT60 is supposed to be powered-off by
				 * a write to $FA800000
				 */

				(long)rv = (long)(lobo);
			}

			/* 
			 * At this moment the machine will either reset, or stop. 
			 * Stopping is currently simulated by an endless loop
			 */
			
			Supexec(rv);
		}
	}
	
	/* Just quit the desktop */		

	appl_exit();
	return 0;
}

/* That's all ! */






/*

/* 
 * This routine displays the incrementation of the 200Hz timer; 
 * it is to be used only for evaluation of the duration of some 
 * operations during development.
 * Call this twice, first with mode=0 (reset timer counter), 
 * then with mode=1 (display timer count)
 */

long show_timer(int mode)
{
	static long t0, t1;
	long old = Super(0);

	t1 = *(long *)(0x4ba);

	if(mode == 0)
	{
		t0 = t1;
		t1 = 0;
	}
	else
	{
		t1 -= t0;				
	}

	Super((void *)old);

	printf("\n T:%ld", t1);

	return t1;
}

*/
