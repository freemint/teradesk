/*
 * Teradesk. Copyright (c) 1993, 1994, 2002 W. Klaren.
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

#include <np_aes.h>			/* HR 151102: modern */
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "resource.h"
#include "showinfo.h"
#include "xfilesys.h"
#include "icon.h"
#include "file.h"
#include "window.h"

static void cv_ttoform(char *tstr, unsigned int time)
{
	unsigned int sec, min, hour, h;

	sec = (time & 0x1F) * 2;
	h = time >> 5;
	min = h & 0x3F;
	hour = (h >> 6) & 0x1F;
	digit(tstr, hour);
	digit(tstr + 2, min);
	digit(tstr + 4, sec);
}

static void cv_dtoform(char *tstr, unsigned int date)
{
	unsigned int day, mon, year, h;

	day = date & 0x1F;
	h = date >> 5;
	mon = h & 0xF;
	year = ((h >> 4) & 0x7F) + 80;
	digit(tstr, day);
	digit(tstr + 2, mon);
	digit(tstr + 4, year);
}

static int si_error(const char *name, int error)
{
	return xhndl_error(MESHOWIF, error, name);
}

static int si_dialog(OBJECT *tree, int start, int *x, int *y)
{
	int exit, w = tree->ob_width / 2, h = tree->ob_height / 2;

	tree->ob_x = *x - w;
	tree->ob_y = *y - h;

	exit = xd_dialog(tree, start);

	*x = tree->ob_x + w;
	*y = tree->ob_y + h;

	return exit;
}

#pragma warn -par
static int si_drive(const char *path, int *x, int *y)
{
	char *disklabel;
	char dskl[14];
	long nfolders, nfiles, bytes;
	int drive, error = 0, result = 0;
	DISKINFO diskinfo;

	if (check_drive(path[0] - 'A') != FALSE)
	{
		drive = path[0];

		driveinfo[DIDRIVE].ob_spec.obspec.character = drive;
		disklabel = driveinfo[DILABEL].ob_spec.tedinfo->te_ptext;

		graf_mouse(HOURGLASS, NULL);

		if ((error = cnt_items(path, &nfolders, &nfiles, &bytes, 0x11 | options.attribs)) == 0)
			if ((error = x_getlabel(drive - 'A', dskl)) == 0)
				x_dfree(&diskinfo, drive - 'A' + 1);

		graf_mouse(ARROW, NULL);

		if (error == 0)
		{
			long clsize;

			clsize = diskinfo.b_secsiz * diskinfo.b_clsiz;

			cv_fntoform(disklabel, dskl, 12);		/* HR 271102 */
			rsc_ltoftext(driveinfo, DIFOLDER, nfolders);
			rsc_ltoftext(driveinfo, DIFILES, nfiles);
			rsc_ltoftext(driveinfo, DIBYTES, bytes);
			rsc_ltoftext(driveinfo, DIFREE, diskinfo.b_free * clsize);
			rsc_ltoftext(driveinfo, DISPACE, diskinfo.b_total * clsize);
			if (si_dialog(driveinfo, 0, x, y) == DIABORT)
				result = XABORT;
		}
		else
			result = si_error(path, error);
	}

	return result;
}

#pragma warn .par

static int frename(const char *oldname, const char *newname)
{
	int error;

	graf_mouse(HOURGLASS, NULL);
	error = x_rename(oldname, newname);
	graf_mouse(ARROW, NULL);

	if (error == 0)
		wd_set_update(WD_UPD_MOVED, oldname, newname);

	return error;
}

static int folder_info(const char *oldname, const char *fname, XATTR *attr, int *x, int *y)
{
	char *name, *time, *date;
	char nfname[256], *newname;
	long nfolders, nfiles, bytes;
	int error, start, button, result = 0;

	name = xd_get_obspec(folderinfo + FINAME).tedinfo->te_ptext;	/* HR 021202 */
	time = folderinfo[FITIME].ob_spec.tedinfo->te_ptext;
	date = folderinfo[FIDATE].ob_spec.tedinfo->te_ptext;

	graf_mouse(HOURGLASS, NULL);
	error = cnt_items(oldname, &nfolders, &nfiles, &bytes, 0x11 | options.attribs);
	graf_mouse(ARROW, NULL);

	if (error == 0)
	{
		cv_fntoform(name, fname, 64);		/* HR 271102 */
		cv_ttoform(time, attr->mtime);
		cv_dtoform(date, attr->mdate);
		rsc_ltoftext(folderinfo, FIFOLDER, nfolders);
		rsc_ltoftext(folderinfo, FIFILES, nfiles);
		rsc_ltoftext(folderinfo, FIBYTES, bytes);
		if (tos1_4())
		{
			start = FINAME;
			folderinfo[FINAME].ob_flags |= EDITABLE;
		}
		else
		{
			start = 0;
			folderinfo[FINAME].ob_flags &= ~EDITABLE;
		}
		button = si_dialog(folderinfo, start, x, y);
		if (button == FIOK)
		{
			cv_formtofn(nfname, name);

			if (strcmp(nfname, fname) != 0)
			{
				if ((newname = fn_make_newname(oldname, nfname)) != NULL)
				{
					if ((error = frename(oldname, newname)) != 0)
						result = si_error(fname, error);
					free(newname);
				}
			}
		}
		else if (button == FIABORT)
			result = XABORT;
	}
	else
		result = si_error(fname, error);

	return result;
}

