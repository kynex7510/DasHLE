cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

project(DasHLE)

option(DASHLE_TESTS "Build tests (disabled by default)" off)

if (NOT DEFINED DASHLE_PLATFORM)
    # TODO: detect host.
    message(FATAL_ERROR "Target platform not set.")
endif()

if (NOT DEFINED DASHLE_TARGETS)
    message(WARNING "No target defined, all targets support enabled")
    set(DASHLE_TARGETS "ELF32;ELF64" CACHE STRING "Select binaries to support")
endif()

# Dependencies
include(cmake/Deps.cmake)
include_directories("${CMAKE_SOURCE_DIR}/source")

# Linux
if (DASHLE_PLATFORM STREQUAL "Linux")
    file(GLOB DasHLE_PLATFORM_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/Linux/*.cpp")
endif()

# Common
file(GLOB DasHLE_SOURCES 
    "${CMAKE_SOURCE_DIR}/source/DasHLE/Error.cpp" # TODO: pick any.
    "${CMAKE_SOURCE_DIR}/source/DasHLE/Binary/*.cpp"
#    "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/*.cpp"
#    "${CMAKE_SOURCE_DIR}/source/DasHLE/Guest/*.cpp"
)

# Tests
if (DASHLE_TESTS)
    add_subdirectory(tests)
endif()

# Build
add_executable(DasHLE ${DasHLE_PLATFORM_SOURCES} ${DasHLE_SOURCES})
add_dependencies(DasHLE dynarmic poly::standalone)
target_link_libraries(DasHLE dynarmic poly::standalone)