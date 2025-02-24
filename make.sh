#!/bin/bash

prjroot=`pwd`
prjname="VSPDriver"

basedir="${HOME}/Library/Developer/Xcode/DerivedData"
prjpath=`basename ${basedir}/${prjname}-*`

echo ":> ${basedir}"
echo ":> ${prjpath}"

echo ":> Build project..."

xcodebuild -arch `uname -m` -project ${prjname}.xcodeproj \
    -target VSPDriver \
    -target VSPClient
