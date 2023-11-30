include(config.pri)

TEMPLATE      = app
TARGET        = coppeliaSim
CONFIG        += console debug_and_release
CONFIG        += app_bundle
CONFIG        += WITH_QT
QT            -= gui

DEFINES += QT_FRAMEWORK

*-msvc* {
    QMAKE_CXXFLAGS += /std:c++17
    QMAKE_CXXFLAGS += -O2
    QMAKE_CXXFLAGS += -W3
} else {

    CONFIG(debug,debug|release) {
        QMAKE_CXXFLAGS += -g -ggdb
        message( debug )
    } else {
        QMAKE_CFLAGS_RELEASE += -O3
        QMAKE_CXXFLAGS += -O3
        message( release )
    }

    CONFIG += c++17
    QMAKE_CXXFLAGS += -Wall
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_CFLAGS_RELEASE += -Wall
}

INCLUDEPATH += $$BOOST_INCLUDEPATH
INCLUDEPATH += "../include"

win32 {
    DEFINES += WIN_SIM
    LIBS += -lwinmm
    LIBS += -L$$BOOST_LIB_PATH
#    RC_ICONS += coppeliaSim.ico
}

macx {
    DEFINES += MAC_SIM
}

unix:!macx {
    # Following 2 required to have Lua extension libraries work under LINUX. Very strange indeed.
    # Probably not needed anymore... make sure luarocks installs by adjusting the rpath
    INCLUDEPATH += $$LUA_INCLUDEPATH
    LIBS += $$LUA_LIBS
    LIBS += -lboost_system
    DEFINES += LIN_SIM
}

SOURCES += sourceCode/main.cpp \
    ../include/simLib/simLib.cpp

HEADERS += ../include/simLib/simLib.h





