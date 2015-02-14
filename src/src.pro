#-------------------------------------------------
#
# Project created by QtCreator 2014-04-10T11:24:42
#
#-------------------------------------------------

QT       += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rfkilltray
TEMPLATE = app


SOURCES += main.cpp\
    tray.cpp \
    dbus.cpp

HEADERS  += \
    tray.h

FORMS    +=

RESOURCES += \
    icons.qrc
