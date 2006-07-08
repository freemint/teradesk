/*
 * Multitos Library for Pure C 1.0. Copyright (c)       1994, 2002  W. Klaren,
 *                                                      2002, 2003  H. Robbers
 *                                                2004, 2005, 2006  Dj. Vukovic
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
#include <multitos.h>


int appl_getinfo(int ap_gtype,
				 int *ap_gout1, int *ap_gout2,
				 int *ap_gout3, int *ap_gout4)
{
	int *g = &(_GemParBlk.contrl[0]);

	*g++ = 130; /* [0] */
	*g++ = 1;	/* [1] */
	*g++ = 5;	/* [2] */
	*g++ = 0;	/* [3] */
	*g   = 0;	/* [4] */

	_GemParBlk.intin[0] = ap_gtype;

	aes();

	*ap_gout1 = _GemParBlk.intout[1];
	*ap_gout2 = _GemParBlk.intout[2];
	*ap_gout3 = _GemParBlk.intout[3];
	*ap_gout4 = _GemParBlk.intout[4];

	return _GemParBlk.intout[0];
}


int objc_sysvar(int mo, int which, int  ivall, int  ival2, int *oval1, int *oval2)
{
	int *g = &(_GemParBlk.contrl[0]);

	*g++ = 48;	/* [0] */
	*g++ = 4;	/* [1] */
	*g++ = 3;	/* [2] */
	*g++ = 0;	/* [3] */
	*g   = 0;	/* [4] */


	_GemParBlk.intin[0] = mo;
	_GemParBlk.intin[1] = which;
	_GemParBlk.intin[2] = ivall;
	_GemParBlk.intin[3] = ival2;

	aes();

	*oval1 = _GemParBlk.intout[1];
	*oval2 = _GemParBlk.intout[2];

	return _GemParBlk.intout[0];
}


int appl_control(int ap_cid, int ap_cwhat, void *ap_cout)
{
	int *g = &(_GemParBlk.contrl[0]);

	*g++ = 129;	/* [0] */
	*g++ = 2;	/* [1] */
	*g++ = 1;	/* [2] */
	*g++ = 1;	/* [3] */
	*g   = 0;	/* [4] */


	_GemParBlk.intin[0] = ap_cid;
	_GemParBlk.intin[1] = ap_cwhat;
	_GemParBlk.addrin[0] = ap_cout;

	aes();

	return _GemParBlk.intout[0];
}