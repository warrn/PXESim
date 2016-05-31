cmake_minimum_required(VERSION 3.0)
project(PXESim)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ltins -g")
set(SOURCE_FILES main.cpp PXEClient.cpp PXEClient.h)
add_executable(PXESim ${SOURCE_FILES})