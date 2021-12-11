macro(CUnitTest name)
    add_mocked_test(${name}_C
            SOURCES ${name}_test.c
            LINK_LIBRARIES neoast
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            ENVIRONMENT ${EXEC_ENV}
            INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src
            )
endmacro()

add_mocked_test(bootstrap_lexer_C
        SOURCES bootstrap_lexer_test.c
        LINK_LIBRARIES neoast neoast-bootstrap-lexer
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src
        )

CUnitTest(neoast_lexer)

add_mocked_test(lr_C
        SOURCES lr_test.c
        LINK_LIBRARIES neoast neoast-bootstrap-lexer
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src
        )

add_mocked_test(macro_C
        SOURCES macro_test.cc
        LINK_LIBRARIES neoast neoast-codegen
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src
        )

add_mocked_test(tablegen_C
        SOURCES tablegen_test.c
        LINK_LIBRARIES neoast neoast-codegen m neoast-util neoast-bootstrap-lexer
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT ${EXEC_ENV}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src
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
        # TODO Link tests against reflex generated lexer
        LINK_LIBRARIES neoast m
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src
        ENVIRONMENT ${EXEC_ENV}
        )

add_executable(reflex_test reflex_test.cpp)
target_link_libraries(reflex_test reflex)
target_include_directories(reflex_test PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/reflex/include)