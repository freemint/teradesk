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
#include <stdlib.h>
#include <string.h>
#include <vdi.h>
#include <xdialog.h>
#include <mint.h>

#include "desk.h"
#include "error.h"
#include "resource.h"
#include "slider.h"
#include "xfilesys.h"
#include "file.h"
#include "icon.h"
#include "prgtype.h"
#include "window.h"
#include "icontype.h"

#define NLINES		4
#define END			32767

typedef struct icontype
{
	char type[14];
	int icon;
	char icon_name[14];
	struct icontype *next;
} ICONTYPE;

typedef struct
{
	int icon;
	int resvd;
} ITYPE;

static ICONTYPE *files, *folders, **curlist;

static int find_icon(const char *name, ICONTYPE *list)
{
	ICONTYPE *p = list;

	while (p)
	{
		if (cmp_wildcard(name, p->type) == TRUE)
			return min(n_icons - 1, p->icon);
		p = p->next;
	}
	return -1;
}

/* HR 151102: find icons by name, not index */
int icnt_geticon(const char *name, ITMTYPE type)
{
	int icon;

	if ((type == ITM_PREVDIR) || (type == ITM_FOLDER))
	{
		if ((icon = find_icon(name, folders)) < 0)
			icon = rsrc_icon("FOLDER");		/* HR 151102 */
	}
	else
	{
		if ((icon = find_icon(name, files)) < 0)
		{
			if (prg_isprogram(name) == FALSE)
				icon = rsrc_icon("FILE");		/* HR 151102 */
			else
				icon = rsrc_icon("APP");		/* HR 151102 */
		}
	}

	if (icon < 0)				/* HR 151102 */
		icon = 0;

	return icon;
}

static void free_list(ICONTYPE **list)
{
	ICONTYPE *p = *list, *next;

	while (p)
	{
		next = p->next;
		free(p);
		p = next;
	}

	*list = NULL;
}

static ICONTYPE *add(ICONTYPE **list, char *name, int icon, int pos)
{
	ICONTYPE *p, *prev, *n;
	int i = 0;

	p = *list;
	prev = NULL;

	while ((p != NULL) && (i != pos))
	{
		prev = p;
		p = p->next;
		i++;
	}

	if ((n = malloc(sizeof(ICONTYPE))) == NULL)
		xform_error(ENSMEM);
	else
	{
		strcpy(n->type, name);
		strncpy(n->icon_name,
		        icons[icon].ob_spec.ciconblk->monoblk.ib_ptext,
		        12);							/* HR 151102: maintain rsrc icon name */
		n->icon_name [12] = 0;
		n->icon = icon;
		n->next = p;

		if (prev == NULL)
			*list = n;
		else
			prev->next = n;
	}

	return n;
}

static void rem(ICONTYPE **list, ICONTYPE *item)
{
	ICONTYPE *p, *prev;

	p = *list;
	prev = NULL;

	while ((p != NULL) && (p != item))
	{
		prev = p;
		p = p->next;
	}

	if (p == item)
	{
		if (prev == NULL)
			*list = p->next;
		else
			prev->next = p->next;

		free(p);
	}
}

static boolean copy(ICONTYPE **copy, ICONTYPE *list)
{
	ICONTYPE *p;

	p = list;
	*copy = NULL;

	while (p != NULL)
	{
		if (add(copy, p->type, p->icon, END) == NULL)
		{
			free_list(copy);
			return FALSE;
		}
		p = p->next;
	}

	return TRUE;
}

static int cnt_types(ICONTYPE *list)
{
	int n = 1;
	ICONTYPE *p = list;

	while (p != NULL)
	{
		n++;
		p = p->next;
	}

	return n;
}

static ICONTYPE *get_item(ICONTYPE *list, int item)
{
	int i = 0;
	ICONTYPE *p = list;

	while ((p != NULL) && (i != item))
	{
		i++;
		p = p->next;
	}

	return p;
}

