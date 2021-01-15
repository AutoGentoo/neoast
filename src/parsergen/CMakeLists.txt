add_library(
        neoast-codegen
        clr_lalr.c clr_lalr.h
        canonical_collection.c canonical_collection.h
        )

include_directories(neoast-codegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_SOURCE_DIR}/../../include)