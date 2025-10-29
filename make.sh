#!/bin/bash

PROJECT_DIR=`pwd`
ARCH=`uname -m`

echo ":> Build project: ${ARCH} ..."

sudo rm -fR build

XC_PROJECT=VSPDriver.xcodeproj

xcodebuild -arch ${ARCH} -project ${XC_PROJECT} -target VSPClient

