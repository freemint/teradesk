/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                                     2003  Dj. Vukovic
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


int alert_msg(const char *string, ...);	
int alert_printf(int def, int message,...);
int alert_iprint( int message );

void xform_error(int error);
void hndl_error(int msg, int error);
int xhndl_error(int msg, int error, const char *file);
char *get_freestring( int stringid );
char *get_message(int error);
