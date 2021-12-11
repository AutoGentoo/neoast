add_subdirectory(lexer)

add_executable(neoast-bootstrap
        codegen_grammar.c
        main.c
        grammar.h)
target_include_directories(neoast-bootstrap PRIVATE
        ${PROJECT_SOURCE_DIR}/src/
        )

target_link_libraries(neoast-bootstrap
        neoast-codegen          # Bootstrap will invoke codegen to generate the full compiler-compiler
        neoast-bootstrap-lexer  # Bootstrap lexer implementation
        )