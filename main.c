/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren. 
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
#include <boolean.h>
#include <library.h>
#include <mint.h>
#include <system.h>
#include <xdialog.h>
#include <internal.h>
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

#define EVNT_FLAGS	(MU_MESAG|MU_BUTTON|MU_KEYBD) /* For the main loop */

#define MAX_PLINE 132 	/* maximum printer line length */
#define MIN_PLINE 32	/* minimum printer line length */
#define DEF_PLINE 80	/* defeult printer line length */

extern char 
	*xd_cancelstring;	/* from xdialog; possible 'Cancel' texts */

Options 
	options;		/* configuration options */

int 
	ap_id,			/* application id. of this program (TeraDesk) */
	vdi_handle, 	/* current VDI station handle   */
	max_w,			/* screen width */ 
	max_h,			/* screen height */ 
	nfonts;			/* number of available fonts    */

SCRINFO 
	screen_info;	/* Screen handle, size, font size */ 

FONT 
	def_font;		/* Data for the default (system) font */

/* Names of menu boxes */

static const int 
	menu_items[6] = {MINFO, TDESK, TLFILE, TVIEW, TWINDOW, TOPTIONS};

#if _MINT_
boolean			/* No need to define values, set when starting main() */
	mint, 		/* true if Mint  is present  */
	magx,		/* True if MagiC is present  */	
	naes,		/* TRUE if N.AES is present  */
	geneva;		/* True if Geneva is present */

int 
	have_ssystem = 0;
#endif

static boolean
	chrez = FALSE, 		/* true if resolution sould be changed */
	quit = FALSE,		/* true if teradesk should finish      */ 
	shutdown = FALSE;	/* true if system shutdown is required */

char
	*teraenv,		/* pointer to value of TERAENV environment variable */ 
	*fsdefext,		/* default filename extension in current OS */
	*optname,		/* name of old configuration file (teradesk.cfg) */ 
	*infname,		/* name of V3 configuration file (teradesk.inf)  */ 
	*palname,		/* name of colour palette file (teradesk.pal)    */
	*global_memory,	/* Globally available buffer for passing params */
	*infide = "TeraDesk-inf", /* File identifier header */
	*empty = "\0",		/* often used string */
	*bslash = "\\",		/* often used string */
	*adrive = "A:\\";	/* often used string */

/*
 * Below is supposed to be the only text embedded in the code:
 * information (in several languages) that a resource file 
 * can not be found. It is shown in an alert box.
 */

static char 
	msg_resnfnd[] =  "[1][Unable to find resource file.|"
					 "Resource file niet gevonden.|"
					 "Impossible de trouver le|fichier resource.|"
					 "Resource Datei nicht gefunden.][ OK ]";

int 
	tos_version,	/* detected version of TOS; interpret in hex format */
	aes_version;	/* detected version of AES; interpret in hex format */


int hndlmessage(int *message);
void Shutdown(long mode);

CfgNest				/* Elsewhere defined configuration routines */ 
	opt_config, 
	short_config, 
	dsk_config;

/* Root level of configuration data */

CfgEntry Config_table[] =
{
	{CFG_NEST, 0, "options",	 opt_config	 },
	{CFG_NEST, 0, "shortcuts",	 short_config},
	{CFG_NEST, 0, "deskicons",	 dsk_config	 },
	{CFG_NEST, 0, "filetypes",	 ft_config	 },
	{CFG_NEST, 0, "icontypes",	 icnt_config },
	{CFG_NEST, 0, "apptypes",    prg_config	 },
	{CFG_NEST, 0, "applications",app_config	 },
	{CFG_NEST, 0, "windows",	 wd_config	 },
	{CFG_NEST, 0, "avstats",	 va_config   },
	{CFG_FINAL}, /* file completness check */
	{CFG_LAST}
};


/*
 * Configuration table for desktop options
 * Take care to keep "infv" at the first place.
 * Bit flags are written in hex format for easier 
 * manipulation by humans.
 */

