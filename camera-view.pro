QT       += core gui multimediawidgets

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

LIBS += -lv4l2

local.path = $$OUT_PWD
local.path ~= s/builds.*/
target.path = /userdata/admin
INSTALLS += target
