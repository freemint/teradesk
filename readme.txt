Tera Desktop V1.41, 20-09-95, Copyright 1991, 1992, 1993, 1994, 1995 W. Klaren.
             V2.1   15-11-02, Copyright 2002  H. Robbers.
             V2.3           , Copyright 2003  H. Robbers, Dj. Vukovic
             V3.0   14-12-03, Copyright 2003  H. Robbers, Dj. Vukovic        



This is version 3.0 of the Tera Desktop. It is a replacement of the desktop 
of the ST and TT. This program is Freeware and Open Source, published under 
GPL license, which means that it may be copied and modified freely, providing 
that the original authorships are recognized where appropriate. See the 
included COPYING file for the details on GPL.
 
Tera Desktop may not be sold. If this program is included in a PD-library, 
only the costs of disks etc. may be charged.


Version History
===============

Version 1.41 was released in '94 and here the development stopped for some 
time.

Version 1.42 was the first one published (in the year 2002) under the GPL.

Version 2.0 was an adaptation by H. Robbers to modern AES's (like N.Aes, 
XaAES & MagiC); A single binary runs on ALL environments. However, a somewhat 
smaller binary compiled for Single-TOS only is included too, for the benefit 
of users with machines having minimum RAM.

Version 2.1 was mostly a bugfix update which followed some time later.

Version 2.3 was the first result of joined efforrts of H.Robbers and 
Dj.Vukovic, comprising a number of new features.

Version 3.0 is a significant further improvement of TeraDesk's functionality, 
the program having undergone a extensive redesign.

Future upgrades of Teradesk, IF they ever materialize, will be numbered as 
follows: TeraDesk VX.YZ, where "X" will increment with major revisions, "Y" 
will increment when new features are added, and "Z" will increment with bug 
fixes and minor optimizations. (Thus, TeraDesk V3.01 will NOT be same as 
TeraDesk V3.1; trailing zeros in version numbering might be omitted).


Package manifest:
=================

Tera Desktop binary distribution currently consists of the following files:

        COPYING         (A copy of the GPL license)
        DESKTOP.PRG	(All-environments Desktop)
	DESKTOS.PRG	(Somewhat smaller Desktop compiled for Single-TOS only)
        DESKTOP.RSC  	(English resource file)
	DESKTOP.RSD	(Resource definition file)
        ICONS.RSC    	(Essential Mono icons)
        CICONS.RSC   	(Essential Colour icons)
        README.TXT	(This file)
        HIST_V3.TXT	(Log of changes)
        TERADESK.HYP	(English manual in ST-Guide hypertext)
        TERADESK.REF    (Reference file for the hypertext)
        TERADESK.INF    (Sample configuration file)
        TERADESK.PAL    (Sample palette file)
        TERADESK.BAT    (sample startup file)


Usage of Icon files
===================

The icon files supplied have a minimum set of icons only. Users can add icons 
at will, or use other icon files (e.g. one can rename DESKICON.RSC used by 
the built-in desktop of TOS 2/3/4 to ICONS.RSC and use it with TeraDesk).

Teradesk, since V2.0, handles icons by name, not by index. Icons with the 
following names should always be present in the icons resource file:

FLOPPY, HARD DISC, TRASH, PRINTER, FOLDER, FILE, APP

If a required icon name cannot be found in the icons resource file, icon name 
FILE is tried. If that does not exist either, you loose an icon.

When used in an environment which is supposed to support colour icons, 
TeraDesk tries to load the colour icon file CICONS.RSC. If this file can not 
be found, TeraDesk falls back to monochrome icons file.

To facilitate adaptation of TeraDesk to other languages, the seven essential 
icon names are not hard-coded but are read from DESKTOP.RSC. It is possible, 
by editing this file, to change the names by which the icons will be searched 
for in the icon resource file(s).


Some Version 3- specific issues:
================================

An extensive log of changes made since V2.3 exists in the file HIST_V3.TXT; 
here follow some highlights on the new or enhanced features:

