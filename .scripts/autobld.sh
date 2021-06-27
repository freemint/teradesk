#!mupfel

#
# a mupfel script, executed inside ARAnyM
#

set -e

cd C:\src\teradesk
echo "%033v%c"

rm -f pcerr.txt
pcmake -B -F desktop.prj >> pcerr.txt
pcmake -B -F desktos.prj >> pcerr.txt

set +e
