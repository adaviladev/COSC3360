cmake_minimum_required(VERSION 3.6)
project(Sem_v2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(Sem_v2 ${SOURCE_FILES})