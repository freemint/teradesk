#!/bin/sh -x

UPLOAD_DIR=web196@server43.webgo24.de:/home/www/snapshots

ARCHIVE_PATH="${DEPLOY_DIR}/${ARCHIVE_NAME}"

eval "$(ssh-agent -s)"

PROJECT_DIR="$PROJECT_NAME"
case $PROJECT_DIR in
	m68k-atari-mint-gcc) PROJECT_DIR=gcc ;;
	m68k-atari-mint-binutils-gdb) PROJECT_DIR=binutils ;;
esac

upload_file() {
	local from="$1"
	local to="$2"
	for i in 1 2 3
	do
		scp -o "StrictHostKeyChecking no" "$from" "$to"
		[ $? = 0 ] && return 0
		sleep 1
	done
	exit 1
}

upload_file "$ARCHIVE_PATH" "${UPLOAD_DIR}/${PROJECT_DIR}/${ARCHIVE_NAME}"
if test -z "${CPU_TARGET}"
then
	upload_file "$ARCHIVE_PATH" "${UPLOAD_DIR}/${PROJECT_DIR}/${PROJECT_DIR}-latest.${DEPLOY_ARCHIVE}"
fi

echo ${ARCHIVE_NAME} > .latest_version
upload_file .latest_version "${UPLOAD_DIR}/${PROJECT_DIR}/.latest_version"
