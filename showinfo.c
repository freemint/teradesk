/* 
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                               2003, 2004  Dj. Vukovic
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


#include <np_aes.h>	
#include <vdi.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>
#include <mint.h>
#include <xdialog.h>

#include "desk.h"
#include "error.h"
#include "resource.h"
#include "xfilesys.h"
#include "lists.h"  /* must be before slider.h */
#include "slider.h" 
#include "file.h"
#include "font.h"
#include "config.h"
#include "viewer.h"
#include "window.h"
#include "icon.h"
#include "showinfo.h"
#include "copy.h"
#include "dir.h"
#include "va.h"

XDINFO dinfo; 	
int dopen = FALSE;    			/* flag that dialog is open */ 


extern boolean
	can_touch;

extern DOSTIME 
		now,
		optime;	

extern int opattr;
extern int tos_version;

unsigned int Tgettime(void);
unsigned int Tgetdate(void);


/*
 * Closeinfo closes the object info dialog if it was open.
 * Now, closeinfo must come after object_info because
 * object_info does not close the dialog after each item.
 */

void closeinfo(void)
{
	if ( dopen )
	{
		xd_close (&dinfo);
		dopen = FALSE;
	}
}


/* 
 * Convert time to a string to be displayed in a form 
 */

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


/* 
 * Convert date to a string to be displayed in a form 
 */

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


/*
 * Convert a string (format: ddmmyy) to DOS date; for brevity, date is only 
 * partially checked: day can be 1:31 in any month, month is 1:12, year is 0:99;
 * return -1 if invalid date is entered;  
 */

static int cv_formtod( char *dstr )
{
	char b[3] = {0,0,0};
	int d,m,y,dd;

	if ( dstr[0] == 0 )
		return 0;

	b[0] = dstr[0];
	b[1] = dstr[1];
	b[2] = 0;
	d = atoi(b);

	b[0] = dstr[2];
	b[1] = dstr[3];
	m = atoi(b);

	b[0] = dstr[4];
	b[1] = dstr[5];

	y = atoi(b);
	if ( y < 80 )
		y += 100;
	y -= 80;

	dd = d | (m<<5) | (y<<9);

	if ( d < 1 || d > 31 || m < 1 || m > 12 )
		return -1;

	return dd;
} 

/*
 * Convert a string to DOS time
 */

static int cv_formtot( char *tstr )
{
	int h, m, s, tt;

	char b[3] = {0,0,0};

	if ( tstr[0] == 0 )
		return 0;

	b[0] = tstr[4];
	b[1] = tstr[5];

	s = atoi(b);

	b[0] = tstr[2];
	b[1] = tstr[3];

	m = atoi(b);

	b[0] = tstr[0];
	b[1] = tstr[1];

	h = atoi(b);

	tt = (h << 11) | (m << 5) | (s/2); 

	if ( h > 23 || m > 59 || s > 59 )
		return -1;

	return tt;
}

/*
 * Search parameters: pattern, string, date range, size range...
 */

LNAME 
	search_pattern, /* store filename pattern for search */
	search_txt;	    /* store string to search for */

long
	find_offset,	/* offset of found string from file start */
	search_losize,	/* low end of file size range for search, inclusive */
	search_hisize;	/* high end of file size range for search, inclusive */

int 
	search_lodate,	/* low end of file/folder date for search, inclusive */
	search_hidate,	/* high end of file/folder date for search, inclusive */
	search_nsm,		/* number of found string matches */	
	search_icase;	/* if not 0, ignore string character case */


size_t
	search_length;	/* length of string to search for */

char 
	*search_buf,	/* buffer to store file to serch string */
	**search_finds;	/* pointers to found strings */


/* 
 * Handle dialog to set data relevant for file, folder or string search.
 */

