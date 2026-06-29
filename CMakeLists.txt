cmake_minimum_required(VERSION 3.22)

project(Reflex LANGUAGES CXX)

# Reflex SDK entry point for FetchContent / CPM consumers.
#
# Consumed as a subproject (FetchContent_MakeAvailable / CPMAddPackage), this
# exposes the Reflex::* targets and the reflex_add_* helper functions. Prebuilt
# libraries for the host platform are downloaded from this repo's GitHub Release
# on first configure (see cmake/FetchReflexLibs.cmake) — no manual install step.
#
#   include(FetchContent)
#   FetchContent_Declare(reflex
#       GIT_REPOSITORY https://github.com/reflexplusplus/reflex_public.git
#       GIT_TAG        v0.3.1)
#   FetchContent_MakeAvailable(reflex)
#
#   reflex_add_audio_plugin(MyPlugin
#       FORMATS Standalone VST3 CLAP AU
#       SOURCES code/entry.cpp code/instance.cpp code/view.cpp
#               ${REFLEX_ROOT}/src/reflex_ext.cpp
#       NAME "My Plugin" VENDOR "My Company" VERSION "1.0.0")
#
# Consumers who already have a local SDK checkout can skip FetchContent and call
# find_package(Reflex CONFIG PATHS <sdk>/cmake) directly instead.

find_package(Reflex REQUIRED CONFIG
    PATHS "${CMAKE_CURRENT_LIST_DIR}/cmake"
    NO_DEFAULT_PATH)
