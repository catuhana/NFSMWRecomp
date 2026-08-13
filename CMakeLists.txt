cmake_minimum_required(VERSION 3.25)
project(nfsmwrecomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

set(NFSMWRECOMP_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(nfsmwrecomp WIN32 ${NFSMWRECOMP_SOURCES})
else()
    add_executable(nfsmwrecomp ${NFSMWRECOMP_SOURCES})
endif()

rexglue_setup_target(nfsmwrecomp)