static void set_file_attribs(int attribs)		/* HR 151102: preserve extended bits */
{
	if (attribs & FA_READONLY)
		fileinfo[ISWP].ob_state |= SELECTED;
	else
		fileinfo[ISWP].ob_state &= ~SELECTED;

	if (attribs & FA_HIDDEN)
		fileinfo[ISHIDDEN].ob_state |= SELECTED;
	else
		fileinfo[ISHIDDEN].ob_state &= ~SELECTED;

	if (attribs & FA_SYSTEM)
		fileinfo[ISSYSTEM].ob_state |= SELECTED;
	else
		fileinfo[ISSYSTEM].ob_state &= ~SELECTED;
}

static int get_file_attribs(int old_attribs)
{
	int attribs = (old_attribs & 0xFFF8);

	attribs |= (fileinfo[ISWP].ob_state & SELECTED) ? FA_READONLY : 0;			/* HR 151102 */
	attribs |= (fileinfo[ISHIDDEN].ob_state & SELECTED) ? FA_HIDDEN : 0;
	attribs |= (fileinfo[ISSYSTEM].ob_state & SELECTED) ? FA_SYSTEM : 0;

	return attribs;
}

static int fattrib(const char *name, int attribs)
{
	int error;

	graf_mouse(HOURGLASS, NULL);
	error = x_fattrib(name, 1, attribs);
	graf_mouse(ARROW, NULL);

	return (error >= 0) ? 0 : error;
}

static int file_info(const char *oldname, const char *fname, XATTR *attr, int *x, int *y)
{
	char *name, *time, *date, nfname[256], *newname;
	int button, attrib = attr->attr, result = 0;

	name = xd_get_obspec(fileinfo + FLNAME).tedinfo->te_ptext;
	time = fileinfo[FLTIME].ob_spec.tedinfo->te_ptext;
	date = fileinfo[FLDATE].ob_spec.tedinfo->te_ptext;

	cv_fntoform(name, fname, 64);		/* HR 271102 */
	cv_ttoform(time, attr->mtime);
	cv_dtoform(date, attr->mdate);
	rsc_ltoftext(fileinfo, FLBYTES, attr->size);
#if _MINT_				/* HR 151102 */
	if (!mint)
#endif
		set_file_attribs(attrib);

	button = si_dialog(fileinfo, FLNAME, x, y);

	if (button == FLOK)
	{
		int error = 0, new_attribs;

#if _MINT_				/* HR 151102 */
		if (mint)
			new_attribs = attrib;
		else
#endif
			new_attribs = get_file_attribs(attrib);

		cv_formtofn(nfname, name);

		if ((newname = fn_make_newname(oldname, nfname)) != NULL)
		{
#if _MINT_				/* HR 151102 */
			if (!mint)
#endif
			{
				if (((new_attribs & FA_READONLY) == 0) && (new_attribs != attrib))
					error = fattrib(oldname, new_attribs);
			}

			if ((strcmp(nfname, fname) != 0) && (error == 0))
				error = frename(oldname, newname);

#if _MINT_				/* HR 151102 */
			if (!mint)
#endif
			{
				if (((new_attribs & FA_READONLY) != 0) && (error == 0) && (new_attribs != attrib))
					error = fattrib(newname, new_attribs);
			}

			if (error != 0)
				result = si_error(fname, error);

			if (result != XFATAL)
			{
				if ((new_attribs != attrib) && (strcmp(nfname, fname) == 0))
					wd_set_update(WD_UPD_COPIED, oldname, NULL);
			}

			free(newname);
		}
	}
	else if (button == FLABORT)
		result = XABORT;

	return result;
}

void item_showinfo(WINDOW *w, int n, int *list)
{
	int i, item, error, x, y, oldmode, result = 0;
	ITMTYPE type;
	boolean curr_pos;
	XATTR attrib;
	const char *path, *name;

	x = -1;
	y = -1;
	curr_pos = FALSE;

	for (i = 0; i < n; i++)
	{
		item = list[i];

		if ((x >= 0) && (y >= 0))
		{
			oldmode = xd_setposmode(XD_CURRPOS);
			curr_pos = TRUE;
		}

		type = itm_type(w, item);

		if ((type == ITM_FOLDER) || (type == ITM_FILE) || (type == ITM_PROGRAM) || (type == ITM_DRIVE))
		{
			if ((path = itm_fullname(w, item)) == NULL)
				result = XFATAL;
			else
			{
				if (type == ITM_DRIVE)
					result = si_drive(path, &x, &y);
				else
				{
					name = itm_name(w, item);

					graf_mouse(HOURGLASS, NULL);
					error = itm_attrib(w, item, 0, &attrib);
					graf_mouse(ARROW, NULL);

					if (error != 0)
						result = si_error(name, error);
					else
					{
						if (type == ITM_FOLDER)
							result = folder_info(path, name, &attrib, &x, &y);
						else
							result = file_info(path, name, &attrib, &x, &y);
					}
				}
				free(path);
			}
		}

		if (curr_pos == TRUE)
			xd_setposmode(oldmode);

		if ((result == XFATAL) || (result == XABORT))
			break;
	}

	wd_do_update();
}
