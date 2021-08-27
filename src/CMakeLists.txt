add_library(neoast_builtin_lexer STATIC
        codegen/builtin_lexer/builtin_lexer.cc
        codegen/builtin_lexer/builtin_lexer.h)

add_library(neoast STATIC
        lr.c parser.c
        ../include/neoast.h)

target_include_directories(neoast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(neoast
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/reflex/include)

target_include_directories(neoast_builtin_lexer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(neoast_builtin_lexer
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/reflex/include)

target_link_libraries(neoast_builtin_lexer PUBLIC reflex)

# TODO(tumbar) Replace this when pure reflex is builtin
target_link_libraries(neoast PUBLIC
        builtin-lexer  # TODO(tumbar) REMOVE this
        )

add_subdirectory(parsergen)
add_subdirectory(codegen)
add_subdirectory(util)