* TeraDesk now uses an ASCII (i.e. text) configuration file TERADESK.INF, and 
a separate file TERADESK.PAL for the saved colour palette. The change was 
made primarily to facilitate maintaining of compatibility between upgrades, 
not for the purpose of manual editing. Editing of TeraDesk's configuration 
files by hand is not encouraged.

* A conversion program CFG2INF.PRG (not included in the binary distribution) 
can be used to convert V2 *.CFG files to V3 *.INF files. This program is 
basically a hacked compilation of the Desktop itself, with the capability to 
read the old file format and to write the new format- hence a very large size 
of this utility. Use it to convert your old *.CFG files, then promptly 
dispatch it to the trash bin. Note that this utility uses the resource files 
of TeraDesk V3, so do not attempt to put CFG2INF.PRG in a directory 
containing an older version of DESKTOP.RSC. This utility will be supported by 
the source code only in TeraDesk Version 3.00

* The dialogs of TeraDesk underwent an extensive redesign, Beside being more 
functional, they look much better now, especially in newer "3D" AESses. 
Another result of the redesign is that now -all- dialogs work even in 320x200 
ST-low resolution. There has also been some changes in the handling of 
certain items in the dialogs, as will be seen below.

* TeraDesk maintains lists of filetype masks, window icon types, executable 
program types, installed applications and assigned filetypes. The same type 
of dialog, containing a scrollable listbox and five edit buttons is used to 
maintain all these lists.

* Items can be added (or edited) in all of these lists, except in the lists 
of document-types assigned to applications, in the same way: either you can 
manually enter all data related to an item, or you can select an item in a 
directory window or the desktop itself, and then open the desired dialog to 
modify the settings which TeraDesk assigned to an item added to a list in 
this way. Document-types asigned to appliactions can be edited only manually, 
being in the second-level of dialog (i.e. they can not be selected in a ' 
window).

* Dialog fields referring to file names will have a 8+3 form if TeraDesk is 
run in an environment which does not support long file names. In the opposite
case (i.e. if Mint or MagiC are detected), filename forms will be longer and 
scrollable, and permit entering of filenames up to 127 characters long.

* Pressing the [Insert] key while the cursor is in a scrolled-text field
opens the file-selector. Selected file or folder name is inserted into the 
already-existing text in the editable field at cursor position.

* If TeraDesk is to be used exclusively in high screen resolutions and 
long-file-names-capable environment, length of the scrollable filename fields 
can be increased in the dialogs. TeraDesk uses these fields in such a way 
that it is safe to use a resource editor and increase the widths of the 
relevant dialogs and scrollable fields, until they suit the user's esthetic 
and other requirements. Note that lengths of other alphanumeric editable 
fields (i.e. filetype masks and search strings) should -not- be enlarged.

* If you want to edit DESKTOP.RSC, beware that it contains a number of 
objects which are initially set to invisible.

* If [Shift][Help] keys are pressed, TeraDesk will attempt to call ST-Guide 
if it is installed, to open TERADESK.HYP. (currently, the .HYP file has not 
been completely updated to reflect all changes and new features og TeraDesk 3 
in a satisfactory way). Pressing [Help] only invokes a Help dialog with brief 
explanations of basic keyboard commands.
 
* Capability is added to search for files containing a specified string. Note 
that each file being searched for strings is first completely read into 
memory. Therefore, searching in large files may not be possible, depending on 
the amount of free memory. If a search string is not specified, search will 
involve only file and folder names, without actually reading any files.

* Capability is added to compare contents of files. It is enabled only when a 
selection of exactly one or two files is made in directory window(s). If only 
one fle is selected, the file selector is invoked to point to the second 
file. File comparison function tries to use some intelligence and 
resynchronize further search in the files after a difference is found. Width 
of the resynchronization window is currently fixed to 10 characters. This 
should be enough to continue comparison of a pair of files after a difference 
of only several bytes is encountered, e.g. a single different word, or a 
single blank line in one of the files. Same as the search function, this 
function completely reads both files into memory, and may be of limited use 
on machines with minimum RAM.

