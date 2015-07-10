cmake_minimum_required (VERSION 2.8.11)
project (CLOUD_PROTECT)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ")

add_executable(7zToTar src/main.cpp src/7z.cpp src/tar.cpp src/crc32.cpp)
