if(CMAKE_SCRIPT_MODE_FILE)
    get_filename_component(_destination_dir "${LINK_DESTINATION}" DIRECTORY)
    file(MAKE_DIRECTORY "${_destination_dir}")

    if(EXISTS "${LINK_DESTINATION}")
        file(REMOVE "${LINK_DESTINATION}")
    endif()

    file(CREATE_LINK "${LINK_SOURCE}" "${LINK_DESTINATION}"
        SYMBOLIC
        COPY_ON_ERROR
        RESULT _result
    )

    if(NOT _result EQUAL 0)
        message(WARNING "Could not link compile_commands.json: ${_result}")
    endif()
else()
    function(link_compile_commands)
        set(oneValueArgs SOURCE DESTINATION)
        cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

        if(NOT ARG_SOURCE)
            set(ARG_SOURCE "${CMAKE_BINARY_DIR}/compile_commands.json")
        endif()
        if(NOT ARG_DESTINATION)
            set(ARG_DESTINATION "${CMAKE_SOURCE_DIR}/build/compile_commands.json")
        endif()

        if(NOT TARGET link_compile_commands)
            add_custom_target(link_compile_commands ALL
                COMMAND ${CMAKE_COMMAND}
                    -DLINK_SOURCE=${ARG_SOURCE}
                    -DLINK_DESTINATION=${ARG_DESTINATION}
                    -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
                COMMENT "Pointing root compile_commands.json at ${ARG_SOURCE}"
                VERBATIM
            )
        endif()
    endfunction()
endif()
