cmake_minimum_required(VERSION 3.5)

project(v4l2-camera LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SRC
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/*.cxx"
    "${SRC_DIR}/*.cc"
    "${SRC_DIR}/*.c"
)

file(GLOB HEADERS
    "${SRC_DIR}/*.h"
    "${SRC_DIR}/*.hpp"
    "${SRC_DIR}/*.hxx"
)

file(GLOB RES
    "${SRC_DIR}/*.ui"
    "${SRC_DIR}/*.qrc"
)

add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${RES})

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)