static int search_dialog(void)
{
	XDINFO 
		info;		/* dialog info */

	int 
		button = 0;		/* code of pressed button */


	/* Clear all fields */

	*(searching[SGREP].ob_spec.tedinfo->te_ptext) = 0;
	*spattfld = 0;

	search_txt[0] = 0;

	*(searching[SLOSIZE].ob_spec.tedinfo->te_ptext) = 0;
	*(searching[SHISIZE].ob_spec.tedinfo->te_ptext) = 0;
	*(searching[SLODATE].ob_spec.tedinfo->te_ptext) = 0;
	*(searching[SHIDATE].ob_spec.tedinfo->te_ptext) = 0;
	set_opt( searching, search_icase, 1, IGNCASE); 

	/* Open the dialog */

	xd_open(searching, &info);

	/* Edit data */

	while (button != SOK && button != SCANCEL)
	{
		/* Wait for appropriate button */

		button = xd_form_do ( &info, ROOT );

		xd_change(&info, button, NORMAL, TRUE);

		if ( button == SOK )
		{
			/* OK, get data out of dialog, but check it */

			cv_formtofn( search_pattern, &searching[SMASK] );
			search_losize = atol(searching[SLOSIZE].ob_spec.tedinfo->te_ptext);
			search_hisize = atol(searching[SHISIZE].ob_spec.tedinfo->te_ptext);
			if ( search_hisize == 0L )
				search_hisize = 0x7FFFFFFFL; /* a very large size */
				
			search_lodate = cv_formtod(searching[SLODATE].ob_spec.tedinfo->te_ptext);
			search_hidate = cv_formtod(searching[SHIDATE].ob_spec.tedinfo->te_ptext);
			if ( search_hidate == 0 )
				search_hidate = 32671; 	     /* a very late date  */

			strcpy (search_txt, searching[SGREP].ob_spec.tedinfo->te_ptext);
			search_length = strlen(search_txt);

			get_opt( searching, &search_icase, 1, IGNCASE); 

			/* Check if parameters entered have sensible values */

			if (  *search_pattern == 0 ||		/* must have a pattern */
	              search_lodate == -1  ||		/* valid low date      */ 
	              search_hidate == -1  ||		/* valid high date     */
	              search_hidate < search_lodate || /* valid date range */
	              search_hisize < search_losize /* valid size range    */ 
               )
			{
				alert_iprint(MINVSRCH);
				button = 0;
			}
		} 		/* ok? */
	} 			/* while */

	/* Close the dialog */

	xd_close( &info );
	return button;
}


/*
 * Find first occurence of a string in the text read from a file into the buffer;
 * Currently this is a very primitive and -very- inefficient routine
 * but there's room for improvement (e.g. another basic algorithm, use of 
 * wildcards, etc). Routine returns pointer to the found string, or NULL if 
 * not found. For multiple searches, pointer to buffer has to be updated 
 * between calls, so that each new pointer points after the previous one 
 */

char *find_string
( 
	char *buffer,	/* buffer in which the string is searched for */ 
	size_t lb		/* length of data in the buffer */ 
)
{
	char 
		*p,			/* pointer to currently examined location              */
		*pend;		/* pointer to last location to examine (near file end) */


	p = buffer;							/* start from the beginning */
	pend = p + (lb - search_length);	/* go to (almost) file end */

	while ( p < pend )
	{
		if ( search_icase ) 
		{
			if (strnicmp (p, search_txt, search_length) == 0)
				return p;
		}
		else
		{
			if (strncmp (p, search_txt, search_length) == 0)
				return p;
		}
		p++;
	}
	return NULL;
}


/*
 * Match file/folder data against search criteria 
 * (name, size, date, or containing string),
 * return TRUE if matched
 */

