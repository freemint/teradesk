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

#include <aes.h>
#include <stdlib.h>
#include <vdi.h>
#include <xdialog.h>
#include <mint.h>

#include "desk.h"
#ifdef _MINT_
#include "mintdesk.h"
#else
#include "desktop.h"
#endif
#include "error.h"
#include "xfilesys.h"
#include "screen.h"

/* clip een rechthoek op de grenzen van de desktop.
   Als de rechthoek geheel buiten de desktop ligt, wordt
   als resultaat FALSE teruggegeven. */

boolean clip_desk(GRECT *r)
{
	return xd_rcintersect(r, (GRECT *) & screen_info.dsk_x, r);
}

void clipdesk_on(void)
{
	int clip_rect[4];

	xd_rect2pxy((GRECT *) & screen_info.dsk_x, clip_rect);
	vs_clip(vdi_handle, 1, clip_rect);
}

void clear(GRECT *r)
{
	int pxy[8];
	MFDB mfdb;

	xd_rect2pxy(r, pxy);
	xd_rect2pxy(r, &pxy[4]);
	mfdb.fd_addr = NULL;
	vro_cpyfm(vdi_handle, 0, pxy, &mfdb, &mfdb);
}

boolean rc_intersect2(GRECT *r1, GRECT *r2)
{
	GRECT r;

	return xd_rcintersect(r1, r2, &r);
}

boolean inrect(int x, int y, GRECT *r)
{
	if ((x >= r->g_x) && (x < (r->g_x + r->g_w)) && (y >= r->g_y) && (y < (r->g_y + r->g_h)))
		return TRUE;
	else
		return FALSE;
}

void invert(GRECT *r)
{
	int pxy[8];
	MFDB mfdb;

	xd_rect2pxy(r, pxy);
	xd_rect2pxy(r, &pxy[4]);
	mfdb.fd_addr = NULL;
	vro_cpyfm(vdi_handle, D_INVERT, pxy, &mfdb, &mfdb);
}

/* Funktie vooor het verschuiven van een deel van het scherm. */

void move_screen(GRECT *dest, GRECT *src)
{
	MFDB mfdb;
	int pxy[8];

	mfdb.fd_addr = NULL;
	xd_rect2pxy(src, pxy);
	xd_rect2pxy(dest, &pxy[4]);

	vro_cpyfm(vdi_handle, S_ONLY, pxy, &mfdb, &mfdb);
}

/* Funktie voor het initialiseren van het vdi. */

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

void set_colors(int *colors)
{
	int i, *h = colors;

	for (i = 0; i < ncolors; i++)
	{
		vs_color(vdi_handle, i, h);
		h += 3;
	}
}

int load_colors(XFILE *file)
{
	int nc, *colors;
	long n, h;

	if ((n = x_fread(file, &nc, sizeof(int))) < 0)
		 return (int) n;

	h = (long) nc *3L * sizeof(int);

	if ((colors = malloc(h)) == NULL)
		return ENSMEM;

	if ((n = x_fread(file, colors, h)) != h)
		return (n < 0) ? (int) n : EEOF;

	if (nc == ncolors)
		set_colors(colors);
	else
		alert_printf(1, MECOLORS);

	free(colors);

	return 0;
}

int save_colors(XFILE *file)
{
	int *colors;
	long n;

	if ((n = x_fwrite(file, &ncolors, sizeof(int))) < 0)
		 return (int) n;

	if ((colors = get_colors()) == NULL)
		return ENSMEM;

	if ((n = x_fwrite(file, colors, (long) ncolors * 3L * sizeof(int))) < 0)
		 return (int) n;

	free(colors);

	return 0;
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

void draw_rect(int x1, int y1, int x2, int y2)
{
	int p[10];

	graf_mouse(M_OFF, NULL);
	xd_clip_on((GRECT *) &screen_info.dsk_x);

	p[0] = p[6] = p[8] = x1;
	p[1] = p[3] = p[9] = y1;
	p[2] = p[4] = x2;
	p[5] = p[7] = y2;

	v_pline(vdi_handle, 5, p);

	xd_clip_off();
	graf_mouse(M_ON, NULL);
}
