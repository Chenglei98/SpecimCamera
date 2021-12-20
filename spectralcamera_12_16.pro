QT += core network
QT -= gui

CONFIG += c++11

TARGET = spectralcamera_12_16
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    spectralcamera.cpp


HEADERS += \
    spectralcamera.h


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


LIBS += -L"C:/Program Files (x86)/Specim/SDKs/SpecSensor/2020_519/bin/x64" -lSpecSensor
LIBS += -L"D:/Opencv-3.4.12/opencv/build/x64/vc15/lib" -lopencv_world3412

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include


INCLUDEPATH += D:/Opencv-3.4.12/opencv/build/include \
               D:/Opencv-3.4.12/opencv/build/include/opencv \
               D:/Opencv-3.4.12/opencv/build/include/opencv2

