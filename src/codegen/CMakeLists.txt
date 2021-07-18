add_library(neoast-codegen STATIC
        codegen.h
        codegen.cc
        regex.h
        compiler.c compiler.h common.c
        cg_util.cc cg_util.h
        cg_builtin_lexer.cc cg_builtin_lexer.h codegen_priv.h cg_lexer.h cg_grammar.cc cg_grammar.h codegen_template.cc codegen_impl.h)

add_library(builtin-lexer STATIC
        builtin_lexer/builtin_lexer.cc
        builtin_lexer/builtin_lexer.h)


add_executable(neoast-bootstrap
        bootstrap/codegen_grammar.c
        bootstrap/main.c bootstrap/grammar.h)

set(compiler_compiler
        ${CMAKE_CURRENT_BINARY_DIR}/neoast_compiler_compiler.c
        ${CMAKE_CURRENT_BINARY_DIR}/neoast_compiler_compiler.h
        )

add_custom_command(OUTPUT ${compiler_compiler}
        COMMAND $<TARGET_FILE:neoast-bootstrap>
        ARGS compiler_compiler.y ${compiler_compiler}
        DEPENDS neoast-bootstrap neoast-codegen compiler_compiler.y
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(neoast-exec ${compiler_compiler} main.c)
add_dependencies(neoast-exec neoast-bootstrap)

include_directories(neoast-codegen PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${PROJECT_SOURCE_DIR}/third_party/reflex/src
        ${PROJECT_SOURCE_DIR}/third_party/inja/single_include
        ${PROJECT_SOURCE_DIR}/third_party/inja/third_party/include
        )

include_directories(builtin-lexer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/third_party/reflex/include
        )
target_link_libraries(builtin-lexer reflex)

target_link_libraries(neoast-codegen neoast neoast-parsergen neoast-util builtin-lexer)
target_link_libraries(neoast-exec neoast-codegen)
target_link_libraries(neoast-bootstrap neoast-codegen)
