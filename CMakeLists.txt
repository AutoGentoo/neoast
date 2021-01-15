cmake_minimum_required(VERSION 3.0)
enable_language(C)

project(neoast VERSION "1.0")

option(SANITIZER_BUILD "Build with AddressSanitizer" OFF)
set(ASAN_LIB "libasan.so" CACHE STRING "Path to address sanitizer library")
set(RE2_BUILD_TESTING CACHE STRING OFF)

add_compile_options(-g -fno-omit-frame-pointer)
add_link_options(-fno-omit-frame-pointer)

if (SANITIZER_BUILD)
    add_compile_options(-fsanitize=address -fsanitize=leak)
    add_link_options(-fsanitize=address -fsanitize=leak)
    message(STATUS "Building with AddressSanitizer")
endif()

include(cmake/policy.cmake)

# Unit and integration testing
include(cmake/test.cmake)

include_directories(${PROJECT_SOURCE_DIR}/third_party/cre2/src)

add_subdirectory(src)
add_subdirectory(third_party)

include(cmake/neoast.cmake)
add_subdirectory(test)
