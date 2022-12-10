/*
 * resource set indices for desktop
 *
 * created by ORCS 2.18
 */

/*
 * Number of Strings:        862
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       104
 * Number of Free Strings:   190
 * Number of Free Images:    0
 * Number of Objects:        601
 * Number of Trees:          26
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          31422
 */

#ifdef RSC_NAME
#undef RSC_NAME
#endif
#ifndef __ALCYON__
#define RSC_NAME "desktop"
#endif
#ifdef RSC_ID
#undef RSC_ID
#endif
#ifdef desktop
#define RSC_ID desktop
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 862
#define NUM_FRSTR 190
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 0
#define NUM_TI 104
#define NUM_OBS 601
#define NUM_TREE 26
#endif



#define MENU               0 /* menu */
#define TDESK              3 /* TITLE in tree MENU */
#define TLFILE             4 /* TITLE in tree MENU */
#define TVIEW              5 /* TITLE in tree MENU */
#define TWINDOW            6 /* TITLE in tree MENU */
#define TOPTIONS           7 /* TITLE in tree MENU */
#define MINFO             10 /* STRING in tree MENU */
#define MNFILEBX          18 /* BOX in tree MENU */
#define MOPEN             19 /* STRING in tree MENU */
#define MSHOWINF          20 /* STRING in tree MENU */
#define MNEWDIR           21 /* STRING in tree MENU */
#define MCOMPARE          22 /* STRING in tree MENU */
#define MSEARCH           23 /* STRING in tree MENU */
#define MPRINT            24 /* STRING in tree MENU */
#define MDELETE           25 /* STRING in tree MENU */
#define SEP1              26 /* STRING in tree MENU */
#define MSELALL           27 /* STRING in tree MENU */
#define SEP2              28 /* STRING in tree MENU */
#define MFCOPY            29 /* STRING in tree MENU */
#define MFFORMAT          30 /* STRING in tree MENU */
#define SEP3              31 /* STRING in tree MENU */
#define MQUIT             32 /* STRING in tree MENU */
#define MNVIEWBX          33 /* BOX in tree MENU */
#define MSHOWICN          34 /* STRING in tree MENU */
#define MAARNG            35 /* STRING in tree MENU */
#define SEP4              36 /* STRING in tree MENU */
#define MSNAME            37 /* STRING in tree MENU */
#define MSEXT             38 /* STRING in tree MENU */
#define MSDATE            39 /* STRING in tree MENU */
#define MSSIZE            40 /* STRING in tree MENU */
#define MSUNSORT          41 /* STRING in tree MENU */
#define MREVS             42 /* STRING in tree MENU */
#define MNOCASE           43 /* STRING in tree MENU */
#define SEP5              44 /* STRING in tree MENU */
#define MSHSIZ            45 /* STRING in tree MENU */
#define MSHDAT            46 /* STRING in tree MENU */
#define MSHTIM            47 /* STRING in tree MENU */
#define MSHATT            48 /* STRING in tree MENU */
#define MSHOWN            49 /* STRING in tree MENU */
#define SEP6              50 /* STRING in tree MENU */
#define MSETMASK          51 /* STRING in tree MENU */
#define MNWINBOX          52 /* BOX in tree MENU */
#define MICONIF           53 /* STRING in tree MENU */
#define MFULL             54 /* STRING in tree MENU */
#define MCLOSE            55 /* STRING in tree MENU */
#define MCLOSEW           56 /* STRING in tree MENU */
#define MCLOSALL          57 /* STRING in tree MENU */
#define MDUPLIC           58 /* STRING in tree MENU */
#define MCYCLE            59 /* STRING in tree MENU */
#define MNOPTBOX          60 /* BOX in tree MENU */
#define MAPPLIK           61 /* STRING in tree MENU */
#define MPRGOPT           62 /* STRING in tree MENU */
#define SEP7              63 /* STRING in tree MENU */
#define MIDSKICN          64 /* STRING in tree MENU */
#define MIWDICN           65 /* STRING in tree MENU */
#define SEP8              66 /* STRING in tree MENU */
#define MOPTIONS          67 /* STRING in tree MENU */
#define MCOPTS            68 /* STRING in tree MENU */
#define MWDOPT            69 /* STRING in tree MENU */
#define MVOPTS            70 /* STRING in tree MENU */
#define SEP9              71 /* STRING in tree MENU */
#define MLOADOPT          72 /* STRING in tree MENU */
#define MSAVESET          73 /* STRING in tree MENU */
#define MSAVEAS           74 /* STRING in tree MENU */

#define INFOBOX            1 /* form/dialog */
#define INFOVERS           2 /* TEXT in tree INFOBOX */
#define INFOSYS            3 /* TEXT in tree INFOBOX */
#define COPYRGHT           4 /* TEXT in tree INFOBOX */
#define OTHRINFO           5 /* TEXT in tree INFOBOX */
#define INFSTMEM           8 /* FTEXT in tree INFOBOX */
#define INFTTMEM           9 /* FTEXT in tree INFOBOX */
#define INFOTV            10 /* FTEXT in tree INFOBOX */
#define INFOAV            11 /* FTEXT in tree INFOBOX */

