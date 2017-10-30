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


#include <library.h>
#include "xdialog.h"


static MFDB cursor_mfdb = { NULL, 1, 0, 1, 0, 0, 0, 0, 0 };


/********************************************************************
 *																	*
 * Funkties voor het tekenen van de userdefined objects.			*
 *																	*
 ********************************************************************/

/*
 * Functie die de achtergrondkleur van een object bepaald.
 *
 * Parameters:
 *
 * flags	- De objectvlaggen van een object.
 *
 * Resultaat : kleur van het object.
 *
 *
 * Get appropriate colour for activator/indicator/background object
 */

static _WORD xd_get_3d_colour(_WORD flags)
{
	_WORD colour;

	if (IS_ACT(flags))
		colour = xd_act_col;
	else if (IS_IND(flags))
		colour = xd_ind_col;
	else if (IS_BG(flags))
		colour = xd_bg_col;
	else
		colour = G_WHITE;

	return (colour >= xd_ncolours) ? G_WHITE : colour;
}



/*
 * Obtain length of the string subtracting "#" characters
 */

static long xd_strlen(char *s)
{
	long l;

	l = strlen(s);

#if 0									/* This capability is currently not used anywhere in Teradesk resource */
	if (strchr(s, '#') != NULL)
		l--;
#endif

	return l;
}


/*
 * Note: in earlier versions, this routine had an additional
 * argument: attrib; but it was allways called with attrib = 0
 */

static void prt_text(char *s, _WORD x, _WORD y, _WORD state)
{
	_WORD attrib = 0;
	char tmp[42];
	char *p = NULL;

	if (state & OS_DISABLED)
		attrib ^= 2;

	vst_effects(xd_vhandle, attrib);

										/* h = */ strcpy(tmp, s);
										/* h is not needed for the time being, but tmp is */

	/* uses AES 4 WHITEBAK */

	if (state & OS_WHITEBAK)
	{
		_WORD und = (state << 1) >> 9;

		if (und >= 0)
		{
			und &= 0x7F;
			if (und < (_WORD)strlen(tmp))
				p = tmp + und;
		}
	}
#if 0									/* This capability is currently not used anywhere in Teradesk */

	else
		/* I_A enhanced: now you can place '#' in text itself! */

		while ((h = strchr(h, '#')) != NULL)
		{
			strcpy(h, h + 1);
			if (*h != '#')
			{
				/* remember location of single '#' in text! */
				p = h--;
			}
			/* else: double '#': make it one! */
			h++;
		}
#endif

	v_gtext(xd_vhandle, x, y, tmp);

	if (p)
	{
		/* Do underline some character! Use red colour */

		_WORD xtnd[8];

		*p = 0;
		vqt_extent(xd_vhandle, tmp, xtnd);
		vst_effects(xd_vhandle, attrib ^ 8);	/* XOR due to text-style extensions! */
		vst_color(xd_vhandle, G_RED);
		v_gtext(xd_vhandle, x + (xtnd[2] - xtnd[0]), y, " ");
		vst_color(xd_vhandle, G_BLACK);
	}
}


/*
 * Draw a simple rectangle
 * Note: smaller code is generated when pointers to pxy are used
 */
void draw_xdrect(_WORD x, _WORD y, _WORD w, _WORD h)
{
	_WORD p, pxy[10], *pxyp = pxy;

	*pxyp++ = x;						/* [0] */
	*pxyp++ = y;						/* [1] */
	p = x + w - 1;
	*pxyp++ = p;						/* [2] */
	*pxyp++ = y;						/* [3] */
	*pxyp++ = p;						/* [4] */
	p = y + h - 1;
	*pxyp++ = p;						/* [5] */
	*pxyp++ = x;						/* [6] */
	*pxyp++ = p;						/* [7] */
	*pxyp++ = x;						/* [8] */
	*pxyp = y;							/* [9] */

	v_pline(xd_vhandle, 5, pxy);
}


/*
 * Draw a frame by drawing concentric rectangles to obtain 
 * desired thickness of border
 */

static void draw_frame(GRECT *frame, _WORD start, _WORD eind)
{
	_WORD i, s, e;

	/* Note: this is slightly shorter than using min() and max() */

	if (start > eind)
	{
		s = eind;
		e = start;
	} else
	{
		s = start;
		e = eind;
	}

	for (i = s; i <= e; i++)
	{
		draw_xdrect(frame->g_x + i, frame->g_y + i, frame->g_w - 2 * i, frame->g_h - 2 * i);
	}
}


/*
 * Set suitable (default) line attributes. Full line, width 1 pixel.
 */

void set_linedef(_WORD colour)
{
	vsl_color(xd_vhandle, colour);
	vsl_ends(xd_vhandle, 0, 0);
	vsl_type(xd_vhandle, 1);
	vsl_width(xd_vhandle, 1);
}


/*
 * Set suitable (default) text attributes. 
 * Font is the regular system font. Colour is always black.
 * Writing mode is always transparent.
 */

static void set_textdef(void)
{
	_WORD dummy;

	vst_font(xd_vhandle, xd_regular_font.id);
	vst_rotation(xd_vhandle, 0);
	vst_alignment(xd_vhandle, 0, 5, &dummy, &dummy);
	xd_vst_point(xd_regular_font.size, &dummy);
	vst_color(xd_vhandle, G_BLACK);
	xd_vswr_trans_mode();
}


/* 
 * Set 'transparent' and 'replace' drawing modes
 */
void xd_vswr_trans_mode(void)
{
	vswr_mode(xd_vhandle, MD_TRANS);
}


void xd_vswr_repl_mode(void)
{
	vswr_mode(xd_vhandle, MD_REPLACE);
}


/*
 * Clear an object by drawing a borderless filled rectangle  
 * of desired colour and pattern (except when pattern is -1, then draw solid).
 * Rectangle is always drawn in replace mode.
 */

void clr_object(GRECT *r, _WORD colour, _WORD pattern)
{
	_WORD pxy[4], pn, fillmode = FIS_SOLID;

	xd_rect2pxy(r, pxy);

	if (pattern > 0)
	{
		if (pattern <= xd_npatterns)
		{
			pn = pattern;
			fillmode = FIS_PATTERN;
		} else
		{
			pn = pattern - xd_npatterns;
			fillmode = FIS_HATCH;
		}

		vsf_style(xd_vhandle, pn);
	}

	vsf_interior(xd_vhandle, fillmode);
	vsf_color(xd_vhandle, colour);
	vswr_mode(xd_vhandle, MD_REPLACE);	/* for speed do not use xd_vswr_repl_mode() */
	v_bar(xd_vhandle, pxy);
}


/*
 * Divine whether a userdef object should be drawn in 3D:
 * - 3d flags must be set for indicator or activator (not background)
 * - there must be a 3D-capable AES (i.e. AES 4)
 * - there must be a 3D enlargement, or else at least 16 colours 
 *   (this should in most (but not all) cases detect whether Magic is in 3d mode)
 * Note: this doesn't handle the cases of Magic or Geneva being capable of
 * drawing in 3d but that option being disabled by the user!
 */

