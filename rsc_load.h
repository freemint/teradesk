/*
 * rsc_load.h Copyright(c) 2023 Thorsten Otto
 *
 * This file is in the public domain.
 *
 * substitute rsrc_load/rsrc_free/rsrc_gaddr:
 *
 * These functions support the extended resource header format
 * for resource files >64K.
 * They also support the new resource file format with coloricons,
 * but do *not* transform the icon data. This is left to the application.
 */

#ifndef _WORD
#  define _WORD short
#endif
#ifndef _UWORD
#  define _UWORD unsigned short
#endif
#ifndef _LONG
#  define _LONG long
#endif
#ifndef _ULONG
#  define _ULONG unsigned long
#endif
#ifndef FALSE
#  define FALSE 0
#  define TRUE  1
#endif


/*
 * Format of the extended resource header, as used
 * by ORCS, Interface, and RSM
 */
#ifndef __RSXHDR
#define __RSXHDR
typedef struct rsxhdr
{
	unsigned short	rsh_vrsn;     /* should be 3                                 */
	unsigned short	rsh_extvrsn;  /* not used, initialised to 'OR' for ORCS      */

	unsigned long	rsh_object;
	unsigned long	rsh_tedinfo;
	unsigned long	rsh_iconblk;  /* list of ICONBLKS                            */
	unsigned long	rsh_bitblk;
	unsigned long	rsh_frstr;
	unsigned long	rsh_string;
	unsigned long	rsh_imdata;   /* image data                                  */
	unsigned long	rsh_frimg;
	unsigned long	rsh_trindex;

	unsigned long	rsh_nobs;     /* counts of various structs                   */
	unsigned long	rsh_ntree;
	unsigned long	rsh_nted;
	unsigned long	rsh_nib;
	unsigned long	rsh_nbb;
	unsigned long	rsh_nstring;
	unsigned long	rsh_nimages;
	unsigned long	rsh_rssize;   /* total bytes in resource                     */
} RSXHDR;
#endif


/*
 * Flags in the rsh_vrsn field
 */
#define RSC_VERSION_MASK    0x03
#define RSC_EXT_FLAG        0x04
#define XRSC_VRSN_ORCS      0x4F52

#define IS_XRSC_HEADER(handle) (((handle)->rsh_vrsn & RSC_VERSION_MASK) == 0x03)


/*
 * according to Compendium, the icon text for color
 * icons is limited to 12 characters, including terminating '\0'
 */
#define CICON_STR_SIZE 12


/*
 * As a wrapper for rsrc_load(), these functions behave identical.
 * When using multiple resources, be sure to set up the
 * variables in the AES global array accordingly before
 * calling any of these functions:
 * aes_global[5/6] with the tree pointer array, and
 * aes_global[7/8] with the address of the resource header
 */
_WORD xrsrc_load(const char *fname);
_WORD xrsrc_free(void);
_WORD xrsrc_gaddr(_WORD re_gtype, _WORD re_gindex, void *gaddr);
