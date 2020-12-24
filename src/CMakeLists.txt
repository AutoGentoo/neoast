add_library(neoast STATIC
        common.h
        lexer.c
        lexer.h
        parsers/left_scan.c
        parser.h
        parsers/lr.c)

target_include_directories(neoast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