#define GETCML             2 /* form/dialog */
#define CMLOK              4 /* BUTTON in tree GETCML */
#define CMDLINE            5 /* FTEXT in tree GETCML */

#define NEWDIR             3 /* form/dialog */
#define NDTITLE            1 /* STRING in tree NEWDIR */
#define NEWDIRC            4 /* BUTTON in tree NEWDIR */
#define NEWDIROK           5 /* BUTTON in tree NEWDIR */
#define DIRNAME            6 /* FTEXT in tree NEWDIR */
#define OPENNAME           7 /* FTEXT in tree NEWDIR */
#define NEWDIRTO           8 /* STRING in tree NEWDIR */

#define ADDICON            4 /* form/dialog */
#define AITITLE            1 /* STRING in tree ADDICON */
#define DRIVEID            3 /* FTEXT in tree ADDICON */
#define INAMBOX            4 /* IBOX in tree ADDICON */
#define INAMET             5 /* STRING in tree ADDICON */
#define IFNAME             6 /* FTEXT in tree ADDICON */
#define ICNTYPE            7 /* FTEXT in tree ADDICON */
#define ICNLABEL           8 /* FTEXT in tree ADDICON */
#define ICNTYPT            9 /* STRING in tree ADDICON */
#define CHNBUTT           10 /* IBOX in tree ADDICON */
#define CHNICNOK          11 /* BUTTON in tree ADDICON */
#define CHICNSK           12 /* BUTTON in tree ADDICON */
#define CHNICNAB          13 /* BUTTON in tree ADDICON */
#define CHICNRM           14 /* BUTTON in tree ADDICON */
#define ADDBUTT           15 /* IBOX in tree ADDICON */
#define ADDICNOK          16 /* BUTTON in tree ADDICON */
#define ADDICNAB          17 /* BUTTON in tree ADDICON */
#define ICSELBOX          18 /* BOX in tree ADDICON */
#define ICONBACK          19 /* BOX in tree ADDICON */
#define ICONDATA          20 /* IBOX in tree ADDICON */
#define ICPARENT          21 /* BOX in tree ADDICON */
#define ICSLIDER          22 /* BOX in tree ADDICON */
#define ICNUP             23 /* BOXCHAR in tree ADDICON */
#define ICNDWN            24 /* BOXCHAR in tree ADDICON */
#define AIPARENT          25 /* BUTTON in tree ADDICON */
#define ICBTNS            26 /* IBOX in tree ADDICON */
#define ADISK             27 /* BUTTON in tree ADDICON */
#define APRINTER          28 /* BUTTON in tree ADDICON */
#define ATRASH            29 /* BUTTON in tree ADDICON */
#define AFILE             30 /* BUTTON in tree ADDICON */
#define ICSHFIL           31 /* STRING in tree ADDICON */
#define ICSHFLD           32 /* STRING in tree ADDICON */
#define ICSHPRG           33 /* STRING in tree ADDICON */
#define IFOLLNK           34 /* BUTTON in tree ADDICON */

#define APPLIKAT           5 /* form/dialog */
#define APDTITLE           1 /* STRING in tree APPLIKAT */
#define APT1               3 /* STRING in tree APPLIKAT */
#define APT2               4 /* STRING in tree APPLIKAT */
#define APT3               5 /* STRING in tree APPLIKAT */
#define APT4               6 /* STRING in tree APPLIKAT */
#define APPATH             7 /* FTEXT in tree APPLIKAT */
#define APNAME             8 /* FTEXT in tree APPLIKAT */
#define APCMLINE           9 /* FTEXT in tree APPLIKAT */
#define APLENV            10 /* FTEXT in tree APPLIKAT */
#define APFKEY            11 /* FTEXT in tree APPLIKAT */
#define SETUSE            12 /* BUTTON in tree APPLIKAT */
#define APPPTYPE          13 /* BUTTON in tree APPLIKAT */
#define APPFTYPE          14 /* BUTTON in tree APPLIKAT */
#define APOK              15 /* BUTTON in tree APPLIKAT */

