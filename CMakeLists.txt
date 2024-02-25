cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

project(DasHLE)

option(DASHLE_TESTS "Build tests (disabled by default)" off)

if (NOT DEFINED DASHLE_HOST)
    # TODO: detect host.
    message(FATAL_ERROR "Host platform not set.")
endif()

if (NOT DEFINED DASHLE_GUESTS)
    message(WARNING "No guest defined, all guests support enabled")
    set(DASHLE_TARGETS "ARM;AArch64" CACHE STRING "Select binaries to support")
endif()

# Dependencies
include(cmake/Deps.cmake)

# Hosts
if (DASHLE_HOST STREQUAL "Linux")
    file(GLOB DasHLE_HOST_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/Linux/*.cpp")
endif()

# Guests
if ("ARM" IN_LIST DASHLE_GUESTS)
    file(GLOB DasHLE_ARM_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/ARM/*.cpp")
    list(APPEND DasHLE_GUEST_SOURCES ${DasHLE_ARM_SOURCES})
endif()

if ("AArch64" IN_LIST DASHLE_GUESTS)
    file(GLOB DasHLE_AARCH64_SOURCES "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/AArch64/*.cpp")
    list(APPEND DasHLE_GUEST_SOURCES ${DasHLE_AARCH64_SOURCES})
endif()

# Common
file(GLOB DasHLE_SOURCES 
    "${CMAKE_SOURCE_DIR}/source/DasHLE/Error.cpp" # TODO: pick any.
    "${CMAKE_SOURCE_DIR}/source/DasHLE/Binary/*.cpp"
    "${CMAKE_SOURCE_DIR}/source/DasHLE/Host/*.cpp"
    "${CMAKE_SOURCE_DIR}/source/DasHLE/Guest/*.cpp"
)

# DasHLE
add_executable(DasHLE
    ${DasHLE_HOST_SOURCES}
    ${DasHLE_GUEST_SOURCES}
    ${DasHLE_SOURCES})

target_include_directories(DasHLE "${CMAKE_SOURCE_DIR}/source")

if ("ARM" IN_LIST DASHLE_TARGETS)
    target_compile_definitions(DasHLE PUBLIC DASHLE_HAS_GUEST_ARM)
endif()

if ("AArch64" IN_LIST DASHLE_TARGETS)
    target_compile_definitions(DasHLE PUBLIC DASHLE_HAS_GUEST_AARCH64)
endif()

add_dependencies(DasHLE dynarmic poly::standalone)
target_link_libraries(DasHLE dynarmic poly::standalone)

# Tests
if (DASHLE_TESTS)
    add_subdirectory(tests)
endif()