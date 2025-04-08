QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = v4l2-camera
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        camera_capture.cpp \
        main.cpp \
        main_window.cpp

HEADERS += \
        camera_capture.h \
        main_window.h

unix:!macx {
    LIBS += -ljpeg
}

local.path = $$OUT_PWD
local.path ~= s/builds.*/
target.path = /root
INSTALLS += target
