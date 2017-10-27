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

#define PM_TXT 0	/* print data in text mode (line wrap) */
#define PM_HEX 1	/* print data as hex dump */
#define PM_RAW 2	/* print data as it is, no intervention */

extern int printmode;		/* see above */
extern XFILE *printfile;	/* print file; if NULL print to port */

bool check_print( WINDOW *w, int n, int *list); 
bool print_list(WINDOW *w, int n, int *list, long *folders, long *files, LSUM *bytes, int function);
