cmake_minimum_required(VERSION 3.9)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

project(RtspServer)

include_directories("../common/")

add_executable(${CMAKE_PROJECT_NAME} main.cc rtsp_server.cc ../common/utility.cc)
