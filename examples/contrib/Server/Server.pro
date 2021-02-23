QT += core network
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

macx {
    # Note: This is required because we use advanced C++ features such as std::optional
    # which requires newer Mojave+ C++ libs.  On a recent compiler SDK, this will
    # compile ok even on High Sierra with latest Xcode for High Sierra, so this requirement
    # isn't too bad.  It just affects what C++ runtime we link to on MacOS.
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
}

DEFINES += QT_DEPRECATED_WARNINGS
topdir = $$PWD/../..
INCLUDEPATH += $$topdir

HEADERS += \
           myserver.h              \
           client.h                \
           $$topdir/WebSocket.h


SOURCES += main.cpp                \
           myserver.cpp            \
           client.cpp              \
           $$topdir/WebSocket.cpp