CfgEntry Options_table[] =
{
	{CFG_HDR,0,"options" },
	{CFG_BEG},
	/* file version */
	{CFG_X, 0, "infv", &options.version	    }, 		/* file version */
	/* desktop preferences */
	{CFG_D, 0, "dial", &options.dial_mode	},
	{CFG_D, 0, "save", &options.sexit		},
	{CFG_S, 0, "help", &options.helpprg		},
	/* Copy preferences */
	{CFG_X, 0, "pref", &options.cprefs		}, 		/* bit flags !!! copy prefs */
	/* sizes of diverse items */
	{CFG_D, 0, "buff", &options.bufsize	    }, 		/* copy buffer size */
	{CFG_L, 0, "maxd", &options.max_dir	}, 			/* initial dir size */
	{CFG_D, 0, "plin", &options.plinelen	},		/* printer line length */
	{CFG_D, 0, "tabs", &options.tabsize	    }, 		/* tab size    */
	{CFG_D, 0, "cwin", &options.cwin	    }, 		/* compare match size */
	/* settings of the View menu */
	{CFG_H, 0, "mode", &options.mode		}, 		/* text/icon mode */
	{CFG_H, 0, "aarr", &options.aarr		}, 		/* auto arrange */
	{CFG_H, 0, "sort", &options.sort		}, 		/* sorting key */
	{CFG_X, 0, "attr", &options.attribs	    }, 		/* Bit flags !!! global attributes to show */
	{CFG_X, 0, "flds", &options.fields	}, 			/* Bit flags !!! dir. fields to show  */
	/* video options */
	{CFG_X, 0, "vidp", &options.vprefs	}, 			/* Bit flags ! */
	{CFG_D, 0, "vres", &options.vrez	},			/* video resolution */
	/* patterns and colours */
	{CFG_H,0, "dpat", &options.dsk_pattern  }, 		/* desk pattern */
	{CFG_H,0, "dcol", &options.dsk_color	},		/* desk colour  */
	{CFG_H,0, "wpat", &options.win_pattern  },		/* window pattern */
	{CFG_H,0, "wcol", &options.win_color	},		/* window colour  */

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
	{CFG_HDR,0, "shortcuts" },
	{CFG_BEG},

	/* File menu */
	{CFG_X, 0, "open", &options.kbshort[MOPEN		- MFIRST]	},
	{CFG_X, 0, "show", &options.kbshort[MSHOWINF	- MFIRST]	},
	{CFG_X, 0, "newd", &options.kbshort[MNEWDIR		- MFIRST]	},
	{CFG_X, 0, "comp", &options.kbshort[MCOMPARE	- MFIRST]	},
	{CFG_X, 0, "srch", &options.kbshort[MSEARCH		- MFIRST]	},
	{CFG_X, 0, "prin", &options.kbshort[MPRINT		- MFIRST]	},
	{CFG_X, 0, "dele", &options.kbshort[MDELETE		- MFIRST]	},
	{CFG_X, 0, "clos", &options.kbshort[MCLOSE		- MFIRST]	},
	{CFG_X, 0, "wdup", &options.kbshort[MDUPLIC		- MFIRST]	},
	{CFG_X, 0, "wclo", &options.kbshort[MCLOSEW		- MFIRST]	},
	{CFG_X, 0, "cycl", &options.kbshort[MCYCLE		- MFIRST]	},
	{CFG_X, 0, "sela", &options.kbshort[MSELALL		- MFIRST]	},
#if MFCOPY
	{CFG_X, 0, "copy", &options.kbshort[MFCOPY		- MFIRST]	},
	{CFG_X, 0, "form", &options.kbshort[MFFORMAT	- MFIRST]	},
#else
	{CFG_X, CFG_INHIB, "copy", &inhibit	},
	{CFG_X, CFG_INHIB, "form", &inhibit	},
#endif
	{CFG_X, 0, "quit", &options.kbshort[MQUIT		- MFIRST]	},

	/* View menu */
#ifdef MSHOWTXT
	{CFG_X, 0, "shtx", &options.kbshort[MSHOWTXT	- MFIRST]	},
#endif
	{CFG_X, 0, "shic", &options.kbshort[MSHOWICN	- MFIRST]	},
	{CFG_X, 0, "sarr", &options.kbshort[MAARNG		- MFIRST]	},
	{CFG_X, 0, "snam", &options.kbshort[MSNAME		- MFIRST]	},
	{CFG_X, 0, "sext", &options.kbshort[MSEXT		- MFIRST]	},
	{CFG_X, 0, "sdat", &options.kbshort[MSDATE		- MFIRST]	},
	{CFG_X, 0, "ssiz", &options.kbshort[MSSIZE		- MFIRST]	},
	{CFG_X, 0, "suns", &options.kbshort[MSUNSORT	- MFIRST]	},
	{CFG_X, 0, "revo", &options.kbshort[MREVS		- MFIRST]	},
	{CFG_X, 0, "asiz", &options.kbshort[MSHSIZ		- MFIRST]	},
	{CFG_X, 0, "adat", &options.kbshort[MSHDAT		- MFIRST]	},
	{CFG_X, 0, "atim", &options.kbshort[MSHTIM		- MFIRST]	},
	{CFG_X, 0, "aatt", &options.kbshort[MSHATT		- MFIRST]	},
	{CFG_X, 0, "smsk", &options.kbshort[MSETMASK	- MFIRST]	},

	/* Options menu */
	{CFG_X, 0, "appl", &options.kbshort[MAPPLIK		- MFIRST]	},
	{CFG_X, 0, "ptyp", &options.kbshort[MPRGOPT		- MFIRST]	},
	{CFG_X, 0, "dski", &options.kbshort[MIDSKICN	- MFIRST]	},
	{CFG_X, 0, "wini", &options.kbshort[MIWDICN		- MFIRST]	},
	{CFG_X, 0, "remi", &options.kbshort[MREMICON	- MFIRST]	},
	{CFG_X, 0, "pref", &options.kbshort[MOPTIONS	- MFIRST]	},
	{CFG_X, 0, "copt", &options.kbshort[MCOPTS		- MFIRST]	},
	{CFG_X, 0, "wopt", &options.kbshort[MWDOPT		- MFIRST]	},
	{CFG_X, 0, "vopt", &options.kbshort[MVOPTS		- MFIRST]	},
	{CFG_X, 0, "ldop", &options.kbshort[MLOADOPT	- MFIRST]	},
	{CFG_X, 0, "svop", &options.kbshort[MSAVESET	- MFIRST]	},
	{CFG_X, 0, "svas", &options.kbshort[MSAVEAS		- MFIRST]	},

	{CFG_ENDG},
	{CFG_LAST}
};


/* 
 * Try to llocate some memory and check success.
 * There will generally be some loss in speed, 
 * so use with discretion.
 */

void *malloc_chk(size_t size)
{
	void *address = malloc(size);

	if (address == NULL)
		xform_error(ENSMEM);

	return address;
}


/*
 * Show information on current versions of TeraDesk, TOS, AES...
 * Note: constant part of info for this dialog is handled in resource.c 
 * This involves setting teradesk name and version, etc.
 */

