cmake_minimum_required(VERSION 3.10)

project(DX12)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_executable(DX12 WIN32 main.cpp)