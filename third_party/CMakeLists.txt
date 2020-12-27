add_subdirectory(cmocka EXCLUDE_FROM_ALL)

set(RE2_BUILD_TESTING OFF)

add_subdirectory(re2)
add_subdirectory(cre2)