boolean searched_found
( 
	char *path,		/* pointer to file/folder path */ 
	char *name, 	/* pointer to file/folder name */
	XATTR *attr		/* attributes of above */ 
)
{
	boolean 
		found = FALSE;	/* true if match found */

	char 
		*fpath = NULL,	/* pointer to full file/folder name */
		*p;				/* local pointer to string found */

	long
		i,				/* local counter */
		fl;				/* length of file just read */

	int 
		error;			/* error code */


	/* Nothing found yet */

	search_nsm = 0;

	/* if name matched... */

	if ( (found = cmp_wildcard ( name, search_pattern )) != 0 ) 
	{
		/* ...and date range matched... */

		if ( (found = (attr->mdate >= search_lodate && attr->mdate <= search_hidate ) ) == TRUE )
		{
			/* ...and size range matched... */

			if ( (found == ( ((attr->mode & S_IFMT) == S_IFDIR) || (attr->size >= search_losize && attr->size <= search_hisize))))
			{
				/* Find string, if specified */

				if ( search_txt[0] > 0 )
				{
					fpath = x_makepath(path, name, &error);
					found = FALSE;

					if ( !error )
					{
						/* 
						 * First read complete file 
						 * (remember to free buffer when finished)
                         */

						if ( (error = read_txtfile( fpath,  &search_buf, &fl, NULL, NULL )) == 0)
						{
							/* 
							 * for the sake of display of found locations, 
							 * convert all 0-chars to something else 
							 * (should be some char which can't be entered into
							 * the string from the keyboard, e.g. [DEL])
							 * Note 2: there is a similar substitution of  
							 * zeros in viewer.c;
							 * take care to use the same substitute (maybe define a macro)
							 */

							for ( i = 0; i < fl; i++ )
								if ( search_buf[i] == 0 )
									search_buf[i] = 127; /* [DEL], usually represented by a triangle  */

							/* 
							 * Allocate memory for pointers to found strings
							 * ( max. MFINDS of them ); 
							 * remember to free it when finished! 
							 */

							if ((search_finds = malloc( (MFINDS + 2) * sizeof(char *))) != NULL)
							{
								/* Find and count all instances of searched string */

								p = search_buf;
						
								while ( p && search_nsm < MFINDS )
								{
									p = find_string(p, (size_t)(search_buf + fl - p) );
									if ( p )
									{
										/* Match found! */

										found = TRUE;
										search_finds[search_nsm] = p;
										search_nsm++;

										/* 
										 * Which behaviour best to select here?
										 * if it is "p++", next search starts from
										 * the next character, e.g. if string
										 * "ABCABC" is searched in "ABCABCABCABC"
										 * 3 instances are found, otherwise
										 * only 2 instances are found- but then
										 * the search is finished sooner 
										 */

										/* p++; */
										p = p + search_length;
									}
								}

								/* 
								 * Free pointers to finds if nothing found 
								 * (otherwise they will be neeed later)
								 */

								if ( search_nsm == 0 )
									free(search_finds);	
							}

							/* Free buffer if nothing found */

							if ( search_nsm == 0 )
								free(search_buf); /* contains file being examined */
	
						} /* read textfile OK ? */

					} /* no error ? */

					free(fpath);
  
				} 	/* search string specified ? */
			} 		/* size matched ? */
		}			/* date matched ? */
	}				/* name matched ? */

	return found;
}


/*
 * Display alert: Can't read/set info for...
 */

static int si_error(const char *name, int error)
{
	return xhndl_error(MESHOWIF, error, name);
}


/* 
 * Enter state of file attributes into dialog buttons.
 * This preserves extended attribute bits 
 */

static void set_file_attribs(int attribs)
{
	set_opt( fileinfo, attribs, FA_READONLY, ISWP );
	set_opt( fileinfo, attribs, FA_HIDDEN,   ISHIDDEN );
	set_opt( fileinfo, attribs, FA_SYSTEM,   ISSYSTEM );
	set_opt( fileinfo, attribs, FA_ARCHIVE,  ISARCHIV );
}


/* 
 * Get state of file attributes from dialog, change attributes 
 * (preserve other attributte bits) 
 */

static int get_file_attribs(int old_attribs)
{
	int attribs = (old_attribs & 0xFFD8);

	get_opt ( fileinfo, &attribs, FA_READONLY, ISWP );
	get_opt ( fileinfo, &attribs, FA_HIDDEN, ISHIDDEN );
	get_opt ( fileinfo, &attribs, FA_SYSTEM, ISSYSTEM );
	get_opt ( fileinfo, &attribs, FA_ARCHIVE, ISARCHIV );

	return attribs;
}


/*
 * Prepare for display the surroundings of the found string, if there are any
 */

