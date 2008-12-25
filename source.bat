echo Copying source files of TeraDesk 3.*                                
mkdir e:\tera3s\include
mkdir e:\tera3s\pure_c
mkdir e:\tera3s\library
mkdir e:\tera3s\library\ahcm
mkdir e:\tera3s\library\mint
mkdir e:\tera3s\library\multitos
mkdir e:\tera3s\library\utility
mkdir e:\tera3s\library\xdialog

cd e:\teradesk

copy -n copying      e:\tera3s\
copy -n readme.txt   e:\tera3s\
copy -n resource.txt   e:\tera3s\
copy -n cfg2inf.txt  e:\tera3s\
copy -n hist_v34.txt  e:\tera3s\
copy -n ann*.txt     e:\tera3s\
copy -n teradesk.stg e:\tera3s\
copy -n udo_fo.img   e:\tera3s\

copy -n *.c         e:\tera3s\
copy -n *.h         e:\tera3s\
copy -n *.s         e:\tera3s\
copy -n *.prj       e:\tera3s\
copy -n desktop.rs* e:\tera3s\
copy -n *.bat       e:\tera3s\

copy -n .\newicons\icons.rsc  e:\tera3s\
copy -n .\newicons\cicons.rsc e:\tera3s\

copy -n .\include\*.h e:\tera3s\include\*.h

copy -n .\library\ahcm\*.c    e:\tera3s\library\ahcm\*.c
copy -n .\library\ahcm\*.h    e:\tera3s\library\ahcm\*.h 
copy -n .\library\ahcm\*.lib  e:\tera3s\library\ahcm\*.lib
copy -n .\library\ahcm\*.prj  e:\tera3s\library\ahcm\*.prj


copy -n .\library\mint\*.c    e:\tera3s\library\mint\*.c
copy -n .\library\mint\*.h    e:\tera3s\library\mint\*.h 
copy -n .\library\mint\*.s    e:\tera3s\library\mint\*.s
copy -n .\library\mint\*.prj  e:\tera3s\library\mint\*.prj

copy -n .\library\multitos\*.c    e:\tera3s\library\multitos\*.c
copy -n .\library\multitos\*.h    e:\tera3s\library\multitos\*.h 
copy -n .\library\multitos\*.s    e:\tera3s\library\multitos\*.s
copy -n .\library\multitos\*.prj  e:\tera3s\library\multitos\*.prj

copy -n .\library\utility\*.c    e:\tera3s\library\utility\*.c
copy -n .\library\utility\*.h    e:\tera3s\library\utility\*.h 
copy -n .\library\utility\*.s    e:\tera3s\library\utility\*.s
copy -n .\library\utility\*.prj  e:\tera3s\library\utility\*.prj

copy -n .\library\xdialog\*.c    e:\tera3s\library\xdialog\*.c
copy -n .\library\xdialog\*.h    e:\tera3s\library\xdialog\*.h 
copy -n .\library\xdialog\*.prj  e:\tera3s\library\xdialog\*.prj
copy -n .\library\xdialog\*.hlp  e:\tera3s\library\xdialog\*.hlp

copy -n .\pure_c\*.s e:\tera3s\pure_c\*.s

cd e:\teradesk