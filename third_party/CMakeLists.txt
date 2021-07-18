if (ENABLE_NEOAST_TESTS)
    add_subdirectory(cmocka EXCLUDE_FROM_ALL)
endif()

add_subdirectory(reflex)
add_subdirectory(inja)
