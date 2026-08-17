include(cmake/merge_game_configs.cmake)

include(generated/rexglue.cmake)

add_executable(NFSMWRecomp WIN32
    src/NFSMWRecomp.cpp
)

rexglue_setup_target(NFSMWRecomp GPU_PLUGINS xenos)

# Workaround to imgui issue for now
target_link_libraries(NFSMWRecomp PRIVATE rex::ui)

merge_game_configs(
    TARGET NFSMWRecomp
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/header.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/paths.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/runtime.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/keyboard_binds.toml
    OUTPUT_FILE $<TARGET_FILE_DIR:NFSMWRecomp>/NFSMWRecomp.toml
)
