#!/bin/bash
#-arch x86_64

if [ ! -d ${HOME}/EoF/tests/VSPDriver/build/GeneratedModuleMaps ]; then
    mkdir -p ${HOME}/EoF/tests/VSPDriver/build/GeneratedModuleMaps
fi
 
cd ${HOME}/EoF/tests/VSPDriver/build/GeneratedModuleMaps

prjmame="VSPDriver-fpldlsylchvopmfkgecwnfowmmsz"
basedir="${HOME}/Library/Developer/Xcode/DerivedData"
orspath="/SourcePackages/checkouts/ORSSerialPort/build/GeneratedModuleMaps"
mappath="${basedir}/${prjmame}/${orspath}"
echo ":> ${prjmame}
ln -fs ${mappath}/ORSSerial.modulemap
echo ":>"
"
cd ${HOME}/EoF/tests/VSPDriver
xcodebuild -project VSPDriver.xcodeproj \
	-target VSPDriver \
	-target VSPInstall
