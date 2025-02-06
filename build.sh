#!/bin/bash

prjroot=`pwd`

# create build directory with ORSSerial module.map
if [ ! -d ${prjroot}/build/GeneratedModuleMaps ]; then
    mkdir -p ${prjroot}/build/GeneratedModuleMaps
fi
 
cd ${prjroot}/build/GeneratedModuleMaps

basedir="${HOME}/Library/Developer/Xcode/DerivedData"
orspath="/SourcePackages/checkouts/ORSSerialPort/build/GeneratedModuleMaps"
prjname=`basename ${basedir}/VSPDriver-*`

echo ":> ${basedir}"
echo ":> ${prjname}"
mappath="${basedir}/${prjname}${orspath}"
ln -fs ${mappath}/ORSSerial.modulemap
echo ":>"

cd ${prjroot}

#-arch x86_64
xcodebuild -project VSPDriver.xcodeproj \
	-target VSPDriver \
	-target VSPInstall
