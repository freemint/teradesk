/*      TOS.H

        Tos Definition Includes

        Copyright (c) Borland International 1990
        All Rights Reserved.
*/


#if !defined( __TOS )
#define __TOS

#include <trap.h>		/* AHCC: __syscall__ */

typedef struct          /* used by Cconrs */
{
        unsigned char maxlen;
        unsigned char actuallen;
        char    buffer[255];
} LINE;


typedef struct          /* used by Fsetdta, Fgetdta */
{
    char            d_reserved[21];
    unsigned char   d_attrib;
    unsigned int    d_time;
    unsigned int    d_date;
    unsigned long   d_length;
    char            d_fname[14];
} DTA;


typedef struct          /* used by Dfree */
{
    unsigned long   b_free;
    unsigned long   b_total;
    unsigned long   b_secsiz;
    unsigned long   b_clsiz;
} DISKINFO;


typedef struct baspag   /* used by Pexec */
{
        void    *p_lowtpa;
        void    *p_hitpa;
        void    *p_tbase;
        long    p_tlen;
        void    *p_dbase;
        long    p_dlen;
        void    *p_bbase;
        long    p_blen;
        DTA     *p_dta;
        struct baspag *p_parent;
        long    p_resrvd0;
        char    *p_env;
        char    p_stdfh[6];
        char    p_resrvd1;
        char    p_curdrv;
        long    p_resrvd2[18];
        char    p_cmdlin[128];
} BASPAG;


typedef struct          /* used by Getbpb */
{
        int     recsiz;
        int     clsiz;
        int     clsizb;
        int     rdlen;
        int     fsiz;
        int     fatrec;
        int     datrec;
        int     numcl;
        int     bflags;
} BPB;


typedef struct
{
        unsigned int  time;
        unsigned int  date;
} DOSTIME;


typedef struct          /* used by Iorec */
{
        void    *ibuf;
        int     ibufsiz;
        int     ibufhd;
        int     ibuftl;
        int     ibuflow;
        int     ibufhi;
} IOREC;


typedef struct          /* used by Kbdvbase */
{
        void    (*kb_midivec)();
        void    (*kb_vkbderr)();
        void    (*kb_vmiderr)();
        void    (*kb_statvec)();
        void    (*kb_mousevec)();
        void    (*kb_clockvec)();
        void    (*kb_joyvec)();
        void    (*kb_midisys)();
        void    (*kb_kbdsys)();
} KBDVBASE;


typedef struct          /* used by Pexec */
{
        unsigned char   length;
        char            command_tail[128];
} COMMAND;


typedef struct          /* used by Initmouse */
{
        char    topmode;
        char    buttons;
        char    x_scale;
        char    y_scale;
        int     x_max;
        int     y_max;
        int     x_start;
        int     y_start;
} MOUSE;


typedef struct          /* used by Prtblk */
{
        void    *pb_scrptr;
        int     pb_offset;
        int     pb_width;
        int     pb_height;
        int     pb_left;
        int     pb_right;
        int     pb_screz;
        int     pb_prrez;
        void    *pb_colptr;
        int     pb_prtype;
        int     pb_prport;
        void    *pb_mask;
} PBDEF;


typedef struct          /* used by Keytbl */
{
        char *unshift;
        char *shift;
        char *capslock;
} KEYTAB;


typedef struct __md
{
        struct __md *m_link;
        void        *m_start;
        long        m_length;
        BASPAG      *m_own;
} MD;


typedef struct          /* used by Getmpb */
{
        MD *mp_mfl;
        MD *mp_mal;
        MD *mp_rover;
} MPB;

typedef struct
{
        int   (*Bconstat) ();
        long  (*Bconin) ();
        int   (*Bcostat) ();
        void  (*Bconout) ();
        long  (*Rsconf) ();
        IOREC *iorec;
} MAPTAB;

typedef struct
{
        MAPTAB *maptab;
        int    maptabsize;
} BCONMAP;

