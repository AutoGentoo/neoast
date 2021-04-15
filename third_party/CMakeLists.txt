if (ENABLE_NEOAST_TESTS)
    add_subdirectory(cmocka EXCLUDE_FROM_ALL)
endif()

set(REFLEX_TEST OFF)

add_subdirectory(reflex)
