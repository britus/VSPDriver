QT += core
QT += gui
QT += widgets
QT += network
QT += concurrent
QT += serialbus
QT += serialport
QT += xml

lessThan(QT_MAJOR_VERSION, 6) {
QT += macextras
}

# Generate framework bundle
TEMPLATE = lib

# By default the framework name
TARGET = VSPClientUI

CONFIG += c++20
CONFIG += sdk_no_version_check
CONFIG += debug_and_release
CONFIG += lrelease
CONFIG += embed_translations
CONFIG += embed_libraries
CONFIG += incremental
CONFIG += global_init_link_order
CONFIG += lib_version_first
CONFIG += lib_bundle
CONFIG += create_prl

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
DEFINES += VSP_TARGET_NAME=$$shell_quote('$$TARGET')
DEFINES	+= VSPCLIENT_LIBRARY

INCLUDEPATH += $$PWD/../VSPController
INCLUDEPATH += $$PWD/../VSPSetup

PRE_TARGETDEPS += $$PWD/../VSPController
PRE_TARGETDEPS += $$PWD/../VSPSetup

# for QT Creator and Visual Studio project.
QMAKE_PROJECT_NAME = $${TARGET}

QMAKE_MACOSX_DEPLOYMENT_TARGET = 12.2

## sources
include(driver/driver.pri)
include(model/model.pri)
include(serialio/serialio.pri)
include(themes/themes.pri)
include(session/session.pri)
include(ui/ui.pri)

SOURCES += \
	$$PWD/main.cpp

OBJECTIVE_SOURCES += \
	$$PWD/vspversion.mm

RESOURCES += \
	$$PWD/vspui.qrc

TRANSLATIONS += \
	$$PWD/vspui_en_US.ts \
	$$PWD/vspui_de_DE.ts

DISTFILES += \
	$$PWD/LICENSE \
	$$PWD/README.md \
	$$PWD/qt-bundle-bugfix.sh \
	$$PWD/VSPClient.entitlements \
	$$PWD/makelocales.sh

QMAKE_CFLAGS += -mmacosx-version-min=12.2
QMAKE_CXXFLAGS += -mmacosx-version-min=12.2
QMAKE_CXXFLAGS += -fno-omit-frame-pointer
QMAKE_CXXFLAGS += -funwind-tables

release {
	#QMAKE_LFLAGS += -s
}
debug {
	QMAKE_CXXFLAGS += -ggdb3
}

#otool -L
LIBS += -dead_strip
LIBS += -liconv

LIBS += -F../../VSPController/xcode/Release -framework VSPController
LIBS += -F../../VSPSetup/xcode/Release      -framework VSPSetup

LIBS += -F../../VSPController/build         -framework VSPController
LIBS += -F../../VSPSetup/build              -framework VSPSetup

# Update translation files
LOC_QM_FILES.depends = $$TRANSLATIONS
LOC_QM_FILES.files = $$TRANSLATIONS
LOC_QM_FILES.depends = FORCE
LOC_QM_FILES.commands = $$PWD/makelocales.sh $$shell_quote($$PWD)
LOC_QM_FILES.target = \
	$$PWD/vspui_en_US.qm \
	$$PWD/vspui_de_DE.qm
#message($${LOC_QM_FILES.files})
PRE_TARGETDEPS	    += $${LOC_QM_FILES.files}
QMAKE_EXTRA_TARGETS += LOC_QM_FILES   #'.NOTPARALLEL'

QMAKE_PRE_LINK = $$PWD/makelocales.sh $$shell_quote($$PWD)

QMAKE_FRAMEWORK_BUNDLE_NAME = $${TARGET}
QMAKE_FRAMEWORK_VERSION = A
QMAKE_BUNDLE_EXTENSION = .framework
#QMAKE_INFO_PLIST = $$PWD/Info.plist

# Important for the App with embedded framework
QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Frameworks/

LICENSE.version = Versions
LICENSE.files = $$PWD/LICENSE
LICENSE.path = Resources
QMAKE_BUNDLE_DATA += LICENSE

translations_en.version = Versions
translations_en.files = \
	$$PWD/assets/en.lproj/InfoPlist.strings \
	$$PWD/vspui_en_US.qm
translations_en.path = Resources/en.lproj
QMAKE_BUNDLE_DATA += translations_en

translations_de.version = Versions
translations_de.files = \
	$$PWD/assets/de.lproj/InfoPlist.strings \
	$$PWD/vspui_de_DE.qm
translations_de.path = Resources/de.lproj
QMAKE_BUNDLE_DATA += translations_de

icons.version = Versions
icons.files = \
	$$PWD/assets/icns/vspclient.icns \
	$$PWD/assets/png/vspclient_512x512.png
icons.path = Resources
QMAKE_BUNDLE_DATA += icons

#message("Build: $${TARGET}")
