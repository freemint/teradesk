/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
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


/* Font effects */

#define FE_NONE		0x0000
#define FE_BOLD		0x0001
#define FE_LIGHT	0x0002
#define FE_ITALIC	0x0004
#define FE_ULINED	0x0008
#define FE_OUTLIN	0x0010
#define FE_SHADOW	0x0020
#define FE_INVERS	0x0040


extern XDFONT def_font;

void fnt_setfont(_WORD font, _WORD height, XDFONT *data);
bool fnt_dialog(_WORD title, XDFONT *font, bool prop);
void fnt_mdialog(_WORD ap_id, _WORD win, _WORD id, _WORD size, _WORD colour, _WORD effect, _WORD prop);
