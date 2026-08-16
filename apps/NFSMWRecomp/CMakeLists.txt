include(generated/rexglue.cmake)

add_executable(NFSMWRecomp WIN32
    src/NFSMWRecomp.cpp
)

rexglue_setup_target(NFSMWRecomp GPU_PLUGINS xenos)

# Workaround to imgui issue for now
target_link_libraries(NFSMWRecomp PRIVATE rex::ui)
