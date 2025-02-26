#!/bin/bash

PROJECT_DIR=`pwd`

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

prjroot=`pwd`
prjname="VSPDriver"

basedir="${HOME}/Library/Developer/Xcode/DerivedData"
prjpath=`basename ${basedir}/${prjname}-*`

echo ":> ${basedir}"
echo ":> ${prjpath}"

echo ":> Build project..."

rm -fR build

xcodebuild -arch `uname -m` -project ${prjname}.xcodeproj \
    -target VSPDriver \
    -target VSPClient
