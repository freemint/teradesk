echo Packing archives of TeraDesk 3.*

del e:\tera3r\tera%1b.zip
del e:\tera3r\tera%1s.zip

del e:\tera3s\library\ahcm\ahcm.c
del e:\tera3s\library\ahcm\ahcm.h

cd e:\tera3s
d:\archivers\stzip\zipjr.ttp -ar e:\tera3r\tera%1s.zip  *.*  

cd e:\tera3b
d:\archivers\stzip\zipjr.ttp -ar e:\tera3r\tera%1b.zip  *.*  

cd e:\teradesk