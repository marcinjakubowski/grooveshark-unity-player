#-------------------------------------------------
#
# Project created by QtCreator 2014-04-30T00:48:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webkitwidgets

TARGET = grooveshark-unity-player
TEMPLATE = app

SOURCES += main.cpp\
        grooveshark.cpp \
    optionsdialog.cpp

HEADERS  += grooveshark.h \
    optionsdialog.h

FORMS    += grooveshark.ui \
    optionsdialog.ui

CONFIG += link_pkgconfig
PKGCONFIG += unity libnotify gtk+-2.0
