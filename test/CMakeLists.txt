macro(CUnitTest name)
    add_mocked_test(${name}_C
            SOURCES ${name}_test.c
            LINK_LIBRARIES neoast
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            ENVIRONMENT ${EXEC_ENV}
            )
endmacro()


CUnitTest(lexer)
