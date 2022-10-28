add_library(neoast STATIC
        lr.c parser.c
        lexer/matcher.c
        lexer/container.c
        lexer/input.c
)

target_include_directories(neoast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_subdirectory(parsergen)
add_subdirectory(yacc_codegen)
add_subdirectory(lark_codegen)
add_subdirectory(util)
