cmake_minimum_required(VERSION 3.0)
enable_language(C)

project(neoast VERSION "1.0")

option(SANITIZER_BUILD "Build with AddressSanitizer" OFF)
set(ASAN_LIB "libasan.so" CACHE STRING "Path to address santizer library")

add_compile_options(-g -fno-omit-frame-pointer)
add_link_options(-fno-omit-frame-pointer)

if (SANITIZER_BUILD)
    add_compile_options(-fsanitize=address -fsanitize=leak)
    add_link_options(-fsanitize=address -fsanitize=leak)
    message(STATUS "Building with AddressSanitizer")
endif()

# Unit and integration testing
include(cmake/test.cmake)

find_library(HYPERSCAN_LIBRARY NAMES hs hyperscan REQUIRED)
message(STATUS "Found Hyperscan library ${HYPERSCAN_LIBRARY}")

find_path(HYPERSCAN_INCLUDE_DIR hs/hs.h REQUIRED)
message(STATUS "Found Hyperscan header in ${HYPERSCAN_INCLUDE}/hs/hs.h")

add_subdirectory(src)
add_subdirectory(third_party)
add_subdirectory(test)
