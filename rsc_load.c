/*
 * rsc_load.c Copyright(c) 2023 Thorsten Otto
 *
 * This file is in the public domain.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __PUREC__
#include <aes.h>
#include <tos.h>
#else
#include <gem.h>
#include <osbind.h>
#endif
#include "rsc_load.h"

/*
 * configuration option:
 * enable/disable some extra checks.
 * Disabling them will save a few bytes
 * (AES does not do these checks either)
 */
#define WITH_CHECKS 0
#define WITH_COLORICONS 1

/*
 * entries in the array that is only present in resources with color-icons
 */
#define RSC_EXT_FILESIZE	0
#define RSC_EXT_CICONBLK	1
#define RSC_EXT_PALETTE		2
#define RSC_EXT_ENDMARK		3
#define RSC_EXT_SIZE		4	/* number of known extension slots */

#ifndef G_EXTBOX
#define G_EXTBOX          40                              /* XaAES */
#endif

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void rsc_obfix(OBJECT *tree, _ULONG count)
{
	_WORD wchar, hchar, dummy;
	
	graf_handle(&wchar, &hchar, &dummy, &dummy);
	while (count)
	{
		tree->ob_x = wchar * (tree->ob_x & 0xff) + (tree->ob_x >> 8);
		tree->ob_y = hchar * (tree->ob_y & 0xff) + (tree->ob_y >> 8);
		tree->ob_width = wchar * (tree->ob_width & 0xff) + (tree->ob_width >> 8);
		tree->ob_height = hchar * (tree->ob_height & 0xff) + (tree->ob_height >> 8);
		tree++;
		count--;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void xrsrc_hdr2xrsc(RSXHDR *xrsc_header, const RSHDR *header)
{
	xrsc_header->rsh_vrsn	 = 3 | (header->rsh_vrsn & ~RSC_VERSION_MASK);
	xrsc_header->rsh_extvrsn = XRSC_VRSN_ORCS; /* informational only; not used in code */
	xrsc_header->rsh_object  = header->rsh_object;
	xrsc_header->rsh_tedinfo = header->rsh_tedinfo;
	xrsc_header->rsh_iconblk = header->rsh_iconblk;
	xrsc_header->rsh_bitblk  = header->rsh_bitblk;
	xrsc_header->rsh_frstr	 = header->rsh_frstr;
	xrsc_header->rsh_string  = header->rsh_string;
	xrsc_header->rsh_imdata  = header->rsh_imdata;
	xrsc_header->rsh_frimg	 = header->rsh_frimg;
	xrsc_header->rsh_trindex = header->rsh_trindex;
	xrsc_header->rsh_nobs	 = header->rsh_nobs;
	xrsc_header->rsh_ntree	 = header->rsh_ntree;
	xrsc_header->rsh_nted	 = header->rsh_nted;
	xrsc_header->rsh_nib	 = header->rsh_nib;
	xrsc_header->rsh_nbb	 = header->rsh_nbb;
	xrsc_header->rsh_nstring = header->rsh_nstring;
	xrsc_header->rsh_nimages = header->rsh_nimages;
	xrsc_header->rsh_rssize  = header->rsh_rssize;
}

/*** ---------------------------------------------------------------------- ***/

#if WITH_CHECKS
static _ULONG filesize;
#endif

static int xrsrc_fix(RSHDR *rscHdr)
{
	RSXHDR xrsc_header;
	_ULONG UObj;
	char *buf = (char *)rscHdr;
#if WITH_COLORICONS
	CICONBLK *cicon_p = NULL;
#endif
	
	if (!IS_XRSC_HEADER(rscHdr))
		xrsrc_hdr2xrsc(&xrsc_header, rscHdr);
	else
		xrsc_header = *((RSXHDR *)rscHdr);

#define rsc_pointer(offset) (buf + (size_t)offset)
#define rschdr_pointer(offset) rsc_pointer(xrsc_header.offset)
	
	/* find color icons */
	if (xrsc_header.rsh_vrsn & RSC_EXT_FLAG)
	{
#if WITH_COLORICONS
		_LONG *p;
		_LONG *cp;
#if WITH_CHECKS
		int ok = TRUE;
#endif

		p = (_LONG *)rschdr_pointer(rsh_rssize);
#if WITH_CHECKS
		if ((_ULONG)p[RSC_EXT_FILESIZE] != filesize)
		{
			ok = FALSE;
		} else if (p[RSC_EXT_CICONBLK] < 0 || (_ULONG)p[RSC_EXT_CICONBLK] >= filesize)
		{
			ok = FALSE;
		} else
#endif
		{
			/*
			 * p now points to the extension header.
			 * p[0] is the total file size.
			 * p[1] is the offset to the CICONBLK ptrs list
			 * The list is terminated by a zero,
			 * we check whether a palette entry is present.
			 */
			cp = p + RSC_EXT_CICONBLK + 1;
	
			for (;;)
			{
#if WITH_CHECKS
				if ((_ULONG)((char *)cp - buf) >= filesize)
				{
					ok = FALSE;
					break;
				}
#endif
				if (*cp == 0)
					break;
				if (*cp != -1)
				{
					if (cp == (p + RSC_EXT_PALETTE))
					{
#if 0
						_WORD *palette = (_WORD *) (buf + (size_t) (*cp));
	
						/* save palette here if needed */
#endif
					}
					break;
				}
				cp++;
			}
		}

#if WITH_CHECKS
		if (ok != FALSE)
#endif
		{
			p = (_LONG *)rsc_pointer(p[RSC_EXT_CICONBLK]);
			/*
			 * p now points to the CICONBLKs list
			 * In the file, it is just an empty list, terminated by -1.
			 */
#if WITH_CHECKS
			while (ok != FALSE)
			{
				if ((_ULONG)((char *)p - buf) >= filesize)
					ok = FALSE;
				else if (*p == -1)
					break;
				else if (*p != 0)
					ok = FALSE;
				else
					p++;
			}
			if (ok != FALSE && *p == -1)
#else
			for (;;)
			{
				if (*p == -1)
					break;
				else
					p++;
			}
#endif
			{
				p++;
				/*
				 * CICONBLKs follow here
				 */
				cicon_p = (CICONBLK *)p;
			}
		}
#if WITH_CHECKS
		if (ok == FALSE)
		{
			Mfree(buf);
			return FALSE;
		}
#endif
#else /* !WITH_COLORICONS */
		Mfree(buf);
		return FALSE;
#endif
	}

	/* fix objects */
	{
		OBJECT *_rs_object;

		_rs_object = (OBJECT *)rschdr_pointer(rsh_object);
		rsc_obfix(_rs_object, xrsc_header.rsh_nobs);
		for (UObj = 0; UObj < xrsc_header.rsh_nobs; UObj++)
		{
			switch (_rs_object->ob_type & 0xff)
			{
			case G_BOX:
			case G_IBOX:
			case G_BOXCHAR:
			case G_EXTBOX:
				break;
			case G_STRING:
			case G_TITLE:
			case G_BUTTON:
			case G_TEXT:
			case G_FTEXT:
			case G_BOXTEXT:
			case G_FBOXTEXT:
			case G_IMAGE:
			case G_ICON:
				_rs_object->ob_spec.free_string = (char *)rsc_pointer(_rs_object->ob_spec.index);
				break;
			case G_SHORTCUT:
				/* not changed to G_STRING here;
				   might be displayed in window
				   where we use our own functions
				 */
				_rs_object->ob_spec.free_string = (char *)rsc_pointer(_rs_object->ob_spec.index);
				break;
#if WITH_COLORICONS
			case G_CICON:
				if (cicon_p == NULL)
				{
					/* !!! */
					Mfree(buf);
					return FALSE;
				} else
				{
					CICON *dp;
					CICONBLK *cicon;
					_LONG size;
					char *p;
					_LONG num_cicons;
					_LONG idx;

					cicon = cicon_p;
					cicon_p++;
					_rs_object->ob_spec.ciconblk = cicon;
					size = ((cicon->monoblk.ib_wicon + 15) / 16) * 2l * cicon->monoblk.ib_hicon;
					p = (char *)cicon_p;
					cicon->monoblk.ib_pdata = (_WORD *)p;
					p += (size_t)size;
					cicon->monoblk.ib_pmask = (_WORD *)p;
					p += (size_t)size;
					idx = (_LONG)cicon->monoblk.ib_ptext;
					if (idx <= 0 || (buf + (size_t)idx) == p || idx < (_LONG)xrsc_header.rsh_string || idx >= (_LONG)xrsc_header.rsh_rssize)
						cicon->monoblk.ib_ptext = (char *)p;
					else
						cicon->monoblk.ib_ptext = (char *)rsc_pointer(idx);
					p += CICON_STR_SIZE; /* skip reserved space for icon text */
					dp = (CICON *)p;
					num_cicons = (_LONG)(cicon->mainlist);
#if WITH_CHECKS
					if (p > (buf + (size_t)filesize))
					{
						Mfree(buf);
						return FALSE;
					}
#endif
					if (num_cicons == 0)
					{
						cicon->mainlist = NULL;
					} else
					{
						cicon->mainlist = dp;
						while (num_cicons != 0)
						{
							p += sizeof(CICON);
							dp->col_data = (_WORD *)p;
							p += (size_t)(size * dp->num_planes);
							dp->col_mask = (_WORD *)p;
							p += (size_t)size;
							if (dp->sel_data != NULL)
							{
								dp->sel_data = (_WORD *)p;
								p += (size_t)(size * dp->num_planes);
								dp->sel_mask = (_WORD *)p;
								p += (size_t)size;
							} else
							{
								dp->sel_data = NULL;
								dp->sel_mask = NULL;
							}
							num_cicons--;
							if (num_cicons == 0)
							{
								dp->next_res = NULL;
							} else
							{
								dp->next_res = (CICON *)p;
							}
							dp = (CICON *)p;
						}
					}
					cicon_p = (CICONBLK *)p;
				}
				break;
#endif /* WITH_COLORICONS */
			case G_USERDEF:
				_rs_object->ob_spec.userblk = (USERBLK *)rsc_pointer(_rs_object->ob_spec.index);
				/*
				 * It's up to the application to set the appropiate function.
				 * To be on the safe side, let it point to some function
				 * that draws a box only, or simply does nothing.
				 */
				_rs_object->ob_spec.userblk->ub_code = 0;
				break;
			}
			_rs_object++;
		}
	}

	/* fix tedinfos */
	{
		TEDINFO *_rs_tedinfo;

		_rs_tedinfo = (TEDINFO *)rschdr_pointer(rsh_tedinfo);
		for (UObj = 0; UObj < xrsc_header.rsh_nted; UObj++)
		{
			_rs_tedinfo->te_ptext += (size_t)buf;
			_rs_tedinfo->te_ptmplt += (size_t)buf;
			_rs_tedinfo->te_pvalid += (size_t)buf;
			_rs_tedinfo++;
		}
	}

	/* fix iconblks */
	{
		ICONBLK *_rs_iconblk;

		_rs_iconblk = (ICONBLK *)rschdr_pointer(rsh_iconblk);
		for (UObj = 0; UObj < xrsc_header.rsh_nib; UObj++)
		{
			_rs_iconblk->ib_pmask = (_WORD *)rsc_pointer(_rs_iconblk->ib_pmask);
			_rs_iconblk->ib_pdata = (_WORD *)rsc_pointer(_rs_iconblk->ib_pdata);
			_rs_iconblk->ib_ptext = (char *)rsc_pointer(_rs_iconblk->ib_ptext);
			_rs_iconblk++;
		}
	}

	/* fix bitblks */
	{
		BITBLK *_rs_bitblk;

		_rs_bitblk = (BITBLK *)rschdr_pointer(rsh_bitblk);
		for (UObj = 0; UObj < xrsc_header.rsh_nbb; UObj++)
		{
			_rs_bitblk->bi_pdata = (_WORD *)rsc_pointer(_rs_bitblk->bi_pdata);
			_rs_bitblk++;
		}
	}

	/* fix free strings */
	{
		char **_rs_frstr;

		_rs_frstr = (char **)rschdr_pointer(rsh_frstr);
		for (UObj = 0; UObj < xrsc_header.rsh_nstring; UObj++)
		{
			*_rs_frstr += (size_t)buf;
			_rs_frstr++;
		}
	}

	/* fix free images */
	{
		BITBLK **_rs_frimg;

		_rs_frimg = (BITBLK **)rschdr_pointer(rsh_frimg);
		for (UObj = 0; UObj < xrsc_header.rsh_nimages; UObj++)
		{
			*_rs_frimg = (BITBLK *)rsc_pointer(*_rs_frimg);
			_rs_frimg++;
		}
	}

	/* fix tree indices */
	{
		OBJECT **_rs_trindex;

		_rs_trindex = (OBJECT **)rschdr_pointer(rsh_trindex);
#ifdef __GNUC__
		/* avoid type-punned pointer */
		{
			OBJECT ***rscfile = (OBJECT ***)&aes_global[5];
			*rscfile = _rs_trindex;
		}
		{
			void **rscmem = (void **)&aes_global[7];
			*rscmem = buf;
		}
#else
		_AESrscfile = _rs_trindex;
		_AESrscmem = buf;
#endif

		for (UObj = 0; UObj < xrsc_header.rsh_ntree; UObj++)
		{
			*_rs_trindex = (OBJECT *)rsc_pointer(*_rs_trindex);
			_rs_trindex++;
		}
	}
	
#undef rsc_pointer
#undef rschdr_pointer

	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

_WORD xrsrc_load(const char *fname)
{
	int fd;
	long size;
	char pathname[128];
	void *useRsc;

	strcpy(pathname, fname);
	if (!shel_find(pathname) || (fd = (int)Fopen(pathname, FO_READ)) < 0)
		return FALSE;
	size = Fseek(0L, fd, SEEK_END);
	Fseek(0L, fd, SEEK_SET);
	useRsc = (RSHDR *)Malloc(size);
	if (useRsc == NULL)
	{
		form_error(-(-39) - 31); /* ENOMEM */
		Fclose(fd);
		return FALSE;
	}
	if (Fread(fd, size, useRsc) != size)
	{
		Mfree(useRsc);
		Fclose(fd);
		return FALSE;
	}
	Fclose(fd);

#if WITH_CHECKS
	filesize = size;
#endif

	return xrsrc_fix(useRsc);
}

/*** ---------------------------------------------------------------------- ***/

_WORD xrsrc_free(void)
{
	RSHDR *useRsc;

#ifdef __GNUC__
	/* avoid type-punned pointer */
	{
		void **rscmem = (void **)&aes_global[7];
		useRsc = *rscmem;
	}
#else
	useRsc = _AESrscmem;
#endif
	Mfree(useRsc);
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

_WORD xrsrc_gaddr(_WORD type, _WORD idx, void *gaddr)
{
	RSHDR *useRsc;
	RSXHDR xrsc_header;

#ifdef __GNUC__
	/* avoid type-punned pointer */
	{
		void **rscmem = (void **)&aes_global[7];
		useRsc = *rscmem;
	}
#else
	useRsc = _AESrscmem;
#endif
#if WITH_CHECKS
	if (gaddr == NULL || useRsc == NULL)
		return FALSE;
#endif
	*(void **)gaddr = NULL;
	if (IS_XRSC_HEADER(useRsc))
	{
		xrsc_header = *((RSXHDR *)useRsc);
	} else
	{
		xrsrc_hdr2xrsc(&xrsc_header, useRsc);
	}

#define rsc_pointer(offset) ((char *)useRsc + (size_t)offset)
#define rschdr_pointer(offset) rsc_pointer(xrsc_header.offset)

#if WITH_CHECKS
#define CHECK_IDX(field) \
		if (idx < 0 || (_ULONG)idx >= xrsc_header.field) \
			return FALSE;
#else
#define CHECK_IDX(field)
#endif

	switch (type)
	{
	case R_TREE:
		CHECK_IDX(rsh_ntree)
		{
			OBJECT **_rs_trindex = (OBJECT **)rschdr_pointer(rsh_trindex);
			*((OBJECT **)gaddr) = _rs_trindex[idx];
		}
		break;
	case R_OBJECT:
		CHECK_IDX(rsh_nobs)
		{
			OBJECT *_rs_object = (OBJECT *)rschdr_pointer(rsh_object);
			*((OBJECT **)gaddr) = &_rs_object[idx];
		}
		break;
	case R_TEDINFO:
		CHECK_IDX(rsh_nted)
		{
			TEDINFO *_rs_tedinfo = (TEDINFO *)rschdr_pointer(rsh_tedinfo);
			*((TEDINFO **)gaddr) = &_rs_tedinfo[idx];
		}
		break;
	case R_ICONBLK:
		CHECK_IDX(rsh_nib)
		{
			ICONBLK *_rs_iconblk = (ICONBLK *)rschdr_pointer(rsh_iconblk);
			*((ICONBLK **)gaddr) = &_rs_iconblk[idx];
		}
		break;
	case R_BITBLK:
		CHECK_IDX(rsh_nbb)
		{
			BITBLK *_rs_bitblk = (BITBLK *)rschdr_pointer(rsh_bitblk);
			*((BITBLK **)gaddr) = &_rs_bitblk[idx];
		}
		break;
	case R_STRING:
		CHECK_IDX(rsh_nstring)
		{
			char **_rs_frstr = (char **)rschdr_pointer(rsh_frstr);
			*((char **)gaddr) = _rs_frstr[idx];
		}
		break;
	case R_IMAGEDATA:
		CHECK_IDX(rsh_nimages)
		{
			BITBLK **_rs_frimg = (BITBLK **)rschdr_pointer(rsh_frimg);
			*((BITBLK **)gaddr) = _rs_frimg[idx];
		}
		break;
	case R_OBSPEC:
		CHECK_IDX(rsh_nobs)
		{
			OBJECT *_rs_object = (OBJECT *)rschdr_pointer(rsh_object);
			*((_LONG **)gaddr) = &_rs_object[idx].ob_spec.index;
		}
		break;
	case R_TEPTEXT:
		CHECK_IDX(rsh_nted)
		{
			TEDINFO *_rs_tedinfo = (TEDINFO *)rschdr_pointer(rsh_tedinfo);
			*((char ***)gaddr) = &_rs_tedinfo[idx].te_ptext;
		}
		break;
	case R_TEPTMPLT:
		CHECK_IDX(rsh_nted)
		{
			TEDINFO *_rs_tedinfo = (TEDINFO *)rschdr_pointer(rsh_tedinfo);
			*((char ***)gaddr) = &_rs_tedinfo[idx].te_ptmplt;
		}
		break;
	case R_TEPVALID:
		CHECK_IDX(rsh_nted)
		{
			TEDINFO *_rs_tedinfo = (TEDINFO *)rschdr_pointer(rsh_tedinfo);
			*((char ***)gaddr) = &_rs_tedinfo[idx].te_pvalid;
		}
		break;
	case R_IBPMASK:
		CHECK_IDX(rsh_nib)
		{
			ICONBLK *_rs_iconblk = (ICONBLK *)rschdr_pointer(rsh_iconblk);
			*((char ***)gaddr) = (char **)&_rs_iconblk[idx].ib_pmask;
		}
		break;
	case R_IBPDATA:
		CHECK_IDX(rsh_nib)
		{
			ICONBLK *_rs_iconblk = (ICONBLK *)rschdr_pointer(rsh_iconblk);
			*((char ***)gaddr) = (char **)&_rs_iconblk[idx].ib_pdata;
		}
		break;
	case R_IBPTEXT:
		CHECK_IDX(rsh_nib)
		{
			ICONBLK *_rs_iconblk = (ICONBLK *)rschdr_pointer(rsh_iconblk);
			*((char ***)gaddr) = &_rs_iconblk[idx].ib_ptext;
		}
		break;
	case R_BIPDATA:
		CHECK_IDX(rsh_nbb)
		{
			BITBLK *_rs_bitblk = (BITBLK *)rschdr_pointer(rsh_bitblk);
			*((char ***)gaddr) = (char **)&_rs_bitblk[idx].bi_pdata;
		}
		break;
	case R_FRSTR:
		CHECK_IDX(rsh_nstring)
		{
			char **_rs_frstr = (char **)rschdr_pointer(rsh_frstr);
			*((char ***)gaddr) = &_rs_frstr[idx];
		}
		break;
	case R_FRIMG:
		CHECK_IDX(rsh_nimages)
		{
			BITBLK **_rs_frimg = (BITBLK **)rschdr_pointer(rsh_frimg);
			*((BITBLK ***)gaddr) = &_rs_frimg[idx];
		}
		break;
	default:
		return FALSE;
	}

#undef rsc_pointer
#undef rschdr_pointer
#undef CHECK_IDX

	return TRUE;
}
