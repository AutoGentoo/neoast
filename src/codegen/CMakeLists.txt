add_library(neoast-codegen STATIC
        codegen.h
        codegen.c
        regex.c regex.h compiler.c compiler.h common.c)

add_executable(neoast-bootstrap
        bootstrap/codegen_grammar.c
        bootstrap/main.c bootstrap/grammar.h)

set(compiler_compiler ${CMAKE_CURRENT_BINARY_DIR}/neoast_compiler_compiler.c)

add_custom_command(OUTPUT ${compiler_compiler}
        COMMAND $<TARGET_FILE:neoast-bootstrap>
        ARGS compiler_compiler.y ${compiler_compiler}
        DEPENDS compiler_compiler.y neoast-bootstrap neoast-codegen
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(neoast-exec ${compiler_compiler} main.c)

include_directories(neoast-codegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(neoast-codegen neoast neoast-parsergen neoast-util)
target_link_libraries(neoast-exec neoast-codegen)
target_link_libraries(neoast-bootstrap neoast-codegen)
