add_library(neoast-codegen STATIC
        codegen_grammar.c codegen.h
        codegen.c
        regex.c regex.h)
add_executable(neoast-exec main.c)

include_directories(neoast-codegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(neoast-codegen neoast neoast-parsergen neoast-util)
target_link_libraries(neoast-exec neoast-codegen)