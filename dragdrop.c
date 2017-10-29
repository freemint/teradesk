/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2013  Dj. Vukovic
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */


#include <library.h>
#include <errno.h>
#include <signal.h>
#include <error.h>
#include "dragdrop.h"

#if _MINT_
static char pipename[24];
#endif
static __mint_sighandler_t oldpipesig;


/*
 * Create a pipe for doing the drag & drop,
 * and send an AES message to the receipient application 
 * telling it about the drag & drop operation.
 *
 * Input Parameters:
 * apid:	   AES id of the window owner
 * winid:	   target window (0 for background)
 * msx, msy:   mouse X and Y position
 *		       (or -1, -1 if a fake drag & drop)
 * kstate:	   shift key state at time of event
 *
 * Output Parameters:
 * exts:	   A 32 byte buffer into which the
 *		       receipient's 8 favorite
 *		       extensions will be copied.
 *
 * Returns:
 * A positive file descriptor (of the opened
 * drag & drop pipe) on success.
 * -1 if the receipient doesn't respond or
 *    returns DD_NAK
 * -2 if appl_write fails
 *
 * Note: example code for this that can be found in various documents is wrong:
 * it doesn't correctly search through pipes .AA to .ZZ
 */

_WORD ddcreate(_WORD dpid, _WORD spid, _WORD winid, _WORD msx, _WORD msy, _WORD kstate, char *exts )
{
#if _MINT_

	long 
		fd_mask;

	_WORD 
		msg[8];		/* message buffer */
	_WORD *mp = msg,	/* pointer to */
		fd;			/* pipe handle */ 
	char
		c = 0;

	strcpy(pipename, "U:\\PIPE\\DRAGDROP.A@");

	/* Find the first available pipe; extensions are .AA to .ZZ */

	fd = -1;
	do
	{
		/* Try .AA, .AB, .AC ... .AZ, .BA, .BB ... .ZZ */

		pipename[18]++;

		if (pipename[18] > 'Z')
		{
			pipename[18] = 'A'; 

			pipename[17]++;

			if (pipename[17] > 'Z')
				break;
		}

		/* Mode 2 means "get EOF if nobody has pipe open for reading" */

		fd = (_WORD)Fcreate(pipename, 0x02);
	} 
	while (fd == -ETOS_ACCES);

	if (fd < 0) /* fcreate error */
		return fd;

	/* Construct and send the AES message to destination app */

	*mp++ = AP_DRAGDROP;
	*mp++ = spid;
	*mp++ = 0;
	*mp++ = winid;
	*mp++ = msx;
	*mp++ = msy;
	*mp++ = kstate;
	*mp   = ( ( ((_WORD)pipename[17]) << 8) + pipename[18] );

	if(appl_write(dpid, 16, msg) == 0)
	{
		/* appl_write error */
		Fclose(fd);
		return -2;
	}

	/* Now wait for a response */

	fd_mask = 1L << fd;

	if (Fselect(DD_TIMEOUT, &fd_mask, 0L, 0L) && fd_mask) /* no timeout */
	{
		/* Read the 1 byte response */

		if( ((_WORD)Fread(fd, 1L, &c) == 1) && (c == DD_OK) ) /* no read error or DD_NAK */
		{
			/* Now read the "preferred extensions" */

			if ((_WORD)Fread(fd, DD_EXTSIZE, exts) == DD_EXTSIZE) /* no error reading extensions */
			{
				oldpipesig = Psignal(__MINT_SIGPIPE, __MINT_SIG_IGN);
				return fd;
			}
		}
	}

	Fclose(fd);

#else
	(void) exts;
	(void) kstate;
	(void) msx;
	(void) msy;
	(void) winid;
	(void) spid;
	(void) dpid;
#endif

	return -1;
}


/*
 * see if the receipient is willing to accept a certain
 * type of data (as indicated by "ext")
 *
 * Input parameters:
 * fd		file descriptor returned from ddcreate()
 * ext		pointer to the 4 byte file type
 * name		pointer to the name of the data
 * size		number of bytes of data that will be sent
 *
 * Output parameters: none
 *
 * Returns:	 see above DD_...	*/

