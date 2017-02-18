#-------------------------------------------------
#
# Project created by QtCreator 2016-12-05T11:11:10
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Bin2Text
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

RC_ICONS += ./images/window.ico

RESOURCES += \
    res.qrc
