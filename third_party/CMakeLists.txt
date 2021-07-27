if (ENABLE_NEOAST_TESTS)
    add_subdirectory(cmocka EXCLUDE_FROM_ALL)
endif()

add_subdirectory(reflex)

set(INJA_INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/inja/include
        PARENT_SCOPE)
