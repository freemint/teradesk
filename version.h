/*
 * Teradesk. Copyright (c) 1993, 1994, 1995, 1997, 2002  W. Klaren,
 *                                           2002, 2003  H. Robbers,
 *                                           2003, 2004  Dj. Vukovic
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

#define CFG_VERSION		0x0300		/* current version id. of cfg/inf file */ 

#if TEXT_CFG_IN 
#define MIN_VERSION		0x0300		/* min.acceptabe version id. of inf file */	 
#else
#define MIN_VERSION		0x0200		/* min.acceptabe version id. of cfg file */	 
#endif

#define INFO_VERSION	"Tera Desktop V3.01  07-01-2004"
#define INFO_COPYRIGHT	"\xBD W.Klaren, H.Robbers, Dj.Vukovic"
#define INFO_OTHER "1991-2004"

#if _MINT_

#if TEXT_CFG_IN
  #define INFO_SYSTEMS 	  "for TOS, MiNT, MagiC and Geneva"
#else 
  #define INFO_SYSTEMS     "Config.file Conversion Utility" 
#endif

#else
  #define INFO_SYSTEMS	"for single TOS only"
#endif




