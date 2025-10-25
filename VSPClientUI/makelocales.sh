#!/bin/bash

ARCH=`uname -m`

SOURCE_DIR=${1}

echo "++++ Generate .qm files from translations"
QTVER="6.10.0"
QTDIR=~/Qt/${QTVER}/macos
TOOL_DIR=${QTDIR}/libexec
${TOOL_DIR}/lprodump ${SOURCE_DIR}/VSPClientUI.pro  -out  ${SOURCE_DIR}/VSPClientUI.json
${QTDIR}/bin/lrelease -removeidentical -compress -project  ${SOURCE_DIR}/VSPClientUI.json
