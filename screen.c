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
#include <vdi.h>
#include <xdialog.h>
#include <mint.h>
#include <string.h>			/* for load_colors */
#include <library.h>

#include "desk.h"
#include "desktop.h"
#include "error.h"
#include "xfilesys.h"
#include "screen.h"
#include "config.h"
#include "font.h"
#include "window.h"


/* 
 * Clip een rechthoek op de grenzen van de desktop.
 * Als de rechthoek geheel buiten de desktop ligt, wordt
 * als resultaat FALSE teruggegeven. 
 */

boolean clip_desk(RECT *r)
{
	return xd_rcintersect(r, &screen_info.dsk, r);
}


void clipdesk_on(void)
{
	int clip_rect[4];

	xd_rect2pxy(&screen_info.dsk, clip_rect);
	vs_clip(vdi_handle, 1, clip_rect);
}


/*
 * Clear a rectangle with white
 */

void clear(RECT *r)		/* use v_bar for a white rectangle (for true colour) */
{
	clr_object( r, WHITE, 0);
}


/*
 * Similar to clear() above but clears with window pattern and colour 
 * (pattern and colour are those defined for window background)
 */

void pclear(RECT *r)
{
	clr_object( r, options.V2_2.win_color, options.V2_2.win_pattern);
}


boolean rc_intersect2(RECT *r1, RECT *r2)
{
	RECT r;

	return xd_rcintersect(r1, r2, &r);
}


/*
 * Determine whether a location is within a rectangle
 */

boolean inrect(int x, int y, RECT *r)
{
	if (   x >= r->x
	    && x < (r->x + r->w)
	    && y >= r->y
	    && y < (r->y + r->h)
	   )
		return TRUE;
	else
		return FALSE;
}


void set_rect_default(void)
{
	vswr_mode(vdi_handle, MD_XOR);

	vsl_color(vdi_handle, 1);
	vsl_ends(vdi_handle, 0, 0);
	vsl_type(vdi_handle, 7);
	vsl_udsty(vdi_handle, 0xCCCC);
	vsl_width(vdi_handle, 1);
}


/*
 * Draw a rectangle
 */

void draw_rect(int x1, int y1, int x2, int y2)
{
	int p[10];

	graf_mouse(M_OFF, NULL);
	xd_clip_on(&screen_info.dsk);

	p[0] = p[6] = p[8] = x1;
	p[1] = p[3] = p[9] = y1;
	p[2] = p[4] = x2;
	p[5] = p[7] = y2;

	v_pline(vdi_handle, 5, p);

	xd_clip_off();
	graf_mouse(M_ON, NULL);
}


void invert(RECT *r)
{
	int pxy[8];
	MFDB mfdb;

	xd_rect2pxy(r, pxy);
	xd_rect2pxy(r, &pxy[4]);
	mfdb.fd_addr = NULL;
	vro_cpyfm(vdi_handle, D_INVERT, pxy, &mfdb, &mfdb);
}


/* 
 * Funktie vooor het verschuiven van een deel van het scherm. 
 */

void move_screen(RECT *dest, RECT *src)
{
	MFDB mfdb;
	int pxy[8];

	mfdb.fd_addr = NULL;
	xd_rect2pxy(src, pxy);
	xd_rect2pxy(dest, &pxy[4]);

	vro_cpyfm(vdi_handle, S_ONLY, pxy, &mfdb, &mfdb);
}


/* 
 * Funktie voor het initialiseren van het vdi. 
 */

void set_txt_default(int font, int height)
{
	int dummy;

	vswr_mode(vdi_handle, MD_REPLACE);

	vst_font(vdi_handle, font);
	vst_color(vdi_handle, 1);
	vst_rotation(vdi_handle, 0);
	vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);
	vst_point(vdi_handle, height, &dummy, &dummy, &dummy, &dummy);
	vst_effects(vdi_handle, 0);
}


/*
 * Allocate space for a palette table and return pointer to it.
 * Table format: 3 * int16 per colour (red, green, blue in promiles)
 */

