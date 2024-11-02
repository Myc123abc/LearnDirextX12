cmake_minimum_required(VERSION 3.10)

project(DX12)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*cpp")

add_executable(DX12 WIN32 ${SOURCES})

target_link_libraries(DX12 PRIVATE dxgi d3d12)

target_include_directories(DX12 PRIVATE "${CMAKE_SOURCE_DIR}/include")