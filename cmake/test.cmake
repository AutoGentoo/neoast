function(setup_cmocka)
    include(${CMAKE_SOURCE_DIR}/third_party/cmocka/AddCMockaTest.cmake)
    include(${CMAKE_SOURCE_DIR}/cmake/AddMockedTest.cmake)
endfunction()

setup_cmocka()
enable_testing()