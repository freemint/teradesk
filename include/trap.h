/*	TRAP.H

	syscall definitions
	(c) 2005 H. Robbers @ Amsterdam.

       All Rights Reserved.
*/

#ifndef __TRAP_H
#define __TRAP_H

/* __syscall__(a[,b[,c]])
 *
 * __syscall__ sits in the syntactical position of cdecl, pascal
 *              and the like in function prototypes.
 *	a = trap number
 *  b,c = optional subsystem opcodes
 * The parameters could also have been enumerated.
 *
 */

#if __AHCC__
	#define TRAP(a)   cdecl __syscall__(a)

	#define GEMDOS(b) cdecl __syscall__( 1,b)
	#define BIOS(b)   cdecl __syscall__(13,b)
	#define XBIOS(b)  cdecl __syscall__(14,b)
#else
	#define TRAP(a)

	#define GEMDOS(b)
	#define BIOS(b)
	#define XBIOS(b)
#endif

#endif /* __TRAP_H */
