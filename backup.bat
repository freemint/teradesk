echo Backup of TeraDesk 3 source files to a floppy disk
echo (-n = overwrite without confirmation)

format A: -dh -vteradesk

mkdir a:\include
mkdir a:\pure_c
mkdir a:\library
mkdir a:\library\ahcm
mkdir a:\library\mint
mkdir a:\library\multitos
mkdir a:\library\utility
mkdir a:\library\xdialog

e:
cd terdesk3.40


copy -n copying      a:\
copy -n readme.txt   a:\
copy -n cfg2inf.txt  a:\
copy -n hist_v3.txt  a:\
copy -n ann*.txt     a:\
copy -n teradesk.stg a:\
copy -n udo_fo.img   a:\

copy -n *.c         a:\
copy -n *.h         a:\
copy -n *.s         a:\
copy -n *.prj       a:\
copy -n desktop.rs* a:\
copy -n *.bat       a:\

copy -n .\include\*.h a:\include\*.h


copy -n .\library\ahcm\*.s    a:\library\ahcm\*.c
copy -n .\library\ahcm\*.s    a:\library\ahcm\*.h
copy -n .\library\ahcm\*.s    a:\library\ahcm\*.lib
copy -n .\library\ahcm\*.prj  a:\library\ahcm\*.prj

copy -n .\library\mint\*.c    a:\library\mint\*.c
copy -n .\library\mint\*.h    a:\library\mint\*.h 
copy -n .\library\mint\*.s    a:\library\mint\*.s
copy -n .\library\mint\*.prj  a:\library\mint\*.prj

copy -n .\library\multitos\*.c    a:\library\multitos\*.c
copy -n .\library\multitos\*.h    a:\library\multitos\*.h 
copy -n .\library\multitos\*.s    a:\library\multitos\*.s
copy -n .\library\multitos\*.prj  a:\library\multitos\*.prj

copy -n .\library\utility\*.c    a:\library\utility\*.c
copy -n .\library\utility\*.h    a:\library\utility\*.h 
copy -n .\library\utility\*.s    a:\library\utility\*.s
copy -n .\library\utility\*.prj  a:\library\utility\*.prj

copy -n .\library\xdialog\*.c    a:\library\xdialog\*.c
copy -n .\library\xdialog\*.h    a:\library\xdialog\*.h 
copy -n .\library\xdialog\*.prj  a:\library\xdialog\*.prj
copy -n .\library\xdialog\*.hlp  a:\library\xdialog\*.hlp

copy -n .\pure_c\*.s a:\pure_c\*.s

