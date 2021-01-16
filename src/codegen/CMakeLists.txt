add_executable(
        neoast-exec
        main.c
        codegen_grammar.c codegen.h codegen.c)

include_directories(neoast-codegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(neoast-exec neoast neoast-codegen neoast-util)