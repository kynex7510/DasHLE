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
# HACK
include_directories(external/dynarmic/externals/mcl/include)
include_directories(external/dynarmic/externals/fmt/include)
include_directories(external/boost)

# Project

include_directories(source)

set(DasHLE_SOURCES
    ${CMAKE_SOURCE_DIR}/source/DasHLE/Base.cpp
    ${CMAKE_SOURCE_DIR}/source/DasHLE/Guest/ARM/ARMContext.cpp
    ${CMAKE_SOURCE_DIR}/source/DasHLE/Guest/ARM/ARMEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/source/DasHLE/Guest/Context.cpp
    ${CMAKE_SOURCE_DIR}/source/DasHLE/Host/Memory.cpp
    ${CMAKE_SOURCE_DIR}/source/DasHLE/Host/Linux/Base.cpp
)

#add_subdirectory(tests)

add_executable(DasHLE ${DasHLE_SOURCES} ${CMAKE_SOURCE_DIR}/source/DasHLE/Main.cpp)
add_dependencies(DasHLE dynarmic)
target_link_libraries(DasHLE dynarmic)