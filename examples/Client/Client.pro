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

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
topdir = $$PWD/../..
INCLUDEPATH += $$topdir

HEADERS += \
        client.h \
        $$topdir/WebSocket.h

SOURCES += \
        main.cpp \
        client.cpp \
        $$topdir/WebSocket.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
