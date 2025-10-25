TEMPLATE = lib

TARGET = VSPSetup

CONFIG -= qt
CONFIG += c++20
#CONFIG += sdk_no_version_check
CONFIG += debug_and_release
CONFIG += lrelease
CONFIG += embed_translations
CONFIG += embed_libraries
CONFIG += incremental
CONFIG += global_init_link_order
CONFIG += lib_version_first
CONFIG += lib_bundle
CONFIG += create_prl
CONFIG += objective_c

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
DEFINES += VSPSETUP_LIBRARY

QMAKE_PROJECT_NAME = $${TARGET}

QMAKE_MAC_SDK = macosx26.0
QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.5

QMAKE_CFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET
QMAKE_CXXFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET
QMAKE_CXXFLAGS += -fno-omit-frame-pointer
QMAKE_CXXFLAGS += -funwind-tables

debug {
    QMAKE_CXXFLAGS += -ggdb3
}

#/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks
#otool -L
LIBS += -dead_strip
LIBS += -framework IOKit
LIBS += -framework CoreFoundation
LIBS += -framework Foundation
LIBS += -framework SystemExtensions
LIBS += -framework SystemConfiguration
LIBS += -liconv

QMAKE_FRAMEWORK_BUNDLE_NAME = $${TARGET}
QMAKE_FRAMEWORK_VERSION = A
QMAKE_BUNDLE_EXTENSION = .framework

# Important for the App with embedded framework
QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Frameworks/

FRAMEWORK_HEADERS.version = Versions
FRAMEWORK_HEADERS.files = $${OBJECTIVE_HEADERS}
FRAMEWORK_HEADERS.path = Headers
QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS

LICENSE.version = Versions
LICENSE.files = $$PWD/LICENSE
LICENSE.path = Resources
QMAKE_BUNDLE_DATA += LICENSE

icons.version = Versions
icons.files = $$PWD/vspsetup.icns
icons.path = Resources
QMAKE_BUNDLE_DATA += icons

INCLUDEPATH += $$PWD

OBJECTIVE_SOURCES += $$PWD/vsploadermodel.m
OBJECTIVE_SOURCES += $$PWD/vspsmloader.m
OBJECTIVE_SOURCES += $$PWD/vspdriversetup.mm

OBJECTIVE_HEADERS += $$PWD/vspsetup_global.h
OBJECTIVE_HEADERS += $$PWD/vspdriversetup.hpp
OBJECTIVE_HEADERS += $$PWD/vsploadermodel.h
OBJECTIVE_HEADERS += $$PWD/vspsmloader.h
OBJECTIVE_HEADERS += $$PWD/vspsetup.h

HEADERS += $$PWD/vspsetup_global.h

DISTFILES += LICENSE

#############################
#message("Build: $${TARGET}")