static bool xd_is3dobj(_WORD flags)
{
	_WORD f3d = flags & (AES3D_1 | AES3D_2);

	if (f3d && (f3d != AES3D_2) && (xd_has3d || aes_hor3d > 0 || aes_ver3d > 0 || xd_colaes))
		return TRUE;

	return FALSE;
}


/*
 * Calculate size of an object enlargement, including 3D effects
 */

static void xd_3dbrd(_WORD ob_state, _WORD *xl, _WORD *xr, _WORD *yu, _WORD *yd)
{
	*xl = aes_hor3d;
	*xr = aes_hor3d;
	*yu = aes_ver3d;
	*yd = aes_ver3d;

	if (ob_state & OS_OUTLINED)
	{
		/* don't use max() here, this is shorter */

		if (brd_l > *xl)
			*xl = brd_l;
		if (brd_l > *xr)
			*xr = brd_r;
		if (brd_u > *yu)
			*yu = brd_u;
		if (brd_d > *yd)
			*yd = brd_d;
	}
}


/*
 * Draw a rectangular box. Most progdefined objects in TeraDesk in fact require
 * that a rectangular box be drawn, possibly with 3d effects.
 *
 * "r" defines size of the basic box; routine may change the size 
 * by the width of 3d enlargements, "outline" state 
 * and by "default" and "exit" flags.
 *
 * "flags": "default", "exit" "activator" and "indicator" are recognized
 *
 * "state": "selected" and "outlined" bits are recognized
 *
 * Border thickness is not recognized.
 * 
 * Box colour is determined from extended flags
 *
 * Clipping is NOT performed in this routine, must be set beforehand
 * 
 */


static void xd_drawbox(GRECT *r,		/* size of the box      */
					   _WORD flags,		/* object flags relevant to drawing this box */
					   _WORD state,		/* object state         */
					   _WORD xtype		/* extended object type */
	)
{
	GRECT outsize;						/* external object dimensions (final) */
	_WORD hxl;							/* horizontal enlargement  */
	_WORD vxu;							/* vertical enlargement    */
	_WORD hxr;							/* horizontal enlargement  */
	_WORD vxd;							/* vertical enlargement    */
	_WORD border;						/* border thickness        */
	_WORD colour;						/* current colour          */
	_WORD colour2;						/* other (inverse) colour  */
	_WORD p;							/* aux.                    */
	_WORD pxy[10];						/* point array for drawing */

	/* 
	 * Determine size of this object
	 * (are combinations of border and 3d enlargement really 
	 * to be implemented like in xd_3dbrd? not sure). 
	 */

	xd_3dbrd(state, &hxl, &hxr, &vxu, &vxd);

	outsize.g_x = r->g_x - hxl;
	outsize.g_y = r->g_y - vxu;
	outsize.g_w = r->g_w + hxl + hxr;
	outsize.g_h = r->g_h + vxu + vxd;

	/* Writing mode is generally "replace" */

	xd_vswr_repl_mode();

	/* 
	 * Fill the area with appropriate colour (except for the scrolled text);
	 * For any object other than a button, it is always the background one
	 */

	if (xtype != XD_SCRLEDIT && xtype != XD_BCKBOX)
	{
		if (xd_is3dobj(flags))
		{
			colour = xd_get_3d_colour(flags);
		} else
		{
			if ((state & OS_SELECTED) && (xtype == XD_BUTTON))
				colour = xd_sel_col;
			else
				colour = xd_bg_col;
		}

		clr_object(&outsize, colour, -1);
	}

	/* 
	 * Draw a black frame of appropriate thickness (going outside of object);
	 * This is drawn only for the button and the dragbox. For other object
	 * types the frame will be created later by drawing 3d effect only.
	 * On the other hand, if there is not a sufficient number of colours, 
	 * or if there is not a 3D AES, always draw the frame.
	 * Also draw the frame if the object is flagged as an activator.
	 */

	if (!xd_colaes || xtype == XD_BUTTON || xtype == XD_DRAGBOX || xtype == XD_BCKBOX || IS_ACT(flags))
	{
		if (flags & OF_DEFAULT)
			border = 2;					/* actual border thickness will be 3 pixels */
		else if (flags & OF_EXIT)
			border = 1;					/* actual border thickness will be 2 pixels */
		else
			border = 0;					/* actual border thickness will be 1 pixel  */

		if (xtype == XD_DRAGBOX)
			colour = xd_sel_col;
		else
			colour = G_BLACK;

		set_linedef(colour);
		draw_frame(&outsize, 0, -border);
	}

	/* 
	 * Set 3D effect, if necessary, by drawing one-pixel-wide
	 * contour in lighter/darker colour, diagonally just inside the object.
	 * Note1: this could be done by XORing as well, but probably not worth the trouble;
	 * Note2: AESses are not consistent in treating progdef objects if they are
	 * flagged as "indicator" or "activator"; some seem to draw 3d effect anyway
	 * (AES4.1, NAES), and some do not (Geneva?). Therefore, those flags are
	 * elsewhere saved for the sake of this routine and then disabled. 
	 * Btw. as the AES can't have the faintest idea what an userdef type 
	 * of object should look like, better not to let it draw the 3d effect.
	 */

	if (xd_is3dobj(flags))
	{
		_WORD *pxyp = pxy;

		p = outsize.g_x + outsize.g_w - 2;
		*pxyp++ = p;					/* [0] upper right corner */
		*pxyp++ = outsize.g_y + 1;		/* [1] */

		*pxyp++ = p;					/* [2] */
		p = outsize.g_y + outsize.g_h - 2;
		*pxyp++ = p;					/* [3] lower right corner */

		*pxyp++ = outsize.g_x + 1;		/* [4] lower left corner  */
		*pxyp++ = p;					/* [5] */

		*pxyp++ = pxy[4];				/* [6] upper left corner  */
		*pxyp++ = pxy[1];				/* [7] */

		*pxyp++ = pxy[0];				/* [8] upper right corner */
		*pxyp = pxy[1];					/* [9] */

		/* Alternate shadowing for selected and deselected object */

		if (state & OS_SELECTED)
		{
			colour = G_WHITE;
			colour2 = xd_sel_col;
		} else
		{
			colour = xd_sel_col;
			colour2 = G_WHITE;
		}

		set_linedef(colour);
		v_pline(xd_vhandle, 3, pxy);	/* lower & right sides of rectangle */
		vsl_color(xd_vhandle, colour2);
		v_pline(xd_vhandle, 3, &pxy[4]);	/* upper & left sides of rectangle */
	}
}


/*
 * Code for drawing progdefined "dragbox" ear on dialogs
 */