* Capability is added to limit memory available to an installed application 
in a multitasking environment, and also to activate an application as 
non-multitasking. This enables running of some older applications which 
failed in earlier version(s). However, the implementation of these options is 
currently only rudimentary and not nearly as good as desired. For the time 
being, memory limit works only on applications which have not been marked as 
no-multitasking.

* Capability is added to "touch" files, i.e. to change the date/time stamp of 
the file. A choice is possible of whether to (re)set date/time, (GEM)DOS file 
attributes, or both. This functionality is currently only available for 
files, i.e. it does not work on folders. However, TeraDesk is able to 
correctly display (DOS) folder attributes if they are set (e.g. on a floppy 
disk volume manipulated by DOS/Windows). Touch function is activated by 
clicking on the "All" button in the "Object Info" dialog. The operation is 
recursive- it affects files in all the subfolders of a selected folder. If 
date- or time field is cleared in "Object info", current date/time is set. If 
a file iw write-prottected, it must first be unprotected before other data is 
changed. An unwanted product of this is currently that, if a write-protected 
file has an illegal date or time (as can sometime happen), its attributes can 
not be changed in TeraDesk because date/time-check will not permit leaving 
the dialog, and so it will not be possible to remove protection.

* Capability is added to reset date/time stamp and file attributes while 
copying or moving files.

* Functionality of the "Open" dialog is enhanced. TeraDesk tries to divine 
the type of the item to be opened: a file, a folder, an application or a 
document assigned to an application. If an application is specified, its name 
can be followed by a command tail (after a blank character).

* If [Alt] key is pressed while a file is opened, the assignment of that file 
or filetype (i.e. as a program, or a documenttype assigned to an application) 
will be overriden and Show/Edit/Cancel dialog will appear.

* Previous version of TeraDesk handled filename conflicts during copying in 
such a way that it was possible to rename both the existing file and/or the 
new one (i.e. the one creating the conflict). This feature has been lost, 
because of a mistake made during code optimizartion, and now only the new 
file can be renamed (same as in the built-in TOS desktop). No time to change 
it now; maybe in the next upgrade...


TOS-versions Compatibility
==========================

* Although TeraDesk has been tested extensively, the program may still 
contain bugs. It is possible as well, that the program does not work properly 
on configurations other than the configurations it was tested on.

Initially TeraDesk was tested on:

TOS 1.4 + NVDI 2.0 
TOS 3.05 + NVDI 2.0 

H. Robbers: also tested with:

TOS 3.06 
Milan TOS 4.09 + MiNT 1.15.12 + N.Aes 1.2, XaAES.
MagiC 6.0 + NVDI 5.0 

Dj. Vukovic: also tested with: 

TOS 1.00 (RAM loaded)
TOS 1.04 (RAM loaded)
TOS 2.06
TOS 2.06 + NVDI 4.11
TOS 2.06 + Wdialog 2
TOS 2.06 + Geneva 4 & 6 + NVDI 4.11
TOS 2.06 + Mint 1.15.0  + NVDI 4.11 + AES 4.1, N.Aes 1.1
MagiC 6.10, 6.20 + NVDI 4.11

D.W. van der Burg: also tested with:

TOS 1.02
TOS 1.04
TOS 4.04 
TOS 1.02, 1.04, 2.06, 4.04 + Mint 1.15.12 + N.Aes 2.0, XaAES 0.963, AES 4.1 

* The program is supposed to work (with some restrictions) on TOS 1.0; 
however, the last version of TeraDesk has not been much tested with it. The 
usage of the program with versions of TOS older than TOS 1.4 is discouraged, 
because of some bugs in these TOSses (see the manual as well). Possibility of 
encountering hitherto undetected problems with old TOS versions is suspected.

