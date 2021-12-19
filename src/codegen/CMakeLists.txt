add_library(neoast-codegen STATIC
        codegen.h
        codegen.cc
        regex.h
        compiler.c compiler.h common.c
        cg_util.cc cg_util.h
        cg_neoast_lexer.cc cg_neoast_lexer.h
        codegen_priv.h cg_lexer.h
        cg_pattern.cc cg_pattern.h
        cg_grammar.cc cg_grammar.h
        codegen_template.cc codegen_impl.h
        input_file.cc input_file.h regex.cc)

target_compile_definitions(neoast-codegen PRIVATE NEOAST_CODEGEN_BACKDOOR)

# Build the bootstrap compiler with the pre-generated parser
add_executable(neoast-bootstrap bootstrap/bootstrap.c bootstrap/bootstrap.h main.cc)
target_link_libraries(neoast-bootstrap neoast-codegen)

set(compiler_compiler
        ${CMAKE_CURRENT_BINARY_DIR}/neoast_compiler_compiler.c
        ${CMAKE_CURRENT_BINARY_DIR}/neoast_compiler_compiler.h
        )

add_custom_command(OUTPUT ${compiler_compiler}
        COMMAND $<TARGET_FILE:neoast-bootstrap>
        ARGS compiler_compiler.y ${compiler_compiler}
        DEPENDS neoast-bootstrap neoast-codegen compiler_compiler.y
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_library(neoast_compiler_compiler STATIC ${compiler_compiler})
target_include_directories(neoast_compiler_compiler PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_executable(neoast-exec main.cc)
add_dependencies(neoast-exec neoast-bootstrap)

target_include_directories(neoast-codegen PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/reflex/src
        ${PROJECT_SOURCE_DIR}/third_party/reflex/include
        ${INJA_INCLUDE_DIRECTORIES}
        )
target_include_directories(neoast-codegen PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(neoast-codegen
        neoast
        neoast-parsergen
        neoast-util
        reflex          # used to build regex DFAs
        reflex-unicode
        )
target_link_libraries(neoast-exec
        neoast_compiler_compiler
        neoast-codegen
        # Link twice to work around circular dependencies
        neoast_compiler_compiler
        neoast-codegen
)
add_executable(neoast-debug debug.cc)
target_link_libraries(neoast-debug neoast_compiler_compiler neoast-codegen neoast-util)

