#!/bin/bash

if [ "x" == "x${1}" ] ; then
	echo "1: Target name expected."
	exit 1
fi
if [ "x" == "x${2}" ] ; then
	echo "2: Target type expected. Use app or framework"
	exit 1
fi
if [ "x" == "x${3}" ] ; then
	echo "2: Framework name expected."
	exit 1
fi

OUT_PWD=`pwd`
TARGET=${1}.${2}
FRAMEWORK=${3}
FMWKPATH=${OUT_PWD}/${TARGET}/Contents/Frameworks/${FRAMEWORK}.framework

echo "Link: ${FRAMEWORK} into ${TARGET} framework path"


cd ${FMWKPATH}

if [ -e ${FRAMEWORK} ]; then
	echo "${FRAMEWORK} link exist, OK."
	exit 0
fi

if [ ! -e ${FMWKPATH}/Versions/Current/${FRAMEWORK} ] ; then
	echo "Invalid QT build! Missing file ${FMWKPATH}/Versions/Current/${FRAMEWORK}."
	exit 1
fi

ln -vsf Versions/Current/${FRAMEWORK} ${FRAMEWORK}

if [ ! -L ${FRAMEWORK} ]; then
	echo "Bundle QT-BUGIFX ${TARGET}: ${FRAMEWORK} failed."
	exit 1
fi

echo "${FRAMEWORK} QT-BUNDLE BUGFIX OKAY!"
