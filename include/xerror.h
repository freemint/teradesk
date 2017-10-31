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

/*
 * we need the TOS/MiNT error codes here,
 * not the ones the C library might use
 */

/*
 * BIOS error codes
 */
#undef EBUSY
#define EBUSY               -2		/* Resource busy.  */
#undef ESPIPE
#define ESPIPE              -6		/* Illegal seek.  */
#undef EMEDIUMTYPE
#define EMEDIUMTYPE         -7		/* Wrong medium type.  */
#undef ESECTOR
#define ESECTOR             -8		/* Sector not found.  */
#undef EROFS
#define EROFS               -13		/* Write protect.  */
#undef ECHMEDIA
#define ECHMEDIA            -14		/* Media change.  */
#undef EBADSEC
#define EBADSEC             -16		/* Bad sectors found.  */
#undef ENOMEDIUM
#define ENOMEDIUM           -17		/* No medium found.  */

/*
 * TOS error codes
 */
#undef ENOSYS
#define ENOSYS              -32		/* Function not implemented.  */
#undef ENOENT
#define ENOENT              -33		/* No such file or directory.  */
#undef ENOTDIR
#define ENOTDIR             -34		/* Not a directory.  */
#undef EACCES
#define EACCES              -36     /* Permission denied.  */
#undef EPERM
#define EPERM               -38		/* Operation not permitted.  */
#undef ENOMEM
#define ENOMEM              -39		/* Cannot allocate memory.  */
#undef ENXIO
#define ENXIO               -46		/* No such device or address.  */
#undef ENMFILES
#define ENMFILES            -49		/* No more matching file names.  */
#undef ELOCKED
#define ELOCKED             -58		/* Locking conflict.  */
#undef EBADARG
#define EBADARG             -64		/* Bad argument.  */
#undef ENOEXEC
#define ENOEXEC             -66		/* Invalid executable file format.  */
#undef ENAMETOOLONG
#define ENAMETOOLONG        -86		/* Pathname too long.  */
#undef EIO
#define EIO                 -90		/* I/O error */
#undef ENOSPC
#define ENOSPC              -91		/* No space left on device.  */
#undef E2BIG
#define E2BIG              -125		/* Argument list too long.  */

/* -2048 t/m -2063 file fouten */

#define EEOF		-2051		/* End of file */

/* Some other more-less file related errors */

#define EFRVAL		-2053		/* Invalid value read from file */
#undef ENOMSG
#define ENOMSG		-2054		/* Don't display message */

/* -2064 t/m -2079 lengte fouten */

#define EFNTL		-2066		/* filename too long. */

/* -2096 t/m -2112 algemene fouten */

#define XUNKNOWN	-2096		/* Unknown GEMDOS error */

/* -4096 t/m 6120 fouten in Xdialog bibliotheek */

#define XDVDI		-4096		/* no more VDI handles */
#define XDNMWINDOWS -4097		/* no more windows available */


_WORD xerror( _WORD toserror );
