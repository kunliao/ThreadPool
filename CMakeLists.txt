cmake_minimum_required(VERSION 3.14)
project(threadpool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "/utf-8")
include_directories(.)
add_executable(threadpool main.cpp )
set_target_properties(threadpool PROPERTIES COMPILE_FLAGS "/EHsc")