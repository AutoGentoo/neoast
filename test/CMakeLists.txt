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
        LINK_LIBRARIES neoast neoast-codegen m neoast-util
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        )

BuildParser(calculator_parser ${CMAKE_CURRENT_SOURCE_DIR}/input/calculator.y)
add_mocked_test(integration_C
        SOURCES
        input/calculator.y
        integration_test.c ${calculator_parser_OUTPUT}
        LINK_LIBRARIES neoast m
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        )