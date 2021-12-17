add_library(
        neoast-parsergen
        canonical_collection.cc canonical_collection.h
        derivation.cc derivation.h
        c_pub.cc)

target_include_directories(neoast-parsergen
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
)

target_include_directories(neoast-parsergen
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
