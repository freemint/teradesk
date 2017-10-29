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


#include <xerror.h>

/* Resultaten voor hndl_error */

#define XERROR		-1024		/* Non fatal error */
#define XABORT		-1025		/* Abort copy */
#define XFATAL		-1026		/* Fatal error during copy */

/* Resultaten hndl_nameconflict */

#define XSKIP		-1027		/* Skip file */
#define XOVERWRITE	-1028		/* Overwrite file */
#define XEXIST		-1029		/* File exists */
#define XALL 		-1031		/* mark all */


_WORD alert_msg(const char *string, ...);	
_WORD alert_printf(_WORD def, _WORD message,...);
void alert_iprint( _WORD message );
void alert_cantdo(_WORD msg1, _WORD msg2);
void alert_abort( _WORD message );
_WORD alert_query( _WORD message );

void xform_error(_WORD error);
void hndl_error(_WORD msg, _WORD error);
_WORD xhndl_error(_WORD msg, _WORD error, const char *file);
char *get_freestring( _WORD stringid );
char *get_message(_WORD error);
