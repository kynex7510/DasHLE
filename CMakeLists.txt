cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

project(GDACL)

add_subdirectory(external/dynarmic)

include_directories(
	include
	dynarmic/src
)

set(GDACL_SOURCES
    ./source/Binary.cpp
    ./source/Main.cpp
)

add_executable(GDACL GDACL_SOURCES)
add_dependencies(GDACL dynarmic)
target_link_libraries(GDCrypto dynarmic)