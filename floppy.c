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


#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <np_aes.h>
#include <vdi.h> 
#include <error.h>
#include <xerror.h>
#include <xdialog.h>
#include <boolean.h>

#include "resource.h"
#include "desk.h"
#include "file.h"
#include "xfilesys.h" /* because of config.h */
#include "config.h"	  /* because of font.h and window.h */
#include "font.h" 	  /* because of window.h */
#include "window.h"	  /* because of showinfo.h */
#include "showinfo.h"
#include "events.h"

static XDINFO fdinfo;


/* 
 * Routine fpartoftext inserts data about current floppy disk format 
 * into ftext to be displayed and/or edited in the dialog;
 * then it re-draws these fields                     
 */
 
static void fpartoftext 
( 
	int tsides,  /* number of sides */
	int tspt,    /* sectors per track */
	int ttracks, /* number of tracks */ 
	int dirsize  /* number of directory entries in root */
)
{
	/* rsc_ltoftext instead of itoa for nicer (right-justified) looks */

	rsc_ltoftext(fmtfloppy, FSIDES,   (long)tsides );
	rsc_ltoftext(fmtfloppy, FSECTORS, (long)tspt );
	rsc_ltoftext(fmtfloppy, FTRACKS,  (long)ttracks ); 
	rsc_ltoftext(fmtfloppy, FDIRSIZE,  (long)dirsize );

	xd_drawdeep( &fdinfo, FPAR3);
} 


/* 
 * Routine fpenable enables/disables editing of floppy format params
 * bu (re)setting the DISABLED ob_state flags 
 */

static void fpenable 
( 
	int e  /* e=1: enable; e=0: disable */
)
{
	if ( e )
	{ 
		obj_enable(fmtfloppy[FSIDES]);
		obj_enable(fmtfloppy[FTRACKS]);
		obj_enable(fmtfloppy[FSECTORS]);
		obj_enable(fmtfloppy[FDIRSIZE]);
		obj_enable(fmtfloppy[FLABEL]);
	}
	else
	{
		obj_disable(fmtfloppy[FSIDES]);
		obj_disable(fmtfloppy[FTRACKS]);
		obj_disable(fmtfloppy[FSECTORS]);
		obj_disable(fmtfloppy[FDIRSIZE]);
		obj_disable(fmtfloppy[FLABEL]);  
	}
}


/* 
 * Display formatting/copying progress percentage 
 */

void prdisp 
( 
	int current,  /* current track, =-1 for 0% */
	int total     /* total number of tracks */
)
{
	long perc;			/* percentage of progress */
	
	perc = (current + 1) * 100 / total;
	rsc_ltoftext(fmtfloppy, FPROGRES,  perc );
	obj_unhide(fmtfloppy[FPROGRES]);
	xd_draw ( &fdinfo, PROGRBOX, 1 );
}


/*
 * Convert two consecutive bytes to an integer
 * (but the bytes are in the wrong order: low byte on lower address)
 */

void cctoi(unsigned char *out, unsigned char *in)
{
	out[1] = in[0];
	out[0] = in[1];
} 


/*
 * Return additional text string about XBIOS error.
 * Currently only for write-protect.
 */

char *xbioserr(long istat)
{
	if ( istat == WRITE_PROTECT )
		return get_freestring( TWPROT );
	else
		return empty;
}


/*
 * Write or read one complete track (with verify)
 * Note: there are conflicting data about these functions.
 * In tos.h, xbios() is defined as long, elsewhere as int;
 * Also, filler is sometime int, sometime long, sometime *long!
 * 
 */

long onetrack
(
	int what, 	/* 8 = write, 9= read */
	void *buf,	/* Pointer to the buffer used */
	int devno,	/* device id */
	int itrack,	/* current track number */
	int iside,	/* floppy side */
	int spt		/* number of sectors to read/write */
)
{
	long filler = 0L; /* required but unused filler */
	long istat;		/* xbios return status */

	/* These xbios calls are Flopwr() / Floprd() and Flopver() */

	istat = xbios( what, buf, filler, devno, 1, itrack, iside, spt );
	if ( !istat )
		istat = xbios( 19, buf, filler, devno, 1, itrack, iside, spt );
	return istat;
}


/* 
 *  Format or copy a floppy disk with FAT-12 filesystem 
 */

