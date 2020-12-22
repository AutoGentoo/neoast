cmake_minimum_required(VERSION 3.0)
enable_language(C)

project(neoast VERSION "1.0")

# Unit and integration testing
include(cmake/test.cmake)

add_subdirectory(src)
add_subdirectory(third_party)
add_subdirectory(test)
