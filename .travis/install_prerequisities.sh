#!/bin/sh

echo rvm_autoupdate_flag=0 >> ~/.rvmrc

# needed for mtools
sudo apt-get update

sudo apt-get install -y \
	curl \
	mtools \
	libjson-perl \
	libwww-perl \
	libsdl1.2-dev \

if [ -z "$BINTRAY_API_KEY" ]
then
CURL_USER=""
else
BINTRAY_USER="${BINTRAY_USER:-th-otto}"
CURL_USER="-u ${BINTRAY_USER}:${BINTRAY_API_KEY}"
fi

sudo mkdir -p "/usr/m68k-atari-mint"
cd "/usr/m68k-atari-mint"
for package in mintlib fdlibm gemlib
do
	unset PACKAGE_VERSION
	unset PACKAGE_PATH
	PACKAGE_VERSION=$(curl -s ${CURL_USER} -X GET "https://api.bintray.com/packages/freemint/freemint/$package" | jq -r '.latest_version')
	echo PACKAGE_VERSION=$PACKAGE_VERSION
	PACKAGE_PATH=`curl -s ${CURL_USER} -X GET "https://api.bintray.com/packages/freemint/freemint/$package/versions/$PACKAGE_VERSION/files" \
			| jq -r '.[].path'`
	echo fetching https://dl.bintray.com/freemint/freemint/$PACKAGE_PATH
	wget -q -O - "https://dl.bintray.com/freemint/freemint/$PACKAGE_PATH" | sudo tar xjf -
done
cd -


mkdir -p ~/tmp/udo
(
cd ~/tmp/udo
curl --get "https://www.tho-otto.de/download/udo-7.12-linux.tar.bz2" --output udo.tar.bz2
tar xjf udo.tar.bz2
)

(
cd ~/tmp
curl --get "https://tho-otto.de/download/hcp-1.0.3-linux.tar.bz2" --output hcp.tar.bz2
tar xjf hcp.tar.bz2
mv hcp-1.0.3 hcp
)

ARANYM="${HOME}/.aranym"
(
mkdir "$ARANYM"
cd "$ARANYM"

curl --get https://tho-otto.de/download/mag-hdd.tar.bz2 --output mag-hdd.tar.bz2
#curl --get https://tho-otto.de/download/aranym-1.0.2-trusty-x86_64-a9de1ec.tar.xz --output aranym.tar.xz
curl --get https://tho-otto.de/download/aranym-1.1.0-xenial-x86_64-4ebb186.tar.xz --output aranym.tar.xz

tar xvf mag-hdd.tar.bz2
test -f config-hdd || exit 1

tar xvf aranym.tar.xz

image=hdd.img
offset=0

echo "drive c: file=\"$PWD/$image\" MTOOLS_SKIP_CHECK=1 MTOOLS_LOWER_CASE=1 MTOOLS_NO_VFAT=1 offset=$offset" > ~/.mtoolsrc

)

