Tera Desktop V1.41, 20-09-95, Copyright 1991, 1992, 1993, 1994, 1995 W. Klaren.
             V2.1   15-11-02, Copyright 2002  H. Robbers.
             V2.3           , Copyright 2003  H. Robbers, Dj. Vukovic
             V3.0   14-12-03, Copyright 2003  H. Robbers, Dj. Vukovic        
             V3.01  07-01-04, Copyright 2003, 2004  H. Robbers, Dj. Vukovic        
             V3.30  23-04-04, Copyright 2004  H. Robbers, Dj. Vukovic
             V3.31       -04, Copyright 2004  H. Robbers, Dj. Vukovic


This is version 3.31 of the Tera Desktop, a replacement of the built-in TOS 
desktop for 16-bit and 32-bit Atari computers. This program is Freeware and 
Open Source.  It is published under GPL license, which means that it may be 
copied  and modified freely,  providing  that the  original authorships are 
recognized where appropriate,  and that it,  or its derivatives, may not be 
sold. See the included COPYING file for the details on GPL.
 

Package manifest:
=================

Tera Desktop binary distribution currently consists of the following files:

        COPYING         (A copy of the GPL license)
        DESKTOP.PRG	(All-environments Desktop)
	DESKTOS.PRG	(Somewhat smaller Desktop compiled for Single-TOS)
        DESKTOP.RSC  	(English resource file)
	DESKTOP.RSD	(Resource object names definition file)
        ICONS.RSC    	(Essential Mono icons)
        CICONS.RSC   	(Essential Colour icons)
        README.TXT	(This file)
        HIST_V3.TXT	(Log of changes since Version 3.0)
        TERADESK.HYP	(English manual in ST-Guide hypertext)
        TERADESK.REF    (Reference file for the hypertext manual)
        TERADESK.INF    (Sample configuration file)
        TERADESK.PAL    (Sample palette file)

There also exists a source distribution which contains the complete source 
tree of TeraDesk, to be compiled and linked with Pure-C 1.1; source of the 
AHCM memory-allocation system can be downloaded from:
http://members.ams.chello.nl/h.robbers/ahcm.zip


Hardware and Operating System Requirements
==========================================

Tera Desktop can be used on any Atari ST series computer and their offspring,
TT, Falcon,  Hades, Milan or emulators. It uses about 200-250 KB of memory 
(depending on the complexity of configuration).  

Although Tera Desktop can be used without the aid of a hard disk, the use  
of one is strongly recommended.  

Tera Desktop should work with all  existing versions of  TOS (i.e. starting
with TOS 1.0) but it is much more useful with TOS versions 1.04 (also known 
as TOS 1.4) and above.  

Since Version 2.0  Tera Desktop runs on modern  multitasking  environments,  
such as:  MagiC, TOS with Geneva,  TOS with Mint and N.Aes,  XaAES or Atari 
AES 4.1, etc.  

Some features of Tera Desktop may be nonfunctional, depending on the version
of the OS and the AES used.

Tera Desktop  makes  several  inquires  trying  to determine  TOS- and AES-  
versions and their capabilities and limitations.  If some version of TOS or 
AES is not able to answer these queries, Tera Desktop tries to make guesses
which may not always be correct.  It is also possible that incorrect answer
to a query is supplied by TOS/AES, in which case Tera Desktop may work with 
unnecessary limitations or else try to activate features which may not work.  


New Features in This Version
============================

Please, see HIST_V3.TXT for a list of new features and bug fixes since the
last released version (V3.30). Also read the manual TERADESK.HYP (you will 
need ST-Guide for this) for more detailed information.


Installation
============

1. A folder named e.g. DESKTOP or TERADESK can be created anywhere on your  
floppy or hard disk, or in a RAM disk,  to hold Tera Desktop files.  It is 
also possible,  although a bit untidy,  to run Tera Desktop  from the root 
directory of a disk volume. 

2. The following files  should  be copied  to the location  specified  for 
TeraDesk:  
  
       DESKTOP.PRG  (if  you  intend  to use multitasking)  OR   
       DESKTOS.PRG  (if you  will  work  in  Single-TOS  only)    
       DESKTOP.RSC    
       ICONS.RSC    AND/OR  
       CICONS.PRG   (if your AES can support colour icons)   
 
Note that DESKTOP.PRG will work  in single-TOS as well;  DESKTOS.PRG just 
saves a  few kilobytes  of memory  by not  containing code  which is only 
relevant in  multitasking environment,  and by having a somewhat  limited
support for the AV-protocol.

If only DESKTOS.PRG is to be used, it may be renamed to DESKTOP.PRG after
copying,  but this is not required  (the program will register  itself as
"DESKTOP" anyway).  

You can (but need not) also copy  into this folder the files  TERADESK.INF
and  TERADESK.PAL  from the  \EXAMPLES folder.  Note, however,  that these 
example configuration files are set for one particular configuration,  and 
may not be appropriate  for your setup  (Tera Desktop will attempt to obey 
everything specified in the configuration files, no matter what the actual 
environment is).

3. If you use ST-GUIDE,  copy TERADESK.HYP and TERADESK.REF from the  \DOC 
folder to the folder where your other .HYP files are.  

