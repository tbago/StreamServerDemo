cmake_minimum_required(VERSION 3.9)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

project(HlsServer)

include_directories("utility")

add_executable(${CMAKE_PROJECT_NAME} main.cc hls_server.cc utility/utility.cc)

