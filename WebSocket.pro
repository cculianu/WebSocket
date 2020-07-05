QT -= gui
QT += network

TEMPLATE = lib
DEFINES += WEBSOCKET_LIBRARY
VERSION = 1.0.1

CONFIG += c++17

macx {
    # Note: This is required because we use advanced C++ features such as std::optional
    # which requires newer Mojave+ C++ libs.  On a recent compiler SDK, this will
    # compile ok even on High Sierra with latest Xcode for High Sierra, so this requirement
    # isn't too bad.  It just affects what C++ runtime we link to on MacOS.
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    WebSocket.cpp

HEADERS += \
    WebSocket.h

# Default rules for deployment.
unix {
    target.path = /usr/local/lib
}
!isEmpty(target.path): INSTALLS += target