4. ONLY if you need to convert TeraDesk V2.* configuration files to the new 
format of TeraDesk V3.*, you need to  get also the  Configuration convertor 
CFG2INF.PRG;  This  utility  (not included  in the binary distribution  but 
available  separately  on  TeraDesk 3  home page)  is basically  a  tweaked 
compilation of the Tera Desktop version 3.01, with the  capability  to read
only the old file format and to write only the new format. After converting
the old configuration files,  CFG2INF.PRG is not needed anymore  and can be 
removed.

5. Cooperation  of  Tera Desktop   with  some other  applications  will  be 
improved if  it is  announced  that certain  protocols  can be  handled. In 
order to do so, the following environment variables can be defined:

 AVSERVER=DESKTOP
 FONTSELECT=DESKTOP
 
These  declare  TeraDesk  as the  AV-Server and  as  the  font-selector. In 
Single-TOS configuration  these protocols  can be used  by some accessories 
(such as ST-Guide);  in multitasking  configurations  they  can be  used by 
a much larger number of concurrently running applications.

The manner of declaration of environmental variables depend  on variants of 
the OS and utilities used.

6. It is convenient to set TeraDesk to start automatically at system boot. 

If you use (Single) TOS version 1.04  (also known as  TOS 1.4)  or greater, 
you can set it up by installing it as an application,  and then setting its 
boot status to 'Auto' (remember to save the desktop configuration). 
    
If you have TOS version 1.0 or 1.02 you must use a program such as STARTGEM 
to run DESKTOP from an AUTO folder.  
  
If you use Atari AES 4.1,  you can put something similar  to the  following 
directive into your GEM.CNF file:  
    
     shell path\to\TeraDesk\desktop.prg  
  
Then, the built-in  desktop of AES 4.1  will not be  loaded  at startup and 
TeraDesk will run as the desktop instead.

If you use Geneva, N.AES or XaAES, you should in a similar way specify
TeraDesk as the shell in the appropriate  places in the  configuration
files  of these  AESses (i.e. in  GEM.CNF,  N_AES.CNF and XA_EXEC.SCL, 
respectively). 
    
If you use MagiC, TeraDesk should be specified as a shell via the #_SHL
directive in MAGX.INF:

     #_SHL path\to\TeraDesk\desktop.prg 

7. All text strings used by Tera Desktop are located in DESKTOP.RSC (except
a warning that DESKTOP.RSC can not be found).  It is possible to completely
adapt TeraDesk to other languages by a translated DESKTOP.RSC  (if someone
is willing to supply it).

8. The icon files supplied contain a basic set of icons only. Users can add 
icons at will,  or use other icon files  (e.g. one can rename  DESKICON.RSC 
and/or DESKCICN.RSC used by the built-in desktop of TOS 2/3/4  to ICONS.RSC  
and CICONS.RSC respectively, and use them with TeraDesk).

TeraDesk, since V2.0, handles icons by name,  not by index.  Icons with the 
following names should always be present in the icons resource file:

FLOPPY, HARD DISC, TRASH, PRINTER, FOLDER, FILE, APP

If a required icon name  can not be found in the icons resource file,  icon 
name FILE is tried. If that does not exist either, you loose an icon.

To facilitate adaptation of TeraDesk to other languages, names of the seven 
essential icons are  not  hard-coded but are read from  DESKTOP.RSC.  It is 
possible, by editing this file, to change the names by which the icons will
be searched for in the icon resource file(s).

9. When used  in an environment  which  is  supposed  to  support   colour  
icons, TeraDesk tries to load the colour  icon  file  CICONS.RSC.  If this 
file can  not  be  found,  TeraDesk falls  back  to monochrome icons  file 
ICONS.RSC.

Some versions  of AES  (e.g. Geneva 4)  declare  themselves  as capable  of 
handling colour icons, but that doesn't seem to work with TeraDesk. In such 
cases, CICONS.RSC  file should be removed,  and TeraDesk will fall  back to  
monochrome  icons.   Colour  icons  file  can  also  be  removed  in  other 
environments if there is a need to preserve as much free memory as possible.


Some Possible Future Developments 
=================================

- Optimization of code to reduce size and memory use
 
- Multi-column display of directory windows 

- Improved manipulation of file/folder attributes and access rights

- Complete compliance to AV-protocol and Drag & Drop protocol

- Improved handling of symbolic links

- Better handling of memory-limit and no-multitask options

- More functionality in Show/Edit/Cancel dialog/alert

- Integration of a non-modal, windowed, long-names-capable fileselector


Comments and Bug-Reports
========================

It would be appreciated, if problems are reported with a complete description 
of the problem  and the configuration  you are using  (machine,  TOS-version, 
autoboot programs, accessories etc.).  Mention  TeraDesk  in the subject line 
of your e-mail.

THE AUTHORS CAN NOT BE HELD RESPONSIBLE  for  any form of damage  caused by 
this program; the usage of all components of TeraDesk is at your own risk.

PLEASE read the manual before you use the program.  You will need ST-Guide
(not supplied with TeraDesk) for this.


For the time being, comments should be sent to vdjole@EUnet.yu

If you intend to use TeraDesk, please send an e-mail to the above address, 
I may at some time ask a question or two about TeraDesk's behaviour. 


                                            Djordje Vukovic
                                            Beograd; May 28th 2004


