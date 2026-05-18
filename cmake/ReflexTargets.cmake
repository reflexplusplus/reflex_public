# ReflexTargets.cmake - IMPORTED library targets for Reflex SDK
#
# This file creates CMake IMPORTED targets for linking against Reflex libraries.
# It is included by ReflexConfig.cmake and should not be used directly.

# Platform-specific system dependencies
if(REFLEX_PLATFORM STREQUAL "macos")
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    find_library(DISKARBITRATION_FRAMEWORK DiskArbitration REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(COREMIDI_FRAMEWORK CoreMIDI REQUIRED)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    set(_REFLEX_PLATFORM_LIBS
        ${COCOA_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${DISKARBITRATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREAUDIO_FRAMEWORK}
        ${COREMIDI_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    )
elseif(REFLEX_PLATFORM STREQUAL "ios")
    find_library(UIKIT_FRAMEWORK UIKit REQUIRED)
    find_library(OPENGLES_FRAMEWORK OpenGLES REQUIRED)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    set(_REFLEX_PLATFORM_LIBS
        ${UIKIT_FRAMEWORK}
        ${OPENGLES_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    )
elseif(REFLEX_PLATFORM STREQUAL "win")
    # Windows libraries are auto-linked via #pragma comment(lib, ...)
    set(_REFLEX_PLATFORM_LIBS "")
elseif(REFLEX_PLATFORM STREQUAL "linux")
    find_package(Threads REQUIRED)
    find_package(CURL REQUIRED)
    set(_REFLEX_PLATFORM_LIBS
        Threads::Threads
        CURL::libcurl
    )
    set(_REFLEX_PLATFORM_UI_LIBS
        wayland-client
        wayland-egl
        EGL
        GLESv2
    )
endif()

# Helper function to create an IMPORTED static library target
function(_reflex_create_imported_lib target_name lib_name)
    if(TARGET reflex::${target_name})
        return()
    endif()

    add_library(reflex::${target_name} STATIC IMPORTED GLOBAL)

    # Set library location based on platform
    get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(_is_multi_config)
        # Multi-config generators: set per-configuration locations
        if(REFLEX_PLATFORM STREQUAL "macos" OR REFLEX_PLATFORM STREQUAL "ios")
            set_target_properties(reflex::${target_name} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${REFLEX_DIR}/bin/lib/${REFLEX_PLATFORM}/Debug/lib${lib_name}.a"
                IMPORTED_LOCATION_RELEASE "${REFLEX_DIR}/bin/lib/${REFLEX_PLATFORM}/Release/lib${lib_name}.a"
                IMPORTED_LOCATION_MINSIZEREL "${REFLEX_DIR}/bin/lib/${REFLEX_PLATFORM}/Release/lib${lib_name}.a"
                IMPORTED_LOCATION_RELWITHDEBINFO "${REFLEX_DIR}/bin/lib/${REFLEX_PLATFORM}/Release/lib${lib_name}.a"
            )
        elseif(REFLEX_PLATFORM STREQUAL "linux")
            set_target_properties(reflex::${target_name} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${REFLEX_DIR}/bin/lib/linux/Debug/lib${lib_name}.a"
                IMPORTED_LOCATION_RELEASE "${REFLEX_DIR}/bin/lib/linux/Release/lib${lib_name}.a"
                IMPORTED_LOCATION_MINSIZEREL "${REFLEX_DIR}/bin/lib/linux/Release/lib${lib_name}.a"
                IMPORTED_LOCATION_RELWITHDEBINFO "${REFLEX_DIR}/bin/lib/linux/Release/lib${lib_name}.a"
            )
        elseif(REFLEX_PLATFORM STREQUAL "win")
            set_target_properties(reflex::${target_name} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${REFLEX_DIR}/bin/lib/win/Debug/${REFLEX_ARCH}/${lib_name}.lib"
                IMPORTED_LOCATION_RELEASE "${REFLEX_DIR}/bin/lib/win/Release/${REFLEX_ARCH}/${lib_name}.lib"
                IMPORTED_LOCATION_MINSIZEREL "${REFLEX_DIR}/bin/lib/win/Release/${REFLEX_ARCH}/${lib_name}.lib"
                IMPORTED_LOCATION_RELWITHDEBINFO "${REFLEX_DIR}/bin/lib/win/Release/${REFLEX_ARCH}/${lib_name}.lib"
            )
        endif()
    else()
        # Single-config generators: use CMAKE_BUILD_TYPE
        if(REFLEX_PLATFORM STREQUAL "macos" OR REFLEX_PLATFORM STREQUAL "ios")
            set(_lib_path "${REFLEX_DIR}/bin/lib/${REFLEX_PLATFORM}/${CMAKE_BUILD_TYPE}/lib${lib_name}.a")
        elseif(REFLEX_PLATFORM STREQUAL "linux")
            set(_lib_path "${REFLEX_DIR}/bin/lib/linux/${CMAKE_BUILD_TYPE}/lib${lib_name}.a")
        elseif(REFLEX_PLATFORM STREQUAL "win")
            set(_lib_path "${REFLEX_DIR}/bin/lib/win/${CMAKE_BUILD_TYPE}/${REFLEX_ARCH}/${lib_name}.lib")
        endif()
        set_target_properties(reflex::${target_name} PROPERTIES
            IMPORTED_LOCATION "${_lib_path}"
        )
    endif()

    set_target_properties(reflex::${target_name} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${REFLEX_INCLUDE_DIR}"
    )
endfunction()

# Create all library targets
_reflex_create_imported_lib(ReflexCommon ReflexCommon)
_reflex_create_imported_lib(ReflexCommonUi ReflexCommonUi)
_reflex_create_imported_lib(ReflexCommonVm ReflexCommonVm)
_reflex_create_imported_lib(ReflexCommonVmUi ReflexCommonVmUi)
_reflex_create_imported_lib(ReflexTargetApp ReflexTargetApp)
_reflex_create_imported_lib(ReflexTargetConsole ReflexTargetConsole)
_reflex_create_imported_lib(ReflexTargetAudioApp ReflexTargetAudioApp)
_reflex_create_imported_lib(ReflexTargetAU ReflexTargetAU)
_reflex_create_imported_lib(ReflexTargetVST2 ReflexTargetVST2)
_reflex_create_imported_lib(ReflexTargetVST3 ReflexTargetVST3)
_reflex_create_imported_lib(ReflexTargetDynamicLibrary ReflexTargetDynamicLibrary)

if(REFLEX_PLATFORM STREQUAL "linux")
    foreach(_linux_target reflex::ReflexTargetConsole reflex::ReflexTargetLibrary reflex::ReflexTargetDynamicLibrary reflex::ReflexTargetAudioApp reflex::ReflexTargetApp)
        if(TARGET ${_linux_target})
            set_property(TARGET ${_linux_target} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${_REFLEX_PLATFORM_LIBS}"
            )
        endif()
    endforeach()

    foreach(_linux_ui_target reflex::ReflexTargetApp reflex::ReflexTargetAudioApp)
        if(TARGET ${_linux_ui_target})
            set_property(TARGET ${_linux_ui_target} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${_REFLEX_PLATFORM_UI_LIBS}"
            )
        endif()
    endforeach()
elseif(NOT REFLEX_PLATFORM STREQUAL "win")
    set_property(TARGET reflex::ReflexCommon APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${_REFLEX_PLATFORM_LIBS}"
    )
endif()

# Define library sets for different target types
set(REFLEX_FULL_LIBS
    reflex::ReflexCommonVmUi
    reflex::ReflexCommonVm
    reflex::ReflexCommonUi
    reflex::ReflexCommon
)

# Console apps only need the base library
set(REFLEX_CONSOLE_LIBS
    reflex::ReflexCommon
)

# Main helper function for configuring user targets
function(reflex_configure_target target_name target_type)
    # Require C++20
    target_compile_features(${target_name} PRIVATE cxx_std_20)

    # Platform-specific compile options
    if(REFLEX_PLATFORM STREQUAL "macos" OR REFLEX_PLATFORM STREQUAL "ios")
        target_compile_options(${target_name} PRIVATE
            -Werror
            -Wno-ambiguous-reversed-operator
            -Wno-deprecated-volatile
            -Wno-switch
            -Wno-uninitialized
            -Wno-invalid-offsetof
            -fno-rtti
        )
    elseif(REFLEX_PLATFORM STREQUAL "win")
        target_compile_definitions(${target_name} PRIVATE
            UNICODE
            _UNICODE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
        target_compile_options(${target_name} PRIVATE
            /GR-        # Disable RTTI
            /fp:fast    # Fast floating point
            /MP         # Multi-processor compilation
            /W3         # Warning level 3
        )
    endif()

    # Set bootstrap type based on target type
    string(TOUPPER "${target_type}" _type_upper)
    target_compile_definitions(${target_name} PRIVATE
        REFLEX_BOOTSTRAP_TYPE_${_type_upper}
    )

    # Link libraries based on target type
    if(target_type STREQUAL "CONSOLE_APP")
        # Console apps only need ReflexCommon + ReflexTargetConsole
        target_link_libraries(${target_name} PRIVATE
            reflex::ReflexTargetConsole
            reflex::ReflexCommon
        )
    else()
        # GUI apps need full library set (order matters - dependencies last)
        target_link_libraries(${target_name} PRIVATE ${REFLEX_FULL_LIBS})

        # Add target-specific library
        if(target_type STREQUAL "APP")
            target_link_libraries(${target_name} PRIVATE reflex::ReflexTargetApp)
        elseif(target_type STREQUAL "AUDIOAPP")
            target_link_libraries(${target_name} PRIVATE reflex::ReflexTargetAudioApp)
        elseif(target_type STREQUAL "AUDIOPLUGIN")
            target_link_libraries(${target_name} PRIVATE reflex::ReflexTargetAudioApp)
        elseif(target_type STREQUAL "AU")
            target_link_libraries(${target_name} PRIVATE reflex::ReflexTargetAU)
        elseif(target_type STREQUAL "VST2")
            target_link_libraries(${target_name} PRIVATE reflex::ReflexTargetVST2)
        elseif(target_type STREQUAL "VST3")
            target_link_libraries(${target_name} PRIVATE reflex::ReflexTargetVST3)
        endif()
    endif()
endfunction()

# Helper function to add reflex_ext.cpp to a target
function(reflex_add_extension_source target_name)
    target_sources(${target_name} PRIVATE "${REFLEX_SRC_DIR}/reflex_ext.cpp")
endfunction()

# Helper function to run ReflexResourceBuilder as a pre-build step
# Note: On macOS, ReflexResourceBuilder is distributed as a DMG. You can either:
#   1. Mount the DMG and set REFLEX_RESOURCE_BUILDER_EXE to the app path
#   2. Use the update-resources script from an existing project template
#   3. Skip this step (resources.cpp already contains pre-built resources)
function(reflex_build_resources target_name resources_xml)
    if(REFLEX_PLATFORM STREQUAL "win")
        if(EXISTS "${REFLEX_RESOURCE_BUILDER}")
            add_custom_command(
                TARGET ${target_name} PRE_BUILD
                COMMAND "${REFLEX_RESOURCE_BUILDER}" "${resources_xml}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                COMMENT "Building Reflex resources..."
                VERBATIM
            )
        endif()
    elseif(REFLEX_PLATFORM STREQUAL "macos" OR REFLEX_PLATFORM STREQUAL "ios")
        # Check if user has set the actual executable path
        if(DEFINED REFLEX_RESOURCE_BUILDER_EXE AND EXISTS "${REFLEX_RESOURCE_BUILDER_EXE}")
            add_custom_command(
                TARGET ${target_name} PRE_BUILD
                COMMAND "${REFLEX_RESOURCE_BUILDER_EXE}" "${resources_xml}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                COMMENT "Building Reflex resources..."
                VERBATIM
            )
        else()
            message(STATUS "Note: ReflexResourceBuilder not configured. Using pre-built resources.")
        endif()
    endif()
endfunction()