static _WORD cdecl ub_drag(PARMBLK *pb)
{
	_WORD dhl, dhr, dvu, dvd, pxy[6], b;
	GRECT size;
	OBSPEC *obspec = xd_get_obspecp(&pb->pb_tree[pb->pb_obj]);

	/* Define clipping area */

	xd_clip_on((GRECT *) & pb->pb_xc);

	/* 
	 * Define object size and position (up-  and right-justify with dialog edge)
	 * Note: must compensate for ST-low/med res aspect ratio oddities
	 */

	xd_3dbrd(pb->pb_tree[pb->pb_obj].ob_state, &dhl, &dhr, &dvu, &dvd);

	if (xd_regular_font.ch > 9 || xd_desk.g_w > 2 * xd_desk.g_h)
	{
		size.g_w = pb->pb_w;
		size.g_x = pb->pb_x;
	} else								/* ST-low res; must change aspect ratio */
	{
		size.g_w = pb->pb_w / 2;
		size.g_x = pb->pb_x + size.g_w;
	}

	size.g_y = pb->pb_y;
	size.g_h = pb->pb_h;

	xd_vswr_repl_mode();

	/* Draw object box */

	xd_drawbox(&size, ((XUSERBLK *) (pb->pb_parm))->ob_flags, pb->pb_tree[pb->pb_obj].ob_state, XD_DRAGBOX);

	/* Draw "\" diagonal in the box */

	pxy[0] = size.g_x - dhl;
	pxy[1] = size.g_y - dvu;
	pxy[2] = size.g_x + size.g_w + dhr - 1;
	pxy[3] = size.g_y + size.g_h + dvd - 1;

	set_linedef(xd_sel_col);
	v_pline(xd_vhandle, 2, pxy);

	/* Draw something looking like a part of the border visible behind the "ear" */

	b = abs((_WORD) obspec->obspec.framesize);	/* border thickness */

	if (b != 0)
	{
		_WORD i;

		vsl_color(xd_vhandle, (_WORD) (obspec->obspec.framecol));

		pxy[0] = size.g_x + 1;
		pxy[1] = size.g_y;
		pxy[2] = size.g_x + size.g_w - 1;
		pxy[5] = size.g_y + size.g_h - 2;

		/* Draw the border */

		for (i = 0; i < b; i++)
		{
			pxy[3] = pxy[1];
			pxy[4] = pxy[2];
			v_pline(xd_vhandle, 3, pxy);
			pxy[0]++;
			pxy[1]++;
			pxy[2]--;
			pxy[5]--;
		}
	}

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}

/*
 * Code for drawing progdefined "background box"
 */

_WORD cdecl ub_bckbox(PARMBLK *pb)
{
	_WORD flags;
	GRECT size;
	XUSERBLK *blk = (XUSERBLK *) (pb->pb_parm);

	if (((blk->ob_type) & 0x00FF) == G_BOX)	/* Don't draw an IBOX */
	{
		/* Only the indicator flag is recognized */

		flags = blk->ob_flags & AES3D_1;

		xd_clip_on((GRECT *) & pb->pb_xc);	/* define clipping area */

		size = *(GRECT *) (&(pb->pb_x));	/* object size */

		xd_vswr_repl_mode();			/* 'replace' drawing mode */

		/* Draw object rectangle */

		if (flags)
			size.g_h -= 1;

		clr_object(&size, blk->uv.fill.colour, blk->uv.fill.pattern);	/* coloured rectangle */

		/* Draw (3D) frame if flags set */

		if (flags)
		{
			size.g_h += 1;

			if (!(xd_has3d && aes_hor3d == 0))	/* all but Magic (and MyAES) */
			{
				size.g_x -= 1;
				size.g_w += 2;
			}

			xd_drawbox(&size, flags, pb->pb_tree[pb->pb_obj].ob_state, XD_BCKBOX);
		}

		xd_clip_off();
	}

	return 0;
}


/*
 * Code for drawing progdefined circular radiobutton
 */

