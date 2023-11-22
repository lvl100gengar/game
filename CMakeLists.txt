cmake_minimum_required(VERSION 3.12)
project(mapbuilder C)

# Set the C standard to C99
set(CMAKE_C_STANDARD 99)

# Set the output directory for compiled code
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Specify the path to SDL2
set(SDL2_DIR "C:/Users/seanf/Desktop/Programming/SDL2-2.24.0/cmake")

# Find SDL2 package
find_package(SDL2 REQUIRED)

# Include SDL2 headers
include_directories(${SDL2_INCLUDE_DIRS})

# Add the source code subdirectories
add_subdirectory(src)
add_subdirectory(test)