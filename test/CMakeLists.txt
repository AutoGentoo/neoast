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

add_mocked_test(tablegen_C
        SOURCES tablegen_test.c
        LINK_LIBRARIES neoast neoast-codegen m
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        )
