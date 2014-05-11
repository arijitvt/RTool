#-------------------------------------------------
#
# Project created by QtCreator 2014-05-09T00:58:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Rui
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        PythonInterface.cpp \
        custompushbutton.cpp

HEADERS  += mainwindow.h\
            PythonInterface.h \
            custompushbutton.h

FORMS    += mainwindow.ui
