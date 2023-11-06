cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)

project(DasHLE)

# Dependencies setup

set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/external/boost")
set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/boost")
set(Boost_NO_SYSTEM_PATHS ON)

add_subdirectory(external/dynarmic)

include_directories(external/dynarmic/src)
include_directories(external/boost)

# Project

include_directories(source)

set(DasHLE_SOURCES
    ${CMAKE_SOURCE_DIR}/source/Base.cpp
    ${CMAKE_SOURCE_DIR}/source/Memory.cpp
)

add_subdirectory(tests)

add_executable(DasHLE ${DasHLE_SOURCES} ${CMAKE_SOURCE_DIR}/source/Main.cpp)
add_dependencies(DasHLE dynarmic)
target_link_libraries(DasHLE dynarmic)