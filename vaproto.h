/*
 * Teradesk. Copyright (c) 1997, 2002 W. Klaren.
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

#define AV_PROTOKOLL	0x4700		/* Send supported features. */
#define VA_PROTOSTATUS	0x4701		/* Return supported features. */
#define	AV_SENDKEY		0x4710		/* Send key. */
#define VA_START		0x4711		/* Pass command line to program. */
#define AV_ASKFILEFONT	0x4712		/* Ask the currently selected font for files. */
#define VA_FILEFONT		0x4713		/* Return the currently selected font for files. */
#define AV_LAST			0x4736		/* Last AV message. */

#define FONT_CHANGED	0x7A18
#define FONT_SELECT		0x7A19		/* Call to font selector. */
#define FONT_ACK		0x7A1A

extern int va_start_prg(const char *program, const char *cmdline);
extern void handle_av_protocol(const int *message);
