include(../../mythconfig.mak)
include(../../settings.pro)
include(../../programs-libs.pro)

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

# Input
HEADERS += \
    mythapps.h mythsettings.h mythinput.h imageThread.h searchSuggestions.h shared.h controls.h programData.h programLink.h netRequest.h \
    mythappsdbcheck.h container.h libs/xbmcclient.h browser.h fileBrowserHistory.h NetSocketRequest.cpp \
    plugins/plugin_manager.h plugins/plugin_api.h netSocketRequest.h dialog.h FunctionLogger.h \ 
    plugins/favourites.h plugins/videos.h plugins/watchlist.h plugins/ytCustom.h plugins/music.h \
    plugins/music/IMediaSource.h plugins/music/KodiMediaSource.h

SOURCES += \
    main.cpp mythapps.cpp mythsettings.cpp mythinput.cpp imageThread.cpp searchSuggestions.cpp netRequest.cpp \
    shared.cpp controls.cpp programData.cpp programLink.cpp mythappsdbcheck.cpp browser.cpp fileBrowserHistory.cpp \
    plugins/plugin_manager.cpp netSocketRequest.cpp mythappsCleanup.cpp dialog.cpp FunctionLogger.cpp \ 
    plugins/favourites.cpp plugins/videos.cpp plugins/watchlist.cpp plugins/ytCustom.cpp plugins/music.cpp \ 
    plugins/music/KodiMediaSource.cpp

INCLUDEPATH += plugins

MOC_DIR = moc
OBJECTS_DIR = obj

DEFINES += MPLUGIN_API

use_hidesyms {
    QMAKE_CXXFLAGS += -fvisibility=hidden
}

android {
    TARGET = mythplugin$${TARGET}
}

include(../../libs-targetfix.pro)

QMAKE_CLEAN += Makefile libmythapps.so
