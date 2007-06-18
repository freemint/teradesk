/*
 * Xdialog Library. Copyright (c) 1993 - 2002  W. Klaren,
 *                                2002 - 2003  H. Robbers,
 *                                2003 - 2007  Dj. Vukovic
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


/* These objects are currently never used in Teradesk

static int cdecl ub_rectbuttri(PARMBLK *pb)
{
	int x = pb->pb_x, y = pb->pb_y, pxy[4];
	int state;
	RECT clip;

	clip.x = pb->pb_xc;
	clip.y = pb->pb_yc;
	clip.w = pb->pb_wc;
	clip.h = pb->pb_hc;

	xd_clip_on(&clip);

	pxy[0] = x + 2;				/* <- fixed I_A: effects TRISTATE_1 too! */
	pxy[1] = y + 2;				/* <- fixed I_A */
	pxy[2] = x + xd_regular_font.fnt_chw * 2 - 2;
	pxy[3] = y + xd_regular_font.fnt_chh - 2;

	vswr_mode(xd_vhandle, MD_REPLACE);
	set_filldef(0);
	vr_recfl(xd_vhandle, pxy);
	set_linedef(1);
	draw_rect(x + 1, y + 1, 2 * xd_regular_font.fnt_chw - 1, xd_regular_font.fnt_chh - 1);

	/* handle tri-state */

	state = pb->pb_tree[pb->pb_obj].ob_state;

	switch (xd_get_tristate(state))
	{
	case TRISTATE_0:
		break;
	case TRISTATE_1:
		vsf_interior(xd_vhandle, FIS_PATTERN);
		vsf_style(xd_vhandle, 4);
		vsf_color(xd_vhandle, 1);
		v_bar(xd_vhandle, pxy);
		vsf_interior(xd_vhandle, FIS_SOLID);	/* HR 021202 always return to default */
		break;
	case TRISTATE_2:
		pxy[0]--;
		pxy[1]--;
		pxy[2]++;
		pxy[3]++;
		v_pline(xd_vhandle, 2, pxy);
		pxy[0] = pxy[2];
		pxy[2] = x + 1;
		v_pline(xd_vhandle, 2, pxy);
		break;
	}

	vswr_mode(xd_vhandle, /* (IS_BG(pb->pb_tree[pb->pb_obj].ob_flags) &&
						   xd_draw_3d) ? MD_TRANS : */ MD_REPLACE);
	set_textdef();
	prt_text((char *) pb->pb_parm, x + 3 * xd_regular_font.fnt_chw, y, state);

	xd_clip_off();

	return 0;
}


static int cdecl ub_cyclebut(PARMBLK *pb)
{
	int x = pb->pb_x, y = pb->pb_y, w = pb->pb_w, h = pb->pb_h;
	RECT clip;

	clip.x = pb->pb_xc;
	clip.y = pb->pb_yc;
	clip.w = pb->pb_wc;
	clip.h = pb->pb_hc;

	xd_clip_on(&clip);

	vswr_mode(xd_vhandle, MD_REPLACE);
	clr_object(&clip, 0);
	set_linedef(1);
	draw_rect(x, y, w, h);

	xd_clip_off();

	return 0;
}
*/
