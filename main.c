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
#include "edit.h"
#include "va.h"
#include "video.h"

#define RSRCNAME	"desktop.rsc"

#define EVNT_FLAGS	(MU_MESAG|MU_BUTTON|MU_KEYBD)

Options options;
V3_options v3_options;



CfgNest opt_config, short_config, dsk_config;

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
	{CFG_FINAL}, /* file completness check */
	{CFG_LAST}
};

#if !TEXT_CFG_IN
static int dial_mode;

#endif

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
	/* Copy preferences */
	{CFG_X, 0, "pref", &options.cprefs		}, 		/* bit flags !!! copy prefs */
	/* settings of the View menu */
	{CFG_H, 0, "mode", &options.mode		}, 		/* text/icon mode */
	{CFG_H, 0, "sort", &options.sort		}, 		/* sorting key */
	{CFG_X, 0, "attr", &options.attribs	    }, 		/* Bit flags !!! global attributes to show */
	{CFG_X, 0, "flds", &options.V2_2.fields	}, 		/* Bit flags !!! dir. fields to show  */
	/* sizes of diverse items */
	{CFG_D, 0, "plin", &options.V2_2.plinelen	},	/* printer line length */
	{CFG_D, 0, "tabs", &options.tabsize	    }, 		/* tab size    */
	{CFG_D, 0, "buff", &options.bufsize	    }, 		/* copy buffer size */
	{CFG_L, 0, "maxd",&v3_options.max_dir	}, 		/* initial dir size */
	/* desktop preferences */
#if TEXT_CFG_IN
	{CFG_D, 0, "dial", &options.dial_mode	},
#else
	{CFG_D, 0, "dial", &dial_mode			},		/* Cant take the address of a bitfield */
#endif
	/* video options */
	{CFG_X, 0, "vidp", &options.V2_2.vprefs	}, 		/* Bit flags ! */
	{CFG_D, 0, "vres", &options.V2_2.vrez		},	/* video resolution */
	/* patterns and colours */
	{CFG_H,0, "dpat", &options.dsk_pattern }, 		/* desk pattern */
	{CFG_H,0, "dcol", &options.dsk_color	},		/* desk colour  */
	{CFG_H,0, "wpat", &options.V2_2.win_pattern},	/* window pattern */
	{CFG_H,0, "wcol", &options.V2_2.win_color	},	/* window colour  */

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
	{CFG_X, 0, "open", &options.V2_2.kbshort[MOPEN		- MFIRST]	},
	{CFG_X, 0, "show", &options.V2_2.kbshort[MSHOWINF	- MFIRST]	},
	{CFG_X, 0, "newd", &options.V2_2.kbshort[MNEWDIR	- MFIRST]	},
	{CFG_X, 0, "comp", &options.V2_2.kbshort[MCOMPARE	- MFIRST]	},
	{CFG_X, 0, "srch", &options.V2_2.kbshort[MSEARCH	- MFIRST]	},
	{CFG_X, 0, "prin", &options.V2_2.kbshort[MPRINT		- MFIRST]	},
	{CFG_X, 0, "dele", &options.V2_2.kbshort[MDELETE	- MFIRST]	},
	{CFG_X, 0, "clos", &options.V2_2.kbshort[MCLOSE		- MFIRST]	},
	{CFG_X, 0, "wclo", &options.V2_2.kbshort[MCLOSEW	- MFIRST]	},
	{CFG_X, 0, "cycl", &options.V2_2.kbshort[MCYCLE		- MFIRST]	},
	{CFG_X, 0, "sela", &options.V2_2.kbshort[MSELALL	- MFIRST]	},
#if MFCOPY
	{CFG_X, 0, "copy", &options.V2_2.kbshort[MFCOPY		- MFIRST]	},
	{CFG_X, 0, "form", &options.V2_2.kbshort[MFFORMAT	- MFIRST]	},
#else
	{CFG_X, CFG_INHIB, "copy", &inhibit	},
	{CFG_X, CFG_INHIB, "form", &inhibit	},
