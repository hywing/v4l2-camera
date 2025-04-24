QT += qml quick multimedia

TARGET = v4l2-camera
TEMPLATE = app

SOURCES += main.cpp \
           multicamerahandler.cpp \
           multicameraimageprovider.cpp \
           camera_capture.cpp

HEADERS += multicamerahandler.h \
           multicameraimageprovider.h \
            camera_capture.h

RESOURCES += res.qrc

local.path = $$OUT_PWD
local.path ~= s/builds.*/
target.path = /userdata/admin
INSTALLS += target
