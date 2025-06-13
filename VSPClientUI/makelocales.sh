#!/bin/bash

ARCH=`uname -m`

SOURCE_DIR=${1}

if [ "${ARCH}" == "x86_64" ] ; then
	echo "++++ Generate .qm files from translations"
	QTVER="5.15.2"
	QTDIR=~/Qt/${QTVER}/clang_64
	TOOL_DIR=${QTDIR}/bin
	${TOOL_DIR}/lprodump ${SOURCE_DIR}/VSPClientUI.pro  -out  ${SOURCE_DIR}/VSPClientUI.json
	${TOOL_DIR}/lrelease -removeidentical -compress -project  ${SOURCE_DIR}/VSPClientUI.json
fi

if [ "${ARCH}" == "arm64" ] ; then
	echo "++++ Generate .qm files from translations"
	QTVER="6.9.1"
	QTDIR=~/Qt/${QTVER}/macos
	TOOL_DIR=${QTDIR}/libexec
	${TOOL_DIR}/lprodump ${SOURCE_DIR}/VSPClientUI.pro  -out  ${SOURCE_DIR}/VSPClientUI.json
	${QTDIR}/bin/lrelease -removeidentical -compress -project  ${SOURCE_DIR}/VSPClientUI.json
fi


