add_library(neoast-bootstrap-lexer STATIC
        lexer.cc
        bootstrap_lexer.h)

# Bootstrap uses Reflex at runtime
target_include_directories(neoast-bootstrap-lexer PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/reflex/include
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(neoast-bootstrap-lexer
        reflex
        reflex-unicode  # TODO(tumbar) Merge this into the main reflex lib
        )