#endif
	{CFG_X, 0, "quit", &options.V2_2.kbshort[MQUIT		- MFIRST]	},

	/* View menu */
	{CFG_X, 0, "shtx", &options.V2_2.kbshort[MSHOWTXT	- MFIRST]	},
	{CFG_X, 0, "shic", &options.V2_2.kbshort[MSHOWICN	- MFIRST]	},
	{CFG_X, 0, "snam", &options.V2_2.kbshort[MSNAME		- MFIRST]	},
	{CFG_X, 0, "sext", &options.V2_2.kbshort[MSEXT		- MFIRST]	},
	{CFG_X, 0, "sdat", &options.V2_2.kbshort[MSDATE		- MFIRST]	},
	{CFG_X, 0, "ssiz", &options.V2_2.kbshort[MSSIZE		- MFIRST]	},
	{CFG_X, 0, "suns", &options.V2_2.kbshort[MSUNSORT	- MFIRST]	},
	{CFG_X, 0, "asiz", &options.V2_2.kbshort[MSHSIZ		- MFIRST]	},
	{CFG_X, 0, "adat", &options.V2_2.kbshort[MSHDAT		- MFIRST]	},
	{CFG_X, 0, "atim", &options.V2_2.kbshort[MSHTIM		- MFIRST]	},
	{CFG_X, 0, "aatt", &options.V2_2.kbshort[MSHATT		- MFIRST]	},
	{CFG_X, 0, "smsk", &options.V2_2.kbshort[MSETMASK	- MFIRST]	},

	/* Options menu */
	{CFG_X, 0, "appl", &options.V2_2.kbshort[MAPPLIK	- MFIRST]	},
	{CFG_X, 0, "ptyp", &options.V2_2.kbshort[MPRGOPT	- MFIRST]	},
	{CFG_X, 0, "dski", &options.V2_2.kbshort[MIDSKICN	- MFIRST]	},
	{CFG_X, 0, "wini", &options.V2_2.kbshort[MIWDICN	- MFIRST]	},
	{CFG_X, 0, "remi", &options.V2_2.kbshort[MREMICON	- MFIRST]	},
	{CFG_X, 0, "pref", &options.V2_2.kbshort[MOPTIONS	- MFIRST]	},
	{CFG_X, 0, "copt", &options.V2_2.kbshort[MCOPTS		- MFIRST]	},
	{CFG_X, 0, "wopt", &options.V2_2.kbshort[MWDOPT		- MFIRST]	},
	{CFG_X, 0, "vopt", &options.V2_2.kbshort[MVOPTS		- MFIRST]	},
	{CFG_X, 0, "ldop", &options.V2_2.kbshort[MLOADOPT	- MFIRST]	},
	{CFG_X, 0, "svop", &options.V2_2.kbshort[MSAVESET	- MFIRST]	},
	{CFG_X, 0, "svas", &options.V2_2.kbshort[MSAVEAS	- MFIRST]	},

	{CFG_ENDG},
	{CFG_LAST}
};



int 
	ap_id,		/* application id. of this program (TeraDesk) */
	vdi_handle, /* current VDI station handle   */
	ncolors,	/* number of available colours  */ 
	npatterns,	/* number of available patterns */ 
	max_w,		/* screen width */ 
	max_h,		/* scree height */ 
	nfonts;		/* number of available fonts    */

SCRINFO screen_info;
FONT def_font;

static int 
	menu_items[5] = {MINFO, TDESK, TLFILE, TVIEW, TOPTIONS};

#if _MINT_
boolean
	mint = FALSE, 		/* true if Mint  is present  */
	magx = FALSE,		/* True if MagiC is present  */	
	geneva = FALSE;		/* True if Geneva is present */

int 
	have_ssystem = 0;
#endif

boolean
	chrez = FALSE, 		/* true if resolution sould be changed */
	quit = FALSE,		/* true if teradesk should finish      */ 
	shutdown = FALSE;	/* true if system shutdown is required */

char 
	*optname,	/* name of old configuration file (teradesk.cfg) */ 
	*infname,	/* name of V3 configuration file (teradesk.inf)  */ 
	*palname,	/* name of colour palette file (teradesk.pal)    */
	*global_memory,
	*infide = "TeraDesk-inf"; /* File identifier header */


/*
 * Below is supposed to be the only text embedded in the code:
 * information (in several languages) that a resource file 
 * can not be found. It is shown in an alert box.
 */

char msg_resnfnd[] = "[1][Unable to find resource file.|"
					 "Resource file niet gevonden.|"
					 "Impossible de trouver le|fichier resource.|"
					 "Resource Datei nicht gefunden.][ OK ]";


int 
	tos_version,	/* detected version of TOS; interpret in hex format */
	aes_version;	/* detected version of AES; interpret in hex format */

int hndlmessage(int *message);



/*
 * Execute teradesk.bat file, if there is any
 *
 * Result : FALSE if there is no error,
 * TRUE if there is an error (a very intuitive concept!).
 */

