#!/bin/sh

UPLOAD_DIR=web196@server43.webgo24.de:/home/www/snapshots

ARCHIVE_PATH="${DEPLOY_DIR}/${ARCHIVE_NAME}"

eval "$(ssh-agent -s)"

PROJECT_DIR="$PROJECT_NAME"
case $PROJECT_DIR in
	m68k-atari-mint-gcc) PROJECT_DIR=gcc ;;
	m68k-atari-mint-binutils-gdb) PROJECT_DIR=binutils ;;
esac

scp -o "StrictHostKeyChecking no" "$ARCHIVE_PATH" "${UPLOAD_DIR}/${PROJECT_DIR}/${ARCHIVE_NAME}"
if test -z "${CPU_TARGET}"
then
	scp -o "StrictHostKeyChecking no" "$ARCHIVE_PATH" "${UPLOAD_DIR}/${PROJECT_DIR}/${PROJECT_DIR}-latest.${DEPLOY_ARCHIVE}"
fi
