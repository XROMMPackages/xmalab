cmake_minimum_required(VERSION 3.16)
project(TestOpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

add_executable(test_opengl main.cpp)
target_link_libraries(test_opengl Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
