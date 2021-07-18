function(BuildParser target input_file)
    if (DEFINED ARGV2)
        set(language ${ARGV2})
    else()
        set(language "C")
    endif()

    set(neoast_HEADER ${CMAKE_CURRENT_BINARY_DIR}/neoast_parser__${target}.h)
    if ("${language}" STREQUAL "C" )
        message(STATUS "Building ${target} parser in C")
        set(neoast_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/neoast_parser__${target}.c)
    elseif("${language}" STREQUAL "CXX")
        message(STATUS "Building ${target} parser in C++")
        set(neoast_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/neoast_parser__${target}.cc)
    else()
        message(FATAL "Invalid language output for parser ${language}: C, CXX are allowed")
    endif()

    set(${target}_OUTPUT ${neoast_OUTPUT} PARENT_SCOPE)

    add_custom_command(OUTPUT ${neoast_OUTPUT}
            COMMAND $<TARGET_FILE:neoast-exec>
            ARGS ${input_file} ${neoast_OUTPUT} ${neoast_HEADER}
            DEPENDS ${input_file} neoast-exec
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
endfunction()