static boolean exec_deskbat(void)
{
	char *batname;
	int error;

	/* Remove the ARGV variable from the environment. */

	clr_argv();

	/* 
	 * Initialize the names of the configuration file(s) 
	 * teradesk.inf = main configuration file
	 * teradesk.pal = colour palette file
	 * Note: in teradesk.inf it is specified whether teradesk.pal
	 * will be used at all.
	 */

	if ((palname = strdup("teradesk.pal")) == NULL)
		error = ENSMEM;
	else
		if ((infname = strdup("teradesk.inf")) == NULL)
			error = ENSMEM;
		else
#if !TEXT_CFG_IN
			if ((optname = strdup("desktop.cfg")) == NULL)
				error = ENSMEM;
			else
#endif
			{
				/* Find teradesk.bat file. */

#if _BATFILE
				if ((batname = xshel_find("teradesk.bat", &error)) != NULL)
				{
					/* Execute it. */

					exec_bat(batname);
					free(batname);
				}
#endif
			}


	if ((error != 0) && (error != EFILNF))
	{
		xform_error(error);
		return TRUE;
	}
	else
		return FALSE;
}


/*
 * Show information on current versions of teradesk, TOS, AES...
 */

static void info(void)
{
	/* 
	 * Note: constant part of info for this dialog is handled in resource.c 
	 * This involves setting teradesk name and version, etc.
	 */

	long stsize, ttsize; 			/* sizes of ST-RAM and TT/ALT-RAM */

	/* 
	 * Inquire about memory size. At least TOS 2.06 is needed in order
	 * to inquire about TT/Alt memory.
	 * Note that MagiC reports as TOS 2.00 only; thefefore it is 
	 * explicitely tested for.
	 */
#if _MINT_
	if ( magx || (tos_version >= 0x206) )
#else
	if ( tos_version >=0x206 )
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
 * Display two consecutive help dialogs on HELP key
 * or, if <Shift><Help> is pressed, try to call ST-Guide.
 * Note: currently, there is no notification if call to
 * st-guide is not successful.  
 */

static void showhelp (unsigned int key) 		
{
	int button;

	if ( key & XD_SHIFT )
		button = va_start_prg("\\ST-GUIDE", "*:\\TERADESK.HYP");
	else
	{
		button = xd_dialog( helpno1, 0 );
		if ( button == HELP1OK ) 
			xd_dialog( helpno2, 0 );
	}
}


/*
 * Convert a number (range 0:99 only) into string
 * (leading zeros shown)
 */

void digit(char *s, int x)
{
	x = x % 100;
	s[0] = x / 10 + '0';
	s[1] = x % 10 + '0';
}


/* 
 * Generalized set_opt to change display of any options button from bit flags, 
 * not only those related to cprefs.
 * In fact it can set option buttons from boolean variables as well
 * (then set "opt" to 1)
 */

void set_opt( OBJECT *tree, int flags, int opt, int button)
{
	if ( flags & opt )
		tree[button].ob_state |= SELECTED;
	else
		tree[button].ob_state &= ~SELECTED;		
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
	int i,j;		/* counters */

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
			if ( kbshort ) 
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

				disp_short( where, options.V2_2.kbshort[menui - MFIRST], FALSE);

			} 		/* ' ' ? 			*/
		}			/* string ? 		*/
		else
		{
			options.V2_2.kbshort[menui - MFIRST] = XD_ALT; /* under new title from now on */
		}
	}				/* for... 			*/
}


/* 
 * Check for duplicate or invalid keys in menu shortcuts.
 */

