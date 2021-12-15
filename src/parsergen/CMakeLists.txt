add_library(
        neoast-parsergen
        clr_lalr.c clr_lalr.h
        canonical_collection.c canonical_collection.h
)

target_include_directories(neoast-parsergen
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
