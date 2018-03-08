TEMPLATE    = app
TARGET        = vrep
CONFIG        += console debug_and_release
CONFIG        += app_bundle
CONFIG        += WITH_QT
QT            -= gui

WITH_QT {
    DEFINES += QT_FRAMEWORK
} else {
    DEFINES += SIM_WITHOUT_QT_AT_ALL
    QT -= core
}

*-msvc* {
    QMAKE_CXXFLAGS += -O2
    QMAKE_CXXFLAGS += -W3
}
*-g++* {

    CONFIG(debug,debug|release) {
        QMAKE_CXXFLAGS += -g -ggdb
        message( debug )
    } else {
        QMAKE_CFLAGS_RELEASE += -O3
        QMAKE_CXXFLAGS += -O3
        message( release )
    }

    QMAKE_CXXFLAGS += -Wall
    QMAKE_CFLAGS_RELEASE += -Wall
}


INCLUDEPATH += "../include"

win32 {
    DEFINES += WIN_VREP
    LIBS += -lwinmm
#    RC_ICONS += v_rep.ico
}

macx {
    DEFINES += MAC_VREP
}

# Following required to have Lua extension libraries work under LINUX. Very strange indeed.
unix:!macx {
        contains(QMAKE_HOST.arch, x86_64):{ # 64 Bit
                INCLUDEPATH += "/usr/include/lua5.1"
                LIBS += -Llua5_1_4_Linux26g4_64_lib/ -llua5.1
        }
        !contains(QMAKE_HOST.arch, x86_64):{ # 32 Bit
                INCLUDEPATH += "/usr/include/lua5.1"
                LIBS += -Llua5_1_4_Linux26g4_lib/ -llua5.1
        }
    DEFINES += LIN_VREP
}

SOURCES += main.cpp \
    ../common/v_repLib.cpp

HEADERS += ../include/v_repLib.h