/* system variable _sysbase (0x4F2L) points to next structure         */
typedef struct _syshdr
{
    unsigned       os_entry;   /* $00 BRA to reset handler             */
    unsigned       os_version; /* $02 TOS version number               */
    void           *os_start;  /* $04 -> reset handler                 */
    struct _syshdr *os_base;   /* $08 -> baseof OS                     */
    void           *os_membot; /* $0c -> end BIOS/GEMDOS/VDI ram usage */
    void           *os_rsv1;   /* $10 << unused,reserved >>            */
    long           *os_magic;  /* $14 -> GEM memoryusage parm. block   */
    long           os_gendat;  /* $18 Date of system build($MMDDYYYY)  */
    int            os_palmode; /* $1c OS configuration bits            */
    int            os_gendatg; /* $1e DOS-format date of systembuild   */
/*
    The next three fields are only available in TOS versions 1.2 and
    greater
*/
    void           *_root;     /* $20 -> base of OS pool               */
    long           *kbshift;   /* $24 -> keyboard shift state variable */
    BASPAG         **_run;     /* $28 -> GEMDOS PID of current process */
    void           *p_rsv2;    /* $2c << unused, reserved >>           */
} SYSHDR;



/* attributes for Fcreate/Fsfirst/Fsnext: */

#define FA_READONLY     0x01
#define FA_HIDDEN       0x02
#define FA_SYSTEM       0x04
#define FA_VOLUME       0x08
#define FA_SUBDIR       0x10
#define FA_ARCHIVE      0x20

/* Fopen modes: */

#define FO_READ         0
#define FO_WRITE        1
#define FO_RW           2

extern BASPAG *_BasPag;
extern long _PgmSize;

long    TRAP( 1)	gemdos( void, ... );
long    TRAP(13)	bios  ( void, ... );
long    TRAP(14)	xbios ( void, ... );

/****** Tos *************************************************************/

void    GEMDOS(0)	Pterm0( void );
long    GEMDOS(1)	Cconin( void );
void    GEMDOS(2)	Cconout( int c );
int     GEMDOS(3)	Cauxin( void );
void    GEMDOS(4)	Cauxout( int c );
int     GEMDOS(5)	Cprnout( int c );
long    GEMDOS(6)	Crawio( int w );
long    GEMDOS(7)	Crawcin( void );
long    GEMDOS(8)	Cnecin( void );
int     GEMDOS(9)	Cconws( const char *buf );
void    GEMDOS(10)	Cconrs( LINE *buf );
int     GEMDOS(11)	Cconis( void );
long    GEMDOS(14)	Dsetdrv( int drv );
int     GEMDOS(16)	Cconos( void );
int     GEMDOS(17)	Cprnos( void );
int     GEMDOS(18)	Cauxis( void );
int     GEMDOS(19)	Cauxos( void );
int     GEMDOS(25)	Dgetdrv( void );
void    GEMDOS(26)	Fsetdta( DTA *buf );
long    GEMDOS(32)	Super( void *stack );
unsigned int GEMDOS(42)	Tgetdate( void );
unsigned int GEMDOS(43)	Tsetdate( unsigned int date );
unsigned int GEMDOS(44)	Tgettime( void );
unsigned int GEMDOS(45)	Tsettime( unsigned int time );
DTA *	GEMDOS(47)	Fgetdta( void );
int     GEMDOS(48)	Sversion( void );
void    GEMDOS(49)	Ptermres( long keepcnt, int retcode );
int     GEMDOS(54)	Dfree( DISKINFO *buf, int driveno );
int     GEMDOS(57)	Dcreate( const char *path );
int     GEMDOS(58)	Ddelete( const char *path );
int     GEMDOS(59)	Dsetpath( const char *path );
long    GEMDOS(60)	Fcreate( const char *filename, int attr );
long    GEMDOS(61)	Fopen( const char *filename, int mode );
int     GEMDOS(62)	Fclose( int handle );
long    GEMDOS(63)	Fread( int handle, long count, void *buf );
long    GEMDOS(64)	Fwrite( int handle, long count, void *buf );
int     GEMDOS(65)	Fdelete( const char *filename );
long    GEMDOS(66)	Fseek( long offset, int handle, int seekmode );
int     GEMDOS(67)	Fattrib( const char *filename, int wflag, int attrib );
long    GEMDOS(69)	Fdup( int handle );
int     GEMDOS(70)	Fforce( int stch, int nonstdh );
int     GEMDOS(71)	Dgetpath( char *path, int driveno );
void *	GEMDOS(72)	Malloc( long number );
int     GEMDOS(73)	Mfree( void *block );
int     GEMDOS(74)	Mshrink( int zero, void *block, long newsiz );
long    GEMDOS(75)	Pexec( int mode, char *ptr1, void *ptr2, void *ptr3 );
void    GEMDOS(76)	Pterm( int retcode );
int     GEMDOS(78)	Fsfirst( const char *filename, int attr );
int     GEMDOS(79)	Fsnext( void );
int     GEMDOS(86)	Frename( int zero, const char *oldname, const char *newname );
int     GEMDOS(87)	Fdatime( DOSTIME *timeptr, int handle, int wflag );

