add_library(
        neoast-parsergen
        clr_lalr.c clr_lalr.h
        canonical_collection.c canonical_collection.h
        )

include_directories(neoast-parsergen
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/cre2/src)