#define FILEINFO           6 /* form/dialog */
#define FLITITLE           1 /* STRING in tree FILEINFO */
#define FLBYTES            4 /* FTEXT in tree FILEINFO */
#define ATTRBOX            5 /* IBOX in tree FILEINFO */
#define ISWP               6 /* BUTTON in tree FILEINFO */
#define ISHIDDEN           7 /* BUTTON in tree FILEINFO */
#define ISSYSTEM           8 /* BUTTON in tree FILEINFO */
#define ISARCHIV          10 /* BUTTON in tree FILEINFO */
#define MATCHBOX          11 /* BOX in tree FILEINFO */
#define NSMATCH           12 /* FTEXT in tree FILEINFO */
#define MATCH1            13 /* FTEXT in tree FILEINFO */
#define ISMATCH           14 /* FTEXT in tree FILEINFO */
#define FLCLSIZ           15 /* FTEXT in tree FILEINFO */
#define FLNAMBOX          16 /* IBOX in tree FILEINFO */
#define FLT1              17 /* STRING in tree FILEINFO */
#define FLPATH            18 /* FTEXT in tree FILEINFO */
#define FLT2              19 /* STRING in tree FILEINFO */
#define FLNAME            20 /* FTEXT in tree FILEINFO */
#define TDTIME            21 /* STRING in tree FILEINFO */
#define FLDATE            22 /* FTEXT in tree FILEINFO */
#define FLTIME            23 /* FTEXT in tree FILEINFO */
#define FLFOLBOX          24 /* IBOX in tree FILEINFO */
#define FLFOLDER          25 /* FTEXT in tree FILEINFO */
#define FLFILES           26 /* FTEXT in tree FILEINFO */
#define FLLABBOX          27 /* IBOX in tree FILEINFO */
#define FLDRIVE           28 /* TEXT in tree FILEINFO */
#define FLLABEL           29 /* FTEXT in tree FILEINFO */
#define FLSPABOX          32 /* IBOX in tree FILEINFO */
#define FLSPACE           33 /* FTEXT in tree FILEINFO */
#define FLFREE            34 /* FTEXT in tree FILEINFO */
#define FLLIBOX           35 /* IBOX in tree FILEINFO */
#define FLTGNAME          36 /* FTEXT in tree FILEINFO */
#define RIGHTBOX          38 /* IBOX in tree FILEINFO */
#define GID               40 /* FTEXT in tree FILEINFO */
#define UID               41 /* FTEXT in tree FILEINFO */
#define OWNR              42 /* BUTTON in tree FILEINFO */
#define OWNW              43 /* BUTTON in tree FILEINFO */
#define OWNX              44 /* BUTTON in tree FILEINFO */
#define OWNS              45 /* BUTTON in tree FILEINFO */
#define GRPR              46 /* BUTTON in tree FILEINFO */
#define GRPW              47 /* BUTTON in tree FILEINFO */
#define GRPX              48 /* BUTTON in tree FILEINFO */
#define GRPS              49 /* BUTTON in tree FILEINFO */
#define WRLR              50 /* BUTTON in tree FILEINFO */
#define WRLW              51 /* BUTTON in tree FILEINFO */
#define WRLX              52 /* BUTTON in tree FILEINFO */
#define WRLS              53 /* BUTTON in tree FILEINFO */
#define PFBOX             54 /* IBOX in tree FILEINFO */
#define PFLAGS            55 /* TEXT in tree FILEINFO */
#define PMEM              58 /* TEXT in tree FILEINFO */
#define FLALL             59 /* BUTTON in tree FILEINFO */
#define FLOK              60 /* BUTTON in tree FILEINFO */
#define FLSKIP            61 /* BUTTON in tree FILEINFO */
#define FLABORT           62 /* BUTTON in tree FILEINFO */
#define FOPWITH           63 /* TEXT in tree FILEINFO */
#define FOPWTXT           64 /* STRING in tree FILEINFO */

#define COPYINFO           7 /* form/dialog */
#define COPYBOX            0 /* BOX in tree COPYINFO */
#define CPTITLE            1 /* STRING in tree COPYINFO */
#define CPT1               3 /* STRING in tree COPYINFO */
#define CPT2               4 /* STRING in tree COPYINFO */
#define CPT3               5 /* STRING in tree COPYINFO */
#define CSETBOX            6 /* IBOX in tree COPYINFO */
#define CCHTIME            7 /* BUTTON in tree COPYINFO */
#define CCHATTR            8 /* BUTTON in tree COPYINFO */
#define CIFILES           10 /* STRING in tree COPYINFO */
#define CINFBOX           12 /* BOX in tree COPYINFO */
#define NFOLDERS          13 /* TEXT in tree COPYINFO */
#define NFILES            14 /* TEXT in tree COPYINFO */
#define NBYTES            15 /* TEXT in tree COPYINFO */
#define CNAMBOX           16 /* BOX in tree COPYINFO */
#define CPFOLDER          17 /* FTEXT in tree COPYINFO */
#define CPFILE            18 /* FTEXT in tree COPYINFO */
#define CPDEST            19 /* FTEXT in tree COPYINFO */
#define COKBOX            20 /* IBOX in tree COPYINFO */
#define CPT4              21 /* TEXT in tree COPYINFO */
#define COPYCAN           22 /* BUTTON in tree COPYINFO */
#define COPYOK            23 /* BUTTON in tree COPYINFO */
#define PSETBOX           24 /* IBOX in tree COPYINFO */
#define PRTXT             25 /* BUTTON in tree COPYINFO */
#define PRHEX             26 /* BUTTON in tree COPYINFO */
#define CPRFILE           28 /* BUTTON in tree COPYINFO */
#define CFOLLNK           29 /* BUTTON in tree COPYINFO */

#define NAMECONF           8 /* form/dialog */
#define RNMTITLE           1 /* STRING in tree NAMECONF */
#define NCT1               3 /* STRING in tree NAMECONF */
#define NCT2               4 /* STRING in tree NAMECONF */
#define OLDNAME            5 /* FTEXT in tree NAMECONF */
#define NEWNAME            6 /* FTEXT in tree NAMECONF */
#define NCALL              7 /* BUTTON in tree NAMECONF */
#define NCOK               8 /* BUTTON in tree NAMECONF */
#define NCSKIP             9 /* BUTTON in tree NAMECONF */
#define NCABORT           10 /* BUTTON in tree NAMECONF */
#define NCNINFO           11 /* TEXT in tree NAMECONF */
#define NCCINFO           12 /* TEXT in tree NAMECONF */

