include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )

!exists( ../../settings.pro ) {
    INCLUDEPATH += /usr/include/mythtv/
    LIBDIR = /usr/lib
}

QT += network sql xml widgets websockets

TEMPLATE = lib
CONFIG += plugin thread warn_on debug
TARGET = mythapps
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target

installfiles.path = $${PREFIX}/share/mythtv/mythapps
installfiles.files = mythapps-ui.xml

INSTALLS += installfiles

# Input
HEADERS += mythapps.h mythsettings.h mythinput.h imageThread.h searchSuggestions.h shared.h controls.h programData.h programLink.h netRequest.h
HEADERS += mythappsdbcheck.h container.h libs/xbmcclient.h ytCustomApp.h browser.h

SOURCES += main.cpp mythapps.cpp mythsettings.cpp mythinput.cpp imageThread.cpp music_functions.cpp searchSuggestions.cpp netRequest.cpp 
SOURCES += shared.cpp controls.cpp programData.cpp programLink.cpp mythappsdbcheck.cpp ytCustomApp.cpp browser.cpp

DEFINES += MPLUGIN_API

use_hidesyms {
    QMAKE_CXXFLAGS += -fvisibility=hidden
}

android {
    # to discriminate plugins in a flat directory structure
    TARGET = mythplugin$${TARGET}
}

include ( ../../libs-targetfix.pro )
