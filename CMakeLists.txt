cmake_minimum_required(VERSION 3.14)
project(threadpool)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)
add_executable(threadpool main.cpp )