/****** TOS 030 Gemdos Extension ****************************************/

void *	GEMDOS(68)	Mxalloc( long number, int mode );
long    GEMDOS(20)	Maddalt( void *start, long size );

/****** Network Gemdos Extension ****************************************/

long    GEMDOS(92)	Flock( int handle, int mode, long start, long length );

/****** Bios ************************************************************/

void    BIOS( 0)	Getmpb( MPB *ptr );
int     BIOS( 1)	Bconstat( int dev );
long    BIOS( 2)	Bconin( int dev );
void    BIOS( 3)	Bconout( int dev, int c );
long    BIOS( 4)	Rwabs( int rwflag, void *buf, int cnt, int recnr, int dev );
void    (* BIOS(5)	Setexc( int number, void (*exchdlr)() )) ();
long    BIOS( 6)	Tickcal( void );
BPB *	BIOS( 7)	Getbpb( int dev );
long    BIOS( 8)	Bcostat( int dev );
long    BIOS( 9)	Mediach( int dev );
long    BIOS(10)	BIOS(10)	Drvmap( void );
long    BIOS(11)	Kbshift( int mode );
long    BIOS(12)	Lrwabs( short rwflag, void *buf, short cnt, short recnr, short dev );

/****** XBios ***********************************************************/

void    XBIOS(0)	Initmouse( int type, MOUSE *par, void (*mousevec)() );
void *	XBIOS(1)	Ssbrk( int count );
void *	XBIOS(2)	Physbase( void );
void *	XBIOS(3)	Logbase( void );
int     XBIOS(4)	Getrez( void );
void    XBIOS(5)	Setscreen( void *laddr, void *paddr, int rez );
void    XBIOS(6)	Setpalette( void *pallptr );
int     XBIOS(7)	Setcolor( int colornum, int color );
int     XBIOS(8)	Floprd( void *buf, void *filler, int devno, int sectno,
               int trackno, int sideno, int count );
int     XBIOS(9)	Flopwr( void *buf, void *filler, int devno, int sectno,
               int trackno, int sideno, int count );
int     XBIOS(10)	Flopfmt( void *buf, void *filler, int devno, int spt, int trackno,
                int sideno, int interlv, long magic, int virgin );
void    XBIOS(12)	Midiws( int cnt, void *ptr );
void    XBIOS(13)	Mfpint( int erno, void (*vector)() );
IOREC *	XBIOS(14)	Iorec( int dev );
long    XBIOS(15)	Rsconf( int baud, int ctr, int ucr, int rsr, int tsr, int scr );
KEYTAB *XBIOS(16)	Keytbl( void *unshift, void *shift, void *capslock );
long    XBIOS(17)	Random( void );
void    XBIOS(18)	Protobt( void *buf, long serialno, int disktype, int execflag );
int     XBIOS(19)	Flopver( void *buf, void *filler, int devno, int sectno,
                int trackno, int sideno, int count );
void    XBIOS(20)	Scrdmp( void );
int     XBIOS(21)	Cursconf( int func, int rate );
void    XBIOS(22)	Settime( unsigned long time );
unsigned
long	XBIOS(23)	Gettime( void );
void    XBIOS(24)	Bioskeys( void );
void    XBIOS(25)	Ikbdws( int count, void *ptr );
void    XBIOS(26)	Jdisint( int number );
void    XBIOS(27)	Jenabint( int number );
char    XBIOS(28)	Giaccess( char data, int regno );
void    XBIOS(29)	Offgibit( int bitno );
void    XBIOS(30)	Ongibit( int bitno );
void    XBIOS(31)	Xbtimer( int timer, int control, int data, void (*vector)() );
void *	XBIOS(32)	Dosound( void *buf );
int     XBIOS(33)	Setprt( int config );
KBDVBASE * XBIOS(34)Kbdvbase( void );
int     XBIOS(35)	Kbrate( int initial, int repeat );
void    XBIOS(36)	Prtblk( PBDEF *par );
void    XBIOS(37)	Vsync( void );
typedef long Supfunc(void);
long    XBIOS(38)	Supexec( Supfunc *func);
void    XBIOS(39)	Puntaes( void );
int     XBIOS(41)	Floprate( int devno, int newrate );
int     XBIOS(64)	Blitmode( int mode );