static void disp_smatch( int ism )
{
	int
		fld,		/* length of display field */
		nc1,		/* number of chars to display before searched string */
		nc2;		/* number of chars to display after searched string */

	char
		*s1,		/* string before searched */
		*s2,	 	/* string after searched */ 
		*disp;		/* form string to display */

	/* Show number of matches */

	rsc_ltoftext(fileinfo, ISMATCH, (long)(ism + 1) );
	rsc_ltoftext(fileinfo, NSMATCH, (long)search_nsm);

	disp = fileinfo[MATCH1].ob_spec.tedinfo->te_ptext;	/* pointer to field */
	fld = (int)strlen(fileinfo[MATCH1].ob_spec.tedinfo->te_pvalid); /* length of */

	/* Completely clear display field (otherwise it won't work!) */

	memset( &disp[0], 0, (size_t)fld );

	/* 
	 * Try to display some text just before found string. 
	 * Must not be before beginning of buffer (i.e. file) 
	 */

	s1 = search_finds[ism] - (fld - 5) / 2;

	if ( s1 < search_buf )
		s1 = search_buf;				

	nc1 = (int)(search_finds[ism] - s1); /* number of characters to display */

	/* 
	 * Display the text just after the found string. This will be terminated 
	 * either by field length (i.e. not more than nc2 characters), or by
	 * a nul char at end of buffer, whichever comes sooner 
	 */

	s2 = search_finds[ism] + search_length;
	nc2 = fld - nc1 - 5;				/* not more than field size */

	if ( nc1 > 0 )
		strncpy(disp, s1, (size_t)nc1);

	/*
	 * For display, substitute the string searched for with "[...]"
	 * (which will most often be shorter, so that more of the surrounding
	 * text will be shown
	 */

	strcat(disp, "[...]");				/* substitute for searched text */	
	strncat(disp, s2, (size_t)nc2);		/* add some text after searched */
}


/*
 * Show or set information about a drive, folder or file or link. 
 * Return result code
 */