static _WORD cdecl ub_roundrb(PARMBLK *pb)
{
	/*
	 * Bitmasks for round radio buttons.
	 * There are bitmasks for low res, med res (i.e. different aspect ratio)
	 * and one common for all other (high) res.
	 * For each resolution the bitmasks are in three parts, to enable
	 * creation of 3d effects: 
	 * -upper left arc with centre circle, 
	 * -lower right arc 
	 * -centre circle. 
	 * Button is drawn in three steps, the first one always being drawn 
	 * in replace mode, the other two transparent. If 3d capability does 
	 * not exist, button is drawn from other segments 
	 * (full circle and centre dot)
	 */

	/* low res, full circle (deselected) */
	static const short rbf_8x8[] = { 0x3800, 0x4400, 0x8200, 0x8200, 0x8200, 0x4400, 0x3800, 0x0000 };

	/* low res, center dot */
	static const short rbc_8x8[] = { 0x0000, 0x0000, 0x3800, 0x3800, 0x3800, 0x0000, 0x0000, 0x0000 };

	/* low res, upper left arc and center dot */
	static const short rbu_8x8[] = { 0x3800, 0x4400, 0xB800, 0xB800, 0xB800, 0x0000, 0x0000, 0x0000 };

	/* low res, lower right arc */
	static const short rbl_8x8[] = { 0x0000, 0x0000, 0x0200, 0x0200, 0x0200, 0x4400, 0x3800, 0x0000 };

	/* med res (2:1 aspect ratio), full circle and center dot */
	static const short rbf_16x8[] = { 0x1FD0, 0x6018, 0x8784, 0x8844, 0x8784, 0x6018, 0x1FD0, 0x0000 };

	/* med res (2:1 aspect ratio), center dot */
	static const short rbc_16x8[] = { 0x0000, 0x0000, 0x0780, 0x0F40, 0x0780, 0x0000, 0x0000, 0x0000 };

	/* med res (2:1 aspect ratio), upper left arc and center dot */
	static const short rbu_16x8[] = { 0x1FD0, 0x7810, 0xD780, 0xCFC0, 0xC780, 0x4000, 0x0000, 0x0000 };

	/* med res, lower right arc */
	static const short rbl_16x8[] = { 0x0000, 0x0008, 0x000C, 0x000C, 0x001C, 0x1078, 0x1FD0, 0x0000 };

	/* high res, full circle, deselected center dot, 2D */
	static const short rbf_16x16[] = {
		0x0000, 0x03C0, 0x0C30, 0x1008, 0x2184, 0x2664, 0x4422, 0x4812,
		0x4812, 0x4422, 0x2664, 0x2184, 0x1008, 0x0C30, 0x03C0, 0x0000
	};

	/* high res, center dot */
	static const short rbc_16x16[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x0180, 0x07E0, 0x07E0, 0x0FF0,
		0x0FF0, 0x07E0, 0x07E0, 0x0180, 0x0000, 0x0000, 0x0000, 0x0000
	};

	/* high res, upper left arc and center dot, 3D */
	static const short rbu_16x16x3[] = {
		0x0000, 0x03C0, 0x0FF0, 0x1C18, 0x3180, 0x37E0, 0x67E0, 0x6FF0,
		0x6FF0, 0x67E0, 0x27E0, 0x3180, 0x0000, 0x0000, 0x0000, 0x0000
	};

	/* high res, lower right arc, 3D */
	static const short rbl_16x16x3[] = {
		0x0000, 0x0000, 0x0000, 0x0000, 0x000C, 0x0004, 0x0006, 0x0006,
		0x0006, 0x0006, 0x000C, 0x000C, 0x1C38, 0x0FF0, 0x03C0, 0x0000
	};

	char *string;						/* text beside the button */

	static const _WORD dmode[3] = { MD_REPLACE, MD_TRANS, MD_TRANS };	/* drawing mode: */

	_WORD i;
	_WORD flags;						/* object flags */
	_WORD x = pb->pb_x;
	_WORD y = pb->pb_y;
	_WORD pxy[8] = { 0, 0, 15 };
	_WORD ci[6] = { G_WHITE, G_WHITE, G_BLACK, G_WHITE, G_BLACK, G_WHITE} ;	/* colour indices for three steps */

	const short *t;						/* aux, for swapping pointers to bitmaps   */

	const short *rb[3];					/* pointers to selected bitmaps            */

	const short *bmu;
	const short *bml;
	const short *bmc;
	const short *bmf;					/* pointers to resolution-dependent bitmaps */

	MFDB smfdb;							/* source memory block definition */
	MFDB dmfdb;							/* destination memory block definition */

	bool do3d;							/* true if 3d effects should be employed */

	/* Define clipping area */

	xd_clip_on((GRECT *) & pb->pb_xc);

	/* Define text to display and object flags */

	flags = ((XUSERBLK *) (pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *) (pb->pb_parm))->ob_spec.free_string;

	/* Should 3d effects be drawn? */

	do3d = xd_is3dobj(flags);

	/* 
	 * Determine which masks to use, taking care of aspect ratio, etc. 
	 * Make a selection of four bitmaps, according to resolution
	 */

	if (xd_regular_font.ch < 16)		/* low resolution(s) */
	{
		smfdb.fd_h = 8;
		pxy[3] = 7;

		if (xd_desk.g_w < 2 * xd_desk.g_h)	/* low res */
		{
			bmu = rbu_8x8;
			bml = rbl_8x8;
			bmc = rbc_8x8;
			bmf = rbf_8x8;
		} else							/* med res */
		{
			bmu = rbu_16x8;
			bml = rbl_16x8;
			bmc = rbc_16x8;
			bmf = rbf_16x8;
		}
	} else								/* other (high) resolution(s) */
	{
		smfdb.fd_h = 16;
		pxy[3] = 15;
		bmu = rbu_16x16x3;
		bml = rbl_16x16x3;
		bmc = rbc_16x16;
		bmf = rbf_16x16;
	}

	/* 
	 * If 3d capabilities are not present, do not use shaded arc segments,
	 * but instead use the full circle and centre dot
	 */

	if (do3d)
	{
		rb[0] = bmu;					/* upper left arc */
		rb[1] = bml;					/* lower right arc */
	} else
	{
		rb[0] = bmc;					/* center dot  */
		rb[1] = bmf;					/* full circle */
	}

	/* 
	 * If there are not enough colours for 3d, or if a black outline
	 * should be used, use full circle
	 */

	if (!(xd_colaes && do3d) || (do3d && IS_ACT(flags)))
		rb[2] = bmf;					/* full circle */
	else
		rb[2] = bmc;					/* center dot  */

	smfdb.fd_stand = 0;
	smfdb.fd_nplanes = 1;
	smfdb.fd_w = 16;
	smfdb.fd_wdwidth = 1;
	dmfdb.fd_addr = NULL;
	pxy[4] = x;
	pxy[5] = y;
	pxy[6] = x + pxy[2];
	pxy[7] = y + pxy[3];

	/* If selected, first draw the second chosen bitmap (swap places) */

	ci[4] = 1;

	if (pb->pb_currstate & OS_SELECTED)
	{
		t = rb[0];
		rb[0] = rb[1];
		rb[1] = t;
	} else
	{
		if (xd_colaes && do3d)
			ci[4] = 0;
	}

	ci[1] = xd_bg_col;
	ci[2] = xd_sel_col;

	/* Draw three segments of the button */

	for (i = 0; i < 3; i++)
	{
		smfdb.fd_addr = (void *) (long) rb[i];
		vrt_cpyfm(xd_vhandle, dmode[i], pxy, &smfdb, &dmfdb, &ci[2 * i]);
	}

	/* Write the text, always transparent, beside the button */

	set_textdef();
	prt_text(string, x + (5 * xd_regular_font.cw) / 2, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/* 
 * Code for drawing progdefined rectangular "checkbox" button 
 */

static _WORD cdecl ub_rectbut(PARMBLK *pb)
{
	_WORD x = pb->pb_x, y = pb->pb_y, flags;
	char *string;
	GRECT size;

	/* Define clipping area */

	xd_clip_on((GRECT *) & pb->pb_xc);

	/* Define text to display and object flags */

	flags = ((XUSERBLK *) (pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *) (pb->pb_parm))->ob_spec.free_string;

	/* 
	 * Define object size and position. Height of the object is
	 * 3/4 of character height + 2 pixels + 3D enlargements
	 */

	size.g_x = x;							/* intentionally without "+ aes_hor3d" here */
	size.g_y = y + xd_regular_font.ch / 8 + aes_ver3d - 1;
	size.g_h = pb->pb_h - xd_regular_font.ch / 4 - 2 * aes_ver3d + 2;

	if (xd_desk.g_w > 2 * xd_desk.g_h)
		size.g_w = 2 * size.g_h;			/* better looking in ST-medium */
	else
		size.g_w = size.g_h;

	/* Draw object box. Specified size will be enlarged as appropriate */

	xd_drawbox(&size, flags, pb->pb_currstate, XD_RECTBUT);

	/* Draw additional graphic elements - "X" diagonal lines if selected */

	if (pb->pb_currstate & OS_SELECTED)
	{
		_WORD pxy[8];
		_WORD *pxyp = pxy;
		_WORD b = 0;

		if (xd_is3dobj(flags))
			b = 1;

		*pxyp++ = x + b;				/* x = size.x */
		*pxyp++ = size.g_y + b;
		*pxyp++ = size.g_x + size.g_w - 1 - b;
		*pxyp++ = size.g_y + size.g_h - 1 - b;
		*pxyp++ = pxy[2];
		*pxyp++ = pxy[1];
		*pxyp++ = pxy[0];
		*pxyp = pxy[3];

		set_linedef(G_BLACK);
		v_pline(xd_vhandle, 2, pxy);
		v_pline(xd_vhandle, 2, &pxy[4]);
	}

	/* Draw object text beside the box, always in transparent mode */

	set_textdef();
	prt_text(string, x + 3 * xd_regular_font.cw, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/*
 * Code which handles scrolled editable text fields 
 */

static _WORD cdecl ub_scrledit(PARMBLK *pb)
{
	XUSERBLK *blk = (XUSERBLK *) pb->pb_parm;
	TEDINFO *ted = blk->ob_spec.tedinfo;
	char s[XD_MAX_SCRLED + 1];
	char *text = s;
	char *save = ted->te_ptext;			/* pointer to editable text field */

	_WORD i;							/* counter for padding with "_"s */
	_WORD tmode;						/* transparent or replace */
	_WORD b;								/* border thickness */
	_WORD x = pb->pb_x;
	_WORD y = pb->pb_y;
	_WORD w = pb->pb_w;
	_WORD h = pb->pb_h;
	_WORD xl, xr;						/* positions of < > markers */
	_WORD tw = (_WORD) strlen(save);		/* length of text in the field */
	_WORD ow = (_WORD) strlen(ted->te_pvalid);	/* length of validation field */

	GRECT size;							/* size of the text box */
	GRECT cb;							/* character-sized blanking rectange */

	ted->te_ptext = s;
	b = -ted->te_thickness;				/* border to be positive to outside */

	/* Define clipping area */

	xd_clip_on((GRECT *) & pb->pb_xc);

	/* Calculate some positions... */

	y += (h - xd_regular_font.ch - 1) / 2;
	xl = x - xd_regular_font.cw - 3;
	xr = x + w + 3;

	/* 
	 * If Magic, in colour & 3d mode, is hopefully detected, 
	 * create a background box; for other 3D AES, just clear the area
	 * and set transparent mode
	 */

	size.g_x = x - 1 - b;
	size.g_y = y - 1 - b;
	size.g_w = w + 2 + 2 * b;
	size.g_h = h + 1 + 2 * b;

	/* This is an attempt to get rid of the white text background */

	tmode = MD_REPLACE;

	if (xd_colaes)
	{
		if (aes_hor3d == 0 && ted->te_thickness != 0 && aes_version == 0x399)
		{
			/* hopefully this branch is valid for Magic only */
			xd_drawbox(&size, AES3D_1, OS_SELECTED, XD_SCRLEDIT);
		} else
		{
			/* other "gray background" AESes V4 */
			clr_object(&size, xd_bg_col, -1);
			tmode = MD_TRANS;
		}
	}

	/* Define a character-sized blanking rectangle in background colour */

	cb.g_y = y;
	cb.g_w = xd_regular_font.cw;
	cb.g_h = xd_regular_font.ch;

	/* Set default text parameters and transparent writing mode */

	set_textdef();

	/* 
	 * Put or remove marks that part of the text is not visible 
	 * in the field ( characters < and/or > )
	 */

	if (blk->uv.ob_shift > 0)
	{
		prt_text("<", xl, y, 0);
	} else
	{
		xd_vswr_repl_mode();
		cb.g_x = xl;
		clr_object(&cb, xd_bg_col, -1);
	}

	if (tw - blk->uv.ob_shift > ow)
	{
		xd_vswr_trans_mode();
		prt_text(">", xr, y, 0);
	} else
	{
		xd_vswr_repl_mode();
		cb.g_x = xr;
		clr_object(&cb, xd_bg_col, -1);
	}

	/* Copy visible part of the text to display */

	strsncpy(s, save + blk->uv.ob_shift, min(tw, ow) + 1);

	/* Pad space after the string with "_"s */

	i = ow - (tw - blk->uv.ob_shift);

	if (i > 0)
	{
		text += tw - blk->uv.ob_shift;
		while (i--)
			*text++ = '_';
		*text = 0;
	}

	/* Write the text */

	vswr_mode(xd_vhandle, tmode);
	prt_text(s, x, y, pb->pb_currstate);

	/* Set clipping off */

	xd_clip_off();

	ted->te_ptext = save;
	return 0;
}


/*
 * Code for drawing a progdefined rectangular button with an underlined text
 */

static _WORD cdecl ub_button(PARMBLK *pb)
{
	char *string;
	_WORD x, y, flags, offset = 0;

	/* Define clipping area */

	xd_clip_on((GRECT *) &pb->pb_xc);

	/* Define text to display and object flags */

	flags = ((XUSERBLK *) (pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *) (pb->pb_parm))->ob_spec.free_string;

	/* Draw object box. No change in object size or position */

	xd_drawbox((GRECT *) & pb->pb_x, flags, pb->pb_currstate, XD_BUTTON);

	/* Define text position and writing mode */

	set_textdef();

	if (pb->pb_currstate & OS_SELECTED)
	{
		if (xd_is3dobj(flags))
			offset = 1;
		else
			vswr_mode(xd_vhandle, MD_XOR);
	}

	x = pb->pb_x + (pb->pb_w - (_WORD) xd_strlen(string) * xd_regular_font.cw) / 2 + offset;
	y = pb->pb_y + (pb->pb_h - xd_regular_font.ch) / 2 + offset;

	/* Write button text */

	prt_text(string, x, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/*
 * Code for drawing a progdefined frame with a title on the contour.
 * Basically, a 3D box is drawn, a part of the contour is cleared with the 
 * background colour, and then the text is placed there.
 */

static _WORD cdecl ub_rbutpar(PARMBLK *pb)
{
	_WORD x, y;							/* text position */
	_WORD dh;							/* height change */
	_WORD flags;						/* state flags */
	const _WORD gap = 2;				/* distance between the frame and the text (pixels) */
	char *string;						/* text to be written */
	GRECT size;							/* object frame to be drawn */

	/* Define clipping area */

	xd_clip_on((GRECT *) & pb->pb_xc);

	/* Get object flags and title text */

	flags = ((XUSERBLK *) (pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *) (pb->pb_parm))->ob_spec.free_string;

	/* Define object area */

	dh = xd_regular_font.ch / 2;
	size.g_x = pb->pb_x;
	size.g_y = pb->pb_y + dh;
	size.g_w = pb->pb_w;
	size.g_h = pb->pb_h - dh;

	/* Draw object box */

	xd_drawbox(&size, flags, pb->pb_currstate, XD_RBUTPAR);

	/* Define position of title text */

	x = pb->pb_x + xd_regular_font.cw;
	y = pb->pb_y;

	/* Clear the area for text */

	xd_vswr_repl_mode();

	size.g_x = x - gap;
	size.g_y = y - 1;
	size.g_w = (_WORD) xd_strlen(string) * xd_regular_font.cw + 2 * gap;
	size.g_h = xd_regular_font.ch + 2;

	clr_object(&size, xd_bg_col, -1);

	/* Draw text */

	set_textdef();
	prt_text(string, x, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();
	return 0;
}


/* 
 * Code for drawing a progdefined "underlined title" object.
 * With few colours available, or if there is no 3D AES present,
 * just draw the text with a line below it. Else, draw a box with
 * 3D effects and put the text inside it.
 * Note: this should be improved a bit. The box always displays
 * a little wider then other box objects in the dialog. 
 * What should be done is to move the text a little to the right
 * (but only if a box is drawn), and not increase object width.
 */

static _WORD cdecl ub_title(PARMBLK *pb)
{
	_WORD dx, pxy[4], flags;
	char *string;
	GRECT size;

	/* Define clipping area */

	xd_clip_on((GRECT *) &pb->pb_xc);

	/* Get object flags and text to write */

	flags = ((XUSERBLK *) (pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *) (pb->pb_parm))->ob_spec.free_string;

	/* Draw a box (or an underline, if there are too few colours) */

	if (xd_colaes && xd_is3dobj(flags))
	{
		dx = 2;
		size.g_x = pb->pb_x;
		size.g_y = pb->pb_y - 1;
		size.g_w = pb->pb_w;
		size.g_h = pb->pb_h;
		xd_drawbox(&size, flags, pb->pb_currstate, XD_TITLE);
	} else
	{
		dx = 0;
		pxy[0] = pb->pb_x;
		pxy[3] = pxy[1] = pb->pb_y + pb->pb_h;
		pxy[2] = pxy[0] + pb->pb_w - 1;

		xd_vswr_repl_mode();
		set_linedef(G_BLACK);
		v_pline(xd_vhandle, 2, pxy);
	}

	/* Draw title text in blue, then turn back to black */

	set_textdef();
	vst_color(xd_vhandle, G_LBLUE);
	prt_text(string, pb->pb_x + dx, pb->pb_y + (pb->pb_h - xd_regular_font.ch - 1) / 2, pb->pb_currstate);
	vst_color(xd_vhandle, G_BLACK);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/*
 * Code for (not) drawing a rectangle for an unknown object type
 */
#if 0
static _WORD cdecl ub_unknown(PARMBLK *pb)
{
	/* this would be a mistake anyway, so why draw anything at all ? */

	GRECT *clip = (GRECT *) &pb->pb_xc;
	GRECT *frame = (GRECT *) & pb->pb_x;

	xd_clip_on(clip);

	set_linedef(G_BLACK);
	xd_vswr_repl_mode();
	clr_object(frame, 0, -1);
	draw_frame(frame, 0, 0);

	xd_clip_off();
	return 0;
}
#endif


/********************************************************************
 *																	*
 * Funkties voor het tekenen van de dialoogbox.						*
 *																	*
 ********************************************************************/

/*
 * Funktie voor het berekenen van de positie en grootte van de cursor.
 */

static void xd_calc_cursor(XDINFO *info, GRECT *cursor)
{
	objc_offset(info->tree, info->edit_object, &cursor->g_x, &cursor->g_y);

	cursor->g_x += xd_abs_curx(info->tree, info->edit_object, info->cursor_x) * xd_regular_font.cw;

#if 0									/* A slightly smaller cursor looks better in Magic */
	cursor->g_y -= 1;
	cursor->g_w = 1;
	cursor->g_h = xd_regular_font.ch + 2;
#endif
	cursor->g_w = 1;
	cursor->g_h = xd_regular_font.ch;
}


/*
 * Funktie voor het tekenen van de tekst cursor
 */

static void xd_credraw(XDINFO *info, GRECT *area)
{
	if (cursor_mfdb.fd_addr == NULL)
	{
		cursor_mfdb.fd_h = xd_regular_font.ch + 4;
		cursor_mfdb.fd_nplanes = xd_nplanes;
		cursor_mfdb.fd_addr = xd_malloc((long) cursor_mfdb.fd_wdwidth *
										(long) cursor_mfdb.fd_h * (long) xd_nplanes * 2L);
	}

	if (cursor_mfdb.fd_addr != NULL)
	{
		GRECT cursor, r;
		MFDB smfdb;
		_WORD pxy[8];
		_WORD *pxyp = pxy;

		xd_calc_cursor(info, &cursor);
		smfdb.fd_addr = NULL;

		if (xd_rcintersect(area, &cursor, &r))
		{
			/* Save area below cursor. */

			*pxyp++ = r.g_x;
			*pxyp++ = r.g_y;
			*pxyp++ = r.g_x + r.g_w - 1;
			*pxyp++ = r.g_y + r.g_h - 1;
			*pxyp++ = r.g_x - cursor.g_x;
			*pxyp++ = r.g_y - cursor.g_y;
			*pxyp++ = pxy[4] + r.g_w - 1;
			*pxyp = pxy[5] + r.g_h - 1;

			vro_cpyfm(xd_vhandle, S_ONLY, pxy, &smfdb, &cursor_mfdb);

			/* Draw cursor. */

			xd_clip_on(&r);
			xd_vswr_repl_mode();
			set_linedef(G_BLACK);

			pxy[0] = pxy[2] = cursor.g_x;
			pxy[1] = cursor.g_y;
			pxy[3] = cursor.g_y + cursor.g_h - 1;

			v_pline(xd_vhandle, 2, pxy);
			xd_clip_off();
		}
	}
}


/*
 * Funktie voor het wissen van de cursor.
 */

static void xd_cur_remove(XDINFO *info)
{
	if (cursor_mfdb.fd_addr != NULL)
	{
		GRECT cursor, r1, r2;
		MFDB dmfdb;
		_WORD pxy[8];
		_WORD *pxyp = pxy;

		xd_calc_cursor(info, &cursor);
		xd_mouse_off();
		dmfdb.fd_addr = NULL;

		if (info->dialmode != XD_WINDOW)
		{
			xd_clip_on(&cursor);

			*pxyp++ = 0;
			*pxyp++ = 0;
			*pxyp++ = cursor.g_w - 1;
			*pxyp++ = cursor.g_h - 1;
			*pxyp++ = cursor.g_x;
			*pxyp++ = cursor.g_y;
			*pxyp++ = cursor.g_x + pxy[2];
			*pxyp = cursor.g_y + pxy[3];

			vro_cpyfm(xd_vhandle, S_ONLY, pxy, &cursor_mfdb, &dmfdb);
			xd_clip_off();
		} else
		{
			xw_getfirst(info->window, &r1);

			while (r1.g_w != 0 && r1.g_h != 0)
			{
				if (xd_rcintersect(&r1, &cursor, &r2))
				{
					xd_clip_on(&cursor);

					pxyp = pxy;

					*pxyp++ = r2.g_x - cursor.g_x;
					*pxyp++ = r2.g_y - cursor.g_y;
					*pxyp++ = pxy[0] + r2.g_w - 1;
					*pxyp++ = pxy[1] + r2.g_h - 1;
					*pxyp++ = r2.g_x;
					*pxyp++ = r2.g_y;
					*pxyp++ = r2.g_x + r2.g_w - 1;
					*pxyp = r2.g_y + r2.g_h - 1;

					vro_cpyfm(xd_vhandle, S_ONLY, pxy, &cursor_mfdb, &dmfdb);

					xd_clip_off();
				}

				xw_getnext(info->window, &r1);
			}
		}

		xd_mouse_on();
	}
}


/* 
 * Funktie voor het tekenen van een dialoogbox in een window 
 */

void xd_redraw(XDINFO *info, _WORD start, _WORD depth, GRECT *area, _WORD flags)
{
	GRECT r1, r2, cursor;
	_WORD draw_cur;
	OBJECT *tree = info->tree;

	xd_mouse_off();

	if ((flags & XD_RCURSOR) && (info->edit_object > 0))
	{
		xd_calc_cursor(info, &cursor);
		draw_cur = xd_rcintersect(area, &cursor, &cursor);
	} else
	{
		draw_cur = FALSE;
	}
	
	if (info->dialmode != XD_WINDOW)
	{
		if (flags & XD_RDIALOG)
			objc_draw(tree, start, depth, area->g_x, area->g_y, area->g_w, area->g_h);

		if (draw_cur)
			xd_credraw(info, &cursor);
	} else
	{
		xw_getfirst(info->window, &r1);

		while (r1.g_w != 0 && r1.g_h != 0)
		{
			if ((flags & XD_RDIALOG) && xd_rcintersect(&r1, area, &r2))
				objc_draw(tree, start, depth, r2.g_x, r2.g_y, r2.g_w, r2.g_h);

			if (draw_cur && xd_rcintersect(&r1, &cursor, &r2))
				xd_credraw(info, &r2);

			xw_getnext(info->window, &r1);
		}
	}

	xd_mouse_on();
}


/* 
 * Funkties voor het aan en uitzetten van de cursor 
 */

void xd_cursor_on(XDINFO *info)
{
	info->curs_cnt -= 1;

	if (info->curs_cnt == 0)
		xd_redraw(info, 0, MAX_DEPTH, &info->drect, XD_RCURSOR);
}


void xd_cursor_off(XDINFO *info)
{
	info->curs_cnt += 1;

	if (info->curs_cnt == 1)
		xd_cur_remove(info);
}


/* 
 * Funktie voor het tekenen van een dialoogbox of een deel daarvan 
 */

void xd_draw(XDINFO *info, _WORD start, _WORD depth)
{
	xd_begupdate();
	xd_cursor_off(info);
	xd_redraw(info, start, depth, &info->drect, XD_RDIALOG);
	xd_cursor_on(info);
	xd_endupdate();
}


/*
 * A shorter form of the above, draw with all children.
 */

void xd_drawdeep(XDINFO *info, _WORD start)
{
	xd_draw(info, start, MAX_DEPTH);
}


/*
 * Similar, but draw this level only
 */

void xd_drawthis(XDINFO *info, _WORD start)
{
	xd_draw(info, start, 0);
}



/********************************************************************
 *																	*
 * Funktie voor het veranderen van de status van een object.		*
 *																	*
 ********************************************************************/

/*
 * Do what is necessary to change state of an object on the screen
 */

void xd_change(XDINFO *info, _WORD object, _WORD newstate, _WORD draw)
{
	OBJECT *tree = info->tree;
	_WORD twostates;

	if (object < 0)
		return;

	twostates = (newstate & 0xff) | (tree[object].ob_state & (0xff00 | OS_WHITEBAK));	/* preserve extended states */

	if (info->dialmode != XD_WINDOW)
	{
		objc_change(tree, object, 0, info->drect.g_x, info->drect.g_y, info->drect.g_w, info->drect.g_h, twostates, draw);
	} else
	{
		tree[object].ob_state = twostates;

		if (draw)
			xd_drawthis(info, object);	/* draw only this object */
	}
}


/*
 * Set a button to OS_NORMAL without drawing it
 */

void xd_buttnorm(XDINFO *info, _WORD button)
{
	xd_change(info, button, OS_NORMAL, 0);
}


/*
 * Set a button to OS_NORMAL and draw it
 */

void xd_drawbuttnorm(XDINFO *info, _WORD button)
{
	xd_change(info, button, OS_NORMAL, 1);
}


/********************************************************************
 *																	*
 * Funktie ter vervanging van rsrc_gaddr.							*
 *																	*
 ********************************************************************/

/* 
 * Funktie voor het verschuiven van de kinderen van een object.
 * Vertically move all child objects inside the parent. 
 * Offset is positive downwards.
 */

static void xd_translate(OBJECT *tree, _WORD parent, _WORD offset)
{
	_WORD i = tree[parent].ob_head;

	while ((i >= 0) && (i != parent))
	{
		tree[i].ob_y += offset;
		i = tree[i].ob_next;
	}
}


/*
 * Try to determine if the AES supports required object features,
 * It is assumed that if WHITEBAK is set, and AES does not support WHITEBAK,
 * then must_userdef returns true. 
 * Note: this detects only Magic-style capabilities, but -not- those
 * of e.g. Geneva.
 * If own_userdef = 1, all objects will be drawn by TeraDesk.
 * Unfortunately, there is currently no easy way to put it somewhere in the
 * configuration file, because this data is needed before the file is read.
 */

static bool own_userdef = 0;			/* =1 if Teradesk will always draw all userdef objects */

static bool must_userdef(OBJECT *ob)
{
	if (!own_userdef)
	{
		if (!(aes_flags & GAI_WHITEBAK))
			return TRUE;

		if (ob->ob_state & OS_WHITEBAK)
			return FALSE;
	}

	return TRUE;
}


/*
 * Specify that xdialog will always draw all extended object types by itself,
 * whether an AES can support them or not. sometimes they may look nicer that way.
 * 1= force xdialog to draw all; 0= let AES do it. This could be
 * used elsewhere in TeraDesk to set how the dialogs should be drawn-
 * but currently there is no way to store that information, because
 * the resource is initialized before the configuration file is read.
 */

void xd_own_xobjects(bool setit)
{
	own_userdef = setit;
}


/*
 * Function for counting user-defined objects 
 * (space must be allocated for them later, so need to know the amount).
 *
 * Parameters:
 *
 * tree		- Pointer to the object tree.
 * n		- Pointer to a variable in which the number of
 *			  user-defined objects is placed.
 * nx		- Pointer to a variable in which the number of
 *			  extended user-defined objects is placed.
 *
 * Result	: Total number of user-defined objects (*n + *nx).
 */

static _WORD cnt_user(OBJECT *tree, _WORD *n, _WORD *nx)
{
	OBJECT *object = tree;
	_WORD etype;

	*n = 0;
	*nx = 0;

	for (;;)
	{
		etype = xd_xobtype(object);

#if 0									/* with a good resource file there is no need for this test */
		if (xd_is_xtndelement(etype) && ((object->ob_type & 0xFF) != G_USERDEF))
#endif

			if (xd_is_xtndelement(etype))
			{
				switch (etype)
				{
				case XD_DRAGBOX:	/* dragbox "ear" on a dialog */
				case XD_BCKBOX:		/* background box */
				case XD_SCRLEDIT:	/* scrolled text field */
				case XD_FONTTEXT:	/* sample text in font selector */
					(*nx)++;		/* always userdef, ignore AESses which may support this (none?) */
					break;
				case XD_RECTBUT:	/* checkbox button */
				case XD_ROUNDRB:	/* round radio button */
				case XD_TITLE:		/* title */
				case XD_RBUTPAR:	/* titled fram */
				case XD_BUTTON:		/* rectangular button with text */
					if (must_userdef(object))
						(*nx)++;	/* userdef only if unsuported by AES */
					break;
				default:
					(*n)++;			/* not using extended parameter blocks */
					break;
				}
			}

		if (object->ob_flags & OF_LASTOB)
			return (*n + *nx);

		object++;
	}
}


/* 
 * Funktie voor het zetten van de informatie van userdefined objects.
 * HR 151102: Dont set the progdefs if the AES supports the object types. 
 * This routine defines executable code for drawing diverse userdef objects.
 * It also performs minor object size adjustments.
 */

void xd_set_userobjects(OBJECT *tree)
{
	_WORD etype;						/* extended object type */
	_WORD n;							/* object count */
	_WORD nx;
	_WORD d;
	_WORD hmin;							/* minimum object size */
	_WORD object = 0;					/* object index */
	_WORD xuserblk;						/* true if extended parameterblock is used */
	_WORD newflags;						/* remember object flags */
	OBJECT *c_obj;						/* pointer to analyzed object */
	USERBLK *c_ub;
	XDOBJDATA *data;
	PARMBLKFUNC c_code;

	/* If there is nothing to do, return (count progdefined objects) */

	if (cnt_user(tree, &n, &nx) == 0)
		return;

	/* Allocate memory for objects; return from this routine if failed */

	if ((data = xd_malloc(sizeof(XDOBJDATA) + sizeof(USERBLK) * (long) n + sizeof(XUSERBLK) * (long) nx)) == NULL)
		return;

	data->next = xd_objdata;
	xd_objdata = data;
	c_ub = (USERBLK *) &data[1];

	/* Scan all objects in the resource to find the appropriate one */

	for (;;)
	{
		c_code = 0;						/* no pointer to user code */
		c_obj = tree + object;			/* object index */
		xuserblk = FALSE;				/* no extended block */
		etype = xd_xobtype(c_obj);		/* extended type */

#if 0									/* with a correct resource file there is no need for this test */
		if (xd_is_xtndelement(etype) && ((c_obj->ob_type & 0xFF) != G_USERDEF))
#endif
			if (xd_is_xtndelement(etype))
			{
				/*
				 * It seems that AESes are not consistent in treating progdef objects 
				 * if they are flagged as "indicator" or "activator"; some seem to 
				 * draw 3d effect anyway (AES4.1, NAES), and some do not (Geneva?). 
				 * Therefore, those flags are here saved in the extended userblock  
				 * and then disabled so that only user code will draw 3d effects.
				 */

				newflags = c_obj->ob_flags & ~(AES3D_1 | AES3D_2);
				xuserblk = TRUE;		/* but only if AES does not support this type */

				switch (etype)
				{
				case XD_DRAGBOX:
					/* Dragbox "ear" on the dialogs; always userdefined */
					c_code = ub_drag;
					break;
				case XD_ROUNDRB:
					/* Round radio button */
					if (must_userdef(c_obj))
						c_code = ub_roundrb;
					break;
				case XD_RECTBUT:
					/* Rectangular "checkbox" button */
					if (must_userdef(c_obj))
						c_code = ub_rectbut;
					break;
#if 0 /* Currently not used anywhere in Teradesk */
			   case XD_RECTBUTTRI :
				   if (must_userdef(c_obj))
					   c_code = ub_rectbuttri;
				   break;
			   case XD_CYCLBUT :
				   c_code = ub_cyclebut;
				   break;
#endif
				case XD_BUTTON:
					/* 
					 * Rectangular button with (possibly) underlined text;   
					 * This object should have a minimum height in order   
					 * to accomodate the underline inside the button.
					 * Modification should affect both userdefined and 
					 * AES-supported objects.
					 */
					hmin = xd_regular_font.ch + 2;	/* minimum height */

					if (xd_has3d && (aes_ver3d == 0))	/* 3D but without enlargements */
						hmin += 2;

					if (c_obj->ob_height < hmin)
					{
						c_obj->ob_y -= (hmin - c_obj->ob_height) / 2;
						c_obj->ob_height = hmin;
					}

					if (must_userdef(c_obj))
						c_code = ub_button;

					break;
				case XD_RBUTPAR:
					/* 
					 * Frame with a title on the upper contour;
					 * Increase object height slightly, because
					 * the upper side will always be drawn about
					 * 1/2 character heights inside the object;
					 * without increasing object size, the upper
					 * edge would be too close to objects  
					 * inside. This affects both userdefined and
					 * AES-supported objects.
					 */
					d = xd_regular_font.ch / 2;

					c_obj->ob_y -= d;
					c_obj->ob_height += d;
					xd_translate(tree, object, d);

					if (must_userdef(c_obj))
						c_code = ub_rbutpar;

					break;
				case XD_TITLE:
					/* 
					 * Dialog title; object height must always be
					 * increased a little bit, for better look.
					 * E.g. otherwise, in magic/low-res/colour, the
					 * underline gets too close to the text
					 * (in fact, it is drawn OVER the text)
					 */

					if (must_userdef(c_obj))
						c_code = ub_title;

					d = (xd_colaes && (aes_ver3d == 0)) ? 3 : 1;
					c_obj->ob_height += 2 * d;
					c_obj->ob_y -= d + 1;
					break;
				case XD_SCRLEDIT:
					/* Scrolled editable text; always userdefined */
					c_code = ub_scrledit;
					break;
				case XD_FONTTEXT:
					/* 
					 * Sample text in the font selector dialog;
					 * ub_drag is replaced later by a pointer to
					 * actual code; see font.c
					 */

					c_code = ub_drag;	/* does not matter, so use any existing one */
					break;
				case XD_BCKBOX:
					/* window background box. Always userdefined  */

					if (c_obj->ob_flags & AES3D_1)
					{
						c_obj->ob_y -= 1;
						c_obj->ob_height += 2;
					}

					c_code = ub_bckbox;
					break;
#if 0									/* this should never happen */
				default:
					/* Yet unknown userdef; this should never happen! */
					c_code = ub_unknown;
					xuserblk = FALSE;
					break;
#endif
				}

				if (c_code)
				{
					if (xuserblk)
					{
						XUSERBLK *c_xub = (XUSERBLK *) c_ub;

						xd_xuserdef(c_obj, c_xub, c_code);
						c_ub = (USERBLK *) (c_xub + 1);
						c_obj->ob_flags = newflags;
					} else
					{
						xd_userdef(c_obj, c_ub, c_code);
						c_ub++;
					}
				}

			}

		if (c_obj->ob_flags & OF_LASTOB)
			return;

		object++;
	}
}


/* 
 * Obtain address of an object from the R_TREE group
 * Call this routine only once for each object 
 */

_WORD xd_gaddr(_WORD iindex, void *addr)
{
	_WORD result;

	if ((result = rsrc_gaddr(R_TREE, iindex, addr)) != 0)
		xd_set_userobjects(*(OBJECT **) addr);

	return result;
}


/* 
 * Specify text buffer for scolled-text objects. 
 * Clear the text in the buffer (zero first character only)
 */

char *xd_set_srcl_text(OBJECT *tree, _WORD item, char *txt)
{
	TEDINFO *ted = xd_get_obspecp(tree + item)->tedinfo;

	ted->te_ptext = txt;
	*txt = 0;
	return txt;
}


void xd_fixtree(OBJECT *tree)
{
	_WORD i = 0;

	for (;;)
	{
		rsrc_obfix(tree, i);

		if (tree[i].ob_flags & OF_LASTOB)
			break;

		i++;
	}
}
