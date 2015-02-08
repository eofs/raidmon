#-------------------------------------------------
#
# Project created by QtCreator 2010-06-19T01:00:29
#
#-------------------------------------------------

QT      += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RaidMon
TEMPLATE = app

SOURCES += main.cpp\
        raidmon.cpp \
        scanner.cpp

HEADERS  += raidmon.h \
            scanner.h

FORMS    += raidmon.ui

DEFINES += QT_NO_DEBUG_OUTPUT
