#!/bin/sh

#
# This script uses a pre-build image for ARAnyM,
# where Pure-C, GEMINI and a few other tools are installed,
# a custom pcmake tool to build a Pure-C project file from the
# command line, and mtools to access the image
#
export BUILDROOT="${PWD}/.scripts/tmp"

mkdir -p "${BUILDROOT}"
mkdir -p "${DEPLOY_DIR}"

ARANYM="${HOME}/.aranym"
aranym="$ARANYM/usr/bin/aranym-jit"

unset CC CXX

SRCDIR=c:/src/teradesk

clash_option="-D s"
mmd $clash_option $SRCDIR
# get some files out of the way that we don't want to copy to the harddisk image
mkdir -p ../mcopytmp
mv .git .gitattributes .gitignore ../mcopytmp
mcopy $clash_option -bsov . $SRCDIR
mv ../mcopytmp/.git ../mcopytmp/.gitattributes ../mcopytmp/.gitignore .
rmdir ../mcopytmp

echo "starting build"

export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

mdel C:/status.txt
mcopy -o .scripts/autobld.sh C:/autobld.sh
"$aranym" -c "${ARANYM}/config-hdd"
echo ""
echo "##################################"
echo "error output from emulator run:"
echo "##################################"
mtype C:/errors.txt | grep -v "entering directory" | grep -v ": processing "
mtype "$SRCDIR/pcerr.txt" | grep -F "Error 
Fatal 
Warning "
echo ""
status=`mtype -t C:/status.txt`
test "$status" != "0" && exit 1

make clean
make CPU=v4e
mv desktop.prg desk_cf.prg || exit 1

make -C doc

. .scripts/mkbindist.sh


isrelease=false
export isrelease

ATAG=${PROJECT_VERSION}
ARCHIVE_NAME="${PROJECT_NAME}-${ATAG}-bin.zip"

export ARCHIVE_NAME
echo "ARCHIVE_NAME=$ARCHIVE_NAME" >> $GITHUB_ENV

(
cd "${BUILDROOT}"
zip -r "${DEPLOY_DIR}/${ARCHIVE_NAME}" .
)
ls -l "${DEPLOY_DIR}"
