include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )

QT += sql network widgets websockets

DEFINES += UNICODE

TEMPLATE = app
CONFIG += thread
TARGET = mythpluginloader
target.path = $${PREFIX}/bin
INSTALLS = target

QMAKE_CLEAN += $(TARGET)

HEADERS += commandlineparser.h
SOURCES += main.cpp commandlineparser.cpp 

HEADERS += ../mythapps/mythapps.h ../mythapps/mythsettings.h ../mythapps/mythosd.h ../mythapps/mythinput.h 
HEADERS += ../mythapps/imageThread.h ../mythapps/searchSuggestions.h ../mythapps/getApps.h ../mythapps/shared.h 
HEADERS += ../mythapps/controls.h ../mythapps/programData.h ../mythapps/programLink.h ../mythapps/netRequest.h

SOURCES += ../mythapps/mythapps.cpp ../mythapps/mythsettings.cpp ../mythapps/mythosd.cpp ../mythapps/mythinput.cpp 
SOURCES += ../mythapps/imageThread.cpp ../mythapps/searchSuggestions.cpp ../mythapps/getApps.cpp
SOURCES += ../mythapps/shared.cpp ../mythapps/controls.cpp
SOURCES += ../mythapps/programData.cpp ../mythapps/programLink.cpp ../mythapps/netRequest.cpp

using_x11:DEFINES += USING_X11
