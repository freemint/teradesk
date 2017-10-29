/*
 * Teradesk. Copyright (c) 1997 - 2002  W. Klaren.
 *                         2002 - 2003  H. Robbers
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


#include <vaproto.h>

#define FONT_CHANGED	0x7A18
#define FONT_SELECT		0x7A19		/* Call to font selector. */
#define FONT_ACK		0x7A1A

#define VV_FONTASKED	0x1000		/* flag for av-protocol messaging */

/*
 * Structure for logging AV-client data. 
 * Note: "next" must be at an ofset equal to size of SNAME (i.e. 18)
 * in order for LSTYPE routines to be used
 */

typedef struct avtype
{
	char name[10];				/* AV-client name */
	_WORD ap_id;				/* AV-client AP-id */
	_WORD avcap3;				/* AV-client capabilities (word3) */
	_WORD flags;				/* diverse flags */
	_WORD resvd;				/* placeholder for compatibility */
	struct avtype *next;		/* Pointer to the next item */
} AVTYPE;


/*
 * Structure for logging status of AV clients
 */

typedef struct avstat
{
	SNAME name;
	struct avstat *next;
	char *stat;
} AVSTAT;

#define AVCOPYING 0x01

/*
 * Structure for logging windows opened by other apps.
 * For the time being it is exactly the same as WINDOW structure.
 */

typedef struct
{
	ITM_INTVARS;
} ACC_WINDOW;

/*
 * Structore for data to set size of the next open window
 */

typedef struct
{
	bool flag;
	RECT size;
} AVSETW;

void va_config(XFILE *file, int lvl, int io, int *error);

extern AVSETW avsetw;
extern bool va_reply;
extern AVTYPE *avclients;
extern _WORD av_current;
extern char const thisapp[];

void va_init(void);
WINDOW *va_accw(void);
void va_delall(_WORD ap_id, bool force);
void rem_all_avstat(void);

#if __USE_MACROS
#define vastat_default rem_all_avstat
/*
#define va_close  xw_closedelete
*/
#define va_close xw_close
#else
void vastat_default(void);
void va_close(WINDOW *w);
#endif

void handle_av_protocol(const _WORD *message);
_WORD va_start_prg(const char *program, ApplType type, const char *cmdline);
AVTYPE *va_findclient(_WORD ap_id);
bool va_add_name(_WORD type, const char *name );
bool va_accdrop(WINDOW *dw, WINDOW *sw, _WORD  *list, _WORD  n, _WORD  kstate, _WORD x, _WORD y);
bool va_fontreply(_WORD messid, _WORD dest_ap_id);
bool va_pathupdate(WINDOW *w);
void va_checkclient(void);