#define SETMASK            9 /* form/dialog */
#define DTSMASK            1 /* STRING in tree SETMASK */
#define TSBOX              3 /* IBOX in tree SETMASK */
#define FTSCROLL           4 /* BOX in tree SETMASK */
#define FTPARENT           5 /* BOX in tree SETMASK */
#define FTYPE1             6 /* FTEXT in tree SETMASK */
#define FTYPE2             7 /* FTEXT in tree SETMASK */
#define FTYPE3             8 /* FTEXT in tree SETMASK */
#define FTYPE4             9 /* FTEXT in tree SETMASK */
#define FTUP              10 /* BOXCHAR in tree SETMASK */
#define FTDOWN            11 /* BOXCHAR in tree SETMASK */
#define FTSPAR            12 /* BOX in tree SETMASK */
#define FTSLIDER          13 /* BOX in tree SETMASK */
#define TSBUTT            14 /* IBOX in tree SETMASK */
#define FTMOVEUP          15 /* BUTTON in tree SETMASK */
#define FTMOVEDN          16 /* BUTTON in tree SETMASK */
#define FTADD             17 /* BUTTON in tree SETMASK */
#define FTCHANGE          18 /* BUTTON in tree SETMASK */
#define FTDELETE          19 /* BUTTON in tree SETMASK */
#define FTTEXT            20 /* STRING in tree SETMASK */
#define FILETYPE          21 /* FTEXT in tree SETMASK */
#define FTCANCEL          22 /* BUTTON in tree SETMASK */
#define FTOK              23 /* BUTTON in tree SETMASK */
#define ICATT             24 /* IBOX in tree SETMASK */
#define ITFILES           25 /* BUTTON in tree SETMASK */
#define ITFOLDER          26 /* BUTTON in tree SETMASK */
#define ITPROGRA          27 /* BUTTON in tree SETMASK */
#define MSKATT            28 /* IBOX in tree SETMASK */
#define MSKHID            29 /* BUTTON in tree SETMASK */
#define MSKSYS            30 /* BUTTON in tree SETMASK */
#define MSKDIR            31 /* BUTTON in tree SETMASK */
#define MSKPAR            32 /* BUTTON in tree SETMASK */
#define PGATT             33 /* IBOX in tree SETMASK */
#define PKEY              34 /* BUTTON in tree SETMASK */
#define PSTDERR           35 /* BUTTON in tree SETMASK */

#define OPTIONS           10 /* form/dialog */
#define OPTPAR1            4 /* IBOX in tree OPTIONS */
#define DBUFFERD           5 /* BUTTON in tree OPTIONS */
#define DWINDOW            6 /* BUTTON in tree OPTIONS */
#define OPTPAR2            7 /* IBOX in tree OPTIONS */
#define DMOUSE             8 /* BUTTON in tree OPTIONS */
#define DCENTER            9 /* BUTTON in tree OPTIONS */
#define OPTCANC           10 /* BUTTON in tree OPTIONS */
#define OPTOK             11 /* BUTTON in tree OPTIONS */
#define OPTKKEY           14 /* FTEXT in tree OPTIONS */
#define OPTKCLR           15 /* BUTTON in tree OPTIONS */
#define OPTMNEXT          16 /* BOXCHAR in tree OPTIONS */
#define OPTMPREV          17 /* BOXCHAR in tree OPTIONS */
#define OPTMTEXT          18 /* BUTTON in tree OPTIONS */
#define SEXIT             19 /* BUTTON in tree OPTIONS */

#define ADDPTYPE          11 /* form/dialog */
#define APTITLE            1 /* STRING in tree ADDPTYPE */
#define APTPAR1            3 /* BUTTON in tree ADDPTYPE */
#define ATWINDOW           4 /* BUTTON in tree ADDPTYPE */
#define ATPRG              5 /* BUTTON in tree ADDPTYPE */
#define ATFILE             6 /* BUTTON in tree ADDPTYPE */
#define APTPAR2            7 /* BUTTON in tree ADDPTYPE */
#define APGEM              8 /* BUTTON in tree ADDPTYPE */
#define APGTP              9 /* BUTTON in tree ADDPTYPE */
#define APACC             10 /* BUTTON in tree ADDPTYPE */
#define APTOS             11 /* BUTTON in tree ADDPTYPE */
#define APTTP             12 /* BUTTON in tree ADDPTYPE */
#define PTTEXT            13 /* STRING in tree ADDPTYPE */
#define PRGNAME           14 /* FTEXT in tree ADDPTYPE */
#define ATARGV            15 /* BUTTON in tree ADDPTYPE */
#define ATSINGLE          16 /* BUTTON in tree ADDPTYPE */
#define MEMLIM            17 /* FTEXT in tree ADDPTYPE */
#define ATBACKG           18 /* BUTTON in tree ADDPTYPE */
#define APTOK             19 /* BUTTON in tree ADDPTYPE */
#define APTCANC           20 /* BUTTON in tree ADDPTYPE */

