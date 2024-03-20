cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0003 NEW)

project(mousex)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

SET(OPTIONS WIN32)

add_executable(mousex
    mousex.cpp
    mousex.h
    resource.h
)

target_sources(mousex PRIVATE mousex.rc)