static void pset_selector(SLIDER *slider, boolean draw, XDINFO *info)
{
	int i;
	ICONTYPE *p;
	OBJECT *o;

	for (i = 0; i < NLINES; i++)
	{
		o = &seticntype[ITYPE1 + i];

		if ((p = get_item(*curlist, i + slider->line)) == NULL)
			*o->ob_spec.tedinfo->te_ptext = 0;
		else
			cv_fntoform(o->ob_spec.tedinfo->te_ptext, p->type, 12);		/* HR 271102 */
	}

	if (draw == TRUE)
		xd_draw(info, IPARENT, MAX_DEPTH);
}

static int pfind_selected(void)
{
	int object;

	return ((object = xd_get_rbutton(seticntype, IPARENT)) < 0) ? 0 : object - ITYPE1;
}

static void set_selector(SLIDER *slider, boolean draw, XDINFO *info)
{
#if NEWICON
	OBJECT *h1, *ic;				/* HR 151102 */

	ic = icons + slider->line;
	h1 = addicntype + AITDATA;

	h1->ob_type = ic->ob_type;
	h1->ob_spec = ic->ob_spec;

	h1->ob_x = (addicntype[AITBACK].ob_width  - ic->ob_width ) / 2;
	h1->ob_y = (addicntype[AITBACK].ob_height - ic->ob_height) / 2;

#else
	BITBLK *h1;
	CICONBLK *h2;		/* HR 151102: ciconblk (the largest) */

	h1 = addicntype[AITDATA].ob_spec.bitblk;
	h2 = icons[(long) slider->line].ob_spec.ciconblk;		/* HR 151102 */

	h1->bi_pdata = h2->monoblk.ib_pdata;
	h1->bi_wb = h2->monoblk.ib_wicon / 8;
	h1->bi_hl = h2->monoblk.ib_hicon;

	addicntype[AITDATA].ob_x = (addicntype[AITBACK].ob_width  - h2->monoblk.ib_wicon) / 2;
	addicntype[AITDATA].ob_y = (addicntype[AITBACK].ob_height - h2->monoblk.ib_hicon) / 2;
#endif

	if (draw == TRUE)
		xd_draw(info, AITBACK, 1);
}

static boolean icntype_dialog(char *name, int *icon, boolean edit)
{
	int button;
	SLIDER sl_info;

	rsc_title(addicntype, AITTITLE, (edit == TRUE) ? DTEDTICT : DTADDICT);

	cv_fntoform(icnname, name, 12);		/* HR 271102 */

	sl_info.type = 0;
	sl_info.up_arrow = ITUP;
	sl_info.down_arrow = ITDOWN;
	sl_info.slider = ITSLIDER;
	sl_info.sparent = ITPARENT;
	sl_info.lines = 1;
	sl_info.n = n_icons;
	sl_info.line = *icon;
	sl_info.set_selector = set_selector;
	sl_info.first = 0;
	sl_info.findsel = 0;

	button = sl_dialog(addicntype, AITTYPE, &sl_info);

	if ((button == AITOK) && (strlen(dirname) != 0))
	{
		cv_formtofn(name, icnname);
		*icon = sl_info.line;
		return TRUE;
	}
	else
		return FALSE;
}