#define VIEWMENU          12 /* menu */
#define VMVIEW             3 /* TITLE in tree VIEWMENU */
#define VMTAB              6 /* STRING in tree VIEWMENU */
#define VMHEX              7 /* STRING in tree VIEWMENU */

#define STABSIZE          13 /* form/dialog */
#define STOK               4 /* BUTTON in tree STABSIZE */
#define VTABSIZE           5 /* FTEXT in tree STABSIZE */

#define WOPTIONS          14 /* form/dialog */
#define WDDCOLT            4 /* STRING in tree WOPTIONS */
#define WDWCOLT            6 /* STRING in tree WOPTIONS */
#define WODIR              7 /* BUTTON in tree WOPTIONS */
#define WOVIEWER           8 /* BUTTON in tree WOPTIONS */
#define WOPTOK            10 /* BUTTON in tree WOPTIONS */
#define DSKPDOWN          11 /* BOXCHAR in tree WOPTIONS */
#define DSKPAT            12 /* BOX in tree WOPTIONS */
#define DSKPUP            13 /* BOXCHAR in tree WOPTIONS */
#define DSKCDOWN          14 /* BOXCHAR in tree WOPTIONS */
#define DSKCUP            15 /* BOXCHAR in tree WOPTIONS */
#define WINPDOWN          16 /* BOXCHAR in tree WOPTIONS */
#define WINPAT            17 /* BOX in tree WOPTIONS */
#define WINPUP            18 /* BOXCHAR in tree WOPTIONS */
#define WINCDOWN          19 /* BOXCHAR in tree WOPTIONS */
#define WINCUP            20 /* BOXCHAR in tree WOPTIONS */
#define TABSIZE           21 /* FTEXT in tree WOPTIONS */
#define SOPEN             22 /* BUTTON in tree WOPTIONS */
#define IXGRID            23 /* FTEXT in tree WOPTIONS */
#define IXOF              24 /* FTEXT in tree WOPTIONS */
#define IYGRID            25 /* FTEXT in tree WOPTIONS */
#define IYOF              26 /* FTEXT in tree WOPTIONS */

#define WDFONT            15 /* form/dialog */
#define WDFTITLE           1 /* STRING in tree WDFONT */
#define WDFOEAR            2 /* BOX in tree WDFONT */
#define WDPARENT           4 /* BOX in tree WDFONT */
#define WDFONT1            5 /* TEXT in tree WDFONT */
#define WDFONT2            6 /* TEXT in tree WDFONT */
#define WDFONT3            7 /* TEXT in tree WDFONT */
#define WDFONT4            8 /* TEXT in tree WDFONT */
#define WDFUP              9 /* BOXCHAR in tree WDFONT */
#define FSPARENT          10 /* BOX in tree WDFONT */
#define FSLIDER           11 /* BOX in tree WDFONT */
#define WDFDOWN           12 /* BOXCHAR in tree WDFONT */
#define WDFCANC           14 /* BUTTON in tree WDFONT */
#define WDFOK             15 /* BUTTON in tree WDFONT */
#define WDFSDOWN          16 /* BOXCHAR in tree WDFONT */
#define WDFSIZE           17 /* BUTTON in tree WDFONT */
#define WDFSUP            18 /* BOXCHAR in tree WDFONT */
#define WDFTEXT           20 /* BUTTON in tree WDFONT */
#define WDFCDOWN          22 /* BOXCHAR in tree WDFONT */
#define WDFCOL            23 /* BOX in tree WDFONT */
#define WDFCUP            24 /* BOXCHAR in tree WDFONT */
#define WDFEFF            25 /* IBOX in tree WDFONT */
#define WDFBOLD           26 /* BUTTON in tree WDFONT */
#define WDFLIG            27 /* BUTTON in tree WDFONT */
#define WDFITAL           28 /* BUTTON in tree WDFONT */
#define WDFUNDER          29 /* BUTTON in tree WDFONT */
#define WDFHOLL           30 /* BUTTON in tree WDFONT */
#define WDFSHAD           31 /* BUTTON in tree WDFONT */
#define WDFINV            32 /* BUTTON in tree WDFONT */

#define FLOPPY            16 /* form/dialog */
#define FLTITLE            1 /* STRING in tree FLOPPY */
#define FSRCDRV            3 /* FTEXT in tree FLOPPY */
#define FTGTDRV            4 /* FTEXT in tree FLOPPY */
#define FFTYPE             5 /* BUTTON in tree FLOPPY */
#define FSSIDED            6 /* BUTTON in tree FLOPPY */
#define FDSIDED            7 /* BUTTON in tree FLOPPY */
#define FHSIDED            8 /* BUTTON in tree FLOPPY */
#define FESIDED            9 /* BUTTON in tree FLOPPY */
#define FPREV             10 /* BUTTON in tree FLOPPY */
#define FPAR2             11 /* BUTTON in tree FLOPPY */
#define FPAR3             12 /* IBOX in tree FLOPPY */
#define FSIDES            13 /* FTEXT in tree FLOPPY */
#define FTRACKS           14 /* FTEXT in tree FLOPPY */
#define FSECTORS          15 /* FTEXT in tree FLOPPY */
#define FDIRSIZE          16 /* FTEXT in tree FLOPPY */
#define FRCNT             17 /* FTEXT in tree FLOPPY */
#define FLOT1             18 /* TEXT in tree FLOPPY */
#define FMTCANC           19 /* BUTTON in tree FLOPPY */
#define FMTOK             20 /* BUTTON in tree FLOPPY */
#define FLABEL            21 /* FTEXT in tree FLOPPY */
#define PROGRBOX          22 /* BOX in tree FLOPPY */
#define FPROGRES          23 /* FTEXT in tree FLOPPY */
#define FQWIK             24 /* BUTTON in tree FLOPPY */

