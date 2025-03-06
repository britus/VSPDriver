#!/bin/bash

PROJECT_DIR=`pwd`
ARCH=`uname -m`

if [ -d ${PROJECT_DIR}/../VSPClient ] ; then
    echo "** PRE-BUILD VSPClient Frameworks"
    mkdir -vp ${PROJECT_DIR}/QT
    cd ${PROJECT_DIR}/../VSPClient
    rm -fR build && ./make.sh ${PROJECT_DIR}/QT
    cd ${PROJECT_DIR}
    echo "** PRE-BUILD END"
fi

echo ":> Build project: ${ARCH} ..."

sudo rm -fR build

if [ "${ARCH}" == "arm64" ] ; then
	xcodebuild -arch `uname -m` -project VSPDriver.xcodeproj \
    	-target VSPDriver \
    	-target VSPClient
fi

if [ "${ARCH}" == "x86_64" ] ; then
    xcodebuild -arch `uname -m` -project VSPDriver_QT_5.15.2_x86_64.xcodeproj \
        -target VSPDriver \
        -target VSPClient
fi
