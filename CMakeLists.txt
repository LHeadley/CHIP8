cmake_minimum_required(VERSION 3.30)
project(CHIP8)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(CHIP8 src/main.cpp src/chip8.cpp src/chip8.h src/screen.cpp src/screen.h)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)