#define VOPTIONS          17 /* form/dialog */
#define VREZOL             3 /* BUTTON in tree VOPTIONS */
#define VSTLOW             4 /* BUTTON in tree VOPTIONS */
#define VSTMED             5 /* BUTTON in tree VOPTIONS */
#define VSTHIGH            6 /* BUTTON in tree VOPTIONS */
#define VTTLOW             7 /* BUTTON in tree VOPTIONS */
#define VTTMED             8 /* BUTTON in tree VOPTIONS */
#define VTTHIGH            9 /* BUTTON in tree VOPTIONS */
#define SVCOLORS          11 /* BUTTON in tree VOPTIONS */
#define VBLITTER          12 /* BUTTON in tree VOPTIONS */
#define VOVERSCN          13 /* BUTTON in tree VOPTIONS */
#define FCOLBOX           14 /* BOX in tree VOPTIONS */
#define VNCOL             15 /* BUTTON in tree VOPTIONS */
#define VNCOLDN           16 /* BOXCHAR in tree VOPTIONS */
#define VNCOLUP           17 /* BOXCHAR in tree VOPTIONS */
#define VIDCANC           19 /* BUTTON in tree VOPTIONS */
#define VIDOK             20 /* BUTTON in tree VOPTIONS */

#define ADDFTYPE          18 /* form/dialog */
#define FTYTITLE           1 /* STRING in tree ADDFTYPE */
#define FTYPEOK            4 /* BUTTON in tree ADDFTYPE */
#define FTYPE0             5 /* FTEXT in tree ADDFTYPE */

#define COPTIONS          19 /* form/dialog */
#define CPAR1              3 /* IBOX in tree COPTIONS */
#define CCOPY              4 /* BUTTON in tree COPTIONS */
#define COVERW             5 /* BUTTON in tree COPTIONS */
#define CDEL               6 /* BUTTON in tree COPTIONS */
#define CPRINT             7 /* BUTTON in tree COPTIONS */
#define CSHOWD             8 /* BUTTON in tree COPTIONS */
#define CKEEPS             9 /* BUTTON in tree COPTIONS */
#define CTRUNC            10 /* BUTTON in tree COPTIONS */
#define CPPHEAD           11 /* BUTTON in tree COPTIONS */
#define COPTOK            13 /* BUTTON in tree COPTIONS */
#define COPYBUF           14 /* FTEXT in tree COPTIONS */
#define CPPLINE           15 /* FTEXT in tree COPTIONS */

#define SEARCH            20 /* form/dialog */
#define ST1                3 /* STRING in tree SEARCH */
#define SMASK              4 /* FTEXT in tree SEARCH */
#define SLODATE            5 /* FTEXT in tree SEARCH */
#define SHIDATE            6 /* FTEXT in tree SEARCH */
#define SLOSIZE            7 /* FTEXT in tree SEARCH */
#define SHISIZE            8 /* FTEXT in tree SEARCH */
#define ST2                9 /* STRING in tree SEARCH */
#define SGREP             10 /* FTEXT in tree SEARCH */
#define IGNCASE           11 /* BUTTON in tree SEARCH */
#define SOK               12 /* BUTTON in tree SEARCH */
#define SCANCEL           13 /* BUTTON in tree SEARCH */
#define SKIPSUB           14 /* BUTTON in tree SEARCH */
#define SPT4              15 /* TEXT in tree SEARCH */

#define SPECAPP           21 /* form/dialog */
#define ISSPEC             3 /* IBOX in tree SPECAPP */
#define ISEDIT             4 /* BUTTON in tree SPECAPP */
#define ISVIEW             5 /* BUTTON in tree SPECAPP */
#define ISAUTO             6 /* BUTTON in tree SPECAPP */
#define ISSHUT             7 /* BUTTON in tree SPECAPP */
#define ISVIDE             8 /* BUTTON in tree SPECAPP */
#define ISSRCH             9 /* BUTTON in tree SPECAPP */
#define ISCOMP            10 /* BUTTON in tree SPECAPP */
#define ISFRMT            11 /* BUTTON in tree SPECAPP */
#define ISRBXT            12 /* BUTTON in tree SPECAPP */
#define SPECOK            14 /* BUTTON in tree SPECAPP */

#define OPENW             22 /* form/dialog */
#define OWCANC             3 /* BUTTON in tree OPENW */
#define OWSHOW             4 /* BUTTON in tree OPENW */
#define OWRUN              5 /* BUTTON in tree OPENW */
#define OWUSE              6 /* BUTTON in tree OPENW */
#define OWEDIT             7 /* BUTTON in tree OPENW */

