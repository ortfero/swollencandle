cmake_minimum_required(VERSION 3.20)
project(swollencandle-test)

set(CMAKE_CXX_STANDARD 20)

add_executable(swollencandle-test test.cpp ../include/swollencandle/swollencandle.hpp)

target_include_directories(swollencandle-test PRIVATE ../include)