static void info(void)
{
	long stsize, ttsize; 			/* sizes of ST-RAM and TT/ALT-RAM */

	/* 
	 * Inquire about memory size. At least TOS 2.06 is needed in order
	 * to inquire about TT/Alt memory.
	 * Note that MagiC reports as TOS 2.00 only but it is assumed that
	 * Mint and Magic can always handle Alt/TT-RAM
	 */
#if _MINT_
	if ( mint || (tos_version >= 0x206) )
#else
	if ( tos_version >= 0x206 )
#endif
	{
		stsize = (long)Mxalloc( -1L, 0 );	
		ttsize = (long)Mxalloc( -1L, 1 );
	}
	else
	{
		stsize = (long)Malloc( -1L );
		ttsize = 0L;
	}

	/* Display currently available memory */

	rsc_ltoftext(infobox, INFSTMEM, stsize );
	rsc_ltoftext(infobox, INFTTMEM, ttsize ); 

	xd_dialog(infobox, 0);
}


/*
 * Display three consecutive boxes of text on HELP key
 * or, if <Shift><Help> is pressed, try to call ST-Guide.
 * Note 1: currently, there is no notification if call to
 * ST-Guide is not successful.
 * Note 2: according to AV-protocol documentation all filenames
 * must be input in capitals with complete paths (at least the documentation
 * for ST-Guide says so. How does this comply with unixoid filesystems? 
 * Maybe should use lowercase after all?)
 * Note 3: ST-guide supports a special path "*:\" meaning "all known paths" 
 */

static void showhelp (unsigned int key) 		
{
	if ( key & XD_SHIFT )
	{
		onfile = TRUE;
		va_start_prg(options.helpprg, PACC,  "*:\\TERADESK.HYP");
	}
	else
	{
		XDINFO 
			info;

		int 
			i = 0, 
			button;

		static const int 
			hbox[3] = {HLPBOX1, HLPBOX2, HLPBOX3};


		obj_unhide(helpno1[HELPOK]);

		xd_open(helpno1, &info);

		do
		{
			obj_unhide( helpno1[hbox[i]] );

			if ( i == 2 )
				obj_hide(helpno1[HELPOK]);

			xd_drawdeep(&info, ROOT);

			button = xd_form_do(&info, ROOT);
			xd_buttnorm(&info, button); 
			
			obj_hide(helpno1[hbox[i]]);

			if ( button == HELPCANC )
				break;

			i++;
		}
		while(i < 3);

		xd_close(&info);
	}
}


/* 
 * Generalized set_opt to change display of any options button from bitflags
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
	xd_setdialmode(options.dial_mode, hndlmessage, menu, (int) (sizeof(menu_items) / sizeof(int)), menu_items);
}


/* See also desk.h */

/*
 * A routine for displaying a keyboard shortcut in an ASCII readable form; 
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
	int i, j;		/* counters */

	i = 3;			/* position of the leftmost character in the shortcut text */

	switch ( kbshort & 0xFF )
	{
		case 0x08: 				/* Backspace */
			string[i--] = 'S';
			string[i--] = 'B';
			break;
		case 0x09: 				/* Tab    */
			string[i--] = 'B';
			string[i--] = 'A';
			string[i--] = 'T';
			break;
		case 0x20: 				/* Space  */
			string[i--] = 'P';
			string[i--] = 'S';
			break;
		case 0x7F: 				/* Delete */
			string[i--] = 'L';
			string[i--] = 'E';
			string[i--] = 'D';
			break;
		default:				/* Other  */
			if ( kbshort != 0 ) 
				string[i--] =  (char)(kbshort & 0xFF);
			break; 	
	}
	
	if ( kbshort & XD_CTRL )	/* Preceded by ^ ? */
		string[i--] = '^';		
	
	for ( j = i; j >= 0; j-- )	/* fill blanks to the left */
		string[j] = ' ';	 
	
	if ( i >= 0 && left )		/* if needed- left justify, terminate with 0 */
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

	for ( menui = MFIRST; menui <= MLAST; menui++ ) 		/* for each menu item */
	{
		if ( menu[menui].ob_type == G_STRING )		 		/* which is a string... */
		{
			if ( menu[menui].ob_spec.free_string[1] == ' ') /* and not a separator line */
			{
				lm = (int)strlen(menu[menui].ob_spec.free_string); /* includes trailing spaces */
				where = menu[menui].ob_spec.free_string + (long)(lm - 5);
				disp_short( where, options.kbshort[menui - MFIRST], FALSE);
			} 		/* ' ' ? 			*/
		}			/* string ? 		*/
		else
		{
			options.kbshort[menui - MFIRST] = XD_ALT; /* under new title from now on */
		}
	}				/* for... 			*/
}


/* 
 * Check for duplicate or invalid keys in menu shortcuts.
 */

static boolean check_key(int button, int i, unsigned int *tmp)
{
	int  j;

	if ( button != OPTCANC && button != OPTKCLR )
	{
		/* 
		 * This is not very well optimized, it checks tmp[i] many times
		 * repeatedly; a legacy from a previous version,
		 * should perhaps be corrected
		 */

		for ( j = 0; j <= NITEM; j++ ) 
		{
			/* XD_ALT below in order to skip items related to menu boxes */
			if (   ( ( (tmp[i] & XD_SCANCODE) != 0 ) && ( (tmp[i] & 0xFF) < 128) )
		    	|| (   (tmp[i] & ~XD_ALT) != 0 && (i != j) && tmp[i] == tmp[j] ) /* duplicate */
				|| tmp[i] == (XD_CTRL | BACKSPC) /* ^BS illegal, can't differentiate */
				|| tmp[i] == (XD_CTRL | TAB)	 /* ^TAB illegal, can't differentiate */
				|| tmp[i] == SPACE				 /* illegal, because used to scroll viewer window */
				|| tmp[i] == XD_CTRL			 /* ^ only, illegal */
		       )
			{
				alert_printf ( 1, ADUPKEY, setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext );
				return TRUE;
			}
		}
	}
	return FALSE;
}


/*
 * routine arrow_form_do handles some redraws around a xd_form_do
 * which are needed to create effect of a pressed (3d) arrow buton
 * related to some fields in dialogs
 */