#define HELP1             23 /* form/dialog */
#define HLPTITLE           1 /* STRING in tree HELP1 */
#define HELPOK             3 /* BUTTON in tree HELP1 */
#define HELPCANC           4 /* BUTTON in tree HELP1 */
#define HLPBOX1            5 /* BOX in tree HELP1 */
#define HLPBOX2           18 /* BOX in tree HELP1 */
#define HLPBOX3           29 /* BOX in tree HELP1 */
#define HLPBOX4           38 /* BOX in tree HELP1 */

#define COMPARE           24 /* form/dialog */
#define COMPCANC           3 /* BUTTON in tree COMPARE */
#define COMPOK             4 /* BUTTON in tree COMPARE */
#define CFILE1             5 /* FTEXT in tree COMPARE */
#define CFILE2             6 /* FTEXT in tree COMPARE */
#define CMPIBOX            9 /* BOX in tree COMPARE */
#define COFFSET           10 /* FTEXT in tree COMPARE */
#define DTEXT1            11 /* TEXT in tree COMPARE */
#define DTEXT2            12 /* TEXT in tree COMPARE */
#define COMPWIN           15 /* FTEXT in tree COMPARE */

#define QUITOPT           25 /* form/dialog */
#define QUITTIT            1 /* STRING in tree QUITOPT */
#define QUITSHUT           5 /* BUTTON in tree QUITOPT */
#define QUITBOOT           6 /* BUTTON in tree QUITOPT */
#define QUITQUIT           7 /* BUTTON in tree QUITOPT */
#define QUITCANC           8 /* BUTTON in tree QUITOPT */

#define DTADDMSK           0 /* Free string */

#define DTCOPY             1 /* Free string */

#define DTCOPYRN           2 /* Free string */

#define DTMOVE             3 /* Free string */

#define DTMOVERN           4 /* Free string */

#define DTDELETE           5 /* Free string */

#define DTRENAME           6 /* Free string */

#define DTNMECNF           7 /* Free string */

#define DTADDICN           8 /* Free string */

#define DTCHNICN           9 /* Free string */

#define DTADDPRG          10 /* Free string */

#define DTEDTPRG          11 /* Free string */

#define DTADDICT          12 /* Free string */

#define DTEDTICT          13 /* Free string */

#define DTDFONT           14 /* Free string */

#define DTVFONT           15 /* Free string */

#define FSTLFILE          16 /* Free string */

#define FSTLPRG           17 /* Free string */

#define FSTLFLDR          18 /* Free string */

#define FSTLOADS          19 /* Free string */

#define FSTSAVES          20 /* Free string */

#define MPRINTER          21 /* Free string */

#define MTRASHCN          22 /* Free string */

#define MDRIVE            23 /* Free string */

#define MFOLDER           24 /* Free string */

#define MPROGRAM          25 /* Free string */

#define MITEMS            26 /* Free string */

#define MSITEMS           27 /* Free string */

#define MIPLURAL          28 /* Free string */

#define MISINGUL          29 /* Free string */

#define MDRVNAME          30 /* Free string */

#define MKEYCONT          31 /* Free string */

#define TERROR            32 /* Free string */

#define TFILNF            33 /* Free string */

#define TPATHNF           34 /* Free string */

#define TACCDN            35 /* Free string */

#define TENSMEM           36 /* Free string */

#define TDSKFULL          37 /* Free string */

#define TLOCKED           38 /* Free string */

#define TPLFMT            39 /* Free string */

#define TFNTLNG           40 /* Free string */

#define TPTHTLNG          41 /* Free string */

#define TREAD             42 /* Free string */

#define TEOF              43 /* Free string */

#define TABORT            44 /* Free string */

#define TSKIPABT          45 /* Free string */

#define FLINAME           46 /* Free string */

#define HDINAME           47 /* Free string */

#define TRINAME           48 /* Free string */

#define PRINAME           49 /* Free string */

#define FIINAME           50 /* Free string */

#define APINAME           51 /* Free string */

#define FOINAME           52 /* Free string */

#define TCMDTLNG          53 /* Free string */

#define DTPRINT           54 /* Free string */

#define DTPRINTD          55 /* Free string */

#define DTFFMT            56 /* Free string */

#define DTFCPY            57 /* Free string */

#define TWPROT            58 /* Free string */

#define TDIROF            59 /* Free string */

#define DTFTYPES          60 /* Free string */

#define DTITYPES          61 /* Free string */

#define DTPTYPES          62 /* Free string */

#define DTINSAPP          63 /* Free string */

#define DTDTYPES          64 /* Free string */

#define DTEDTMSK          65 /* Free string */

#define DTEDTDT           66 /* Free string */

#define TFTYPE            67 /* Free string */

#define TAPP              68 /* Free string */

#define DTADDDT           69 /* Free string */

#define DTADDAPP          70 /* Free string */

#define DTEDTAPP          71 /* Free string */

#define DTSETAPT          72 /* Free string */

#define DTTOUCH           73 /* Free string */

#define MECOPY            74 /* Free string */

#define MEMOVE            75 /* Free string */

#define MEDELETE          76 /* Free string */

#define MEPRINT           77 /* Free string */

#define MESHOWIF          78 /* Free string */

#define MINVSRCH          79 /* Free string */

