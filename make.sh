#!/bin/bash

PROJECT_DIR=`pwd`
ARCH=`uname -m`

if [ -d ${PROJECT_DIR}/../VSPClient ] ; then
    echo "** PRE-BUILD VSPClient Frameworks"
    test -d ${PROJECT_DIR}/QT && \
        rm -fR ${PROJECT_DIR}/QT
    mkdir -vp ${PROJECT_DIR}/QT
    cd ${PROJECT_DIR}/../VSPClient
    ./make.sh ${PROJECT_DIR}/QT
    cd ${PROJECT_DIR}
    echo "** PRE-BUILD END"
fi

echo ":> Build project: ${ARCH} ..."

rm -fR build

if [ "${ARCH}" == "arm64" ] ; then
	xcodebuild -arch `uname -m` -project VSPDriver_QT_6.8.2_arm64.xcodeproj \
    	-target VSPDriver \
    	-target VSPClient
fi

if [ "${ARCH}" == "x86_64" ] ; then
    xcodebuild -arch `uname -m` -project VSPDriver_QT_5.15.2_x86_64.xcodeproj \
        -target VSPDriver \
        -target VSPClient
fi