int *get_colors(void)
{
	int i, *colors, *h;

	if ((colors = malloc((long) ncolors * 3L * sizeof(int))) != NULL)
	{
		h = colors;

		for (i = 0; i < ncolors; i++)
		{
			vq_color(vdi_handle, i, 0, h);
			h += 3;
		}
	}

	return colors;
}


/*
 * Set colour palette from a palette table
 */

void set_colors(int *colors)
{
	int i, *h = colors;

	for (i = 0; i < ncolors; i++)
	{
		vs_color(vdi_handle, i, h);
		h += 3;
	}
}


#if PALETTES

/* This is the newest version */

static char *palide = "TeraDesk-pal";
extern char *palname;

typedef struct rgb
{
	int ind;
	int red;
	int green;
	int blue;
}RGB;

RGB cwork;

int palsize = 0;	/* new (as read from the file) palette size */
int *palette = 0; 	/* Pointer to current palette */

CfgNest rgb_config, pal_config;


static CfgEntry palette_root[] =
{
	{CFG_NEST, 0, "palette", pal_config },
	{CFG_FINAL},
	{CFG_LAST}
};

static CfgEntry palette_table[] =
{
	{CFG_HDR, 0, "palette" },
	{CFG_BEG},
	{CFG_D,   0, "size", &palsize    },
	{CFG_NEST,0, "col",  rgb_config	 },
	{CFG_ENDG},
	{CFG_LAST}
};

static CfgEntry colour_table[] =
{
	{CFG_HDR, 0, "col"  },
	{CFG_BEG},
	{CFG_D,   CFG_INHIB, "ind", &cwork.ind }, /* index is not essential, but accept it */
	{CFG_DDD, 0, "rgb", &cwork.red },
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configuration for one colour
 */

static CfgNest rgb_config
{
	int 
		i,
		*thecolor = palette;

	if ( io == CFG_SAVE )
	{
		for ( i = 0; i < ncolors; i++ )
		{
			cwork.ind = i;

			cwork.red =  *thecolor++;
			cwork.green = *thecolor++;
			cwork.blue =  *thecolor++; 

			*error = CfgSave(file, colour_table, lvl + 1, CFGEMP);

			if (*error < 0)
				break;
		}

	}
	else
	{
		if ( palsize != ncolors )
		{
			alert_iprint(MECOLORS); 
			*error = ENOMSG; /* abort but don't display more error messages */
		}
		else
		{
			/* initialize rgb but not ind */
			cwork.red = 0;
			cwork.green = 0;
			cwork.blue = 0;

			*error = CfgLoad(file, colour_table, MAX_KEYLEN, lvl + 1);

			if ( *error == 0 )
			{
				if ( cwork.ind >= palsize )
					*error = EFRVAL;
				else
				{
					int *p = &cwork.red;

					thecolor += 3 * cwork.ind;
					cwork.ind++;

					for ( i = 0; i < 3; i++ )
						*thecolor++ = min(p[i],1000);
				}
			}
		}
	}
}


static CfgNest pal_config
{
	cwork.ind = 0;

	/* Get current palette; this will allocate space for the new one */

	palette = get_colors();

	/* If space for the palette has been allocated, load data into it */

	if ( palette )
	{
		*error = handle_cfg(file, palette_table, MAX_KEYLEN, lvl + 1, CFGEMP, io, NULL, NULL );
		if (*error != 0)
			free(palette);
	}
	else 
		*error = ENSMEM;
}
 
/* This is the newest version of load_colors() */

int load_colors(void)
{
	int error;

	/* Load palette from the file; temporary space for palette allocated here */

	error = handle_cfgfile( palname, palette_root, palide, 0 );
		
	/* If everything is OK set new colours and free palette table */

	if ( error == 0 )
	{
		set_colors(palette);
		free(palette);
	}

	return error;
}


int save_colors(void)
{
	int error;

	palsize = ncolors;

	/* Write configuration; temporary space for palette is allocated here */

	error = handle_cfgfile( palname, palette_root, palide, CFG_SAVE );

	if ( error == 0 )
		free(palette);

	return error;
}

#endif		/* PALETTES */


