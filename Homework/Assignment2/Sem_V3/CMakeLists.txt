cmake_minimum_required(VERSION 3.6)
project(Sem_V3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES os_hw2_adrian_davila.cpp List.h Node.h Variable.h Problem.h Base.cpp Arithmetic.cpp Arithmetic.h)
add_executable(Sem_V3 ${SOURCE_FILES})