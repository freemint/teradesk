/*
 * Utility functions for Teradesk. Copyright 1993 - 2002  W. Klaren,
 *                                           2002 - 2003  H. Robbers,
 *                                           2003 - 2008  Dj. Vukovic
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
#include <mint/sysvars.h>

#undef _sysbase
#define _sysbase     ( * ( (OSHEADER **) 0x4F2L ) )

/*
 * Return TOS version to be read as a hex number,
 * e.g. 0x104, 0x206, etc.
 */

_WORD get_tosversion( void )
{
	void *stack;
	_WORD version;

	stack = (void *)Super(NULL);
	version = _sysbase->os_version;
	Super(stack);
	return version;
}


/*
 * Return AES version to be read as a hex number,
 * e.g. 0x340, 0x399, etc.
 */

_WORD get_aesversion(void)
{
	return _AESversion;
}

