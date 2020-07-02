#!/bin/sh

echo rvm_autoupdate_flag=0 >> ~/.rvmrc

# needed for mtools
sudo apt-get update

sudo apt-get install -y \
	curl \
	mtools \
	libjson-perl \
	libwww-perl \

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
curl --get https://tho-otto.de/download/aranym-1.0.2-trusty-x86_64-a9de1ec.tar.xz --output aranym.tar.xz

tar xvf mag-hdd.tar.bz2
test -f config-hdd || exit 1

tar xvf aranym.tar.xz

image=hdd.img
offset=0

echo "drive c: file=\"$PWD/$image\" MTOOLS_SKIP_CHECK=1 MTOOLS_LOWER_CASE=1 MTOOLS_NO_VFAT=1 offset=$offset" > ~/.mtoolsrc

)