/* TOS030 XBios */
int     XBIOS(42)	DMAread( long sector, int count, void *buffer, int devno );
int     XBIOS(43)	DMAwrite( long sector, int count, void *buffer, int devno );
long    XBIOS(44)	Bconmap( int devno );
int     XBIOS(46)	NVMaccess( int opcode, int start, int count, void *buffer );
int     XBIOS(80)	Esetshift( int shftMode );
int     XBIOS(81)	Egetshift( void );
int     XBIOS(82)	EsetBank( int bankNum );
int     XBIOS(83)	EsetColor( int colorNum, int color );
void    XBIOS(84)	EsetPalette( int colorNum, int count, int *palettePtr );
void    XBIOS(85)	EgetPalette( int colorNum, int count, int *palettePtr );
int     XBIOS(86)	EsetGray( int swtch );
int     XBIOS(87)	EsetSmear( int swtch );

/* MinT Gemdos */

/* signal handling */

#define NSIG       31       /* number of signals recognized   */

#define SIGNULL     0       /* not really a signal             */
#define SIGHUP      1       /* hangup signal                   */
#define SIGINT      2       /* sent by ^C                      */
#define SIGQUIT     3       /* quit signal                     */
#define SIGILL      4       /* illegal instruction             */
#define SIGTRAP     5       /* trace trap                      */
#define SIGABRT     6       /* abort signal                    */
#define SIGPRIV     7       /* privilege violation             */
#define SIGFPE      8       /* divide by zero                  */
#define SIGKILL     9       /* cannot be ignored               */
#define SIGBUS     10       /* bus error                       */
#define SIGSEGV    11       /* illegal memory reference        */
#define SIGSYS     12       /* bad argument to a system call   */
#define SIGPIPE    13       /* broken pipe                     */
#define SIGALRM    14       /* alarm clock                     */
#define SIGTERM    15       /* software termination signal     */
#define SIGURG     16       /* urgent condition on I/O channel */
#define SIGSTOP    17       /* stop signal not from terminal   */
#define SIGTSTP    18       /* stop signal from terminal       */
#define SIGCONT    19       /* continue stopped process        */
#define SIGCHLD    20       /* child stopped or exited         */
#define SIGTTIN    21       /* read by background process      */
#define SIGTTOU    22       /* write by background process     */
#define SIGIO      23       /* I/O possible on a descriptor    */
#define SIGXCPU    24       /* CPU time exhausted              */
#define SIGXFSZ    25       /* file size limited exceeded      */
#define SIGVTALRM  26       /* virtual timer alarm             */
#define SIGPROF    27       /* profiling timer expired         */
#define SIGWINCH   28       /* window size changed             */
#define SIGUSR1    29       /* user signal 1                   */
#define SIGUSR2    30       /* user signal 2                   */

#define SIG_DFL    0        /* default signal handler          */
#define SIG_IGN    1        /* ignore the signal               */


/* file types */
#define S_IFMT  0170000     /* mask to select file type */
#define S_IFCHR 0020000     /* BIOS special file */
#define S_IFDIR 0040000     /* directory file */
#define S_IFREG 0100000     /* regular file */
#define S_IFIFO 0120000     /* FIFO */
#define S_IMEM  0140000     /* memory region or process */
#define S_IFLNK 0160000     /* symbolic link */

/* special bits: setuid, setgid, sticky bit */
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

/* file access modes for user, group, and other*/
#define S_IRUSR         0400
#define S_IWUSR         0200
#define S_IXUSR         0100
#define S_IRGRP         0040
#define S_IWGRP         0020
#define S_IXGRP         0010
#define S_IROTH         0004
#define S_IWOTH         0002
#define S_IXOTH         0001
#define DEFAULT_DIRMODE (0777)
#define DEFAULT_MODE    (0666)


typedef struct
{
        long     msg1;
        long     msg2;
        int      pid;
} MSG;

typedef struct
{
        unsigned int   mode;
        long           index;
        unsigned int   dev;
        unsigned int   reserved1;
        unsigned int   nlink;
        unsigned int   uid;
        unsigned int   gid;
        long           size;
        long           blksize;
        long           nblocks;
        unsigned int   mtime;
        unsigned int   mdate;
        unsigned int   atime;
        unsigned int   adate;
        unsigned int   ctime;
        unsigned int   cdate;
        unsigned int   attr;
        unsigned int   reserved2;
        long           reserved3[2];
} XATTR;


