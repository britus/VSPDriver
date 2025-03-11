#!/bin/bash

ARCH=`uname -m`

SOURCE_DIR=${1}

if [ "${ARCH}" == "x86_64" ] ; then
	echo "++++ Generate .qm files from translations"
	QTDIR=~/Qt/5.15.2/clang_64
	TOOL_DIR=${QTDIR}/bin
	${TOOL_DIR}/lprodump ${SOURCE_DIR}/VSPClientUI.pro  -out  ${SOURCE_DIR}/VSPClientUI.json
	${TOOL_DIR}/lrelease -removeidentical -compress -project  ${SOURCE_DIR}/VSPClientUI.json
fi

if [ "${ARCH}" == "arm64" ] ; then
	echo "++++ Generate .qm files from translations"
	QTDIR=~/Qt/6.8.2/macos
	${QTDIR}/libexec/lprodump ${SOURCE_DIR}/VSPClientUI.pro  -out  ${SOURCE_DIR}/VSPClientUI.json
	${QTDIR}/bin/lrelease -removeidentical -compress -project  ${SOURCE_DIR}/VSPClientUI.json
fi


