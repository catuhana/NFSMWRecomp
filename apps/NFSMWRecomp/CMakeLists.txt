include(FetchContent)

include(cmake/merge_game_configs.cmake)

include(generated/rexglue.cmake)

FetchContent_Declare(nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG v1.3.0
)
if(LINUX)
    set(NFD_PORTAL ON)
endif()
FetchContent_MakeAvailable(nfd)

FetchContent_Declare(beman.cstring_view
    GIT_REPOSITORY https://github.com/bemanproject/cstring_view
    GIT_TAG bee112df1e994ab842556677d698428c3edbf713
)
FetchContent_MakeAvailable(beman.cstring_view)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

add_executable(NFSMWRecomp WIN32
    src/Patches/BlackEdition.cpp
    src/Patches/PostProcessing.cpp

    src/UI/ISOExtractDialog.cpp

    src/ISOExtract.cpp
    src/NFSMWRecomp.cpp
)

if(WIN32)
    set(PACKAGING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/packaging/Windows)

    set(NFSMW_RECOMP_VERSION_RC "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    set(NFSMW_RECOMP_VERSION_MANIFEST "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0")

    configure_file(
        ${PACKAGING_DIR}/NFSMWRecomp.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/NFSMWRecomp.rc
        @ONLY
    )
    configure_file(
        ${PACKAGING_DIR}/NFSMWRecomp.manifest.in
        ${CMAKE_CURRENT_BINARY_DIR}/NFSMWRecomp.manifest
        @ONLY
    )
    
    target_include_directories(NFSMWRecomp PRIVATE
        ${PACKAGING_DIR}
    )

    target_sources(NFSMWRecomp PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/NFSMWRecomp.rc
        ${CMAKE_CURRENT_BINARY_DIR}/NFSMWRecomp.manifest
    )
endif()

rexglue_setup_target(NFSMWRecomp GPU_PLUGINS xenos)

target_link_libraries(NFSMWRecomp PRIVATE nfd beman::cstring_view)
# Workaround to imgui issue for now.
# https://github.com/rexglue/rexglue-sdk/pull/423
# should fix this I think.
target_link_libraries(NFSMWRecomp PRIVATE rex::ui)

merge_game_configs(
    TARGET NFSMWRecomp
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/header.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/runtime.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/config/game/keyboard_binds.toml
    OUTPUT_FILE $<TARGET_FILE_DIR:NFSMWRecomp>/NFSMWRecomp.toml
)
