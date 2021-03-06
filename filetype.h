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


typedef struct ftype
{
	SNAME filetype;	
	struct ftype *next;
} FTYPE;


extern FTYPE fwork;
extern FTYPE *fthis;
extern FTYPE **ffthis;

extern const char *presets[];

extern const char fas[];			/* file attributes flags */


void ft_config(XFILE *file, int lvl, int io, int *error);

char *wd_filemask(const char *mask);
void ft_init(void);
void ft_default(void);
void copy_ftype(LSTYPE *target, LSTYPE *source);
char *ft_dialog( const char *mask, FTYPE **list, _WORD what );
