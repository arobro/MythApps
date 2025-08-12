include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )

!exists(../../settings.pro) {
    message("WARNING: Using include fallback")

    exists(/usr/local/include/mythtv/) {
        INCLUDEPATH += /usr/local/include/mythtv/
        LIBS += -L/usr/local/lib/
    } else {
        exists(/usr/include/mythtv/) {
            INCLUDEPATH += /usr/include/mythtv/
            LIBS += -L/usr/lib/
        } else {
            error("Include path not found")
        }
    }
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
HEADERS += mythappsdbcheck.h container.h libs/xbmcclient.h browser.h fileBrowserHistory.h NetSocketRequest.cpp
HEADERS += plugins/plugin_manager.h plugins/plugin_api.h netSocketRequest.h dialog.h 
HEADERS += plugins/favourites.h plugins/videos.h plugins/watchlist.h plugins/ytCustom.h

SOURCES += main.cpp mythapps.cpp mythsettings.cpp mythinput.cpp imageThread.cpp music_functions.cpp searchSuggestions.cpp netRequest.cpp 
SOURCES += shared.cpp controls.cpp programData.cpp programLink.cpp mythappsdbcheck.cpp browser.cpp fileBrowserHistory.cpp
SOURCES += plugins/plugin_manager.cpp netSocketRequest.cpp mythappsCleanup.cpp dialog.cpp
SOURCES += plugins/favourites.cpp plugins/videos.cpp plugins/watchlist.cpp plugins/ytCustom.cpp

DEFINES += MPLUGIN_API

use_hidesyms {
    QMAKE_CXXFLAGS += -fvisibility=hidden
}

android {
    # to discriminate plugins in a flat directory structure
    TARGET = mythplugin$${TARGET}
}

include ( ../../libs-targetfix.pro )