int arrow_form_do
(
	XDINFO *treeinfo, 	/* dialog tree info */
	int *oldbutton		/* previously pressed button, 0 if none */
)
{
	OBJECT *tree = treeinfo->tree;
	int button;


	if ( *oldbutton > 0 )
	{
		wait(ARROW_DELAY); 

		if ( (xe_button_state() & 1) == 0 )
		{
			obj_deselect(tree[*oldbutton]); 
			xd_draw ( treeinfo, *oldbutton, 0 ); 
			*oldbutton = 0;
		}	
		else
			return *oldbutton;
	}
	button = xd_form_do(treeinfo, ROOT) & 0x7FFF;

	if ( button != *oldbutton )
	{
		obj_select(tree[button]);
		xd_draw ( treeinfo, button, 0 );
		*oldbutton = button;
	}
	return button;
}


/* 
 * Handle the "Preferences" dialog for setting some 
 * TeraDesk configuration aspects 
 */

static void setpreferences(void)
{
	static int 				/* Static so as to remember position */
		menui = MFIRST;		/* .rsc index of currently displayed menu item */

	int 
		button = OPTMNEXT,	/* current button index */
		oldbutton = -1,		/* aux for arrow_form_do */
		mi,					/* menui - MFIRST */
		redraw = TRUE,		/* true if to redraw menu item and key def */
		lm,					/* length of text field in current menu item */
		lf,					/* length of form for menu item text */
		i;					/* counters */

	unsigned int 
		tmp[NITEM+2];		/* temporary kbd shortcuts (until OK'd) */

	char 
		aux[5],				/* temp. buffer for string manipulation */
		*tabsize = setprefs[TABSIZE].ob_spec.tedinfo->te_ptext;

	XDINFO 
		prefinfo;
 

	/*  Set state of radio buttons and checkbox button(s) */

	xd_set_rbutton(setprefs, OPTPAR2, (options.cprefs & DIALPOS_MODE) ? DMOUSE : DCENTER);
	xd_set_rbutton(setprefs, OPTPAR1, DBUFFERD + options.dial_mode - 1);
	set_opt( setprefs, options.sexit, 1, SEXIT);

	itoa(options.tabsize, tabsize, 10);	

	lf = (int)strlen(setprefs[OPTMTEXT].ob_spec.free_string); /* get length of space for displaying menu items */

	/* Copy shortcuts to temporary storage */

	memcpy( &tmp[0], &options.kbshort[0], (NITEM + 1) * sizeof(int) );

	xd_open(setprefs, &prefinfo);	/* Open dialog; then loop until OK or Cancel */

	/* 
	 * Handle setting of keyboard shortcuts; note: because of limitations in
	 * keyboard scancodes, distinction of some key combinations is impossible
	 */

	while ( button != OPTOK && button != OPTCANC )
	{
		/* Display text of current menu item */

		mi = menui - MFIRST;

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

			/* Display defined shortcut in ASCII form */

			disp_short( setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext, tmp[mi], TRUE );
        
			xd_draw( &prefinfo, OPTMTEXT, 0 );
			xd_draw( &prefinfo, OPTKKEY, 0 );
			redraw = FALSE;
		}

		do 		/* again: */
		{
			button = arrow_form_do ( &prefinfo, &oldbutton ); 

			/* Interpret shortcut from the dialog */

			strip_name( aux, setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext );
			strcpy ( setprefs[OPTKKEY].ob_spec.tedinfo->te_ptext, aux );
			
			i = (int)strlen( aux ); 
			tmp[mi] = 0;

			switch ( i )
			{
				case 0:						/* nothing defined */
					break;
				case 1:						/* single-character shortcut */
					tmp[mi] = aux[0] & 0x00FF;
					break;
				case 2:						/* two-character ^something shortcut */
					if 
					(    
						(aux[0] == '^')  &&
					    ( 
							((aux[1] >= 'A') && (aux[1] <= 'Z')) || 
							( (aux[i] & 80) != 0 )
						)
					)
						tmp[mi] = (aux[1] & 0x00FF) | XD_CTRL;
					else
						tmp[mi] = XD_CTRL;  /* illegal/ignored menu object */
					break;	
				default:					/* longer shortcuts */
					if ( aux[0] == '^' )
					{
						tmp[mi] = XD_CTRL;
						aux[0] = ' ';
						strip_name( aux, aux );
					}
					if ( strcmp( aux, "BS" ) == 0 )
						tmp[mi] |= BACKSPC;
					else if ( strcmp( aux, "TAB" ) == 0 )
						tmp[mi] |= TAB;
					else if ( strcmp( aux, "SP" ) == 0 )
						tmp[mi] |= SPACE;
					else if ( strcmp( aux, "DEL" ) == 0 )
						tmp[mi] |= DELETE;
					else
						tmp[mi] = XD_SCANCODE; /* use this to mark invalid */
					break;
			}
		}
		while (check_key(button, mi, tmp));

		/* 
		 * Only menu items which lay between MFIRST and MLAST are considered;
		 * if menu structure is changed, this interval should be redefined too;
		 * only those menu items with a space in the second char position
		 * are considered; other items are assumed not to be valid menu texts
		 * note: below will crash in the (ridiculous) situation when the
		 * first or the last menu item is not a good text
		 */

		switch ( button )
		{
			case OPTMPREV:
				while ( menui > MFIRST && menu[--menui].ob_type != G_STRING);
				if ( menu[menui].ob_spec.free_string[1] != ' ' ) 
					menui--;
				redraw = TRUE;
				break;
			case OPTMNEXT:
				while ( menui < MLAST && menu[++menui].ob_type != G_STRING);
				if ( menu[menui].ob_spec.free_string[1] != ' ' ) 
					menui++;
				redraw = TRUE;
				break;
			case OPTKCLR:
				memset( &tmp[0], 0, (NITEM + 2) * sizeof(int) );
				redraw = TRUE;
				break;
			default:
				break;
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

		if (xd_get_rbutton(setprefs, OPTPAR2) == DMOUSE)
		{
			options.cprefs |= DIALPOS_MODE;	
			posmode = XD_MOUSE;
		}
		else
			options.cprefs &= ~DIALPOS_MODE;

		options.dial_mode = xd_get_rbutton(setprefs, OPTPAR1) - DBUFFERD + 1;

		get_opt(setprefs, &options.sexit, 1, SEXIT);
 
		if ((options.tabsize = atoi(tabsize)) < 1)
			options.tabsize = 1;

		set_dialmode();
		xd_setposmode(posmode);
	}
}


