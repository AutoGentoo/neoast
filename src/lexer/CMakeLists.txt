add_library(neoast-lexer STATIC
        matcher.c
        container.c
        input.c
        )

target_include_directories(neoast-lexer PRIVATE ${PROJECT_SOURCE_DIR}/include)
