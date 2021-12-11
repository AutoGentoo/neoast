add_library(neoast STATIC
        lr.c parser.c
        ../include/neoast.h)

target_include_directories(neoast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(neoast
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/reflex/include)

add_subdirectory(parsergen)
add_subdirectory(codegen)
add_subdirectory(util)
add_subdirectory(lexer)
