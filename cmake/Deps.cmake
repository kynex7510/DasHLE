include(cmake/CPM.cmake)

# Boost is a required dependency for dynarmic.

CPMAddPackage(
    NAME Boost
    VERSION 1.83.0
    URL "https://github.com/citra-emu/ext-boost/archive/80f97ea03b5412baddaba8bef1222b112076c997.tar.gz"
    URL_HASH SHA256=44906569a06ea0c09a718710f6bdfaa1fe53c01ba282fc4c608a263ae2f877f6
)

if (Boost_ADDED)
    # HACK
    # Delete the CPM generated FindBoost.cmake, so that next find_package() calls rely on CMake's own mechanism.
    string(CONCAT FIND_BOOST_PATH ${CMAKE_MODULE_PATH} "/FindBoost.cmake")
    file(REMOVE ${FIND_BOOST_PATH})
    set(Boost_INCLUDE_DIR ${Boost_SOURCE_DIR})
endif()

# Add dynarmic.

if ("ARM" IN_LIST DASHLE_GUESTS)
    list(APPEND DYNARMIC_FRONTENDS "A32")
endif()

if ("AArch64" IN_LIST DASHLE_GUESTS)
    list(APPEND DYNARMIC_FRONTENDS "A64")
endif()

CPMAddPackage(
    NAME Dynarmic
    VERSION 6.6.3
    URL "https://github.com/merryhime/dynarmic/archive/2c0dc887158d891785f0a613c2794cd99cab9b1f.tar.gz"
    URL_HASH SHA256=c6942a60bf8650fed23b8c6a946919808372e9f024de9eddb7dc329582883714
)

if (Dynarmic_ADDED)
    # Required for using the IR emitter.
    include_directories("${Dynarmic_SOURCE_DIR}/externals/mcl/include")
    include_directories("${Dynarmic_SOURCE_DIR}/externals/fmt/include")
    include_directories("${Boost_SOURCE_DIR}")
endif()

# Poly

CPMAddPackage(
    NAME Poly
    VERSION 1.0.0
    URL "https://github.com/8ightfold/poly-standalone/archive/026ae3f01d834ae1a9726556e93cdf7c510868fa.tar.gz"
    URL_HASH SHA256=4b213fa2f63a33be67aa1c55df2ae9e2d542d650785e29dc42d3984db981280c
)