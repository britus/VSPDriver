#!/bin/bash

PROJECT_DIR=`pwd`
ARCH=`uname -m`

echo ":> Build project: ${ARCH} ..."

sudo rm -fR build

XC_PROJECT=vsp.xcworkspace

if [ "${ARCH}" == "x86_64" ] ; then
    XC_PROJECT=VSPDriver_QT_5.15.2_x86_64.xcodeproj
fi

if [ "${ARCH}" == "arm64" ] ; then
    XC_PROJECT=VSPDriver_arm64.xcodeproj
fi

xcodebuild -arch `uname -m` -project ${XC_PROJECT} -target VSPClient