int object_info
(
	ITMTYPE type,			/* Item type: ITM_FOLDER, ITM_FILE< etc. */ 
	const char *oldname,	/* object path + name */ 
	const char *fname,		/* object name only (why the duplication?) */ 
	XATTR *attr				/* Object's extended attributes */
)
{
	char 
		*time,		/* time string */ 
		*date,		/* date string */ 
		*newname = NULL;

#if _MORE_AV
	char
		*avname;	/* name to be reported to an AV client */

	LNAME
		ltgtname;	/* link target name */
#endif	

	LNAME
		nfname;	

	long 
		nfolders, 	/* number of subfolders */
		nfiles,		/* number of files */ 
		nbytes;		/* sum of bytes used */

	int 
		drive,			/* drive id */
		error = 0,		/* error code */
		ism = 0,		/* ordinal of string search match */
		button,			/* button index */ 
		attrib = 0, 	/* copy state of attributes to temp. storage */ 
		result = 0;

	boolean
		changed = FALSE, 	/* TRUE if objectinfo changed */
		quit = FALSE;		/* TRUE to exit from dialog */ 

	DISKINFO 
		diskinfo;			/* some disk volume information */

	SNAME 
		dskl;				/* disk label */


	/* Pointers to time and date fields */

	time = fileinfo[FLTIME].ob_spec.tedinfo->te_ptext;
	date = fileinfo[FLDATE].ob_spec.tedinfo->te_ptext;

	/* Some fields are set to invisible */

	fileinfo[FLLIBOX].ob_flags |= HIDETREE;
	fileinfo[FLFOLBOX].ob_flags |= HIDETREE;

	/* Put object data into dialog forms */

	if ( type != ITM_DRIVE )
		attrib = attr->attr; /* copy state of attributes to temp. storage */ 

#if _MORE_AV
	avname = (char *)oldname;
#endif	

	switch(type)
	{
		case ITM_FOLDER:

			/* Count the numbers of items in the folder */

			graf_mouse(HOURGLASS, NULL);
			error = cnt_items(oldname, &nfolders, &nfiles, &nbytes, 0x11 | options.attribs, FALSE); 
			graf_mouse(ARROW, NULL);

			if ( error != 0 )
			{
				result = si_error(fname, error);
				return result;
			}

			rsc_ltoftext(fileinfo, FLFOLDER, nfolders);
			rsc_ltoftext(fileinfo, FLFILES, nfiles);
			rsc_ltoftext(fileinfo, FLBYTES, nbytes);

			/* 
			 * Folder could not be renamed before TOS 1.4 
			 * (But can they always be renamed in mint? I suppose so)
			 */

#if _MINT_
			if ( mint || (tos_version >= 0x104) )
#else
			if ( tos_version >= 0x104 )
#endif
				fileinfo[FLNAME].ob_flags |= EDITABLE;
			else
				fileinfo[FLNAME].ob_flags &= ~EDITABLE;

			/* Set states of some other fields */

			fileinfo[FLFOLBOX].ob_flags &= ~HIDETREE;

			rsc_title(fileinfo, FLTITLE, DTFOINF);
			rsc_title(fileinfo, TDTIME, TCRETIM);

			goto setmore;

#if _MINT_
		case ITM_LINK:
			rsc_title(fileinfo, FLTITLE, DTLIINF);
			fileinfo[FLLIBOX].ob_flags &= ~HIDETREE;
			memset(ltgtname, 0, sizeof(ltgtname) );
			error = x_rdlink( (int)sizeof(ltgtname), ltgtname, oldname);
			cv_fntoform(fileinfo + FLTGNAME, ltgtname);	
			goto evenmore;
#endif

		case ITM_FILE:
		case ITM_PROGRAM:

			rsc_title(fileinfo, FLTITLE, DTFIINF);

			evenmore:;

			rsc_title(fileinfo, TDTIME, TACCTIM);
			rsc_ltoftext(fileinfo, FLBYTES, attr->size);

			setmore:;

			strcpy ( nfname, oldname );
			path_to_disp ( nfname );

			cv_fntoform(fileinfo + FLPATH, nfname);	
			cv_fntoform(fileinfo + FLNAME, fname);
			cv_ttoform(time, attr->mtime);
			cv_dtoform(date, attr->mdate);

			fileinfo[FLLABBOX].ob_flags |= HIDETREE;
			fileinfo[FLSPABOX].ob_flags |= HIDETREE;			
			fileinfo[FLCLSIZ].ob_flags  |= HIDETREE;
			fileinfo[FLNAMBOX].ob_flags &= ~HIDETREE;
			fileinfo[ATTRBOX].ob_flags  &= ~HIDETREE;

			if ( can_touch && (search_nsm == 0) && !va_reply ) 
				fileinfo[FLALL].ob_flags &= ~HIDETREE;
			else
				fileinfo[FLALL].ob_flags |= HIDETREE;

			break;

		case ITM_DRIVE:

			rsc_title(fileinfo, FLTITLE, DTDRINF);

			drive = (oldname[0] & 0xDF) - 'A';

			if (check_drive( drive ) != FALSE)
			{
				graf_mouse(HOURGLASS, NULL);

				if ((error = cnt_items(oldname, &nfolders, &nfiles, &nbytes, 0x11 | options.attribs, FALSE)) == 0)
					if ((error = x_getlabel(drive, dskl)) == 0)
						x_dfree(&diskinfo, drive + 1);

				if (error == 0)
				{
					long 
						fbytes,
						tbytes,
						clsize  = diskinfo.b_secsiz * diskinfo.b_clsiz;

					fbytes = diskinfo.b_free * clsize;
					tbytes = diskinfo.b_total * clsize;

					cv_fntoform(fileinfo + FLLABEL, dskl);

					rsc_ltoftext(fileinfo, FLFOLDER, nfolders);
					rsc_ltoftext(fileinfo, FLFILES, nfiles);
					rsc_ltoftext(fileinfo, FLBYTES, nbytes);

					rsc_ltoftext(fileinfo, FLFREE, fbytes);
					rsc_ltoftext(fileinfo, FLSPACE, tbytes);
					rsc_ltoftext(fileinfo, FLCLSIZ, clsize);

					fileinfo[FLDRIVE].ob_spec.tedinfo->te_ptext[0] = drive + 'A';
				}
				else
					result = si_error(oldname, error);

			}

			/* Set states of other fields */

			fileinfo[FLNAMBOX].ob_flags |= HIDETREE;
			fileinfo[ATTRBOX].ob_flags  |= HIDETREE;
			fileinfo[FLALL].ob_flags    |= HIDETREE;
			fileinfo[FLLABBOX].ob_flags &= ~HIDETREE;
			fileinfo[FLSPABOX].ob_flags &= ~HIDETREE;
			fileinfo[FLCLSIZ].ob_flags  &= ~HIDETREE;
			fileinfo[FLFOLBOX].ob_flags &= ~HIDETREE;

			break;

		default:
			break;

	}

	graf_mouse(ARROW, NULL);

	/* Set state of dialog buttons according to attibutes */

	set_file_attribs(attrib);

	/* Loop until told to quit */

	while ( !quit )	
	{
		/* Prepare for display the next string match, if any */

		if ( search_nsm != 0 )
			disp_smatch(ism);

		/* Maybe open (else redraw) the dialog; wait for the button; reset it */

		if ( !dopen )
		{
			xd_open( fileinfo, &dinfo );
			dopen = TRUE;
		}
		else
			xd_draw ( &dinfo, ROOT, MAX_DEPTH );

		button = xd_form_do( &dinfo, ROOT );
		xd_change(&dinfo, button, NORMAL, TRUE);

		/* 
		 * Check if date and time have sensible values; forbid exit otherwise 
		 * Note: this may be a problem with a write-protected file set
		 * to illegal date or time- it can't be unprotected!
		 * Therefore, it is permitted to keep illegal date/time on
		 * write protected files.
		 */

		optime.date = cv_formtod(fileinfo[FLDATE].ob_spec.tedinfo->te_ptext);
		optime.time = cv_formtot(fileinfo[FLTIME].ob_spec.tedinfo->te_ptext);

		now.date = Tgetdate();
		now.time = Tgettime();
		
		if ( optime.date == 0 )
			optime.date = now.date;
		if ( optime.time == 0 )
			optime.time = now.time;
		if ( ((attrib & FA_READONLY) == 0 ) && (optime.time == -1 || optime.date == -1) )
			button = 0;

		switch(button)
		{
			case FLOK:
			{
				if ( type != ITM_DRIVE )
				{
					/* 
					 * Changes are made only for files and folders;
					 * Information on disk volumes is currently readonly
					 */

					int error = 0, new_attribs;
					boolean link = (type == ITM_LINK);

					graf_mouse(HOURGLASS, NULL);

					quit = TRUE;
					if ( search_nsm > 0 )
						find_offset = search_finds[ism] - search_buf;
					else
						find_offset = -1L;

					/* Set states of attributes from the states of dialog buttons */
	
					new_attribs = get_file_attribs(attrib);

					cv_formtofn(nfname, fileinfo + FLNAME);

					if ((newname = fn_make_newname(oldname, nfname)) != NULL)
					{
						/* 
						 * Rename the file only if needed.
						 * If it was successful, try to change attributes.
						 * If the item is write-protected,
						 * error will be generated
						 */

						if ((strcmp(nfname, fname) != 0) && (error == 0))
							error = frename(oldname, newname);

						if ( error == 0 )
							changed = TRUE;

#if _MINT_
						if ( link && strcmp(tgname, ltgtname) != NULL )
						{
							/* It should be checked here whether the target exists */

							if ( !(x_exist(tgname, EX_FILE) || x_exist(tgname, EX_DIR) ) )
								alert_iprint(TNOTGT);

							error = x_unlink( newname );
							if ( !error )
							{
								error = x_mklink( newname, tgname );
								changed = TRUE;
							} 
						}
#endif

						/* Currently, setting of folder attributes is not supported */

						if ( type != ITM_FOLDER && error == 0)
						{
							if 
							( 
								(new_attribs != attrib) || 
								(optime.time != attr->mtime) || 
								(optime.date != attr->mdate) 
/*
								|| ( link && changed )
*/
							)
							{
								/* 
								 * If the file is set to readonly, it must first
								 * be reset before other changes are made
								 */

								if ( (attrib & FA_READONLY) && ( ( (attrib ^ new_attribs) != FA_READONLY) || (optime.date != attr->mdate) || ( optime.time != attr->mtime) ) )
									error = EACCDN;
								else
								{
									changed = TRUE;
									error = touch_file(newname, &optime, new_attribs, link);								
								}
							}
						}
						if (error != 0)
						{
							graf_mouse(ARROW, NULL);
							result = si_error(fname, error);
							graf_mouse(HOURGLASS, NULL);
						}

						if (result != XFATAL)
							if (changed)
								wd_set_update(WD_UPD_COPIED, oldname, NULL);
#if _MORE_AV
						avname = newname;
#endif
						free(newname);
					}
					graf_mouse(ARROW, NULL);
				}
				else /* for the disk volume */
				{
					result = XABORT;
					quit = TRUE;
				}
				break;
			}
			case FLABORT:

				result = XABORT;
				quit = TRUE;
				break; 		

			case FLSKIP:

				if ( ism < search_nsm - 1)
					ism++; /* there are more strings in the same file */
				else
				{
					result = XSKIP;
					quit = TRUE;
				}
				break;

			case FLALL:
			{
				opattr = get_file_attribs(attrib);
				result = XALL;
				quit = TRUE;
				break;
			}
		}

#if _MORE_AV
		if ( va_reply && avname )
			va_add_name(type, avname);
#endif
		free(newname);

		xd_change( &dinfo, button, NORMAL, TRUE );
	}

	/* Free buffers used for string search */

	if ( search_nsm > 0 )
	{
		free( search_buf );			
		free( search_finds );
	}

	return result;
}


