add_library(neoast STATIC
        common.h
        lexer.c
        lexer.h
        parser.h
        lr.c parser.c
        ../include/neoast.h)

target_include_directories(neoast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(neoast
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cre2/src)
target_link_libraries(neoast cre2 re2 pthread)

add_subdirectory(parsergen)
add_subdirectory(codegen)
add_subdirectory(util)
