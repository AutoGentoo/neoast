add_library(neoast-reflex-interface STATIC lexer.cc)
target_include_directories(neoast-reflex-interface PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/reflex/include/
)

target_link_libraries(neoast-reflex-interface reflex)
