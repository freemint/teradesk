/* 
 * Xdialog Library. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                                      2002, 2003  H. Robbers,
 *                                      2003, 2004  Dj. Vukovic
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


#ifdef __PUREC__
 #include <np_aes.h>
 #include <vdi.h>
#else
 #include <aesbind.h>
 #include <vdibind.h>
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <library.h>

#include "xdialog.h"
#include "internal.h"



extern RECT 
	xd_desk;				/* Dimensions of desktop background. */

extern int 
	xd_has3d,
	xd_bg_col,
	xd_ind_col,
	xd_act_col,
	xd_sel_col;

static MFDB cursor_mfdb = {	NULL, 1, 0, 1, 0, 0, 0, 0, 0 };


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

int xd_get_3d_color(int flags)
{
	int color;

	if (IS_ACT(flags))
		color = xd_act_col;
	else if (IS_IND(flags))
		color = xd_ind_col;
	else if (IS_BG(flags))
		color = xd_bg_col;
	else
		color = WHITE;

	return (color >= xd_ncolors) ? WHITE : color;
}



/*
 * Obtain length of the string subtracting "#" characters
 */

static long xd_strlen(char *s)
{
	long l;

	l = strlen(s);

/* This capability is currently not used anywhere in Teradesk resource

	if (strchr(s, '#') != NULL)
		l--;
*/

	return l;
}


#define prt_text(s, x, y, state)  prt_xtndtext(s, x, y, state, 0)

static void prt_xtndtext(char *s, int x, int y, int state, int attrib)
{
	char 
		tmp[80], /* <- buffer is enlarged! (DjV: why so much?) */
		*h, 
		*p = NULL,
		uc; /* underlined character */

	if (state & DISABLED)
		attrib ^= 2;

	vst_effects(xd_vhandle, attrib);

	h = strcpy(tmp, s); /* h is not needed but tmp is */

	/* uses AES 4 WHITEBAK */

	if (state & WHITEBAK)
	{
		int und = (state<<1)>>9;
		if (und >= 0)
		{
			und &= 0x7f;
			if (und < strlen(tmp))
				p = tmp + und;
		}
	}

/* This capability is currently not used anywhere in Teradesk

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
*/

	if ( p )
	{
		uc = *p;
		*p = ' ';	/* to better handle XOR writing */
	}

	v_gtext(xd_vhandle, x, y, tmp);

	if (p)
	{
		/* do underline some character! */

		int xtnd[8];

		*p = 0;

		/* request position of this character! */

		vqt_extent(xd_vhandle, tmp, xtnd);
		vst_effects(xd_vhandle, attrib ^ 8);	/* XOR due to text-style extensions! */
		tmp[0] = uc;
		tmp[1] = 0;
		v_gtext(xd_vhandle, x + (xtnd[2] - xtnd[0]), y, tmp);
	}
}


/*
 * Draw a rectangle
 */

static void draw_rect(int x, int y, int w, int h)
{
	int pxy[10];

	pxy[0] = x;
	pxy[1] = y;

	pxy[2] = x + w - 1;
	pxy[3] = y;

	pxy[4] = pxy[2];
	pxy[5] = y + h - 1;

	pxy[6] = x;
	pxy[7] = pxy[5];

	pxy[8] = x;
	pxy[9] = y;

	v_pline(xd_vhandle, 5, pxy);
}


/*
 * Draw a frame by drawing concentric rectangles to obtain 
 * desired thickness of border
 */

static void draw_frame(RECT *frame, int start, int eind)	
{
	int i, s, e;

	if (start > eind)
	{
		s = eind;
		e = start;
	}
	else
	{
		s = start;
		e = eind;
	}

	for (i = s; i <= e; i++)
		draw_rect
		(
			frame->x + i, 
			frame->y + i, 
			frame->w - 2 * i, 
			frame->h - 2 * i
		);
}


/*
 * Set suitable (default) line attributes. Full line, width 1 pixel.
 */

static void set_linedef(int color)
{
	vsl_color(xd_vhandle, color);
	vsl_ends(xd_vhandle, 0, 0);
	vsl_type(xd_vhandle, 1);
	vsl_width(xd_vhandle, 1);
}


/*
 * Set suitable (default) text attributes. Colour is always black.
 */

static void set_textdef(void)
{
	int dummy;

	vst_font(xd_vhandle, xd_regular_font.fnt_id);
	vst_rotation(xd_vhandle, 0);
	vst_alignment(xd_vhandle, 0, 5, &dummy, &dummy);
	vst_point(xd_vhandle, xd_regular_font.fnt_height, &dummy, &dummy, &dummy, &dummy);
	vst_color(xd_vhandle, BLACK);
}


/*
 * Clear an object by drawing a borderless filled rectangle  
 * of desired colour and pattern (except pattern -1, then draw solid)
 * Rectangle is drawn in replace mode.
 */

