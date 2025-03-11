#!/bin/bash
QTDIR=~/Qt/5.15.2/clang_64
TOOL_DIR=${QTDIR}/bin
SOURCE_DIR=${1}

echo "++++ Generate .qm files from translations"
${TOOL_DIR}/lprodump ${SOURCE_DIR}/VSPClientUI.pro  -out  ${SOURCE_DIR}/VSPClientUI.json
${TOOL_DIR}/lrelease -removeidentical -compress -project  ${SOURCE_DIR}/VSPClientUI.json
