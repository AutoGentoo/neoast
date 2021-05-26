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
add_mocked_test(macro_C
        SOURCES macro_test.c
        LINK_LIBRARIES neoast neoast-codegen
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        )

add_mocked_test(tablegen_C
        SOURCES tablegen_test.c
        LINK_LIBRARIES neoast neoast-codegen m neoast-util
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        )

BuildParser(calculator_parser input/calculator.y)
BuildParser(calculator_ascii_parser input/calculator_ascii.y)
BuildParser(simple_ast_parser input/simple_ast.y)
BuildParser(error_parser input/error_cb.y)
add_mocked_test(integration_C
        SOURCES
        input/calculator.y
        input/calculator_ascii.y
        integration_test.c
        ${calculator_parser_OUTPUT}
        ${calculator_ascii_parser_OUTPUT}
        ${simple_ast_parser_OUTPUT}
        ${error_parser_OUTPUT}
        LINK_LIBRARIES neoast m
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        )