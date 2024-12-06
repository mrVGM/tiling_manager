# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("TilingManager")
set(CMAKE_C_COMPILER "clang")

find_package(Threads REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ready)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ready)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG=1)
ENDIF()

# Include sub-projects.
add_subdirectory ("Manager")
add_subdirectory ("Bridge")
add_subdirectory ("Tasks")
add_subdirectory ("Tiling")
