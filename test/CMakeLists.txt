macro(CUnitTest name)
    add_mocked_test(${name}_C
            SOURCES ${name}_test.c
            LINK_LIBRARIES neoast neoast-codegen
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            ENVIRONMENT ${EXEC_ENV}
            )
endmacro()


CUnitTest(lexer)
CUnitTest(lr)
CUnitTest(tablegen)
