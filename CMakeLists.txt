cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

include(cmake/Deps.cmake)

project(DasHLE)

#add_subdirectory(external/dynarmic)
#include_directories(external/dynarmic/src)

# Required for using the IR emitter
#include_directories(external/dynarmic/externals/mcl/include)
#include_directories(external/dynarmic/externals/fmt/include)
#include_directories(external/boost)

include_directories(source)

#FILE(GLOB DasHLE_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/Linux/*.cpp")

#FILE(GLOB DasHLE_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/*.cpp")
FILE(GLOB DasHLE_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Binary/*.cpp")
#FILE(GLOB DasHLE_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/*.cpp")
#FILE(GLOB DasHLE_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Guest/*.cpp")

#add_subdirectory(tests)

add_executable(DasHLE ${DasHLE_SOURCES})
#${CMAKE_SOURCE_DIR}/source/DasHLE/Main.cpp)
add_dependencies(DasHLE dynarmic poly::standalone)
target_link_libraries(DasHLE dynarmic poly::standalone)