void clr_object(RECT *r, int color, int pattern)
{
	int pxy[4];

	if ( pattern < 0 )
		vsf_interior(xd_vhandle, FIS_SOLID);
	else
	{
		vsf_style(xd_vhandle, pattern);
		vsf_interior(xd_vhandle, FIS_PATTERN);
	}
	vsf_color(xd_vhandle, color);

	xd_rect2pxy(r, pxy);
	vswr_mode(xd_vhandle, MD_REPLACE);
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

static bool xd_is3dobj(int flags) 
{
	int f3d = flags & (AES3D_1 | AES3D_2);

	if ( 
			    f3d
			&& (f3d != AES3D_2)   
			&& (xd_aes4_0) 
			&& (xd_has3d || aes_hor3d > 0 || aes_ver3d > 0 || xd_ncolors > 4) 
		)
		return TRUE;
	
	return FALSE;
}


/*
 * Draw a box. Most progdefined objects in TeraDesk in fact require
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
 * border thickness is not recognized.
 * 
 * Box colour is determined from extended flags
 *
 * Clipping is NOT performed in this routine, must be set beforehand
 * 
 */

#define OUT_BRD 3 /* hard-coded border size for outlining the object */

static void xd_drawbox
(
	RECT *r,		/* size of the box      */
	int flags,		/* object flags relevant to drawing this box */
	int state,		/* object state         */
	int xtype		/* extended object type */
)
{
	RECT
		outsize;	/* external object dimensions (final) */

	int 
		hx,			/* horizontal enlargement  */
		vx,			/* vertical enlargement    */
		ox = 0,		/* outline enlargement     */
		border,		/* border thickness        */
		color,		/* current colour          */
		color2,		/* other (inverse) colour  */		
		pxy[10]; 	/* point array for drawing */


	/* 
 	 * Determine size of object 
	 * (are combinations of border and 3d enlargement really 
	 * to be implemented like below? not sure). 
	 */

	if ( state & OUTLINED )
		ox = OUT_BRD;			/* hardcoded outline width */
	hx = max(aes_hor3d, ox);	/* horizontal enlargement  */
	vx = max(aes_ver3d, ox);	/* vertical enlargement    */

	outsize.x = r->x - hx;
	outsize.y = r->y - vx;
	outsize.w = r->w + 2 * hx;
	outsize.h = r->h + 2 * vx;

	/* Writing mode is generally "replace" */

	vswr_mode(xd_vhandle, MD_REPLACE); 

	/* 
	 * Fill the area with appropriate colour (except for the scrolled text);
	 * For any object other than a button, it is always the background one
	 */

	if ( xd_is3dobj(flags) )
		color = xd_get_3d_color(flags);
	else
	{
		if ( (state & SELECTED) && (xtype == XD_BUTTON) )
			color = xd_sel_col;
		else
 			color = xd_bg_col; 
	}

	if ( xtype != XD_SCRLEDIT )
		clr_object(&outsize, color, -1);

	/* 
	 * Draw a black frame of appropriate thickness (going outside of object);
	 * This is drawn only for the button and the dragbox. For other object
	 * types the frame will be created later by drawing 3d effect only.
	 * On the other hand, if there is not a sufficient number of colours, 
	 * or if there is not a 3D AES, always draw the frame.
	 * Also draw the frame if the object is flagged as an activator.
	 */

	if ( 
		     xd_ncolors < 16 
		  || !xd_aes4_0 
		  || xtype == XD_BUTTON 
		  || xtype == XD_DRAGBOX 
		  || IS_ACT(flags)
	    )
	{
		if (flags & DEFAULT)
			border = 2; /* actual border thickness will be 3 pixels */
		else if (flags & EXIT)
			border = 1; /* actual border thickness will be 2 pixels */
		else
			border = 0; /* actual border thickness will be 1 pixel  */

		if ( xtype == XD_DRAGBOX )
			color = xd_sel_col;
		else
			color = BLACK;
		set_linedef(color);
		draw_frame( &outsize, 0, -border );
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

	if ( xd_is3dobj(flags) )
	{
		pxy[0] = outsize.x + outsize.w - 2;	/* upper right corner */
		pxy[1] = outsize.y + 1;

		pxy[2] = pxy[0];
		pxy[3] = outsize.y + outsize.h - 2;	/* lower right corner */

		pxy[4] = outsize.x + 1;				/* lower left corner  */
		pxy[5] = pxy[3];

		pxy[6] = pxy[4];					/* upper left corner  */
		pxy[7] = pxy[1];
		
		pxy[8] = pxy[0];					/* upper right corner */
		pxy[9] = pxy[1];

		if ( state & SELECTED )
		{
			color = WHITE;
			color2 = xd_sel_col;
		}
		else
		{
			color = xd_sel_col;
			color2 = WHITE;
		}

		set_linedef(color);
		v_pline(xd_vhandle, 3, pxy);		/* lower & right sides of rectangle */
		vsl_color(xd_vhandle, color2);
		v_pline(xd_vhandle, 3, &pxy[4]);	/* upper & left sides of rectangle */		
	}
}


/*
 * Code for drawing progdefined "dragbox" ear for dialogs
 */

static int cdecl ub_drag(PARMBLK *pb)
{
	int 
		dh,
		dv,
		pxy[6],
		b,
		c,
		ox = 0,
		flags;

	RECT 
		size; 

	OBSPEC 
		obspec = xd_get_obspec(&pb->pb_tree[pb->pb_obj]);


	/* Define clipping area */

	xd_clip_on( (RECT *)&pb->pb_xc );

	/* 
	 * Define object size and position (up-  and right-justify with dialog edge)
	 * Note: must compensate for ST-low/med res aspect ratio oddities
	 */

	if (pb->pb_tree[pb->pb_obj].ob_state & OUTLINED )
		ox = OUT_BRD;  /* hard-coded "outline" size */

	dh = max(ox, aes_hor3d); /* Is it really implemented so? not sure. */
	dv = max(ox, aes_ver3d); 

	if ( xd_regular_font.fnt_chh > 9 || xd_desk.w > 2 * xd_desk.h )
	{
		size.w = pb->pb_w;
		size.x = pb->pb_x;
	}
	else	/* ST-low res; must change aspect ratio */
	{
		size.w = pb->pb_w / 2;
		size.x = pb->pb_x + size.w;
	}
	size.y = pb->pb_y;
	size.h = pb->pb_h;

	vswr_mode(xd_vhandle, MD_REPLACE);

	/* Draw object box */

	flags = ((XUSERBLK *)(pb->pb_parm))->ob_flags;
	xd_drawbox( &size, flags, pb->pb_tree[pb->pb_obj].ob_state, XD_DRAGBOX );

	/* Draw "\" diagonal in the box */

	pxy[0] = size.x - dh;
	pxy[1] = size.y - dv;
	pxy[2] = size.x + size.w + dh - 1;
	pxy[3] = size.y + size.h + dv - 1;

	set_linedef(xd_sel_col);
	v_pline(xd_vhandle, 2, pxy);

	/* Draw something like a part of a border visible behind the "ear" */

	b = abs((int)obspec.obspec.framesize);	/* border thickness */ 

	c = (int)obspec.obspec.framecol;		/* border colour */

	if ( b != 0 )
	{
		int i;
		vsl_color(xd_vhandle, c);

		pxy[0] = size.x + 1;
		pxy[1] = size.y;
		pxy[2] = size.x + size.w - 1;
		pxy[5] = size.y + size.h - 2;

		/* Draw the border */

		for ( i = 0; i < b; i++ )
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
 * Code for drawing progdefined circular radiobutton
 */

static int cdecl ub_roundrb(PARMBLK *pb)
{
	/*
	 * Bitmasks for round radio buttons.
	 * There are bitmasks for low res, med res (i.e. different aspect ratio)
	 * and one common for all other (high) res.
	 * For each resolution the bitmasks are in three parts, to enable
	 * creation of 3d effects: upper left arc with centre circle, 
	 * lower right arc and centre circle. Button is drawn in three
	 * steps, the first one always being drawn in replace mode,
	 * the other two transparent. If 3d capability does not exist,
	 * button is drawn from other segments (full circle and centre dot)
	 */

	/* low res, full circle (deselected) */
	static const short rbf_8x8[] = { 0x3800, 
		     		           0x4400, 
	    	     		       0x8200, 
	        	               0x8200, 
	            	       	   0x8200,
	                   	       0x4400, 
	                           0x3800, 
	                           0x0000 };

	/* low res, center dot */
	static const short rbc_8x8[] = { 0x0000, 
	                           0x0000, 
	                           0x3800, 
	                           0x3800, 
	                           0x3800,
	                           0x0000, 
	                           0x0000, 
	                           0x0000 };

	/* low res, upper left arc and center dot */
	static const short rbu_8x8[] = { 0x3800, 
	                           0x4400, 
	                           0xB800, 
	                           0xB800, 
	                           0xB800,
	                           0x0000, 
	                           0x0000, 
	                           0x0000 };

	/* low res, lower right arc */
	static const short rbl_8x8[] = { 0x0000, 
	                           0x0000, 
	                           0x0200, 
	                           0x0200, 
	                           0x0200,
	                           0x4400, 
	                           0x3800, 
	                           0x0000 };

	/* med res (2:1 aspect ratio), full circle and center dot */
	static const short rbf_16x8[] = {0x1FD0, 
	                           0x6018, 
	                           0x8784, 
	                           0x8844, 
	                           0x8784,
	                           0x6018, 
	                           0x1FD0, 
	                           0x0000 };

	/* med res (2:1 aspect ratio), center dot */
	static const short rbc_16x8[] = {0x0000, 
	                           0x0000, 
	                           0x0780, 
	                           0x0F40, 
	                           0x0780,
	                           0x0000, 
	                           0x0000, 
	                           0x0000 };

	/* med res (2:1 aspect ratio), upper left arc and center dot */
	static const short rbu_16x8[] = { 0x1FD0, 
	                            0x7810, 
	                            0xD780, 
	                            0xCFC0, 
	                            0xC780,
	                            0x4000, 
	                            0x0000, 
	                            0x0000 };

	/* med res, lower right arc */
	static const short rbl_16x8[] = {0x0000, 
	                           0x0008, 
	                           0x000C, 
	                           0x000C, 
	                           0x001C,
	                           0x1078, 
	                           0x1FD0, 
	                           0x0000 };

	/* high res, full circle, deselected center dot, 2D */
	static const short rbf_16x16[] = { 0x0000, 
	                             0x03C0, 
	                             0x0C30,
	                             0x1008,
	                             0x2184,
	                             0x2664, 
	                             0x4422, 
	                             0x4812, 
	                             0x4812, 
	                             0x4422,
	                             0x2664, 
	                             0x2184, 
	                             0x1008, 
	                             0x0C30, 
	                             0x03C0,
						         0x0000 };

	/* high res, center dot */
	static const short rbc_16x16[] = { 0x0000, 
	                             0x0000, 
	                             0x0000, 
	                             0x0000, 
	                             0x0180,
	                             0x07E0, 
	                             0x07E0, 
	                             0x0FF0, 
	                             0x0FF0, 
	                             0x07E0,
	                             0x07E0, 
	                             0x0180, 
	                             0x0000, 
	                             0x0000, 
	                             0x0000,
	                             0x0000 };

	/* high res, upper left arc and center dot, 3D */ 
	static const short rbu_16x16x3[] = { 0x0000, 
	                               0x03C0, 
	                               0x0FF0,
	                               0x1C18,
	                               0x3180,
	                               0x37E0, 
	                               0x67E0, 
	                               0x6FF0,  
	                               0x6FF0, 
	                               0x67E0,
	                               0x27E0, 
	                               0x3180, 
	                               0x0000, 
	                               0x0000, 
	                               0x0000,
	                               0x0000 };

	/* high res, lower right arc, 3D */
	static const short rbl_16x16x3[] = { 0x0000, 
	                               0x0000, 
	                               0x0000, 
	                               0x0000, 
	                               0x000C,
	                               0x0004, 
	                               0x0006, 
	                               0x0006, 
	                               0x0006, 
	                               0x0006,
	                               0x000C, 
	                               0x000C, 
	                               0x1C38, 
	                               0x0FF0, 
	                               0x03C0,
	                               0x0000 };

	char
		*string;		/* text beside the button */

	bool
		do3d;			/* true if 3d effects should be employed */

	static const int
		dmode[3] = {MD_REPLACE,	MD_TRANS, MD_TRANS}; /* drawing mode: */

	int
		i, 
		flags,										 /* object flags */
		x = pb->pb_x, 
		y = pb->pb_y, 
		pxy[8] = {0, 0, 15},
		ci[6] = {WHITE, WHITE,   BLACK, WHITE,   BLACK, WHITE};	/* colour indices for three steps */

	void 
		*t,						/* aux, for swapping pointers to bitmaps   */
		*rb[3], 				/* pointers to selected bitmaps            */
		*bmu,*bml,*bmc,*bmf;	/* pointers to resolution-dependent bitmaps */

	MFDB
		smfdb,			/* source memory block definition */
		dmfdb;			/* destination memory block dfinition */


	/* Define clipping area */

	xd_clip_on((RECT *)&pb->pb_xc);

	/* Define text to display and object flags */

	flags = ((XUSERBLK *)(pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *)(pb->pb_parm))->ob_spec.free_string;

	/* Should 3d effects be drawn? */

	do3d = xd_is3dobj(flags);

	/* 
	 * Determine which masks to use, taking care of aspect ratio, etc. 
	 * Make a selection of four bitmaps, according to resolution
	 */

	if (xd_regular_font.fnt_chh < 16)	/* low resolution(s) */
	{
		smfdb.fd_h = 8;
		pxy[3] = 7;

		if ( xd_desk.w < 2 * xd_desk.h ) /* low res */
		{
			bmu = rbu_8x8;
			bml = rbl_8x8;
			bmc = rbc_8x8;
			bmf = rbf_8x8;
		}
		else /* med res */
		{
			bmu = rbu_16x8;
			bml = rbl_16x8;
			bmc = rbc_16x8;
			bmf = rbf_16x8;
		}
	}
	else								/* other (high) resolutuon(s) */
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

	if ( do3d )
	{
		rb[0] = bmu; /* upper left arc */
		rb[1] = bml; /* lower right arc */			
	}
	else
	{
		rb[0] = bmc;  /* center dot  */
		rb[1] = bmf;  /* full circle */
	}

	/* 
	 * If there are not enough colours for 3d, or if a black outline
	 * should be used, use full circle
	 */

	if ( xd_ncolors < 16 || !do3d || (do3d && IS_ACT(flags)) )
		rb[2] = bmf;  /* full circle */
	else
		rb[2] = bmc;  /* center dot  */

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
	if( pb->pb_currstate & SELECTED )
	{
		t = rb[0];
		rb[0] = rb[1];
		rb[1] = t;
	}
	else
	{
		if ( (xd_ncolors > 4) && do3d )
			ci[4] = 0;
	}
	ci[1] = xd_bg_col;
	ci[2] = xd_sel_col; 

	/* Draw three segments of the button */ 

	for ( i = 0; i < 3; i++ )
	{
		smfdb.fd_addr = rb[i];
		vrt_cpyfm(xd_vhandle, dmode[i], pxy, &smfdb, &dmfdb, &ci[2 * i] );
	}

	/* Write text, always transparent, beside the button */

	vswr_mode(xd_vhandle, MD_TRANS); 
	set_textdef();
	prt_text(string, x + (5 * xd_regular_font.fnt_chw) / 2, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/* 
 * Code for drawing progdefined rectangular "checkbox" button 
 */

static int cdecl ub_rectbut(PARMBLK *pb)
{
	int
		pxy[8], 
		flags,
		b,
		x = pb->pb_x, 
		y = pb->pb_y;

	char 
		*string;

	RECT  
		size; 


	/* Define clipping area */

	xd_clip_on((RECT *)&pb->pb_xc);

	/* Define text to display and object flags */

	flags = ((XUSERBLK *)(pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *)(pb->pb_parm))->ob_spec.free_string;	
 
	/* 
	 * Define object size and position. Height of the object is
	 * 3/4 of character height + 2 pixels + 3D enlargements
	 */

	size.x = pb->pb_x; /* intentionally without "+ aes_hor3d" here */
	size.y = pb->pb_y + xd_regular_font.fnt_chh / 8 + aes_ver3d - 1;
 	size.h = pb->pb_h - xd_regular_font.fnt_chh / 4 - 2 * aes_ver3d + 2;

	if ( xd_desk.w > 2 * xd_desk.h )
		size.w = 2 * size.h;	/* better looking in ST-medium */
	else 
		size.w = size.h;

	/* Draw object box */

	xd_drawbox(&size, flags, pb->pb_currstate, XD_RECTBUT );

	/* Draw additional graphic elements - "X" diagonal lines if selected */

	if ( pb->pb_currstate & SELECTED )
	{
		if ( xd_is3dobj(flags) )
			b = (aes_hor3d > 0) ? aes_hor3d - 1 : 1;
		else
			b = 0;

		pxy[0] = size.x + b;
		pxy[1] = size.y + b;
		pxy[2] = size.x + size.w - 1 - b;
		pxy[3] = size.y + size.h - 1 - b;
		pxy[4] = pxy[2];
		pxy[5] = pxy[1];
		pxy[6] = pxy[0];
		pxy[7] = pxy[3];

		set_linedef(BLACK);			
		v_pline(xd_vhandle, 2, pxy);
		v_pline(xd_vhandle, 2, &pxy[4]);
	}

	/* Draw object text beside the box, always in transparent mode */

	set_textdef();
	vswr_mode(xd_vhandle, MD_TRANS); 
	prt_text(string, x + (5 * xd_regular_font.fnt_chw) / 2, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/* DjV 074 ---vvv--- */

/* 

some object types defined in previous versions are never used in TeraDesk;
so, they are here extracted into a separate file

#include "xdunused.h"

*/

/* DjV 074 ---^^^--- */


/*
 * Code which handles scrolled editable text fields 
 */

static int cdecl ub_scrledit(PARMBLK *pb)
{
	XUSERBLK *blk = (XUSERBLK *)pb->pb_parm;
	TEDINFO *ted = blk->ob_spec.tedinfo;

	char 
		s[132],							/* same as size of LNAME !!! */
		*text = s,
	    *save = ted->te_ptext;			/* pointer to editable text field */

	int 
		i,								/* counter for padding with "_"s */
		tmode,							/* transparent or replace */
		b,								/* border thickness */
		x = pb->pb_x,
	    y = pb->pb_y,
	    w = pb->pb_w,
	    h = pb->pb_h,
		xl, xr,								/* positions of < > markers */
	    tw = (int)strlen(save),				/* length of text in the field */
	    ow = (int)strlen(ted->te_pvalid);	/* length of validation field */

	RECT 
		size,							/* size of the text box */
		cb;								/* character-sized blanking rectange */


	ted->te_ptext = s; 
	b = -ted->te_thickness; /* border to be positive to outside */

	/* Define clipping area */

	xd_clip_on((RECT *)&pb->pb_xc);

	/* Calculate some positions... */

	y += (h - xd_regular_font.fnt_chh - 1) / 2;

	xl = x - xd_regular_font.fnt_chw - 3;
	xr = x + w + 3;

	/* 
	 * If Magic, in colour & 3d mode, is hopefully detected, 
	 * create a background box; for other 3D AES, just clear the area
	 * and set transparent mode
	 */

	size.x = x - 1 - b;
	size.y = y - 1 - b;
	size.w = w + 2 + 2 * b;
	size.h = h + 1 + 2 * b;

	/* This is an attempt to get rid of the white text background */

	tmode  = MD_REPLACE;
	if ( xd_aes4_0 && xd_ncolors > 4 ) 
	{
		if ( aes_hor3d == 0 && ted->te_thickness != 0)
			/* hopefully this branch is valid for Magic only */
			xd_drawbox(&size, AES3D_1, SELECTED, XD_SCRLEDIT );
		else
		{
			/* other "gray background" AESses V4 */
			clr_object(&size, xd_bg_col, -1);
			tmode = MD_TRANS;
		}
	}

	/* Define a character-sized blanking rectangle in background colour */

	cb.y = y; 
	cb.w = xd_regular_font.fnt_chw;
	cb.h = xd_regular_font.fnt_chh;

	/* Set default text parameters */

	set_textdef();

	/* 
	 * Put or remove marks that part of the text is not visible in the field 
	 */

	if ( blk->ob_shift > 0 )
	{
		vswr_mode(xd_vhandle, MD_TRANS);
		prt_text("<", xl, y, 0);
	}
	else
	{
		vswr_mode(xd_vhandle, MD_REPLACE);
		cb.x = xl;
		clr_object(&cb, xd_bg_col, -1);
	}

	if ( tw - blk->ob_shift > ow )
	{
		vswr_mode(xd_vhandle, MD_TRANS);
		prt_text(">", xr, y, 0);
	}
	else
	{
		vswr_mode(xd_vhandle, MD_REPLACE);
		cb.x = xr;
		clr_object(&cb, xd_bg_col, -1);
	} 

	/* Copy visible part of the text to display */

	strsncpy(s, save + blk->ob_shift, min(tw, ow) + 1);

	/* Pad space after the string with "_"s */

	i = ow - (tw - blk->ob_shift);
	if ( i > 0 )
	{
		text += tw - blk->ob_shift;
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

static int cdecl ub_button(PARMBLK *pb)
{
	int 
		flags, 
		offset = 0, 
		x, 
		y;

	char 
		*string;


	/* Define clipping area */

	xd_clip_on((RECT *)&pb->pb_xc);

	/* Define text to display and object flags */

	flags = ((XUSERBLK *)(pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *)(pb->pb_parm))->ob_spec.free_string;

	/* Draw object box. No change in object size or position */

	xd_drawbox((RECT *)&pb->pb_x, flags, pb->pb_currstate, XD_BUTTON );

	/* Define text position and writing mode */

	set_textdef();
	vswr_mode(xd_vhandle, MD_TRANS);

	if ( pb->pb_currstate & SELECTED )
	{
		if ( xd_is3dobj(flags) )
			offset = 1;
		else
			vswr_mode(xd_vhandle, MD_XOR);
	}
	
	x = pb->pb_x + (pb->pb_w - (int) xd_strlen(string) * xd_regular_font.fnt_chw) / 2 + offset;
	y = pb->pb_y + (pb->pb_h - xd_regular_font.fnt_chh) / 2 + offset;

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

static int cdecl ub_rbutpar(PARMBLK *pb)
{
	int 
		x, y,		/* text position */
		dh,			/* height change */
		flags,		/* state flags */
		gap = 2; 	/* distance between the frame and thetxtt (pixels) */

	char 
		*string;	/* text to be written */

	RECT 
		size;		/* object frame to be drawn */


	/* Define clipping area */

	xd_clip_on((RECT *)&pb->pb_xc);

	/* Get object flags and title text */

	flags = ((XUSERBLK *)(pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *)(pb->pb_parm))->ob_spec.free_string;

	/* Define object area */

	dh = xd_regular_font.fnt_chh / 2;
	size.x = pb->pb_x;
	size.y = pb->pb_y + dh;
	size.w = pb->pb_w;
	size.h = pb->pb_h - dh;

	/* Draw object box */

	xd_drawbox(&size, flags, pb->pb_currstate, XD_RBUTPAR );

	/* Define position of title text */

	x = pb->pb_x + xd_regular_font.fnt_chw;
	y = pb->pb_y;

	/* Clear the area for text */

	vswr_mode(xd_vhandle, MD_REPLACE);

	size.x = x - gap;
	size.y = y - 1;
	size.w = (int)xd_strlen(string) * xd_regular_font.fnt_chw + 2 * gap;
	size.h = xd_regular_font.fnt_chh + 2; 

	clr_object(&size, xd_bg_col, -1);	

	/* Draw text */

	set_textdef();
	vswr_mode(xd_vhandle, MD_TRANS);
	prt_text(string, x, y, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/* 
 * Code for drawing a progdefined "underlined title" object.
 * With few colours available, or if there is no 3D AES present,
 * just draw the text wit a line beneath. Else, draw a box with
 * 3D effects and put the text inside it.
 * Note: tis should be improved a bit. The box always displays
 * a little wider then other box objects in the dialog. 
 * What should be done is to move the text a little to the right
 * (but only if a box is drawn), and not increase object width.
 */

static int cdecl ub_title(PARMBLK *pb)
{
	int 
		pxy[4],
		flags;

	char 
		*string;

	RECT 
		size;


	/* Define clipping area */

	xd_clip_on((RECT *)&pb->pb_xc);

	/* Get object flags and text to write */

	flags = ((XUSERBLK *)(pb->pb_parm))->ob_flags;
	string = ((XUSERBLK *)(pb->pb_parm))->ob_spec.free_string;

	/* Draw a box (or an underline, if there are too few colours) */

	if ( xd_ncolors > 4 && xd_is3dobj(flags) )
	{
		size.x = pb->pb_x - 2;
		size.y = pb->pb_y - 1;
		size.w = pb->pb_w + 4; /* this is too wide! */
		size.h = pb->pb_h;
		xd_drawbox( &size, flags, pb->pb_currstate, XD_TITLE );
	}
	else
	{		
		pxy[0] = pb->pb_x;
		pxy[1] = pb->pb_y + pb->pb_h;
		pxy[2] = pxy[0] + pb->pb_w - 1;
		pxy[3] = pxy[1];

		vswr_mode(xd_vhandle, MD_REPLACE);
		set_linedef(BLACK);
		v_pline(xd_vhandle, 2, pxy);
	}

	/* Draw title text */

	set_textdef();
	vswr_mode(xd_vhandle, MD_TRANS);
	prt_text(string, pb->pb_x, pb->pb_y + (pb->pb_h - xd_regular_font.fnt_chh - 1) / 2, pb->pb_currstate);

	/* Turn clipping off */

	xd_clip_off();

	return 0;
}


/*
 * Code for (not) drawing a rectangle for an unknown object type
 */

static int cdecl ub_unknown(PARMBLK *pb)
{
/* this would be a mistake anyway, so why draw anything at all ?

	RECT 
		*clip = (RECT *)&pb->pb_xc,
		*frame = (RECT *)&pb->pb_x;

	xd_clip_on(clip);

	set_linedef(BLACK);
	vswr_mode(xd_vhandle, MD_REPLACE);
	clr_object(frame, 0, -1);
	draw_frame(frame, 0, 0);

	xd_clip_off();
*/
	return 0;
}

/********************************************************************
 *																	*
 * Funkties voor het tekenen van de dialoogbox.						*
 *																	*
 ********************************************************************/

/*
 * Funktie voor het berekenen van de positie en grootte van de cursor.
 */

static void xd_calc_cursor(XDINFO *info, RECT *cursor)
{
	objc_offset(info->tree, info->edit_object, &cursor->x, &cursor->y);

	cursor->x += xd_abs_curx(info->tree, info->edit_object, info->cursor_x) * xd_regular_font.fnt_chw;

/* A slightly smaller cursor looks better in Magic
	cursor->y -= 1;
	cursor->w = 1;
	cursor->h = xd_regular_font.fnt_chh + 2;
*/
	cursor->w = 1;
	cursor->h = xd_regular_font.fnt_chh;
}


/*
 * Funktie voor het tekenen van de tekst cursor
 */

static void xd_credraw(XDINFO *info, RECT *area)
{
	if (cursor_mfdb.fd_addr == NULL)
	{
		cursor_mfdb.fd_h = xd_regular_font.fnt_chh + 4;
		cursor_mfdb.fd_nplanes = xd_nplanes;
		cursor_mfdb.fd_addr = xd_malloc((long) cursor_mfdb.fd_wdwidth *
										(long) cursor_mfdb.fd_h *
										(long) xd_nplanes * 2L);
	}

	if (cursor_mfdb.fd_addr != NULL)
	{
		RECT cursor, r;
		MFDB smfdb;
		int pxy[8];

		xd_calc_cursor(info, &cursor);
		smfdb.fd_addr = NULL;

		if (xd_rcintersect(area, &cursor, &r))
		{
			/* Save area below cursor. */
			
			pxy[0] = r.x;
			pxy[1] = r.y;
			pxy[2] = r.x + r.w - 1;
			pxy[3] = r.y + r.h - 1;
			pxy[4] = r.x - cursor.x;
			pxy[5] = r.y - cursor.y;
			pxy[6] = pxy[4] + r.w - 1;
			pxy[7] = pxy[5] + r.h - 1;

			vro_cpyfm(xd_vhandle, S_ONLY, pxy, &smfdb,
					  &cursor_mfdb);

			/* Draw cursor. */
			
			xd_clip_on(&r);

			vswr_mode(xd_vhandle, MD_REPLACE);
			set_linedef(BLACK);

			pxy[0] = pxy[2] = cursor.x;
			pxy[1] = cursor.y;
			pxy[3] = cursor.y + cursor.h - 1;

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
		RECT cursor, r1, r2;
		MFDB dmfdb;
		int pxy[8];

		xd_calc_cursor(info, &cursor);
		xd_mouse_off();
		dmfdb.fd_addr = NULL;

		if (info->dialmode != XD_WINDOW)
		{
			xd_clip_on(&cursor);

			pxy[0] = 0;
			pxy[1] = 0;
			pxy[2] = cursor.w - 1;
			pxy[3] = cursor.h - 1;
			pxy[4] = cursor.x;
			pxy[5] = cursor.y;
			pxy[6] = cursor.x + cursor.w - 1;
			pxy[7] = cursor.y + cursor.h - 1;

			vro_cpyfm(xd_vhandle, S_ONLY, pxy, &cursor_mfdb,
					  &dmfdb);

			xd_clip_off();
		}
		else
		{
			xw_getfirst(info->window, &r1);

			while ((r1.w != 0) && (r1.h != 0))
			{
				if (xd_rcintersect(&r1, &cursor, &r2))
				{
					xd_clip_on(&cursor);

					pxy[0] = r2.x - cursor.x;
					pxy[1] = r2.y - cursor.y;
					pxy[2] = pxy[0] + r2.w - 1;
					pxy[3] = pxy[1] + r2.h - 1;
					pxy[4] = r2.x;
					pxy[5] = r2.y;
					pxy[6] = r2.x + r2.w - 1;
					pxy[7] = r2.y + r2.h - 1;

					vro_cpyfm(xd_vhandle, S_ONLY, pxy, &cursor_mfdb,
							  &dmfdb);

					xd_clip_off();
				}

				xw_get(info->window, WF_NEXTXYWH, &r1);
			}
		}

		xd_mouse_on();
	}
}


/* 
 * Funktie voor het tekenen van een dialoogbox in een window 
 */

void xd_redraw(XDINFO *info, int start, int depth, RECT *area, int flags)
{
	RECT r1, r2, cursor;
	int draw_cur;
	OBJECT *tree = info->tree;

	xd_mouse_off();

	if ((flags & XD_RCURSOR) && (info->edit_object > 0))
	{
		xd_calc_cursor(info, &cursor);
		draw_cur = xd_rcintersect(area, &cursor, &cursor);
	}
	else
		draw_cur = FALSE;

	if (info->dialmode != XD_WINDOW)
	{
		if (flags & XD_RDIALOG)
			objc_draw(tree, start, depth, area->x, area->y, area->w, area->h);
		if (draw_cur)
			xd_credraw(info, &cursor);
	}
	else
	{
		xw_getfirst(info->window, &r1);

		while ((r1.w != 0) && (r1.h != 0))
		{
			if ((flags & XD_RDIALOG) && xd_rcintersect(&r1, area, &r2))
				objc_draw(tree, start, depth, r2.x, r2.y, r2.w, r2.h);

			if (draw_cur && xd_rcintersect(&r1, &cursor, &r2))
				xd_credraw(info, &r2);

			xw_get(info->window, WF_NEXTXYWH, &r1);
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

void xd_draw(XDINFO *info, int start, int depth)
{
	xd_wdupdate(BEG_UPDATE);

	xd_cursor_off(info);
	xd_redraw(info, start, depth, &info->drect, XD_RDIALOG);
	xd_cursor_on(info);

	xd_wdupdate(END_UPDATE);
}


/********************************************************************
 *																	*
 * Funktie voor het veranderen van de status van een object.		*
 *																	*
 ********************************************************************/

/*
 * Do what is necessary to change state of an object on the screen
 */

void xd_change(XDINFO *info, int object, int newstate, int draw)
{
	OBJECT *tree = info->tree;
	int twostates;

	if ( object < 0 ) 
		return;

	twostates = (newstate&0xff) | (tree[object].ob_state & (0xff00 | WHITEBAK));	/* preserve extended states */

	if (info->dialmode != XD_WINDOW)
		objc_change(tree, object, 0, info->drect.x, info->drect.y, info->drect.w, info->drect.h,
					twostates, (int) draw);
	else
	{
		tree[object].ob_state = twostates;

		if (draw)
			xd_draw(info, object, 0); /* 0: draw only this object */
	}
}


/*
 * Set a button to NORMAL without drawing it
 */

void xd_buttnorm(XDINFO *info, int button)
{
	xd_change(info, button, NORMAL, 0);
}


/*
 * Set a button to NORMAL and draw it
 */

void xd_drawbuttnorm(XDINFO *info, int button)
{
	xd_change(info, button, NORMAL, 1);
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

static void xd_translate(OBJECT *tree, int parent, int offset)
{
	int i = tree[parent].ob_head;

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
 * Note: this detects only magic-style capabilities, but -not- those
 * of e.g. Geneva.
 * If own_userdef = 1, all objects will be drawn by teradesk.
 * Unfortunately, there is currenlty no easy way to put it somewhere
 * in the configuration file.
 */

int own_userdef = 0; /* =1 if Teradesk will always draw all userdef objects */

static int must_userdef(OBJECT *ob)
{
	if ( !own_userdef )
	{
		if (!(aes_flags & GAI_WHITEBAK))
			return TRUE;
		if (ob->ob_state & WHITEBAK)
			return FALSE;
	}
	return TRUE;
}


/*
 * Specify that xdialog will always draw all extended object types on itself,
 * whether an AES can support them or not. sometimes they may look nicer that way.
 * 1= force xdialog to draw all; 0= let AES do it. This could be
 * used elsewhere in TeraDesk to set how the dialogs should be drawn-
 * but currently there is no way to store that information, because
 * the resource is initialized before the configuration file is read.
 */
 
void xd_own_xobjects( int setit )
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

static int cnt_user(OBJECT *tree, int *n, int *nx)
{
	OBJECT
		*object = tree;

	int 
		etype;

	*n = 0;
	*nx = 0;

	for (;;)
	{
		etype = (object->ob_type >> 8) & 0xFF;

		if (xd_is_xtndelement(etype) && ((object->ob_type & 0xFF) != G_USERDEF))
		{
			switch(etype)
			{
			case XD_DRAGBOX:	/* dragbox "ear" on thedialog */
			case XD_SCRLEDIT:	/* scrolled text field */
			case XD_FONTTEXT:
				(*nx)++;		/* always userdef, ignore AESses which may support this (none?) */
				break;
			case XD_RECTBUT:	/* checkbox button */
			case XD_ROUNDRB:	/* round radio button */
			case XD_TITLE:		/* title */
			case XD_RBUTPAR: 	/* titled fram */
			case XD_BUTTON :	/* rectangular button with text */
				if ( must_userdef(object) )
					(*nx)++;	/* userdef only if unsuported by AES */
				break;
			default :
				(*n)++;			/* not using extended parameter blocks */
				break;
			}
		}

		if (object->ob_flags & LASTOB)
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
	extern int aes_flags;

	int 
		etype, 		/* extended object type */
		n, 
		nx, 
		d, 
		hmin,		/* minimum object size */
		object = 0,
		xuserblk,	/* true if extended parameterblock is used */
		newflags;	/* remember object flags */

	OBJECT 
		*c_obj;

	USERBLK 
		*c_ub;

	XDOBJDATA 
		*data;

	int 
		cdecl(*c_code) (PARMBLK *parmblock);


	/* If there is nothing to do, return */

	if (cnt_user(tree, &n, &nx) == 0)
		return;

	/* Allocate memory for objects; return from this routine if failed */

	if ((data = xd_malloc(sizeof(XDOBJDATA) + sizeof(USERBLK) * (long) n +
						  sizeof(XUSERBLK) * (long) nx)) == NULL)
		                          return;

	data->next = xd_objdata;
	xd_objdata = data;
	c_ub = (USERBLK *) &data[1];

	/* Scan all objects in the resource */

	for (;;)
	{
		c_code = 0L;							/* no pointer to user code */
		c_obj = tree + object;					/* object index */
		xuserblk = FALSE;						/* no extended block */
		etype = (c_obj->ob_type >> 8) & 0xFF;	/* extended object type */

		if (xd_is_xtndelement(etype) && ((c_obj->ob_type & 0xFF) != G_USERDEF))
		{
			/*
			 * It looks like AESses are not consistent in treating progdef objects 
			 * if they are flagged as "indicator" or "activator"; some seem to 
			 * draw 3d effect anyway (AES4.1, NAES), and some do not (Geneva?). 
			 * Therefore, those flags are here saved in the extended userblock  
			 * and then disabled so that only user code will draw 3d effects.
			 */ 

			newflags = c_obj->ob_flags & ~(AES3D_1 | AES3D_2);
			xuserblk = TRUE; /* currently so for all but "unknown" */

			switch (etype)
			{
			case XD_DRAGBOX :
				/* Dragbox "ear" on the dialogs; always userdefined */ 
				c_code = ub_drag;
				break;
			case XD_ROUNDRB :
				/* Round radio button */
				if (must_userdef(c_obj))
					c_code = ub_roundrb;
				break;
			case XD_RECTBUT :
				/* Rectangular "checkbox" button */
				if (must_userdef(c_obj))
					c_code = ub_rectbut;
				break;

/* DjV 074 ---vvv--- */
/* not used anywhere in Teradesk
			case XD_RECTBUTTRI :
				if (must_userdef(c_obj))
					c_code = ub_rectbuttri;
				break;
			case XD_CYCLBUT :
				c_code = ub_cyclebut;
				break;
*/
/* DjV 074 ---^^^--- */

			case XD_BUTTON :
				/* 
				 * Rectangular button with (possibly) underlined text;   
				 * This object should have a minimum height   
				 * to accomodate the underline inside the button.
				 * Modification should affect both userdefined and 
				 * AES-supported objects.
				 */
				hmin = xd_regular_font.fnt_chh + 2; /* minimum height */
				if ( c_obj->ob_height < hmin )
				{
					c_obj->ob_y -= (hmin - c_obj->ob_height) / 2;
					c_obj->ob_height = hmin;
				}
				if (must_userdef(c_obj))
					c_code = ub_button;
				break;
			case XD_RBUTPAR :
				/* 
				 * Frame with a title on the upper contour;
				 * Increase object height slightly, because
				 * the upper side will alwas be drawn about
				 * 1/2 character heights inside the object;
				 * withut increasing object size, the upper
				 * edge would have been too close to objects  
				 * inside. This affects both userdefined and
				 * AES-supported objects.
				 */
				d = xd_regular_font.fnt_chh / 2;

				c_obj->ob_y -= d;
				c_obj->ob_height += d;
				xd_translate(tree, object, d);
				if (must_userdef(c_obj)) 
					c_code = ub_rbutpar;
				break;
			case XD_TITLE :
				/* 
				 * Dialog title; object height must always be
				 * increased a little bit, for better look.
				 * E.g. otherwise, in magic/low-res/colour, the
				 * underline gets too close to the text
				 * (in fact, it is drawn OVER the text)
				 */
				d = 1;
				if (must_userdef(c_obj))
					c_code = ub_title;
				else if ( xd_ncolors > 4 && aes_ver3d == 0 )
					d += 2; /* mostly for magic */
				c_obj->ob_height += 2 * d; 
				c_obj->ob_y -= d + 1;
				break;
			case XD_SCRLEDIT:	
				/* Scrolled editable text; always userdefined */
				c_code = ub_scrledit;
				break;
			case XD_FONTTEXT:
				c_code = ub_unknown;
				break;
			default:
				/* yet unknown userdef! */
				c_code = ub_unknown;
				xuserblk = FALSE;
				break;
			}

			if (c_code)	
			{
				if (xuserblk)
				{
					XUSERBLK *c_xub = (XUSERBLK *)c_ub;
	
					xd_xuserdef(c_obj,  c_xub, c_code);
					c_ub = (USERBLK *)(c_xub + 1);
					c_obj->ob_flags = newflags;
				}
				else
				{
					xd_userdef(c_obj, c_ub, c_code);
					c_ub++;
				}
			}

		}

		if (c_obj->ob_flags & LASTOB)
			return;

		object++;
	}
}


/* 
 * Obtain address of an object of a certain type 
 */

int xd_gaddr(int type, int index, void *addr)
{
	int result;

	if (((result = rsrc_gaddr(type, index, addr)) != 0) && (type == R_TREE))
		xd_set_userobjects(*(OBJECT **) addr);

	return result;
}


/* 
 * Specify text buffer for scolled-text objects. 
 * Clear the text in the buffer (zero first character only)
 */

char *xd_set_srcl_text(OBJECT *tree, int item, char *txt)
{
	TEDINFO *ted = xd_get_obspec(tree + item).tedinfo;
	ted->te_ptext = txt;
	*txt = 0;
	return txt;
}


void xd_fixtree(OBJECT *tree)
{
	int i = 0;

	for (;;)
	{
		rsrc_obfix(tree, i);
		if (tree[i].ob_flags & LASTOB)
			break;
		i++;
	}

	xd_set_userobjects(tree);
}