* With older (i.e. TOS 1.04 and older) versions of TOS, a limitation exists 
on the size of menus, when in ST-Low resolution. This limitation is
especially serious in TOS 1.00 and 1.02. It manifested itself by program 
crash when using the menu or a dialog activated from the menu. This 
limitation is met by removing some menu items. First the  menu-separator-bars
are removed, and, if it turns not to be enough, some (hopefully) 
less-important menu items are removed. However, this limits the functionality 
ot TeraDesk amd a question can be raised whether it is worth the trouble to 
run TeraDesk at all in such case, because its functionality will reduce to 
one similar to that of the underlying TOS desktop.

* A problem was encountered related to mouse clicks on the desktop icons
when TeraDesk was running in TOS 2.06 (only). An attempt was made to cure 
this, but it is not sure yet whether it has been completely successful.

* A problem related to file selections in TOS 1.02 and TOS 1.04 has been
reported. Unfortunately, it was not possible to reproduce this problem 
after loading TOS 1.00 or 1.04 on the machine on which TeraDesk 3 
is being finalized, so it will remain unsolved until the next upgrade.

* Window de-iconification behaves in a somewhat weird way with XaAES 0.963
if iconified windows have been moved. This problem does not appear with
any other AES TeraDesk was tested with.

* TeraDesk makes several inquires trying to determine TOS- and AES-versions
and their capabilities and limitations. If some version of TOS or AES is not
able to answer these queries, TeraDesk tries to make guesses which may not
always be correct. It is also possible that incorrect answer to a query
is supplied by TOS/AES, in which case TeraDesk may work with unnecessary 
limitations or else try to activate features which may not work. A possible, 
currently untested, such case may be the environment consisting of modern 
Mint and AES over an old version of TOS.

* Change of screen resolution is implemented through a call of an AES
function only and works with various success in different AESses (generally 
supported with AES 4 versions only), e.g. in TOS + Geneva or TOS + Mint + AES4.1 
it works perfectly;  in some other environments it doesn't work so well, or 
doesn't work at all.

* Users of Atari AES 4.1 can put something similar to the following directive 
into GEM.CNF file:

     shell= path\to\TeraDesk\desktop.prg

Then, the built-in desktop of AES 4.1 will not be loaded at startup and
TeraDesk will run as the desktop instead. Beside being more elegant,
this also saves some memory.

* TeraDesk works fine with MagiC, at least with version 6.* of MagiC
(was not tested with older versions); it should be specified as a shell 
in #_SHL directive in MAGX.INF; more adventurous users can also attempt to 
edit MAGIC.RAM with a hex editor, and carefully replace instances of 
"MAGXDESK.APP" with "TERADESK.PRG", with a possible change of path, taking 
care not to exceed original path length. Then, in a case of a major crash, 
TeraDesk should be reactivated automatically, not MAGXDESK.APP.

* Geneva 4 declares itself as capable of handling colour icons. However,
this doesn't seem to work with TeraDesk. In this case, CICONS.RSC file 
should be removed, and TeraDesk will fall back to monochrome icons.


Some Possible Future Developments
=================================

- Optimization of code to reduce size and memory use;
 
- Multiple column display of directory windows (with the capability to
change directory font, it may be useful even on lower-resolution displays);

- Improvement on manipulation of file and folder attributes (depending on 
filesystem, etc);

- Better handling of memory limit and no-multitask flags;

- Integration of a windowed, long-names-capable fileselector (existing
fileselectors with that capability are generally larger than complete
Tera Desktop!) - as a number of routines already exising in TeraDesk
might be reused, it is estimated that the feature might be implemented
at the expense of not more than a few kilobytes;


Comments and Bug-Reports
========================

It would be appreciated, if problems are reported with a complete description 
of the problem and the configuration you are using (so TOS-version, autoboot 
programs, accessories etc.).  Please mention TeraDesk in the subject line of 
your e-mail.

The authors can not be held responsible for any form of damage caused by 
this program, the usage of this program is at your own risk.

Read the manual before you install the program, but note that it has not
been completely updated (yet) to reflect all changes and new features in 
TeraDesk V3.


For the time being, comments should currently be sent to vdjole@EUnet.yu


                                            Djordje Vukovic
                                            Beograd; December 23rd 2003


