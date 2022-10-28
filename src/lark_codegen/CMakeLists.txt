set(compiler_compiler
        ${CMAKE_CURRENT_BINARY_DIR}/neoast_lark_compiler_compiler.cc
        ${CMAKE_CURRENT_BINARY_DIR}/neoast_lark_compiler_compiler.h
        )

add_custom_command(OUTPUT ${compiler_compiler}
        COMMAND $<TARGET_FILE:neoast-bootstrap>
        ARGS grammar/lark_syntax.y ${compiler_compiler}
        DEPENDS neoast-bootstrap neoast-yacc-codegen grammar/lark_syntax.y
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_library(neoast_lark_compiler_compiler STATIC ${compiler_compiler})
target_link_libraries(neoast_lark_compiler_compiler neoast)

add_executable(neoast_lark_codegen
        main.cc
        codegen.cc
        input_file.cc
        compiler.cc compiler.h
        types.cc compiler_resolve.cc compiler_utils.cc)

target_link_libraries(neoast_lark_codegen
        neoast
        neoast_lark_compiler_compiler
        neoast-parsergen)
target_include_directories(neoast_lark_compiler_compiler PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        )
