include(FetchContent)

include(cmake/merge_game_configs.cmake)

include(generated/rexglue.cmake)

FetchContent_Declare(NFDe
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG v1.3.0
)
FetchContent_MakeAvailable(NFDe)

FetchContent_Declare(beman.cstring_view
    GIT_REPOSITORY https://github.com/bemanproject/cstring_view
    GIT_TAG bee112df1e994ab842556677d698428c3edbf713
)
FetchContent_MakeAvailable(beman.cstring_view)

add_executable(NFSMWRecomp WIN32
    src/Patches/PostProcessing.cpp

    src/UI/ISOExtractDialog.cpp

    src/NFSMWRecomp.cpp
)

rexglue_setup_target(NFSMWRecomp GPU_PLUGINS xenos)

target_link_libraries(NFSMWRecomp PRIVATE nfd beman::cstring_view)
# Workaround to imgui issue for now
target_link_libraries(NFSMWRecomp PRIVATE rex::ui)

merge_game_configs(
    TARGET NFSMWRecomp
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/header.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/runtime.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/keyboard_binds.toml
    OUTPUT_FILE $<TARGET_FILE_DIR:NFSMWRecomp>/NFSMWRecomp.toml
)
