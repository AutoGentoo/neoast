cmake_minimum_required(VERSION 3.0)
enable_language(C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(neoast VERSION "1.2")

option(SANITIZER_BUILD "Build with AddressSanitizer" OFF)
set(SANITIZER "" CACHE STRING "Compile with -fsanitize=...")
option(ENABLE_NEOAST_TESTS "Build+execute neoast's tests" OFF)
set(ASAN_LIB "libasan.so" CACHE STRING "Path to address sanitizer library")
set(RE2_BUILD_TESTING CACHE STRING OFF)

if (NOT "${SANITIZER}" STREQUAL "")
    add_compile_options(-fsanitize=${SANITIZER})
    add_link_options(-fsanitize=${SANITIZER})
    message(STATUS "Building with -fsanitize=${SANITIZER}")
    add_compile_options(-g -fno-omit-frame-pointer -O0)
endif()

include(cmake/policy.cmake)

add_subdirectory(src)
add_subdirectory(third_party)

include(cmake/neoast.cmake)

if (ENABLE_NEOAST_TESTS)
    add_compile_options(-g)
    # Unit and integration testing
    include(cmake/test.cmake)
    add_subdirectory(test)
endif()
