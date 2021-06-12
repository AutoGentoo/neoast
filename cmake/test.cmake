function(setup_cmocka)
    include(${PROJECT_SOURCE_DIR}/third_party/cmocka/AddCMockaTest.cmake)
    include(${PROJECT_SOURCE_DIR}/cmake/AddMockedTest.cmake)
endfunction()

setup_cmocka()
enable_testing()