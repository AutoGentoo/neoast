add_library(neoast-util STATIC
        table_dump.c
        util.h)

target_include_directories(neoast-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)