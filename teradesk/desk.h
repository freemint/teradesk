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

#include <boolean.h>
#include <stddef.h>

#ifdef MEMDEBUG
#include <d:\tmp\memdebug\memdebug.h>
#endif

char *itoa(int value, char *string, int radix);
char *ltoa(long value, char *string, int radix);

#undef O_APPEND

#define min(x,y)		(((x) < (y)) ? (x) : (y))
#define max(x,y)		(((x) > (y)) ? (x) : (y))

#define MAGIC		0x87654321L

#define GLOBAL_MEM_SIZE	1024L

typedef enum
{
	PGEM = 0, PGTP, PTOS, PTTP
} ApplType;

typedef struct
{
	int version;
	unsigned long magic;
	int cprefs;
	char mode;
	char sort;
	int attribs;
	int tabsize;
	int bufsize;
	unsigned char dsk_pattern;
	unsigned char dsk_color;
	unsigned int dial_mode:2;
	int resvd1:14;
	int resvd2;
} Options;

typedef struct
{
	int item;					/* nummer van icoon */
	int m_x;					/* coordinaten middelpunt t.o.v. muis, alleen bij iconen */
	int m_y;
	int np;						/* aantal punten */
	int coords[18];				/* coordinaten van schaduw */
} ICND;

typedef struct
{
	int phy_handle;
	int vdi_handle;
	int dsk_x;
	int dsk_y;
	int dsk_w;
	int dsk_h;
	int fnt_w;
	int fnt_h;
} SCRINFO;

/* Opties */

#define CF_COPY			0x001
#define CF_DEL			0x002
#define CF_OVERW		0x004
#define TOS_KEY			0x020	/* 0 = continue, 1 = wait */
#define DIALPOS_MODE	0x040	/* 0 = mouse, 1 = center */
#define SAVE_COLORS		0x100
#define TOS_STDERR		0x200	/* 0 = no redirection, 1 = redirect handle 2 to 1. */

#define TEXTMODE		0
#define ICONMODE		1

#define WD_SORT_NAME	0
#define WD_SORT_EXT		1
#define WD_SORT_DATE	2
#define WD_SORT_LENGTH	3
#define WD_NOSORT		4

extern Options options;
extern SCRINFO screen_info;
extern int vdi_handle, ncolors, max_w, max_h, ap_id, nfonts;
extern boolean quit;

extern char *global_memory;

long btst(long x, int bit);
void set_opt(OBJECT *tree, int opt, int button);

void digit(char *s, int x);
void cv_formtofn(char *dest, const char *source);
void cv_fntoform(char *dest, const char *source);

extern int hndlmessage(int *message);