#define MDIFERR           80 /* Free string */

#define MFPARERR          81 /* Free string */

#define MCANTACC          82 /* Free string */

#define MECOLORS          83 /* Free string */

#define MFSELERR          84 /* Free string */

#define MTMICONS          85 /* Free string */

#define APPNOEXT          86 /* Free string */

#define APPNODD           87 /* Free string */

#define MDRVEXIS          88 /* Free string */

#define MDIRTBIG          89 /* Free string */

#define MDEXISTS          90 /* Free string */

#define MTMWIND           91 /* Free string */

#define MFNEMPTY          92 /* Free string */

#define MCOPYSLF          93 /* Free string */

#define MVALIDCF          94 /* Free string */

#define BADCFG            95 /* Free string */

#define TFRVAL            96 /* Free string */

#define MCOMPOK           97 /* Free string */

#define DTFONSEL          98 /* Free string */

#define DTNEWDIR          99 /* Free string */

#define DTOPENIT         100 /* Free string */

#define MNOITEM          101 /* Free string */

#define TDONTEDI         102 /* Free string */

#define TNOLOCK          103 /* Free string */

#define DTFIINF          104 /* Free string */

#define DTFOINF          105 /* Free string */

#define DTDRINF          106 /* Free string */

#define TCRETIM          107 /* Free string */

#define TACCTIM          108 /* Free string */

#define TNOEDIT          109 /* Free string */

#define DTAFONT          110 /* Free string */

#define DTNEWLNK         111 /* Free string */

#define TLINKTO          112 /* Free string */

#define DTLIINF          113 /* Free string */

#define TNOTGT           114 /* Free string */

#define DTUPDAT          115 /* Free string */

#define DTRESTO          116 /* Free string */

#define TNFILES          117 /* Free string */

#define TFILES           118 /* Free string */

#define TMULTI           119 /* Free string */

#define DWTITLE          120 /* Free string */

#define MENUREG          121 /* Free string */

#define DTSELAPP         122 /* Free string */

#define CANCTXT          123 /* Free string */

#define MSRFINI          124 /* Free string */

#define FSPRINT          125 /* Free string */

#define TFALLOW          126 /* Free string */

#define TFALMED          127 /* Free string */

#define TINTERL          128 /* Free string */

#define MRESTLOW         129 /* Free string */

#define MICNFRD          130 /* Free string */

#define MVDIERR          131 /* Free string */

#define MDUPAPP          132 /* Free string */

#define MFAILPRG         133 /* Free string */

#define MICNFND          134 /* Free string */

#define DTDEVINF         135 /* Free string */

#define DTPIPINF         136 /* Free string */

#define DTMEMINF         137 /* Free string */

#define DTPRGINF         138 /* Free string */

#define MGEMBACK         139 /* Free string */

#define PFFLOAD          140 /* Free string */

#define PFALOAD          141 /* Free string */

#define PFAMEM           142 /* Free string */

#define PPPRIVAT         143 /* Free string */

#define PPGLOBAL         144 /* Free string */

#define PPSUPER          145 /* Free string */

#define PPREAD           146 /* Free string */

#define PPSHARE          147 /* Free string */

#define FSTLANY          148 /* Free string */

#define TDOPEN           149 /* Free string */

#define TFNVALID         150 /* Free string */

#define TFNTMPLT         151 /* Free string */

#define TBYTES           152 /* Free string */

#define MUNKNOWN         153 /* Free string */

#define MNETOB           154 /* Free string */

#define MNOCOPY          155 /* Free string */

#define MNOPRINT         156 /* Free string */

#define MNODRAGP         157 /* Free string */

#define MNOOPEN          158 /* Free string */

#define TBADS            159 /* Free string */

#define TXBERROR         160 /* Free string */

#define TSEEKE           161 /* Free string */

#define TUNKM            162 /* Free string */

#define TFIALL           163 /* Free string */

#define TFIMORE          164 /* Free string */

#define XFNVALID         165 /* Free string */

#define TCFIRE           166 /* Free string */

#define ALOADCFG         167 /* Alert string */

#define APRGNFND         168 /* Alert string */

#define AAPPNFND         169 /* Alert string */

#define AILLCOPY         170 /* Alert string */

#define AILLDEST         171 /* Alert string */

#define APRNRESP         172 /* Alert string */

#define AERADISK         173 /* Alert string */

#define AFMTERR          174 /* Alert string */

#define AERRACC          175 /* Alert string */

#define ADUPKEY          176 /* Alert string */

#define AABOOP           177 /* Alert string */

#define AGENALRT         178 /* Alert string */

#define AFILEDEF         179 /* Alert string */

#define AGFALERT         180 /* Alert string */

#define AUNKRTYP         181 /* Alert string */

#define AVALIDCF         182 /* Alert string */

#define ADUPFLG          183 /* Alert string */

#define AWOPERR          184 /* Alert string */

#define ARESCH           185 /* Alert string */

#define AFABORT          186 /* Alert string */

#define AQUERY           187 /* Alert string */

#define AREMICNS         188 /* Alert string */

#define ACANTDO          189 /* Alert string */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD desktop_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD desktop_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD desktop_rsc_free(void);
#endif
