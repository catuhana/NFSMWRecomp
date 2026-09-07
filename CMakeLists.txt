cmake_minimum_required(VERSION 3.29)

project(NFSMWRecomp
    VERSION 0.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINKER_TYPE LLD)

add_subdirectory(apps/NFSMWRecomp)

if(PROJECT_IS_TOP_LEVEL)
    include(cmake/link_compile_commands.cmake)
    link_compile_commands()
endif()
