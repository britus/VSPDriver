#!/bin/bash

prjroot=`pwd`
prjname="VSPDriver"

# create build directory with ORSSerial modulemap
if [ ! -d ${prjroot}/build/GeneratedModuleMaps ]; then
    mkdir -p ${prjroot}/build/GeneratedModuleMaps
fi
 
cd ${prjroot}/build/GeneratedModuleMaps

basedir="${HOME}/Library/Developer/Xcode/DerivedData"
orspath="/SourcePackages/checkouts/ORSSerialPort/build/GeneratedModuleMaps"
prjpath=`basename ${basedir}/${prjname}-*`

echo ":> ${basedir}"
echo ":> ${prjpath}"
mappath="${basedir}/${prjpath}${orspath}"
ln -fs ${mappath}/ORSSerial.modulemap

cd ${prjroot}

echo ":> Build project..."

#-arch x86_64
xcodebuild -project ${prjname}.xcodeproj \
    -target VSPDriver \
    -target VSPController \
    -target VSPSetup \
    -target VSPInstall