#define SA_NOCLDSTOP    1

typedef struct
{
        void    (*sa_handler)( long sig );
        long    sa_mask;
        int     sa_flags;
} SIGACTION;


int     GEMDOS(0xff)	Syield( void );
int     GEMDOS(0x100)	Fpipe( int *usrh );
int     GEMDOS(0x104)	Fcntl( int f, long arg, int cmd );
long    GEMDOS(0x105)	Finstat( int f );
long    GEMDOS(0x106)	Foutstat( int f );
long    GEMDOS(0x107)	Fgetchar( int f, int mode );
long    GEMDOS(0x108)	Fputchar( int f, long c, int mode );
long    GEMDOS(0x109)	Pwait( void );
int     GEMDOS(0x10a)	Pnice( int delta );
int     GEMDOS(0x10b)	Pgetpid( void );
int     GEMDOS(0x10c)	Pgetppid( void );
int     GEMDOS(0x10d)	Pgetpgrp( void );
int     GEMDOS(0x10e)	Psetpgrp( int pid, int newgrp );
int     GEMDOS(0x10f)	Pgetuid( void );
int     GEMDOS(0x110)	Psetuid( int id );
int     GEMDOS(0x111)	Pkill( int pid, int sig );
void *	GEMDOS(0x112)	Psignal( int sig, void *handler );
int     GEMDOS(0x113)	Pvfork( void );
int     GEMDOS(0x114)	Pgetgid( void );
int     GEMDOS(0x115)	Psetgid( int id );
long    GEMDOS(0x116)	Psigblock( long mask );
long    GEMDOS(0x117)	Psigsetmask( long mask );
long    GEMDOS(0x118)	Pusrval( long arg );
int     GEMDOS(0x119)	Pdomain( int newdom );
void    GEMDOS(0x11a)	Psigreturn( void );
int     GEMDOS(0x11b)	Pfork( void );
long    GEMDOS(0x11c)	Pwait3( int flag, long *rusage );
int     GEMDOS(0x11d)	Fselect( unsigned int timeout, long *rfds, long *wfds, long *xfds );
void    GEMDOS(0x11e)	Prusage( long *r );
long    GEMDOS(0x11f)	Psetlimit( int lim, long value );
long    GEMDOS(0x120)	Talarm( long secs );
void    GEMDOS(0x121)	Pause( void );
long    GEMDOS(0x122)	Sysconf( int n );
long    GEMDOS(0x123)	Psigpending( void );
long    GEMDOS(0x124)	Dpathconf(const char *name, int n );
long    GEMDOS(0x125)	Pmsg( int mode, long mbox, MSG *msg );
long    GEMDOS(0x126)	Fmidipipe( int pid, int in, int out );
int     GEMDOS(0x127)	Prenice( int pid, int delta );
long    GEMDOS(0x128)	Dopendir(const char *name, int flag );
long    GEMDOS(0x129)	Dreaddir( int buflen, long dir, char *buf );
long    GEMDOS(0x12a)	Drewinddir( long dir );
long    GEMDOS(0x12b)	Dclosedir( long dir );
long    GEMDOS(0x12c)	Fxattr( int flag, const char *name, XATTR *buf );
long    GEMDOS(0x12d)	Flink( char *oldname, char *newname );
long    GEMDOS(0x12e)	Fsymlink( char *oldname, char *newname );
long    GEMDOS(0x12f)	Freadlink( int size, char *buf, char *name );
long    GEMDOS(0x130)	Dcntl( int cmd, char *name, long arg );
long    GEMDOS(0x131)	Fchown( char *name, int uid, int gid );
long    GEMDOS(0x132)	Fchmod( char *name, int mode );
long    GEMDOS(0x133)	Pumask( unsigned int mode );
long    GEMDOS(0x134)	Psemaphore( int mode, long id, long timeout );
long    GEMDOS(0x135)	Dlock( int mode, int drive );
void    GEMDOS(0x136)	Psigpause( unsigned long mask );
long    GEMDOS(0x137)	Psigaction( int sig, SIGACTION *act, SIGACTION *oact );
int     GEMDOS(0x138)	Pgeteuid( void );
int     GEMDOS(0x139)	Pgetegid( void );
long    GEMDOS(0x13a)	Pwaitpid( int pid, int flag, long *rusage );

