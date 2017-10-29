/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers
 *                         2003 - 2007  Dj. Vukovic
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


#ifndef DRAGDROP_H
#define DRAGDROP_H 1

#ifndef DD_EXTSIZE
/* AP_DRAGDROP return codes */
#define DD_OK        0
#define DD_NAK       1
#define DD_EXT       2
#define DD_LEN       3
#define DD_TRASH     4
#define DD_PRINTER   5
#define DD_CLIPBOARD 6

#define DD_TIMEOUT	4000		/* Timeout in ms */

#define DD_NUMEXTS	8			/* Number of formats */
#define DD_EXTLEN   4
#define DD_EXTSIZE	(DD_NUMEXTS * DD_EXTLEN)
#endif

_WORD ddcreate(_WORD dpid, _WORD spid, _WORD winid, _WORD msx, _WORD msy, _WORD kstate, char exts[] );
_WORD ddstry(_WORD fd, const char *ext, const char *name, long size);
void ddclose(_WORD fd);
_WORD ddopen(_WORD ddnam, const char *preferext);
_WORD ddrtry(_WORD fd, const char *name, const char *whichext, long *size);
_WORD ddreply(_WORD fd, _WORD ack);

#endif
