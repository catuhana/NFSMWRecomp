if(CMAKE_SCRIPT_MODE_FILE)
    set(MERGED "")

    foreach(file IN LISTS CONFIG_FILES)
        file(READ "${file}" content)
        string(APPEND MERGED "${content}")
    endforeach()

    file(WRITE "${OUTPUT_FILE}" "${MERGED}")
else()
    function(merge_game_configs)
        set(oneValueArgs TARGET OUTPUT_FILE)
        set(multiValueArgs FILES)

        cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        add_custom_command(
            TARGET ${ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                "-DCONFIG_FILES=${ARG_FILES}"
                -DOUTPUT_FILE=${ARG_OUTPUT_FILE}
                -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
            COMMENT "Merge game configuration files into ${ARG_OUTPUT_FILE}"
            VERBATIM
        )
    endfunction()
endif()
