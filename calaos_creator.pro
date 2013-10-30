#-------------------------------------------------
#
# Project created by QtCreator 2012-01-04T13:35:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = calaos_creator
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        MainWindow.cpp \
    RefreshDevices.cpp \
    Device.cpp

HEADERS  += MainWindow.h \
    RefreshDevices.h \
    Device.h

FORMS    += MainWindow.ui

LIBS += -lparted

RESOURCES += \
    resources.qrc



