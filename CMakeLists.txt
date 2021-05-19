cmake_minimum_required(VERSION 3.0)
enable_language(C)

project(neoast VERSION "1.0")

option(SANITIZER_BUILD "Build with AddressSanitizer" OFF)
set(SANITIZER "" CACHE STRING "Compile with -fsanitize=...")
option(ENABLE_NEOAST_TESTS "Build+execute neoast's tests" OFF)
set(ASAN_LIB "libasan.so" CACHE STRING "Path to address sanitizer library")
set(RE2_BUILD_TESTING CACHE STRING OFF)

add_compile_options(-g -fno-omit-frame-pointer)
add_link_options(-fno-omit-frame-pointer)

if (NOT "${SANITIZER}" STREQUAL "")
    add_compile_options(-fsanitize=${SANITIZER})
    add_link_options(-fsanitize=${SANITIZER})
    message(STATUS "Building with -fsanitize=${SANITIZER}")
endif()

include(cmake/policy.cmake)

add_subdirectory(src)
add_subdirectory(third_party)

include(cmake/neoast.cmake)

if (ENABLE_NEOAST_TESTS)
    # Unit and integration testing
    include(cmake/test.cmake)
    add_subdirectory(test)
endif()