/* 
 * Handle the dialog for setting options related to 
 * copying and printing 
 */

static void copyprefs(void)
{
	int 
		i, 
		button;

	static const int 
		bitflags[] = {CF_COPY, CF_OVERW, CF_DEL, CF_PRINT, CF_FOLL, CF_SHOWD, P_HEADER};
 

	char *copybuffer = copyoptions[COPYBUF].ob_spec.tedinfo->te_ptext;	

	/* Set states of appropriate options buttons and copy buffer field */

	for ( i = CCOPY; i <= CPPHEAD; i++ )
		set_opt(copyoptions, options.cprefs, bitflags[i - CCOPY], i);

	itoa(options.bufsize, copybuffer, 10);
	itoa(options.plinelen, copyoptions[CPPLINE].ob_spec.tedinfo->te_ptext, 10 ); 

	/* The dialog itself */

	button = xd_dialog(copyoptions, 0);

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
	
	memset( &options, 0, sizeof(options) );
	options.version = CFG_VERSION;
	options.max_dir = 256;
	options.dial_mode = XD_BUFFERED;
	options.bufsize = 512;
	options.tabsize = 8;
	strcpy(options.helpprg, "ST-GUIDE");
#if _PREDEF
	options.cprefs = CF_COPY | CF_DEL | CF_OVERW | CF_PRINT | CF_TOUCH | CF_SHOWD;
	options.fields = WD_SHSIZ | WD_SHDAT | WD_SHTIM | WD_SHATT;    
	options.plinelen = DEF_PLINE; 							
	options.attribs = FA_SUBDIR | FA_SYSTEM;					
#endif
	options.mode = TEXTMODE;
	options.aarr = 1;										    
	options.sort = WD_SORT_NAME;							

	set_dsk_background((xd_ncolors > 2) ? 7 : 4, 3);
	options.win_pattern = 0;
	options.win_color = 0; 
}


/* 
 * Define some default keyboard shortcuts ... 
 * those offered here have become a de-facto standard
 * in other applications. On the other hand, they are different
 * from those which Atari has adopted for the in-built TOS desktop.
 * As for me, I would prefer some other ones, e.g. "?" for "Info..."
 * and "*" for "Select all"
 */

static void short_default(void)
{
	memset( &options.kbshort[MOPEN - MFIRST], 0, (size_t)(MSAVESET - MFIRST + 1) );

#if _PREDEF
	options.kbshort[MOPEN - MFIRST] =    XD_CTRL | 'O';		/* ^O, etc. */
	options.kbshort[MSHOWINF - MFIRST] = XD_CTRL | 'I';
	options.kbshort[MSEARCH - MFIRST] =  XD_CTRL | 'F';
	options.kbshort[MPRINT - MFIRST] =   XD_CTRL | 'P';
	options.kbshort[MSELALL - MFIRST] =  XD_CTRL | 'A';
	options.kbshort[MQUIT - MFIRST] =    XD_CTRL | 'Q';
	options.kbshort[MDELETE - MFIRST] =  XD_CTRL | 0x7f; 	/* ^DEL */
	options.kbshort[MSAVESET - MFIRST] = XD_CTRL | 'S';
	options.kbshort[MCYCLE - MFIRST] =   XD_CTRL | 'W';
#endif

	ins_shorts();
}


/* 
 * Read configuration from the configuration file 
 */