void icnt_settypes(void)
{
	int button, icon;
	int i;
	XDINFO info;
	boolean stop = FALSE, redraw;
	ICONTYPE *cfiles, *cfolders, *p, **newlist;
	char name[14];
	SLIDER sl;

	curlist = (seticntype[ITFOLDER].ob_state & SELECTED) ? &cfolders : &cfiles;

	cfiles = NULL;
	cfolders = NULL;

	if ((copy(&cfiles, files) == FALSE) || (copy(&cfolders, folders) == FALSE))
	{
		free_list(&cfiles);
		free_list(&cfolders);
	}

	sl.type = 1;
	sl.up_arrow = IUP;
	sl.down_arrow = IDOWN;
	sl.slider = ISLIDER;
	sl.sparent = ISPARENT;
	sl.lines = NLINES;
	sl.n = cnt_types(*curlist);
	sl.line = 0;
	sl.set_selector = pset_selector;
	sl.first = ITYPE1;
	sl.findsel = pfind_selected;

	sl_init(seticntype, &sl);

	xd_open(seticntype, &info);

	while (stop == FALSE)
	{
		redraw = FALSE;

		button = sl_form_do(seticntype, 0, &sl, &info) & 0x7FFF;

		if ((button != ITFOLDER) && (button != ITFILES))
		{
			switch (button)
			{
			case ITADD:
				name[0] = 0;
				icon = 0;

				if (icntype_dialog(name, &icon, FALSE) == TRUE)
				{
					i = pfind_selected() + sl.line;
					add(curlist, name, icon, i);
					sl.n = cnt_types(*curlist);
					redraw = TRUE;
					sl_set_slider(seticntype, &sl, &info);
				}
				break;
			case ITDEL:
				i = pfind_selected() + sl.line;
				if ((p = get_item(*curlist, i)) != NULL)
				{
					rem(curlist, p);
					sl.n = cnt_types(*curlist);
					redraw = TRUE;
					sl_set_slider(seticntype, &sl, &info);
				}
				break;
			case ITCHNG:
				i = pfind_selected() + sl.line;
				if ((p = get_item(*curlist, i)) != NULL)
				{
					redraw = TRUE;
					icntype_dialog(p->type, &p->icon, TRUE);
				}
				break;
			default:
				stop = TRUE;
				break;
			}
			xd_change(&info, button, NORMAL, (stop == FALSE) ? 1 : 0);
		}
		else
		{
			newlist = (button == ITFOLDER) ? &cfolders : &cfiles;
			if (newlist != curlist)
			{
				redraw = TRUE;
				curlist = newlist;
				sl.n = cnt_types(*newlist);
				sl.line = 0;
				sl_set_slider(seticntype, &sl, &info);
			}
		}
		if (redraw == TRUE)
			pset_selector(&sl, TRUE, &info);
	}

	xd_close(&info);

	if (button == ITOK)
	{
		free_list(&files);
		free_list(&folders);
		files = cfiles;
		folders = cfolders;
		wd_seticons();
	}
	else
	{
		free_list(&cfiles);
		free_list(&cfolders);
	}
}

void icnt_init(void)
{
	files = NULL;
	folders = NULL;
}

void icnt_default(void)
{
	free_list(&files);
	free_list(&folders);
}

static int load_list(XFILE *file, ICONTYPE **list)
{
	ITYPE it;
	char name[14], icon_name[14];
	long n;
	int error;

	free_list(list);

	do
	{
		if ((n = x_fread(file, &it, sizeof(ITYPE))) != sizeof(ITYPE))
			return (n < 0) ? (int) n : EEOF;

		if (it.icon != -1)
		{
			int icon;
			if (x_freadstr(file, name, &error) == NULL)
				return error;
			if (x_freadstr(file, icon_name, &error) == NULL)
				return error;

			icon = rsrc_icon(icon_name);		/* HR 151102: find icons by name, not index */
			if (icon >= 0)
				if (add(list, name, icon, END) == NULL)
					return ERROR;
		}
	}
	while (it.icon != -1);

	return 0;
}

int icnt_load(XFILE *file)
{
	int error;

	if ((error = load_list(file, &files)) < 0)
		return error;
	return load_list(file, &folders);
}

static boolean save_list(XFILE *file, ICONTYPE *list)
{
	ITYPE it;
	ICONTYPE *p;
	int error;
	long n;

	p = list;

	while (p)
	{
		it.icon = p->icon;
		it.resvd = 0;

		if ((n = x_fwrite(file, &it, sizeof(ITYPE))) < 0)
			return (int) n;

		if ((error = x_fwritestr(file, p->type)) < 0)
			return error;
		if ((error = x_fwritestr(file, p->icon_name)) < 0)
			return error;

		p = p->next;
	}

	it.icon = -1;

	return ((n = x_fwrite(file, &it, sizeof(ITYPE))) < 0) ? (int) n : 0;
}

int icnt_save(XFILE *file)
{
	int error;

	if ((error = save_list(file, files)) < 0)
		return error;
	return save_list(file, folders);
}
