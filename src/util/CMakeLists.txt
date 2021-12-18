add_library(neoast-util STATIC
        table_dump.cc
        util.h graphiz.cc)

target_include_directories(neoast-util PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/cre2/src)