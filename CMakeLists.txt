cmake_minimum_required(VERSION 3.30)
project(CHIP8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})


add_executable(CHIP8 src/main.cpp src/chip8.cpp src/chip8.h src/screen.cpp src/screen.h)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})