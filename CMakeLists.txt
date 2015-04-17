cmake_minimum_required(VERSION 2.8)
project(wglarb)

find_package(OpenGL)

add_subdirectory(src)

include_directories(src)
add_subdirectory(test)