static boolean check_key(int button, int i, int *tmp)
{
	int  j;

	if ( button != OPTCANC && button != OPTKCLR )
	{
		/* 
		 * This is not very well optimized, it checks tmp[i] many times
		 * repeatedly; a legacy from a previous version,
		 * will perhaps be corrected
		 */

		for ( j = 0; j <= NITEM; j++ ) 
		{
			/* XD_ALT below in order to skip items related to menu boxes */
			if (   ( (tmp[i] & XD_SCANCODE) != 0 )
		    	|| (   (tmp[i] & ~XD_ALT) != 0 && (i != j) && tmp[i] == tmp[j] ) /* duplicate */
				|| tmp[i] == (XD_CTRL | BACKSPC) /* ^BS illegal */
				|| tmp[i] == (XD_CTRL | TAB)	 /* ^TAB illegal */
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
	OBJECT *tree;
	int button;

	tree = treeinfo->tree;

	if ( *oldbutton > 0 )
	{
		evnt_timer(100, 0); /* delay can be adjusted for comfortable feel */		
		if ( (xe_button_state() & 1) == 0 )
		{
			tree[*oldbutton].ob_state &= ~SELECTED; 
			xd_draw ( treeinfo, *oldbutton, 0 ); 
			*oldbutton = 0;
		}	
		else
			return *oldbutton;
	}
	button = xd_form_do(treeinfo, ROOT) & 0x7FFF;

	if ( button != *oldbutton )
	{
		tree[button].ob_state |= SELECTED;
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
	int button;
	int oldbutton;			/* aux for arrow_form_do */

	static /* DjV 075 why static ??? */ XDINFO prefinfo; 
	static int menui = MFIRST;/* .rsc index of currently displayed menu item */
	int mi;					/* menui - MFIRST */
	int redraw;				/* true if to redraw menu item and key def */
	int lm;					/* length of text field in current menu item */
	int lf;					/* length of form for menu item text */
	int i;				 	/* counters */
	int tmp[NITEM+2];		/* temporary kbd shortcuts (until OK'd) */
	char aux[5];			/* temp. buffer for string manipulation */
	char *tabsize = setprefs[TABSIZE].ob_spec.tedinfo->te_ptext;

	/* Set state of dialog mode radio buttons */

	xd_set_rbutton(setprefs, OPTPAR2, (options.cprefs & DIALPOS_MODE) ? DMOUSE : DCENTER);
	xd_set_rbutton(setprefs, OPTPAR1, DNORMAL + options.dial_mode);

	itoa(options.tabsize, tabsize, 10);	

	lf = (int)strlen(setprefs[OPTMTEXT].ob_spec.free_string); /* get length of space for displaying menu items */

	/* Copy shortcuts to temporry storage */

	memcpy( &tmp[0], &options.V2_2.kbshort[0], (NITEM + 1) * sizeof(int) );

	xd_open(setprefs, &prefinfo);	/* Open dialog; then loop until OK or Cancel */

	/* 
	 * Handle setting of keyboard shortcuts; note: because of limitations in
	 * keyboard scancodes, distinction of some key combintions is impossible
	 */

	redraw = TRUE;
	button = OPTMNEXT;  /* anything */
	oldbutton = -1;

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
        
			xd_draw ( &prefinfo, OPTMTEXT, 0 );
			xd_draw ( &prefinfo, OPTKKEY, 0 );
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
					tmp[mi] = (int)aux[0];
					break;
				case 2:						/* two-character ^something shortcut */
					if (    aux[0] == '^' 
					     && aux[1] >= 'A'
						 && aux[1] <= 'Z'
					   )
						tmp[mi] =  (int)aux[1] | XD_CTRL;
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

	xd_close(&prefinfo);

	if (button == OPTOK)
	{
		int posmode = XD_CENTERED;

		/* 
		 * Move kbd. shortcuts into permanent storage,
		 * then insert them into menu texts
		 */

		memcpy( &options.V2_2.kbshort[0], &tmp[0], (NITEM + 1) * sizeof(int) );

		ins_shorts();

		/* Get and then set dialog display mode */

		if (xd_get_rbutton(setprefs, OPTPAR2) == DMOUSE)
		{
			options.cprefs |= DIALPOS_MODE;	
			posmode = XD_MOUSE;
		}
		else
			options.cprefs &= ~DIALPOS_MODE;

		options.dial_mode = xd_get_rbutton(setprefs, OPTPAR1) - DNORMAL;
 
		if ((options.tabsize = atoi(tabsize)) < 1)
			options.tabsize = 1;

		set_dialmode();
		xd_setposmode(posmode);
	}

	xd_change( &prefinfo, button, NORMAL, FALSE );
}


/* 
 * Handle the dialog for setting options related to 
 * copying and printing 
 */

static void copyprefs(void)
{
	int button;
	char *copybuffer = copyoptions[COPYBUF].ob_spec.tedinfo->te_ptext;	

	/* Set states of appropriate options buttons and copy buffer field */

	set_opt(copyoptions, options.cprefs, CF_COPY, CCOPY);
	set_opt(copyoptions, options.cprefs, CF_DEL, CDEL);
	set_opt(copyoptions, options.cprefs, CF_OVERW, COVERW);
	set_opt(copyoptions, options.cprefs, CF_PRINT, CPRINT); 

	set_opt(copyoptions, options.cprefs, CF_SHOWD, CSHOWD); 
	set_opt(copyoptions, options.cprefs, P_HEADER, CPPHEAD);

	itoa(options.bufsize, copybuffer, 10);
	itoa(options.V2_2.plinelen, copyoptions[CPPLINE].ob_spec.tedinfo->te_ptext, 10 ); 

	/* The dialog itself */

	button = xd_dialog(copyoptions, 0);

	/* If OK is selected... */

	if (button == COPTOK) 
	{	  
		/* Get new states of options buttons and new copy buffer size */

		get_opt( copyoptions, &options.cprefs, CF_COPY, CCOPY );
		get_opt( copyoptions, &options.cprefs, CF_DEL, CDEL );
		get_opt( copyoptions, &options.cprefs, CF_OVERW, COVERW );
		get_opt( copyoptions, &options.cprefs, CF_PRINT, CPRINT ); 
		get_opt( copyoptions, &options.cprefs, CF_SHOWD, CSHOWD );
		get_opt( copyoptions, &options.cprefs, P_HEADER, CPPHEAD );

		if ((options.bufsize = atoi(copybuffer)) < 1)
			options.bufsize = 1;

		options.V2_2.plinelen = atoi(copyoptions[CPPLINE].ob_spec.tedinfo->te_ptext);
		if ((options.V2_2.plinelen < 32) || (options.V2_2.plinelen > 132) )
			options.V2_2.plinelen = 80;
	}
}


/*
 * Set default options to be in effect without a configuration file 
 */

static void opt_default(void)
{
	options.version = CFG_VERSION;
	options.magic = MAGIC;
	options.sort = WD_SORT_NAME;							
#if _PREDEF
	options.cprefs = CF_COPY | CF_DEL | CF_OVERW | CF_PRINT | CF_TOUCH | CF_SHOWD;
	options.V2_2.fields = WD_SHSIZ | WD_SHDAT | WD_SHTIM | WD_SHATT;    
	options.V2_2.plinelen = 80; 							
	options.attribs = FA_SUBDIR | FA_SYSTEM;					
	options.tabsize = 8;
	options.bufsize = 512;
#endif
	options.mode = TEXTMODE;										    
	options.dial_mode = XD_NORMAL;
	options.resvd1 = 0;
	options.resvd2 = 0;
	get_set_video(0);			/* get current video mode for default */
	
	set_dsk_background((ncolors > 2) ? 7 : 4, 3);
}


/* 
 * Define some default keyboard shortcuts ... 
 * those offered here have become a de-facto standard
 * in other applications. On the other hand, they are different
 * from those which Atari has adopted for the in-built TOS desktop
 */

static void short_default(void)
{

	memset( &options.V2_2.kbshort[MOPEN - MFIRST], 0, (size_t)(MSAVESET - MFIRST) );

#if _PREDEF
	options.V2_2.kbshort[MOPEN - MFIRST] =    XD_CTRL | 'O';	/* ^O, etc. */
	options.V2_2.kbshort[MSHOWINF - MFIRST] = XD_CTRL | 'I';
	options.V2_2.kbshort[MSEARCH - MFIRST] =  XD_CTRL | 'F';
	options.V2_2.kbshort[MPRINT - MFIRST] =   XD_CTRL | 'P';
	options.V2_2.kbshort[MSELALL - MFIRST] =  XD_CTRL | 'A';
	options.V2_2.kbshort[MQUIT - MFIRST] =    XD_CTRL | 'Q';
	options.V2_2.kbshort[MDELETE - MFIRST] =  XD_CTRL | 0x7f; 	/* ^DEL */
	options.V2_2.kbshort[MSAVESET - MFIRST] = XD_CTRL | 'S';
#endif
	ins_shorts();
}


/* 
 * Read configuration from the configuration file 
 */

static void load_options(void)
{
#if !TEXT_CFG_IN
	XFILE *file;
	Options tmp;
	long n, opt_n;
#endif
	int error = 0;


	get_set_video(0);				/* get current video mode */
	v3_options.max_dir = 256;		/* start amount for the dir pointer array */

#if TEXT_CFG_IN
	error = handle_cfgfile(infname, Config_table, infide, 0); 

#else
  #include "opt_load.h"
#endif

	/* If there was an error when loading options, set default values */

	if (error != 0)
	{
#if !TEXT_CFG_IN
		alert_printf(1, ALOADCFG, fn_get_name(optname), get_message(error));
#endif
		opt_default();
		short_default();
		dsk_default();
		ft_default();
		icnt_default();
		prg_default();
		app_default();
		wd_default();
	}

	/* Set dialogs mode */

	xd_setposmode((options.cprefs & DIALPOS_MODE) ? XD_MOUSE : XD_CENTERED);
	set_dialmode();
}


/*
 * A (possibly temporary) utility functions used to load or save
 * some boolean flags as bitflags 
 */

void bit_to_bool( int bitflags, int bit, boolean *boo )
{
	*boo = ( bitflags & bit ) ? TRUE : false;
}  

void bool_to_bit( int *bitflags, int bit, boolean boo )
{
	*bitflags = ( boo ) ? ( (*bitflags) | bit ) : ( (*bitflags) & ~bit ); 
}


/*
 * Configuration routine for basic destop options
 */

static CfgNest opt_config
{
	if (io == CFG_SAVE)
	{
		/* Save options */

	#if ! TEXT_CFG_IN
		dial_mode = options.dial_mode;
	#endif

		*error = CfgSave(file, Options_table, lvl + 1, CFGEMP); 
	}
	else
	{
		/* Initialize options structure to zero then load options */

		memset ( &options, 0, sizeof(options) );

		*error = CfgLoad(file, Options_table, MAX_KEYLEN, lvl + 1); 

		/* 
		 * Check some critical values against limits; if not checked and
		 * some illegal value happens to be in the file, these variables
		 * may crash the program or have other ugly consequences
		 */

		if ( options.V2_2.plinelen < 32 ) options.V2_2.plinelen = 80; 

		if (   options.version < MIN_VERSION 
			|| options.version > CFG_VERSION 
			|| options.sort > WD_NOSORT
			|| options.V2_2.plinelen > 132
			|| v3_options.max_dir < 64 
			|| v3_options.max_dir > 2048
			|| options.dial_mode > XD_WINDOW
			|| options.V2_2.vrez > 7 
			)
				*error = EFRVAL;

		/* Block possible junk from some fields in config file */

		options.attribs &= 0x0077;
		options.dsk_pattern &= 0x0007;
		options.V2_2.win_pattern &= 0x0007;

		/* 
		 * Currently it makes no sense NOT to confirm touch;
		 */

		options.cprefs |= CF_TOUCH;
		options.cprefs &= ~(CF_CTIME | CF_CATTR);

		/* If all is OK so far, start setting teradesk */

		if ( *error >= 0 )
			set_dsk_background(options.dsk_pattern, options.dsk_color);
		else
			opt_default();

#if PALETTES
		/* Load palette. Ignore errors */

		if (options.cprefs & SAVE_COLORS)
			load_colors(0L);
#endif
		/* Set video state but do not change resolution */

		get_set_video(1);
	}
}


/*
 * Configure (load or save) keyboard shortcuts
 * Note: for loading, this is initialized to zero earlier in opt_config.
 */

static CfgNest short_config
{
	*error = handle_cfg(file, Shortcut_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, NULL, short_default);

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
	graf_mouse(HOURGLASS, NULL);

	/* First save the palette (ignore errors) */
#if PALETTES
	if (options.cprefs & SAVE_COLORS)	/* separate file "teradesk.pal" */
		save_colors(0L);
#endif

	/* Then save the configuration */

	handle_cfgfile( infname, Config_table, infide, CFG_SAVE );
	
	/* Update destination window */

	wd_set_update(WD_UPD_COPIED, infname, NULL);
	wd_do_update();

	graf_mouse(ARROW, NULL);
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

#if TEXT_CFG_IN
	if ((newinfname = locate(infname, L_LOADCFG)) != NULL)
	{
		free(infname);
		infname = newinfname;
	}
#else
	if ((newinfname = locate(optname, L_LOADCFG)) != NULL)
	{
		free(optname);
		optname = newinfname;
	}
#endif

	load_options();

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
 * Result: FALSE if OK, TRUE if error (a VERY intuitive concept!).
 */

static boolean init(void)
{
	xw_get(NULL, WF_WORKXYWH, &screen_info.dsk);

#if !TEXT_CFG_IN

	if ( !find_cfgfiles(&optname, TRUE) )
		return TRUE;

#endif 

	find_cfgfiles(&infname, FALSE); 
	find_cfgfiles(&palname, FALSE);

	if (dsk_init() == TRUE)
		return TRUE;

	/* Clear all definable items */

	ft_init();
	icnt_init();
	prg_init();
	app_init();
	edit_init();
	wd_init();

	menu_bar(menu, 1);

	x_setpath("\\");

	/* Load the configuration file */

	load_options();

	/* Start applications which have been defined as autostart */

	app_specstart(AT_AUTO);

	return FALSE;
}


/* 
 * Initialize stuff related to VDI 
 */

static void init_vdi(void)
{
	int dummy, work_out[58], pix_height;

	screen_info.phy_handle = graf_handle(&screen_info.fnt_w, &screen_info.fnt_h, &dummy, &dummy);

	vq_extnd(vdi_handle, 0, work_out);

	max_w = work_out[0] + 1;	/* Screen width (pixels)  */
	max_h = work_out[1] + 1;	/* Screen height (pixels) */
	ncolors = work_out[13];		/* Number of colours      */
	npatterns = work_out[14];	/* Number of patterns     */
	pix_height = work_out[4];


	/* 
	 * Note: vqt_attributes below uses work_out and destroys information
	 * in work_out[0] to work_out[9]; not a nice thing to do!
	 */

	vqt_attributes(vdi_handle, work_out);

	fnt_setfont(1, (int) (((long) work_out[7] * (long) pix_height * 72L + 12700L) / 25400L), &def_font);

	screen_info.vdi_handle = vdi_handle;
}


static int alloc_global_memory(void)
{
/* This doesn't seem to take into account possibility of alt-ram on a ST?
#if _MINT_
	if (magx || mint)
		global_memory = Mxalloc(GLOBAL_MEM_SIZE, 0x43);
	else
#endif
		global_memory = Malloc(GLOBAL_MEM_SIZE);
*/

	/* so, maybe it is better this way? */

	int mode = 
#if _MINT_
		( mint ) ? 0x43 : 	/* this is, in fact, for magic OR mint */
#endif
		0x3;				/* ST-RAM or Alt/TT-RAM; prefer Alt/TT */ 

#if _MINT_
	if ( magx || (tos_version >= 0x206) )
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
			int qbutton = alert_printf(3, ALRTQUIT);
			switch (qbutton)
			{
				case 3:
					break;
				case 2:
					shutdown=TRUE;
				case 1:      
					menu_tnormal(menu, title, 1);
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
			app_install();
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
			chrez = voptions();
			if ( chrez ) 
				quit = TRUE;
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
	int a;

	a = key & 0xff;
	if ( a <= 'z' && a >= 'a' )
		a &= 0xdf; 							/* to uppercase  */
	if ( kstate == 4 ) 						/* ctrl          */	
		if ( a == SPACE || a == ESCAPE )	/* ^SP ^ESC      */
			a |= XD_CTRL;
		else if ( a == 0x1f ) 				/* ^DEL          */
			a |= ( XD_CTRL | 0x60 );
		else 								/* ^A ... ^\     */
			a |= ( XD_CTRL | 0x40 );
	else
	if ( kstate > 2 || key < 0 ) 			/* everything but shift or plain */
		a = -1;								/* shortcut def. is never this value */
	return a;
}


/* 
 * Handle keybard commands 
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
			app_exec(NULL, appl, NULL, NULL, 0, kstate, FALSE);
	}
	else
	{
		k = scansh ( key, kstate );

		title = TFIRST;
		while (   (options.V2_2.kbshort[i] != k) && (i <= ( MLAST - MFIRST)) )
		{
			if ( (options.V2_2.kbshort[i] & XD_ALT) != 0 ) title++;  
			i++;
		}

		/* Handle various user-defined keyboard shortcuts */

		if ( options.V2_2.kbshort[i] == k )
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

					if ((path = strdup("A:\\")) != NULL)
					{
						path[0] = (char) i + 'A';
						dir_add_window(path, NULL);
						itm_set_menu(xw_top());
					}
					else
						xform_error(ENSMEM);
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
		switch (message[0])
		{
		case AP_TERM:
			if (allow_exit)
				quit = TRUE;
			else
			{
				static int ap_tfail[8] = { AP_TFAIL, 0 };

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

static void evntloop(void)
{
	int event;
	XDEVENT events;

	events.ev_mflags = EVNT_FLAGS;

	/*
	 * Note: right mouse button has the effect of:
	 * left doubleclick + right button pressed
	 * (also convenient for activating programs in a non-selected window)
	 */

	events.ev_mbclicks = 0x102;
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



/* 
 * Main TeraDesk routine- the program itself 
 */

int main(void)
{
	int error;


#if _MINT_

	/* 
	 * Find out some details on the type of OS 
	 * This is relevant only for the multitasking version.
	 * Ssystem() is available only since Mint 1.15.
	 */

	have_ssystem = (Ssystem(-1, 0L, 0L) == 0);		/* use Ssystem where possible */

	mint   = (find_cookie('MiNT') == -1) ? FALSE : TRUE;
	magx   = (find_cookie('MagX') == -1) ? FALSE : TRUE;
	geneva = (find_cookie('Gnva') == -1) ? FALSE : TRUE;
	mint  |= magx;			/* Quick & dirty */

	if (mint)
	{
		Psigsetmask(0x7FFFE14EL);
		Pdomain(1);
	}
#endif

	/* Find if files can be locked in this version of OS */

	x_init();

	/* Register this app with GEM; get its id */

	if ((ap_id = appl_init()) < 0)
		return -1;

	/* aes_version can not be determined earlier than appl_init() */

	tos_version = get_tosversion();
	aes_version = _GemParBlk.glob.version;

	/* 
	 * Inform AES of TeraDesk's capabilities regarding messages 
	 * (should it be tested for version 0x340 or 0x400 here ?)
	 */

	if  (aes_version >= 0x400)
	{
		shel_write(SHW_INFRECGN, 1, 0, NULL, NULL); /* Inform AES  */
		menu_register(ap_id, "  Tera Desktop");
	}

	/* Load resource file */

	if (rsrc_load(RSRCNAME) == 0)
		form_alert(1, msg_resnfnd);
	else
	{
		/* Initialize x-dialogs */

		if ((error = init_xdialog(&vdi_handle, malloc, free,
								  "Tera Desktop", 1, &nfonts)) < 0)
			xform_error(error);
		else
		{
			/* Initialize things related to vdi */

			init_vdi();

			/* Initialize resource structure */

			rsc_init();

			/* If screen resolution is too low (less than 40x25), can't continue */

			if (((max_w / screen_info.fnt_w) < 40) || ((max_h / screen_info.fnt_h) < 25))
				alert_printf(1, ARESTLOW);
			else
			{
				/* Allocate global memory */

				if ((error = alloc_global_memory()) == 0)
				{
					/* Execute startup batch file */

					if (exec_deskbat() == FALSE)
					{
						/* Load icons from icons resource file */

						if (load_icons() == FALSE)
						{
							/* Set defaults, read configuration, etc. */

							if (init() == FALSE)
							{
								graf_mouse(ARROW, NULL);

								/* 
								 * Main event loop of this Desktop 
								 * All of the work in TeraDesk
								 * happens in here
								 */

								evntloop();

								/* Start quitting / shutting down */

								wd_del_all();		/* remove windows       */
								menu_bar(menu, 0);	/* remove menu bar       */
								xw_close_desk();	/* remove desktop window */
							}
							free_icons();

							regen_desktop(NULL);
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

		/* Deallocate resource structures */

		rsrc_free();
	}

	/* 
	 * The following segment handles final system shutdown and resolution change
	 * If a resolution change is required, shutdown is performed first
	 * If only shutdown is required, system will reset at the end.
	 */ 

	if ( chrez || shutdown )						/* If change resolution or shutdown ... */
	{
		/* 
		 * Note: if an alternate shutdown program is to be used some day,
		 * it probably should be activeded here
		 */

		/* Tell all applications which would understand it to end */

		quit = shel_write ( SHW_SHUTDOWN, 2, 0, NULL, NULL ); 	/* complete shutdown */
		evnt_timer( 3000, 0 );						/* Wait a bit? */

#if 0
		/* 
		 * In Mint, must tell all proceseses to terminate nicely ?
		 * but this is only in this group ? What to do?
		 */

		Pkill(0, SIGTERM); 
		evnt_timer(3000, 0); /* Wait a bit? */
#endif
		/* 
		 * After all applications have hopefully been closed,
		 * change the screen resolution if needed;
		 * else- reset the computer
		 */

		if ( chrez )
			get_set_video(2);

#if 1
		else
/* But it won't really shutdown in Magic without a reset, just restarts the desktop ?
	#if _MINT_
		if (!mint)			/* HR 230203: Dont reset under MiNT or MagiC !!!!! */
	#endif
*/
		{
			/*
			 * Perform a reset here 
			 * Note: maybe use Ssystem here as well when appropriate?
			 */

			long (*rv)();				/* reset vector */

			Super ( 0L );				/* Supervisor; old stack won't be needed again */
			memval = 0L; 				
			memval2 = 0L;
			resvalid = 0L;
			(long)rv = *((long *)os_start + 4);	/* routine that handles the reset */
			Supexec(rv);				/* execute it */

		}
#endif
	}
	else	/* Just Quit the desktop */		
		appl_exit();
	
	return 0;
}

/* That's all ! */