_WORD ddstry(_WORD fd, const char *ext, const char *name, long size)
{
#if _MINT_

	char c;

	/* 4 bytes for extension, 4 bytes for size, 1 byte for trailing 0 */

	_WORD hdrlen = 9 + (_WORD)strlen(name); /* in Magic docs it is 8 + ... */

	if ((_WORD)Fwrite(fd, 2L, &hdrlen) == 2) 	/* send header length */
	{
		/* Now send the header */

		if
		(
			(
				(_WORD)Fwrite(fd, 4L, ext) + 
				(_WORD)Fwrite(fd, 4L, &size) + 
				(_WORD)Fwrite(fd, (long)strlen(name) + 1, name) /* in Magic docs there is no + 1 */		
			)
			== hdrlen
		) 
		{
			/* Wait for a reply */

			if ((_WORD)Fread(fd, 1L, &c) == 1) 
				return (_WORD)c;
		}
	}

#else
	(void) fd;
	(void) ext;
	(void) name;
	(void) size;
#endif

	return DD_NAK;
}


/*
 * Close a drag & drop operation. If handle is -1, don't do anything.
 */

void ddclose(_WORD fd)
{
	if ( fd >= 0 )
	{
		(void) Psignal(__MINT_SIGPIPE, oldpipesig);
		Fclose(fd);
	}
}


#if 0 /* All following code is for TeraDesk as the the receiver; currently NOT USED */

/*
 * open a drag & drop pipe
 *
 * Input Parameters:
 * ddnam:       the pipe's name (from the last word of
 *              the AES message)
 * preferext:   a list of DD_NUMEXTS 4 byte extensions we understand
 *              these should be listed in order of preference
 *              if we like fewer than DD_NUMEXTS extensions, the
 *              list should be padded with 0s
 *
 * Output Parameters: none
 *
 * Returns:
 * A (positive) file handle for the drag & drop pipe, on success
 * -1 if the drag & drop is aborted
 * A negative error number if an error occurs while opening the
 * pipe.
 */

_WORD ddopen(_WORD ddnam, const char *preferext)
{
	_WORD fd;
	char outbuf[DD_EXTSIZE + 1];

	pipename[18] = ddnam & 0x00ff;
	pipename[17] = (ddnam & 0xff00) >> 8;

	fd = (_WORD)Fopen(pipename, 2);

	if (fd >= 0) 
	{
		outbuf[0] = DD_OK;
		strncpy(outbuf+1, preferext, DD_EXTSIZE);

		oldpipesig = Psignal(__MINT_SIGPIPE, __MINT_SIG_IGN);

		if (Fwrite(fd, (long)DD_EXTSIZE + 1, outbuf) != DD_EXTSIZE + 1) 
		{
			ddclose(fd);
			return -1;
		}
	}

	return fd;
}


/*
 * ddrtry: get the next header from the drag & drop originator
 *
 * Input Parameters:
 * fd:          the pipe handle returned from ddopen()
 *
 * Output Parameters:
 * name:        a pointer to the name of the drag & drop item
 *              (note: this area must be at least DD_NAMEMAX bytes long)
 * whichext:    a pointer to the 4 byte extension
 * size:        a pointer to the size of the data
 *
 * Returns:
 * 0 on success
 * -1 if the originator aborts the transfer
 *
 * Note: it is the caller's responsibility to actually
 * send the DD_OK byte to start the transfer, or to
 * send a DD_NAK, DD_EXT, or DD_LEN reply with ddreply().
 */

_WORD ddrtry(_WORD fd, const char *name, const char *whichext, long *size)
{
	_WORD
		hdrlen,
		i;

	char
		buf[80];

	if ((_WORD)Fread(fd, 2L, &hdrlen) != 2 || hdrlen < 9)
		return -1;

	if ((_WORD)Fread(fd, 4L, whichext) == 4)
	{
		whichext[4] = 0;

		if ((_WORD)Fread(fd, 4L, size) == 4)
		{
			hdrlen -= 8;

			i = min(hdrlen, DD_NAMEMAX);

			if (Fread(fd, (long)i, name) == i) 
			{
				hdrlen -= i;

				/* skip any extra header, no more than 80 bytes at a time */

				while (hdrlen > 0) 
				{
					Fread(fd, (long)min(80, hdrlen), buf);
					hdrlen -= 80;
				}

				return 0;
			}
		}
	}

	return -1;
}


/*
 * send a 1 byte reply to the drag & drop originator
 *
 * Input Parameters:
 * fd:          file handle returned from ddopen()
 * ack:         byte to send (e.g. DD_OK)
 *
 * Output Parameters:
 * none
 *
 * Returns: 0 on success, -1 on failure
 * in the latter case the file descriptor is closed
 */

_WORD ddreply(_WORD fd, _WORD ack)
{
	char c = ack;

	if (Fwrite(fd, 1L, &c) != 1L)
		Fclose(fd);

	return 0;
}


#endif /* end of currently unused receiver code */
