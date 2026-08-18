# GForce CAD Application build definition.
# Configures the Qt 6/CMake project, source files, and OpenGL dependencies for the CAD editor.
cmake_minimum_required(VERSION 3.21)

project(GforceCadApplication VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

qt_add_executable(GforceCadApplication
    src/main.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/cad/Geometry.h
    src/cad/Entity.h
    src/cad/Entity.cpp
    src/cad/Document.h
    src/cad/Document.cpp
    src/cad/Entities.h
    src/cad/Entities.cpp
    src/cad/Tools.h
    src/cad/Tools.cpp
    src/render/CadViewport.h
    src/render/CadViewport.cpp
)

target_include_directories(GforceCadApplication PRIVATE src)

target_link_libraries(GforceCadApplication PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

if (WIN32)
    set_target_properties(GforceCadApplication PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
