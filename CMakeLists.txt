cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

project(GDACL)

# Dependencies setup

set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/external/boost")
set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/boost")
set(Boost_NO_SYSTEM_PATHS ON)

add_subdirectory(external/dynarmic)

include_directories(external/dynarmic/src)
include_directories(external/boost)
include_directories(include)

# Project

set(GDACL_SOURCES
    ./source/Binary.cpp
    ./source/Main.cpp
)

add_executable(GDACL ${GDACL_SOURCES})
add_dependencies(GDACL dynarmic)
target_link_libraries(GDACL dynarmic)