function(BuildParser target input_file)

    # Link the module to the python runtime
    set(neoast_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/neoast_parser__${target}.c)
    set(${target}_OUTPUT ${neoast_OUTPUT} PARENT_SCOPE)
    add_custom_command(OUTPUT ${neoast_OUTPUT}
            COMMAND $<TARGET_FILE:neoast-exec>
            ARGS ${input_file} ${neoast_OUTPUT}
            DEPENDS ${input_file} neoast-exec)
endfunction()
