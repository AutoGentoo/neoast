cmake_minimum_required(VERSION 3.0)
enable_language(C CXX)

set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(neoast VERSION "1.3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SANITIZER "" CACHE STRING "Compile with -fsanitize=...")
option(ENABLE_NEOAST_TESTS "Build+execute neoast's tests" OFF)
set(ASAN_LIB "libasan.so" CACHE STRING "Path to address sanitizer library")
set(RE2_BUILD_TESTING CACHE STRING OFF)

if (NOT "${SANITIZER}" STREQUAL "")
    add_compile_options(-fsanitize=${SANITIZER})
    add_link_options(-fsanitize=${SANITIZER})
    message(STATUS "Building with -fsanitize=${SANITIZER}")
    if (${SANITIZER} STREQUAL "memory")
        add_compile_options(-fsanitize-memory-track-origins=2 -fPIE -fPIC  -O2 -fsanitize-memory-use-after-dtor)
    else()
        add_compile_options(-O0)
    endif()
    if (${SANITIZER} STREQUAL "address")
        add_compile_options(-fsanitize=leak)
        add_link_options(-fsanitize=leak)
    endif()
    add_compile_options(-g -fno-omit-frame-pointer)
endif()

include(cmake/policy.cmake)

add_subdirectory(third_party)
add_subdirectory(src)

include(cmake/neoast.cmake)

if (ENABLE_NEOAST_TESTS)
    add_compile_options(-g)
    # Unit and integration testing
    include(cmake/test.cmake)
    add_subdirectory(test)
endif()
