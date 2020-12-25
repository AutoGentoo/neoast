add_library(neoast STATIC
        common.h
        lexer.c
        lexer.h
        parser.h
        lr.c)

target_include_directories(neoast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(parser_gen)
