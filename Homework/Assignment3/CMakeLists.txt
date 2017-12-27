cmake_minimum_required(VERSION 3.6)
project(VirtualMemoryManager)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES os_hw3_adrian_davila.cpp)
add_executable(VirtualMemoryManager ${SOURCE_FILES})