/* 
 * Show information on drives, folders and files 
 */

void item_showinfo
(
	WINDOW *w,		/* pointer to the window in which the objects are */ 
	int n,			/* number of objects to show */ 
	int *list,		/* list of selected object indices */ 
	boolean search 	/* true if search results are displayed */
) 
{
	int i, item, error = 0, x, y, oldmode, result = 0;
	ITMTYPE type;
	boolean curr_pos;
	XATTR attrib;
	const char *path, *name;

	long nd, nf, nb;

	x = -1;
	y = -1;
	curr_pos = FALSE;

	if ( search )
	{
		/* open a dialog to input search pattern */

		if ( search_dialog() != SOK )
			return;

		if ( search_txt[0] != 0 )
		{
			fileinfo[ATTRBOX].ob_flags |= HIDETREE;
			fileinfo[MATCHBOX].ob_flags &= ~HIDETREE;
		};
	}

	dopen = FALSE; 	

	for (i = 0; i < n; i++)
	{
		item = list[i];

		if ((x >= 0) && (y >= 0))
		{
			oldmode = xd_setposmode(XD_CURRPOS);
			curr_pos = TRUE;
		}

		type = itm_type(w, item);

		if ((type == ITM_FILE) || (type == ITM_PROGRAM)|| (type == ITM_FOLDER) ||  (type == ITM_DRIVE) )
		{
			if ((path = itm_fullname(w, item)) == NULL)
				result = XFATAL;
			else
			{
				if ( search ) 
				{
					if ( (nd = cnt_items ( path, &nd, &nf, &nb, 0x1 | options.attribs, search ) ) != XSKIP )
						break;
				}
				else
				{
#if _MINT_
					if ( itm_islink(w, item, FALSE) )
						type = ITM_LINK;
#endif
					if (type == ITM_DRIVE)
						result = object_info(ITM_DRIVE, path, NULL, NULL);
					else
					{
						name = itm_name(w, item);
						graf_mouse(HOURGLASS, NULL);

						error = itm_attrib(w, item, (type == ITM_LINK ) ? 1 : 0 , &attrib);

						graf_mouse(ARROW, NULL);

						if (error != 0)
							result = si_error(name, error);
						else
							result = object_info( type, path, name, &attrib );
					}
				}
				free(path);
			}
		}
		else
			result = XSKIP;

		if (curr_pos == TRUE)
			xd_setposmode(oldmode);

		if ((result == XFATAL) || (result == XABORT) || (result == XALL) )
			break;
	}

	/* Close the Show info dialog, if it was open (tested inside) */

	closeinfo();

	/* Touch files, if so said */

	if ( result == XALL )
	{
		int ntouch = n - i;

		itmlist_op(w, ntouch, &list[i], NULL, CMD_TOUCH);
		if ( xw_type(w) == DIR_WIND )
			dir_always_update(w);
		can_touch = FALSE;
	}
	else
		wd_do_update();

	graf_mouse ( ARROW, NULL ); 
}