void formatfloppy
(
	char fdrive,  		/* drive id letter ( 'A'or 'B' ) */
	int format    		/* true for format, false for copy */
)
{
	long 
		filler = 0,		/* unused but required filler */
		serial,			/* 24-bit disk serial number  */
		istat,			/* function execution status  */
#if _MINT_
		lstats,			/* same, for locking source device */
		lstatt,			/* same, for locking target device */
#endif
		mbsize = 0;     /* allocated memory block size (bytes) */

	int 
		button,			/* index of activeded button */
		sectors,		/* total number of disk sectors */
		itrack,			/* current track (counter) */
		iside,			/* current side (counter) */
		sbps,			/* source disk bytes per sector */
		tbps,			/* target disk bytes per sector */
		sdevno,			/* source drive id */
		tdevno,			/* target disk drive id */
		ssides,			/* source disk number of sides */
		stracks,		/* source disk number of tracks */
		sspt,			/* source disk number of sectors per track */
		ipass,			/* copy pass counter */
		npass,			/* number of copy passes */
		finished,		/* =1 if successfully finished operation */
		tpp,        	/* tracks per pass  */
		i,j,n;			/* aux. counters, etc. */
	
	static int     		/* keep these from previous call */
		mspt=11,		/* maximum permitted sectors per track */
		tsides,			/* target disk number of sides */
		ttracks,		/* target disk number of tracks */
		tspt,			/* target disk number of sectors per track */
		intleave=1,		/* sector interleave */
		fatsize=3,		/* FAT size (sectors), take other initial values from dialog */
		dirsize;		/* number of directory entries */
	    
	char 
		drive[4],		/* string with drive id */
		*errtxt,		/* errortext */
		fmtsel;     	/* flag for selected format type */

	unsigned char 
		*sect0,			/* pointer to work buffer for sector/track data */
		*label;			/* pointer to a position within sect0 */
    
	static unsigned char
		fat0;			/* first byte of each FAT */
 
	/* 
	 * Allocate some memory, but at least 40KB; 
	 * if unsuccessfull, exit.
	 * (min.allocation is for two maximum (extra density) tracks,
	 * i.e.  2 * 40*512 bytes = 40960 bytes) 
	 */

	if ( !format )
		mbsize = ((long) options.bufsize) * 1024L; /* use copy buffer size */

	if ( mbsize < 40960L ) 
		mbsize = 40960L; 

	if ( (  sect0 = malloc_chk( mbsize ) ) == NULL ) 
		return;
	
	/* Initial states of some items, depending on action */
  
	drive[1] = 0;	/* terminator, for the time being */
	fmtsel   = 0;	/* no format selected so far */
	finished = 0;	/* no success yet */
  
	fpenable(0);	/* disable editable format params fields */
  
	obj_deselect(fmtfloppy[FSSIDED]); 	/* deselect all format buttons */
	obj_deselect(fmtfloppy[FDSIDED]);
	obj_deselect(fmtfloppy[FHSIDED]);
	obj_deselect(fmtfloppy[FESIDED]);
  
	obj_hide(fmtfloppy[FPROGRES]); 		/* hide progress display */

	/*
	 * Among other things, source and target bios device numbers are set below;
	 * It is assumed that they correspond to GEMDOS drives: 0=A:, 1=B:
	 * Documentation for Dlock() states, that this may not, generally be
	 * the case (is it really possible?); 
	 * If it indeed so, then the formatting routine may fail.
	 * It has been suggested that Fxattr be used to determine bios device numbers.
	 * (well, maybe some other time...)
	 */ 

	if ( format )	/* format disk */
	{
		rsc_title(fmtfloppy, FLTITLE, DTFFMT);		/* title */
		obj_hide(fmtfloppy[FTGTDRV]);  	/* hide "to ... " text */
		obj_enable(fmtfloppy[FSSIDED]); /* enable all format buttons */
		obj_enable(fmtfloppy[FDSIDED]);
		obj_enable(fmtfloppy[FHSIDED]);
		obj_enable(fmtfloppy[FESIDED]);
		obj_unhide(fmtfloppy[FPAR3]); 	/* show editable fields */
		obj_unhide(fmtfloppy[FLABEL]); 	/* show label field */
		obj_enable(fmtfloppy[FLABEL]); 	/* it is editable */
		tdevno = (int)fdrive - 'A';		/* target drive */
	}
	else 			/* copy disk */
	{
		rsc_title(fmtfloppy, FLTITLE, DTFCPY);	/* Title */
		obj_unhide(fmtfloppy[FTGTDRV]);			/* show "to ..." text */
		obj_hide(fmtfloppy[FPAR3]);				/* hide format param fields */
		obj_disable(fmtfloppy[FSSIDED]);		/* disable all format buttons */
		obj_disable(fmtfloppy[FDSIDED]);
		obj_disable(fmtfloppy[FHSIDED]);
		obj_disable(fmtfloppy[FESIDED]);    
		obj_hide(fmtfloppy[FLABEL]);			/* hide label field */
		sdevno = (int)fdrive - 'A';				/* source drive */
		tdevno = 1 & (1^sdevno);  				/* target is the other one */
		drive[0] = tdevno + 'A';    			/* target drive letter */
		strcpy(fmtfloppy[FTGTDRV].ob_spec.tedinfo->te_ptext, drive); /* put into dialog */ 
	}

	/* Put the first drive letter (for "Drive: ") into dialog */

	drive[0] = fdrive; /* source drive for copy, target for format */
	strcpy(fmtfloppy[FSRCDRV].ob_spec.tedinfo->te_ptext, drive); 
  
 	/* Open dialog */
		
	xd_open(fmtfloppy, &fdinfo);
	
	button = FDSIDED; /* anything but OK or Cancel */
	
	/* Loop until OK or Cancel */
	
	while ( button != FMTOK && button != FMTCANC )
	{
		again: /* return here if invalid format parameters */
      
		button = xd_form_do ( &fdinfo, ROOT );
          
    	/* Set formatting parameters */
       
		if ( format )
		{
      		/* 
			 * Read params for diverse floppy disk formats from dialog. 
			 * Previous values are preserved there. Unless a format type
			 * is specified, previous valus will be used
			 */

			tsides =  atoi(fmtfloppy[FSIDES].ob_spec.tedinfo->te_ptext);
			tspt =    atoi(fmtfloppy[FSECTORS].ob_spec.tedinfo->te_ptext);
			ttracks = atoi(fmtfloppy[FTRACKS].ob_spec.tedinfo->te_ptext);
			dirsize = atoi(fmtfloppy[FDIRSIZE].ob_spec.tedinfo->te_ptext); 

			/* 
			 * Configure format; always use "optimized" (i.e. smaller) FAT size 
			 * Previous values will be used if nothing is selected here.
			 * So, do -not- pull common values out of the switch structure.
			 */
			
			switch (button)
			{
				case FSSIDED:		/* SS DD disk */
					fmtsel = 1;
					tsides = 1;
					tspt = 9;
					mspt = 11;
					ttracks = 80;
					intleave = 1;
					fatsize = 3;
					dirsize = 112;
					fat0 = 0xf9;
					break;
				case FDSIDED:		/* DS DD disk */
					fmtsel = 1;
					tsides = 2;
					tspt = 9;
					mspt = 11;
					ttracks = 80;
					intleave = 1;
					fatsize = 3;
					dirsize = 112;
					fat0 = 0xf9;
					break;
				case FHSIDED:		/* DS HD disk */
					fmtsel = 1;
					tsides = 2;
					tspt = 18;
					mspt = 20;
					ttracks = 80;
					intleave = 1;
					fatsize = 5;
					dirsize = 224;
					fat0 = 0xf0;
					break;
				case FESIDED:		/* DS ED disk (not tested) */
					fmtsel = 1;
					tsides = 2;
					tspt = 36;
					mspt = 40;		/* is it so? */
					ttracks = 80;
					intleave = 1;
					fatsize = 9;
					dirsize = 448;  /* is it so? never seen an ED disk */
					fat0 = 0xf0;	/* is it so? */    
					break;
				default:			/* no change */
          			break;       
			} /* switch */   

		}  /* format ? */
   
  		/* Update displayed params regarding selected or found format */
 
		if ( fmtsel ) 
			fpenable(1);
    
		fpartoftext( tsides, tspt, ttracks, dirsize );
  
	} /* while... */

	xd_drawbuttnorm(&fdinfo, button);

	/* If selected OK */
  
	if ( button == FMTOK )
	{
		/* Check format parameters */
	  
		if ( format )
		{
			/*  
			 *  Max.possible number of dir.entries should not, (for my convenience)
			 *  exceed end of second track (disk will be zeroed only so far);
			 *  formula below gives maximum 143 for DD, 287 for HD; 
			 *  FATs should, for convenience too, remain within first track;
			 */
		  
			tbps = 512; /* always 512 bytes per sector */
		  
			if ( ( tsides  <  1  ) || ( tsides > 2 )   ||
			   ( ttracks <  1 ) || ( ttracks > 84 ) || /* 84=physical limit, most often it is 83 */
			   ( tspt < 3 )  || ( tspt > mspt ) ||     /* mspt depends on disk type */
			   ( fatsize < 1 ) || ( fatsize > ((tspt - 1) / 2) ) || /* two fats + sector 0 fit on a track */
			   ( dirsize < 32) || ( dirsize > ( (tspt * tbps) / 32 - 1) ) ) /* 32 byes per entry */
			{
				alert_iprint ( MFPARERR );
				goto again; /* go back to dialog if params not correct */
			}
		}         
                   
		/* Confirm destructive action ("all data will be erased...") */
    
		button = alert_printf ( 2, AERADISK );
    
		if ( button == 1 )
		{
			/* Action confirmed! */

#if _MINT_
			/* 
			 * Try to lock the floppies in a multitasking environment
			 * (beware of the note above about device numbers; this code
			 * may need improvement)
			 */

			if (mint)
			{
				if (format)
					lstats = 1;
				else
					lstats  = Dlock( 1, sdevno );

				lstatt  = Dlock( 1, tdevno ); 

				if ( lstats <  0 || lstatt < 0 ) 
				{
					alert_printf(1, AGENALRT, get_freestring(TNOLOCK));
					istat = 0;
					goto abortfmt;
				}
			}
#endif
    		if ( format ) /* format disk */
 			{
				/* Now format each track... */
				
				prdisp ( -1, 100 );
          
				for ( itrack = 0; itrack < ttracks; itrack++ )
				{       
					for ( iside = 0; iside < tsides; iside++ )
					{
        				retry: /* come here for a retry after formatting error */

						/*
						 * Calling xbios... produces slightly smaller code
						 * than calling Flopfmt, Floprd, Flopwr...
						 * (but it is easier to make a mistake now).
						 * Now, xbios(10...) = Flopfmt(...)
						 */
						 						
						istat = xbios(10, sect0, filler, tdevno, tspt, itrack, iside, intleave, 0x87654321L, 0xE5E5 ); 

						/* In case of error inquire what to do */
						
						if ( istat != 0 )
						{
							/* Disk protected is the most common error */

							errtxt = xbioserr(istat);
							button = alert_printf( 3, AFMTERR, (int)istat, itrack, errtxt );

							switch ( button )
							{
								case 1:				/* retry same track */
									goto retry;
								case 3:				/* abort */
									goto endall;
								default:			/* ignore and continue */
									break;
							} /* switch */
						} /* istat ? */
						else
							if ( escape_abort(FALSE) )
								goto endall;
          
					} /* iside */
          
					/* Report formatting progress after each track */
          
					prdisp ( itrack, ttracks );
                         
				} /* itrack */
        
				/* 
				 * Produce boot sector and write it; 
				 * all bytes not explicitely specified will be zeros.
				 * For simplicity's sake always create optimized 
				 * (smaller) FATs 
				 */

				/* Clear two complete tracks */

				memset( sect0, 0x00, (size_t)( tspt * tbps * 2 ) );
       
				/* Create a MS-DOS-compatible header */

				sect0[0] = 0xeb;
				sect0[1] = 0x34;    /* maybe put here 0x3c for hd ? */
				sect0[2] = 0x90;
				sect0[3] = 'I';		/* OEM code (5 chars) */
				sect0[4] = 'B';
				sect0[5] = 'M';
				sect0[6] = ' ';
				sect0[7] = ' ';

				serial = xbios(17);	 			  /* random serial number */					  										/* produce random number */
				memcpy(&sect0[0x08], &serial, 3); /* turned around, but doesn't matter */
                              
				sect0[0x0c] = (char)(tbps >> 8); /* bytes/128 per sector */
		       	sect0[0x0d] = 0x02;            /* sectors per cluster */
				sect0[0x0e] = 0x01;            /* reserved sectors    */
				sect0[0x10] = 0x02;            /* number of FATS      */

				cctoi(&sect0[0x11], (unsigned char *)(&dirsize));

        		/* 
				 * calculate FAT size; always use optimized 12-bit FATs; 
				 * for each cluster use 3/2 bytes, add 2 for FAT header. 
				 * fomula below will generally give 3- and 5-sector FATs 
				 * for DD and HD respectively 
				 */
		   		   						
				sectors = tspt * ttracks * tsides;   /* total sectors  */
				fatsize = (sectors + 2) * 3 / 2048 + 1; 

				cctoi(&sect0[0x13], (unsigned char *)(&sectors) );
		        
				sect0[0x15] = fat0;            			/* media id. */
				sect0[0x16] = (char)fatsize;   			/* sectors per fat: 3 ... 9 */
				sect0[0x18] = (unsigned char)tspt;      /* sectors per track */				
				sect0[0x1a] = (unsigned char)tsides;    /* sides */
				strcpy ( (char *)(sect0 + 0x20L), " Formatted by TeraDesk " );

				i = tbps;					/* locate at start of sector 1 */	
				sect0[i++] = fat0;			/* start of first FAT */
				sect0[i++] = 0xff;
				sect0[i++] = 0xff;
        
				i = (fatsize + 1) * tbps;	/* locate at start of FAT 2 */

				memcpy(&sect0[i], &sect0[tbps], 3);	/* copy first to second fat */
       
				i = (2 * fatsize + 1) * tbps;	/* locate at start of root dir */

				strcpy ( (char *)(sect0 + (long)i), fmtfloppy[FLABEL].ob_spec.tedinfo->te_ptext ); 
        
				sect0[i + 11] = 0x08;			/* next, insert label attribute */
        
				/* 
				 * write and verify complete first  and second track 
				 * this will surely cover all space used by root dir
				 */
				 				 
				for ( itrack = 0; itrack < 2; itrack++ )
				{
					label = sect0 + tbps * tspt * itrack; /* start of next track in buffer */ 
/*
					istat = xbios( 9, label, filler, tdevno, 1, itrack, 0, tspt );
					if ( !istat )
						istat = xbios( 19, label, filler, tdevno, 1, itrack, 0, tspt );
*/
istat = onetrack(9, (void *)label, tdevno, itrack, 0, tspt);

					if ( istat ) 
						goto abortfmt;
					else
					{
						finished = 1;
						goto endall;
					} /* if istat... */
				}	  /* for itrack... */        
      		}
      		else /* disk copy */
      		{
				int tbpt; /* bytes pee track */

				/* Insert target disk, read boot sector */
        
				drive[0] = 'A' + tdevno; /* target drive letter */

/*
				istat = xbios( 8, sect0, filler, tdevno, 1, 0, 0, 1 );
*/
istat = onetrack(8, sect0, tdevno, 0, 0, 1);
 
				if ( istat ) 
					goto abortfmt;
				
				/* Decode format parameters from boot sector */

				cctoi((unsigned char *)(&tbps), &sect0[0x0b]);
				cctoi((unsigned char *)(&sectors), &sect0[0x13]);
				cctoi((unsigned char *)(&tspt), &sect0[0x18]);
				cctoi((unsigned char *)(&tsides), &sect0[0x1a]);

				ttracks = sectors /( tspt * tsides );
          
				/* Insert source disk, read boot sector */
        
				drive[0] = 'A' + sdevno; /* source drive letter */

/*
				istat = xbios( 8, sect0, filler, sdevno, 1, 0, 0, 1 );
*/
istat = onetrack(8, sect0, sdevno, 0, 0, 1);

				if ( istat ) goto abortfmt;

				cctoi((unsigned char *)(&sbps), &sect0[0x0b]);
				cctoi((unsigned char *)(&sectors), &sect0[0x13]);
				cctoi((unsigned char *)(&sspt), &sect0[0x18]);
				cctoi((unsigned char *)(&ssides), &sect0[0x1a]);
				cctoi((unsigned char *)(&dirsize), &sect0[0x11]);

				stracks = sectors / (sspt * ssides);

				/*  
				 *  Do source and target  diskshave the same format?
				 *  If not, abort all.
				 *  Note: this will, unfortunately, prevent copying of "data" disks
				 * 	without a proper boot sector. Maybe it should be disabled?  
				 */
        
				if ( ( sbps != tbps ) ||        /* bytes per sector */
				     ( ssides != tsides ) ||    /* number of sides */
				     ( stracks != ttracks ) ||  /* number of tracks */ 
				     ( sspt != tspt )  )        /* sectors per track */
				{                 
					alert_iprint ( MDIFERR ); 
					goto endall;
				}
				
				/* 
				 * show format parameters of the source disk 
				 * (they are identical to those of the target disk) 
				 */     
      
				obj_unhide(fmtfloppy[FPAR3]);

				fpartoftext( ssides, sspt, stracks, dirsize );
				prdisp ( -1, 100 );
				
				/* How many tracks to copy in each pass ? how many passes ? */
				
				tbpt = tbps * tspt;
				tpp = (int)(mbsize / (tbpt * tsides));     
				npass = ttracks / tpp;

				if ( ttracks % tpp ) 
					npass++;
								
				/* Now start copying */
			
				i = 0; j = 0; /* this will now be the source and target track counters */
				
				for ( ipass = 0; ipass < npass; ipass++ )
				{
				  	/* 
					 * how many tracks to copy
					 * and not pass the end of the floppy disk? 
					 */
				   
					n= ttracks - i;
					if ( n > tpp ) 
						n = tpp; 
				  
					/* Read and verify (i.e. read twice) numbers of tracks, display progress  */
				  
					for ( itrack = 0; itrack < n; itrack++ )
					{				  			  
						for ( iside = 0; iside < tsides; iside++ )
						{
/*
							label = sect0 + ((long)sbps) * sspt * ( itrack * ssides + iside );
							istat = xbios( 8, label, filler, sdevno, 1, i+itrack, iside, sspt );
							if ( !istat )
								istat = xbios( 19, label, filler, sdevno, 1, i+itrack, iside, sspt );
*/
label = sect0 + ((long)tbpt) * ( itrack * ssides + iside );
istat = onetrack(8, (void *)label, sdevno, i + itrack, iside, sspt);
 
							if ( istat ) 
								goto abortfmt;
							if ( escape_abort(FALSE) )
								goto endall;
						}				    

						prdisp ( i + j + itrack, ttracks * 2 ); /* diplay progress */
 						
					}
					i = i + n;
				  
					/* Write and verify a number of tracks, display progress */
				  
					for ( itrack = 0; itrack < n; itrack++ )
					{
						for ( iside = 0; iside < tsides; iside++ )
						{				    	
							label = sect0 + ((long)tbpt) * ( itrack * tsides + iside );

/*				    	
							istat = xbios( 9, label, filler, tdevno, 1, j + itrack, iside, tspt );
							if ( !istat )
								istat = xbios( 19, label, filler, tdevno, 1, j + itrack, iside, sspt );
*/
istat = onetrack(9, (void *)label, tdevno, j + itrack, iside, tspt);

							if ( istat ) 
								goto abortfmt;
							if ( escape_abort(FALSE) )
								goto endall;
						}
				    
						prdisp ( i + j + itrack, ttracks * 2 );	/* diplay progress */			    
					}
					j = j + n;

				} /* ipass */
	  
				finished = 1;    
				goto endall; /* all is well, and finished */
	              
			} 	/* copy-format */
		}		/* button=? */
    
    	/* 
		 * Come here to report diverse XBIOS errors 
		 * (" error while accessing floppy...")
		 * Specially recognize protected disk as the most common error. 
		 */
    
		abortfmt:	

		if ( (button == 1 ) && !finished ) /* if started but not finished */
		{
			errtxt = xbioserr(istat);
			if ( istat )
				alert_printf( 1, AERRACC, (int)istat, errtxt ); 
		}
	}

	/* Final activities... */

	endall:

#if _MINT_
	/* Unlock the drives */

	if ( mint )
	{
		if ( lstats == 0 )
			lstats = Dlock( 0, sdevno );
		if ( lstatt == 0 )
			lstatt = Dlock( 0, tdevno );
	}
#endif

	/* Close the dialog, free buffer */
  
	xd_close(&fdinfo);

	free(sect0);

	/* 
	 * Convince the computer that it has a new disk in the target drive; 
	 * Display information about this disk 
	 */

	if ( finished )
	{
/*
		drive[0] = 'A' + tdevno;
		drive[1] = ':';
		drive[2] = '\\';
		drive[3] = 0;
*/
		strcpy(drive, adrive);
		drive[0] += tdevno;

		force_mediach ( drive );

		button = object_info(ITM_DRIVE, drive, NULL, NULL);
		closeinfo();
	}
}