long	GEMDOS(0x142)	Dxreaddir( short len, long handle, char * buf, long xattr, long *xret );	/* GEMDOS 0x142 */
long	GEMDOS(0x152)	Dreadlabel(const char *path, char *label, int size);
long	GEMDOS(0x153)	Dwritelabel(const char *path, const char *label);

/* Falcon */

#define VERTFLAG          0x0100
#define STMODES           0x0080
#define OVERSCAN          0x0040
#define PAL               0x0020
#define VGA_FALCON        0x0010
#define TV                0x0000

#define COL80             0x08
#define COL40             0x00

#define BPS16             4
#define BPS8              3
#define BPS4              2
#define BPS2              1
#define BPS1              0

#define NUMCOLS           7


typedef struct
{
    int     blocktype;
    long    blocksize;
    void    *blockaddr;
} DSPBLOCK;


long    locksnd( void );
long    unlocksnd( void );
long    soundcmd( int mode, int data );
long    setbuffer( int reg, void *begaddr, void *endaddr );
long    setmode( int mode );
long    settracks( int playtracks, int rectracks );
long    setmontracks( int montrack );
long    setinterrupt( int src_inter, int cause );
long    buffoper( int mode );
long    dsptristate( int dspxmit, int dsprec );
long    gpio( int mode, int data );
long    devconnect( int src, int dst, int srcclk, int prescale,
                    int protocol );
long    sndstatus( int reset );
long    buffptr( long *ptr );

int     Dsp_LoadProg( char *file, int ability, char *buffer );
long    Dsp_LodToBinary( char *file, char *codeptr );
void    Dsp_DoBlock( char *data_in, long size_in, char *data_out,
                     long size_out );
void    Dsp_BlkHandShake( char *data_in, long size_in, char *data_out,
                          long size_out );
void    Dsp_BlkUnpacked( long *data_in, long size_in, long *data_out,
                         long size_out );
void    Dsp_InStream( char *data_in, long block_size, long num_blocks,
                      long *blocks_done );
void    Dsp_OutStream( char *data_out, long block_size, long num_blocks,
                       long *blocks_done );
void    Dsp_IOStream( char *data_in, char *data_out, long block_insize,
                      long block_outsize, long num_blocks,
                      long *blocks_done );
void    Dsp_RemoveInterrupts( int mask );
int     Dsp_GetWordSize( void );
int     Dsp_Lock( void );
void    Dsp_Unlock( void );
void    Dsp_Available( long *xavailable, long *yavailable );
int     Dsp_Reserve( long xreserve, long yreserve );
void    Dsp_ExecProg( char *codeptr, long codesize, int ability );
void    Dsp_ExecBoot( char *codeptr, long codesize, int ability );
void    Dsp_TriggerHC( int vector );
int     Dsp_RequestUniqueAbility( void );
int     Dsp_GetProgAbility( void );
void    Dsp_FlushSubroutines( void );
int     Dsp_LoadSubroutine( char *codeptr, long codesize, int ability );
int     Dsp_InqSubrAbility( int ability );
int     Dsp_RunSubroutine( int handle );
int     Dsp_Hf0( int flag );
int     Dsp_Hf1( int flag );
int     Dsp_Hf2( void );
int     Dsp_Hf3( void );
void    Dsp_BlkWords( void *data_in, long size_in, void *data_out,
                      long size_out );
void    Dsp_BlkBytes( void *data_in, long size_in, void *data_out,
                      long size_out );
char    Dsp_HStat( void );
void    Dsp_SetVectors( void (*receiver)( ), long (*transmitter)( ));
void    Dsp_MultBlocks( long numsend, long numreceive,
                        DSPBLOCK *sendblocks, DSPBLOCK *receiveblocks );
int     Vsetmode( int modecode );
int     mon_type( void );
long    VgetSize( int mode );
void    VsetSync( int flag );
void    VsetRGB( int index, int count, long *array );
void    VgetRGB( int index, int count, long *array );
void    VsetMask( int ormask, int andmask, int overlay );

enum		/* Ssystem() codes */
{
	S_OSNAME,
	S_OSXNAME,
	S_OSVERSION,
	S_GETCOOKIE = 8,
	S_SETCOOKIE
};

long	GEMDOS(0x154)	Ssystem(int code, long arg1, long arg2);
void	GEMDOS(0x151)	Shutdown( long restart );
long	GEMDOS(0x13b)	Dgetcwd( char *path, short drv, short size );

#define __MINT__ 1		/* for mint.h */
#endif

/************************************************************************/