static void load_options(void)
{
	int error = 0;


	noicons = FALSE;			/* so that missing ones be reported */

	error = handle_cfgfile(infname, Config_table, infide, CFG_LOAD); 

	/* If there was an error when loading options, set default values */

	if (error != 0)
	{
		opt_default();		/* options */
		get_set_video(1);	/* set video */
		short_default();	/* kbd shortcuts */
		dsk_default();		/* desktop */
		ft_default();		/* filetypes */
		icnt_default();		/* icontypes */
		prg_default();		/* programtypes */
		app_default();		/* applications */
		wd_default();		/* windows */
		vastat_default();	/* AV status strings */
	}

	/* Set dialogs mode */

	xd_setposmode((options.cprefs & DIALPOS_MODE) ? XD_MOUSE : XD_CENTERED);
	set_dialmode();
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
			/* "Normal" dialog mode is cancelled; this is for compatibility */

			options.dial_mode = max(options.dial_mode, 1);

			/* 
			 * Check some critical values against limits; if not checked and
			 * some illegal value happens to be in the file, these variables
			 * may crash the program or have other ugly consequences
			 */

			if ( options.plinelen < MIN_PLINE ) /* probably not entered in the dialog */
				options.plinelen = DEF_PLINE; 

			if 
			(   options.version < MIN_VERSION 
				|| options.version > CFG_VERSION 
				|| (options.sort & ~WD_REVSORT) > WD_NOSORT
				|| options.plinelen > MAX_PLINE
				|| options.max_dir < 64 
				|| options.max_dir > 2048
				|| options.dial_mode > XD_WINDOW
				|| options.vrez > 7 
			)
				*error = EFRVAL;

			if ( *error >= 0 )
			{

				/* Block possible junk from some fields in config file */

				options.attribs &= 0x0077;
				options.dsk_pattern &= 0x0007;
				options.win_pattern &= 0x0007;

				/* Currently it makes no sense NOT to confirm touch */

				options.cprefs |= CF_TOUCH;
				options.cprefs &= ~(CF_CTIME | CF_CATTR);

				/* If all is OK so far, start setting TeraDesk */

				set_dsk_background(options.dsk_pattern, options.dsk_color);

#if PALETTES
				/* Load palette. Ignore errors */

				if (options.cprefs & SAVE_COLORS)
					handle_colors(CFG_LOAD);
#endif
				/* Set video state but do not change resolution */

				get_set_video(1);
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

static void save_options(void)
{
	hourglass_mouse();

	/* First save the palette (ignore errors) */
#if PALETTES
	if (options.cprefs & SAVE_COLORS)	/* separate file "teradesk.pal" */
		handle_colors(CFG_SAVE);
#endif

	/* Then save the configuration */

	handle_cfgfile( infname, Config_table, infide, CFG_SAVE );
	
	/* Update destination window */

	wd_set_update(WD_UPD_COPIED, infname, NULL);
	wd_do_update();

	arrow_mouse();
}


/* 
 * Save configuration into an explicitely specified config file
 */

static void save_options_as(void)
{
	char *newinfname;

	if ((newinfname = locate(infname, L_SAVECFG)) != NULL)
	{
		free(infname);
		infname = newinfname;
		save_options();
	}
}


/* 
 * Load configuration from an explicitely specified config file 
 */

static void load_settings(void)
{
	char *newinfname;

	if ((newinfname = locate(infname, L_LOADCFG)) != NULL)
	{
		free(infname);
		infname = newinfname;
		load_options();
	}
}


/*
 * Set complete specification for configuration file(s)
 */

boolean find_cfgfiles(char **cfgname, boolean report)
{
	char *fullname;
	int error;

	if ((fullname = xshel_find(*cfgname, &error)) != NULL)
	{
		free(*cfgname);
		*cfgname = fullname;
	}
	else
	{
		if ((error == EFILNF) && ((fullname = x_fullname(*cfgname, &error)) != NULL))
		{
			free(*cfgname);
			*cfgname = fullname;
		}
		if (report && error != 0)
		{
			xform_error(error);
			return FALSE;
		}
	}
	return TRUE;
}


/*
 * Initiation function (reading configuraton, defaults, etc.).
 *
 * Result: TRUE if OK.
 */

static boolean init(void)
{
	xw_getwork(NULL, &screen_info.dsk);

	find_cfgfiles(&infname, FALSE); 
	find_cfgfiles(&palname, FALSE);

	if (!dsk_init())
		return FALSE;

	/* Clear all definable items */

	ft_init();				/* filetype masks */
	icnt_init();			/* filetype icons */
	prg_init();				/* program types  */
	app_init();				/* installed apps */
	va_init();				/* AV-protocol    */
	wd_init();				/* windows        */

	menu_bar(menu, 1);

	x_setpath(bslash);

	/* Load the configuration file */

	load_options();

	/* Start applications which have been defined as autostart */

	app_specstart(AT_AUTO, NULL, NULL, 0, 0);

	return TRUE;
}


/* 
 * Initialize some stuff related to VDI
 * Note: work_out[] used here is local 
 */

static void init_vdi(void)
{
	int dummy, work_out[10];

	/* Note: graf_handle returns screen physical handle, but it is not needed */

	graf_handle(&screen_info.fnt_w, &screen_info.fnt_h, &dummy, &dummy);
	screen_size();
	vqt_attributes(vdi_handle, work_out);
	fnt_setfont(1, (int) (((long) work_out[7] * (long)xd_pix_height * 72L + 12700L) / 25400L), &def_font);
}


/*
 * Allocate a globally readable buffer for data exchange
 */

static int alloc_global_memory(void)
{
	/* 
	 * 0x40: globally readable
	 * 0x02: Alt/TT-RAM or ST-RAM, prefer ST-RAM
	 * 0x03: Alt/TT-RAM or ST-RAM, prefer Alt/TT-RAM
	 * Maybe better to use ST-RAM, if some other programs have
	 * problems knowing of Alt-RAM ???
	 */

	int mode = 
#if _MINT_
		( mint ) ? 0x43 : 
#endif
		0x03;

	/* Note: Mint or Magic can always handle Alt/TT-RAM */

#if _MINT_
	if ( mint || (tos_version >= 0x206) ) /* Was magic instead of Mint here */
#else
	if ( tos_version >= 0x206 )
#endif
		global_memory = Mxalloc(GLOBAL_MEM_SIZE, mode);
	else	
		global_memory = Malloc(GLOBAL_MEM_SIZE);

	return (global_memory) ? 0 : ENSMEM;
}


/* 
 * Handle TeraDesk's main menu 
 * (but not all of it, some items are handled in window.c) 
 */

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
		{
			int qbutton;

			bell();
			wait(150);
			bell();
			qbutton = alert_printf(3, ALRTQUIT);
			switch (qbutton)
			{
				case 3:		/* cancel */
					break;
				case 2:		/* shutdown */
					if ( app_specstart(AT_SHUT, NULL, NULL, 0, 0) )
						break;
					else
						shutdown=TRUE;
				case 1:		/* quit */      
					quit = TRUE;
					break;
			}
			break;
		}
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
			app_install(LS_APPL);
			break;
		case MIDSKICN:
			dsk_chngicon();
			break;
		case MIWDICN:
			icnt_settypes();
			break;
		case MREMICON:
			dsk_remicon();
			break;
		case MCOPTS:
			copyprefs();
			break;
		case MWDOPT:
			dsk_options();
			break;	
		case MVOPTS:
			if ( !app_specstart(AT_VIDE, NULL, NULL, 0, 0) )
			{
				chrez = voptions();
				if ( chrez ) 
					quit = TRUE;
			}
			break;
		default:
			wd_hndlmenu(item, kstate);	/* handle all the rest in window.c */
			break;
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

int scansh ( int key, int kstate )
{
	int 
		a = key & 0xff,
		h = a & 0x80;		/* foreign characters */


	if ( (a <= 'z' && a >= 'a')  || h )		/* lowercase or foreign */
		a &= 0xdf; 							/* to uppercase  */
	if ( kstate == 4 ) 						/* ctrl          */	
		if ( a == SPACE || a == ESCAPE )	/* ^SP ^ESC      */
			a |= XD_CTRL;
		else if ( a == 0x1f ) 				/* ^DEL          */
			a |= ( XD_CTRL | 0x60 );
		else  if (!h) 						/* ^A ... ^\     */
			a |= ( XD_CTRL | 0x40 );

		else
			a |= (XD_CTRL | (a & 0xdf) );	/* ^foreign */

	else if ( kstate > 2 || key < 0 ) 		/* everything but shift or plain */
		a = -1;								/* shortcut def. is never this value */

	return a;
}


/* 
 * Handle keyboard commands 
 */

static void hndlkey(int key, int kstate)
{
	int i = 0, k;
	unsigned int uk = (unsigned int)key;
	APPLINFO *appl;
	int title; 				/* rsc index of current menu title */

	if ( uk  == HELP || uk == SHIFT_HELP )
		showhelp(uk);

	k = key & ~XD_CTRL; 

	if ((( uk >= 0x803B) && ( uk <= 0x8044)) ||
		(( uk >= 0x8154) && ( uk <= 0x815D)))
	{
		k &= 0xFF;
		k = (k >= 0x54) ? (k - 0x54 + 11) : (k - 0x3B + 1);

		/* Exec application assigned to a F-key */

		if ((appl = find_fkey(k)) != NULL)
			app_exec(NULL, appl, NULL, NULL, 0, kstate);
	}
	else
	{
		k = scansh( key, kstate );

		/* Find if this is defined as a menu shortcut */

		title = TFIRST;
		while (   (options.kbshort[i] != k) && (i <= ( MLAST - MFIRST)) )
		{
			if ( (options.kbshort[i] & XD_ALT) != 0 ) title++;  
			i++;
		}

		/* Handle various user-defined keyboard shortcuts */

		if ( options.kbshort[i] == k )
		{
			menu_tnormal(menu, title, 0 );
			hndlmenu( title, i + MFIRST, kstate );
		}
		else
		{
			i = 0;
			if ((key >= ALT_A) && (key <= ALT_Z))
			{
				/* Handle keys which open windows on drives (Alt-A to Alt-Z) */

				i = key - (XD_ALT | 'A');
				if (check_drive(i))
				{
					char *path;

					if ((path = strdup(adrive)) != NULL)
					{
						path[0] = (char) i + 'A';
						dir_add_dwindow(path);
					}
				}
			}
		}
	}
}


/*
 * Handle AES messages
 */

int _hndlmessage(int *message, boolean allow_exit)
{
	if (   (   message[0] >= AV_PROTOKOLL
	        && message[0] <= VA_HIGH
	       )
	    || (   message[0] >= FONT_CHANGED
	        && message[0] <= FONT_ACK
	       )
	   )
		handle_av_protocol(message);
	else
	{
		/* Perhaps it would make sense to support SH_M_SPECIAL as well? */

		switch (message[0])
		{
			case AP_TERM:
				if (allow_exit)
				{
					quit = TRUE;
				}
				else
				{
					int ap_tfail[8] = { AP_TFAIL, 0 };
					shel_write(SHW_AESSEND, 0, 0, (char *) ap_tfail, NULL); /* send message to AES */
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


/* 
 * Main event loop in the program 
 */

extern int xe_wmess;

static void evntloop(void)
{
	int event;
	XDEVENT events;

	events.ev_mflags = EVNT_FLAGS;

	/*
	 * Set-up parameters for waiting on anevent.
	 * Note: right mouse button has the effect of:
	 * left doubleclick + right button pressed
	 * (also convenient for activating programs in a non-selected window)
	 */

	events.ev_mbclicks = 0x102;
	events.ev_mbmask = 3;
	events.ev_mbstate = 0;

	events.ev_mm1flags = 0;
	events.ev_mm2flags = 0;

	/* events.ev_mtlocount = 0; see below */
	events.ev_mthicount = 0;

	while (!quit)
	{
		/* 
		 * In order to properly execute in Single-TOS some items
		 * related to AV-protocol, window- and menu-updates,
		 * the loop has to executed every once in a while-
		 * like every 500ms defined below. It looks as if this 
		 * is not needed in a multitasking environment.
		 * This is needed only if there are open accessory windows.
		 */

		if
		(
#if _MINT_
			!mint &&
#endif
			va_accw()
		)
		{
			events.ev_mtlocount = 500;
			events.ev_mflags |= MU_TIMER;
		}	
		else
		{
			events.ev_mtlocount = 0;
			events.ev_mflags &= ~MU_TIMER;
		}

		/*
		 * Enable/disable menu items depending on current context.
		 * Note: in order for this to work with AV-protcol clients
		 * in single-TOS, the loop has to be executed 
		 * every once in a while (like 500ms defined above)
		 */

		itm_set_menu(selection.w);

		/* Wait for an event */

		event = xe_xmulti(&events);

		/*		
		 * HR 151102: This imposed a unsolved problem with N.Aes 1.2 	
		 * (lockup of TeraDesk after live moving) 	
		 * It is not a essential function. 
		 */

		clr_key_buf();		

		/* Process messages received */

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


/* 
 * Main TeraDesk routine- the program itself 
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
	 */

	if ( (teraenv=getenv("TERAENV")) == NULL )
		teraenv = empty;

	if ( strchr(teraenv,'D') )
		xd_own_xobjects(1);

	/* 
	 * Find out and set some details depending on the type of OS.
	 * In most cases this is done by searching for certain cookies. 
	 * This is relevant only for the multitasking version.
	 * Ssystem() is available only since Mint 1.15.
	 */

	fsdefext = (char *)TOSDEFAULT_EXT;	/* Set "*.* as a default filename extension */

#if _MINT_

	have_ssystem = (Ssystem(-1, 0L, 0L) == 0);		/* use Ssystem where possible */

	mint   = (find_cookie('MiNT') == -1) ? FALSE : TRUE;
	magx   = (find_cookie('MagX') == -1) ? FALSE : TRUE;
	geneva = (find_cookie('Gnva') == -1) ? FALSE : TRUE;
	naes   = (find_cookie('nAES') == -1) ? FALSE : TRUE;

	/* In most cases behaviour in Magc should be the same as in Mint */

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

	/* Register this app with GEM; get its application id */

	if ((ap_id = appl_init()) < 0)
		return -1;

	/* 
	 * Get the version of the TOS and the AES.
	 * aes_version can not be determined earlier than appl_init()
	 */

	tos_version = get_tosversion();
	aes_version = _GemParBlk.glob.version;

	/* Load the dekstop.rsc resource file */

	if (rsrc_load(RSRCNAME) == 0)
		/* Failed, probably file not found */
		form_alert(1, msg_resnfnd);
	else
	{
		/* 
		 * The resource file has been loaded.
		 * Inform AES of TeraDesk's capabilities regarding messages 
		 * (should it be version 0x340, 0x399 or 0x400 here ?)
		 */

		if ( aes_version >= 0x399 ) 
		{
			/* Inform AES that AP_TERM is understood ("1"= NM_APTERM) */

			shel_write(SHW_INFRECGN, 1, 0, NULL, NULL); 
			menu_register(ap_id, get_freestring(MENUREG));
		}

		/* Initialize x-dialogs */

		if ((error = init_xdialog(&vdi_handle, malloc, free,
								  get_freestring(DWTITLE), 1, &nfonts)) < 0)
			xform_error(error);
		else
		{
			/* Some details about xdialogs (possible words for 'Cancel') */

			xd_cancelstring = get_freestring(CANCTXT);

			/* Initialize things related to vdi */

			init_vdi();

			/* Initialize the resource structure, fix some positions, etc. */

			rsc_init();

			/* If screen resolution is too low (less than 40x25), can't continue */

			if (((max_w / screen_info.fnt_w) < 40) || ((max_h / screen_info.fnt_h) < 25))
				alert_abort(MRESTLOW);
			else
			{
				/* Proceed only if global memory is allocated OK */

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
					 *
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
							/* Proceed if deafults set, configuration loaded, etc. */

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
								 * (remove AV-windows before save)
								 */

								va_delall(-1);		/* remove all pseudowindows  */

								if (options.sexit)	/* save config */
									save_options();

								wd_del_all();		/* remove windows        */
								menu_bar(menu, 0);	/* remove menu bar       */
								xw_close_desk();	/* remove desktop window */
							}
							free_icons();
							regen_desktop(NULL);
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

	/* 
	 * The following segment handles final system shutdown and resolution change
	 * If a resolution change is required, shutdown is performed first.
	 * If only shutdown is required, the system will reset at the end.
	 */ 

	if ( chrez || shutdown )	/* If change resolution or shutdown ... */
	{
		/* 
		 * Tell all GEM applications which would understand it to end.
		 * This was supposed to be done also before resolution change,
		 * but it was found out that Magic and XaAES do not react to
		 * it properly: magic just restarts the desktop and XaAES goes
		 * to shutdown.
		 */

		if (!chrez)
		{
			int ignor;
			quit = shel_write ( SHW_SHUTDOWN, 2, 0, (void *)&ignor, NULL ); 	/* complete shutdown */
			wait(5000);						/* Wait a bit? */
		}

#if 0
		/* 
		 * In Mint/Magic, must tell all proceseses to terminate nicely ?
		 * Probably not needed in a GEM environment anyway, so disabled
		 */

		Pkill(0, SIGTERM); 
		wait(3000); /* Wait a bit? */
#endif
		/* 
		 * After all applications have hopefully been closed,
		 * change the screen resolution if needed;
		 * else- reset the computer.
		 * It seems that a call to shel-write to change resolution
		 * in get_set_video also closes all applications?
		 */

		if ( chrez )
			get_set_video(2);

		else

/* DjV: But it won't really shutdown in Magic without a reset, just restarts the desktop ?
	#if _MINT_
		if (!mint)			/* HR 230203: Dont reset under MiNT or MagiC !!!!! */
	#endif
*/
		{
			/*
			 * Perform a reset here 
			 * Note: maybe use Ssystem here as well, when appropriate?
			 */

			long (*rv)();				/* reset vector */

#if _MINT_
			if ( mint && !magx ) /* has no effect in Magic?? */
			{
				Shutdown(2L);	/* restart */ 
				wait(3000);
			}
#endif
			Super ( 0L );	/* Get into supervisor; old stack won't be needed again */
			memval = 0L;	/* Spoil some variables to cause a reset */ 				
			memval2 = 0L;
			resvector = 0L;
			resvalid = 0L;
			(long)rv = *((long *)os_start + 4);	/* routine that handles the reset */
			Supexec(rv);				/* execute it */

		}
	}
	else	/* Just Quit the desktop */		
		appl_exit();
	
	return 0;
}

/* That's all ! */



