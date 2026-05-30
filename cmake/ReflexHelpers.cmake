# ReflexHelpers.cmake
# Included automatically by ReflexConfig.cmake.
#
# Public API:
#   reflex_add_app(target SOURCES ... NAME ... VENDOR ...)
#   reflex_add_vm_app(target SOURCES ... NAME ... VENDOR ...)
#   reflex_add_audio_plugin(target SOURCES ... FORMATS ... NAME ... VENDOR ... VERSION ...)
#   reflex_add_console_app(target SOURCES ... NAME ... VENDOR ...)

# =========================================================
# Internal: apply MSVC compiler/linker settings to match the VS project template
# =========================================================
# Mirrors the settings in templates/CppApp/project/win/vcxproj/Common.props
# and App.vcxproj. No-op on non-MSVC compilers.

function(_reflex_apply_msvc_options target)

    if(NOT MSVC)
        return()
    endif()

    # ---- Compile options (all configs) ----
    target_compile_options(${target} PRIVATE
        /W3         # Warning level 3
        /fp:fast    # Fast floating point
        /GF         # String pooling
        /MP         # Multi-processor compilation
    )

    # ---- Unicode ----
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE)

    # ---- Debug compile options ----
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Debug>:/Od>      # Disable optimisation
    )

    # ---- Release compile options ----
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Release>:/O2>    # Full optimisation
        $<$<CONFIG:Release>:/Ob2>   # Inline any suitable
        $<$<CONFIG:Release>:/Oi>    # Intrinsic functions
        $<$<CONFIG:Release>:/Ot>    # Favour speed
        $<$<CONFIG:Release>:/Oy>    # Omit frame pointers
        $<$<CONFIG:Release>:/Gy>    # Function-level linking
        $<$<CONFIG:Release>:/GR->   # Disable RTTI
    )

    # ---- SSE2 on 32-bit Release ----
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Release>:/arch:SSE2>
        )
    endif()

    # ---- Linker options ----
    target_link_options(${target} PRIVATE
        /OPT:REF                        # Remove unreferenced functions/data
        /OPT:ICF                        # Identical COMDAT folding
        $<$<CONFIG:Debug>:/DEBUG>       # Debug info in Debug builds
        $<$<CONFIG:Release>:/RELEASE>   # No debug info in Release builds
    )

    # ---- Windows resource file (icons, version info, manifests) ----
    set(_res "${REFLEX_ROOT}/resources/win/reflex_system.res")
    if(EXISTS "${_res}")
        target_sources(${target} PRIVATE "${_res}")
    endif()

endfunction()


# =========================================================
# Internal: pre-build resource compilation step
# =========================================================
# Registers a PRE_BUILD command to run ReflexResourceBuilder on resources.xml.
# On macOS/iOS the tool ships inside a DMG; on Windows/Linux it is a plain
# executable.  Silently skips if the tool/DMG is not present (so configure
# succeeds on machines where resources haven't been regenerated yet).

function(_reflex_add_resource_build target resources_xml)

    if(WIN32)

        set(_tool "${REFLEX_ROOT}/bin/tools/win/ReflexResourceBuilder.exe")
        if(NOT EXISTS "${_tool}")
            return()  # tool not in this distribution, skip silently
        endif()
        add_custom_command(TARGET ${target} PRE_BUILD
            COMMAND "${_tool}" "${resources_xml}"
            COMMENT "[Reflex] Building resources: ${resources_xml}"
            VERBATIM
        )

    elseif(APPLE)

		set(_dmg "${REFLEX_ROOT}/bin/tools/macos/ReflexResourceBuilder.dmg")
		if(NOT EXISTS "${_dmg}")
			return()
		endif()

		if(CMAKE_GENERATOR STREQUAL "Xcode")
			# Xcode supports PRE_BUILD and renders it as an explicit build phase.
			add_custom_command(TARGET ${target} PRE_BUILD
				COMMAND /bin/sh -c
				"set -euo pipefail
		DMG=\"${_dmg}\"
		XML=\"$1\"
		MOUNT_POINT=\"\"
		cleanup() { if [ -n \"${MOUNT_POINT}\" ]; then hdiutil detach \"${MOUNT_POINT}\" -quiet || true; fi; }
		trap cleanup EXIT
		MOUNT_POINT=\$(hdiutil attach -nobrowse -readonly \"${_dmg}\" | awk '/\\/Volumes\\// {print \$3; exit 0}')
		\"\${MOUNT_POINT}/ReflexResourceBuilder.app/Contents/MacOS/ReflexResourceBuilder\" \"\${XML}\"
		"
				-- "${resources_xml}"
				COMMENT "[Reflex] Building resources: ${resources_xml}"
				VERBATIM
			)
		else()
			set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_resources.stamp")
			set(_script "${CMAKE_CURRENT_BINARY_DIR}/${target}_build_resources.sh")

			# Write the build script at configure time to avoid shell quoting
			# issues with VERBATIM + multi-line /bin/sh -c commands.
			file(WRITE "${_script}"
"#!/bin/sh
set -euo pipefail
DMG=\"${_dmg}\"
XML=\"$1\"
MOUNT_POINT=\"\"
cleanup() { if [ -n \"$MOUNT_POINT\" ]; then hdiutil detach \"$MOUNT_POINT\" -quiet || true; fi; }
trap cleanup EXIT
MOUNT_POINT=$(hdiutil attach -nobrowse -readonly \"$DMG\" | awk '/\\/Volumes\\// {print $3; exit 0}')
\"$MOUNT_POINT/ReflexResourceBuilder.app/Contents/MacOS/ReflexResourceBuilder\" \"$XML\"
")

			add_custom_command(
				OUTPUT "${_stamp}"
				COMMAND /bin/sh "${_script}" "${resources_xml}"
				COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
				DEPENDS "${resources_xml}" "${_dmg}"
				COMMENT "[Reflex] Building resources: ${resources_xml}"
				VERBATIM
			)

			add_custom_target(${target}__reflex_resources DEPENDS "${_stamp}")
			add_dependencies(${target} ${target}__reflex_resources)
		endif()

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

        set(_tool "${REFLEX_ROOT}/bin/tools/linux/ReflexResourceBuilder")
        if(NOT EXISTS "${_tool}")
            return()  # tool not in this distribution, skip silently
        endif()
        add_custom_command(TARGET ${target} PRE_BUILD
            COMMAND "${_tool}" "${resources_xml}"
            COMMENT "[Reflex] Building resources: ${resources_xml}"
            VERBATIM
        )

    endif()

endfunction()


# =========================================================
# Internal: apply Apple (macOS/iOS) compiler/linker settings
# =========================================================
# Mirrors the XCBuildConfiguration settings in the Xcode project templates.
# macOS: templates/CppApp/project/macos/...project.pbxproj
# iOS:   templates/CppApp/project/ios/...project.pbxproj

function(_reflex_apply_apple_options target)

    if(NOT APPLE)
        return()
    endif()

    # ---- Settings common to both macOS and iOS ----

    target_compile_options(${target} PRIVATE
        -fno-rtti       # GCC_ENABLE_CPP_RTTI = NO
    )

    target_link_options(${target} PRIVATE
        -Wl,-dead_strip  # DEAD_CODE_STRIPPING = YES
    )

    # ---- Debug (both platforms) ----
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Debug>:-O0>
    )
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:DEBUG=1>
    )

    # ---- Release (both platforms) ----
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-funroll-loops>
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")

        # ---- iOS-specific ----
        # CLANG_CXX_LIBRARY not set → system default (no -stdlib=libc++)
        # OTHER_CFLAGS suppresses noisy warnings that appear on iOS SDK headers
        target_compile_options(${target} PRIVATE
            -Wno-switch
            -Wno-deprecated-volatile
            -Wno-ambiguous-reversed-operator
        )

        set_target_properties(${target} PROPERTIES
            MACOSX_DEPLOYMENT_TARGET "14.0"   # IPHONEOS_DEPLOYMENT_TARGET
        )

    else()

        # ---- macOS-specific ----
        # CLANG_CXX_LIBRARY = libc++  (static C++ runtime linkage)
        target_compile_options(${target} PRIVATE -stdlib=libc++)
        target_link_options(${target} PRIVATE -stdlib=libc++)

        target_compile_options(${target} PRIVATE
            -ffast-math     # GCC_FAST_MATH = YES
            -fpermissive    # OTHER_CPLUSPLUSFLAGS
            -fno-common     # GCC_NO_COMMON_BLOCKS = YES
        )

        set_target_properties(${target} PROPERTIES
            MACOSX_DEPLOYMENT_TARGET "11.0"
        )

    endif()

endfunction()


# =========================================================
# Internal: apply macOS app bundle settings
# =========================================================

function(_reflex_configure_macos_bundle target name vendor)
    _reflex_generate_plist(_plist ${target} "${name}" "${vendor}" "1.0.0" "app")
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE                TRUE
        MACOSX_BUNDLE_BUNDLE_NAME    "${name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.${vendor}.${name}"
        MACOSX_BUNDLE_INFO_PLIST     "${_plist}"
    )
    target_link_libraries(${target} PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework DiskArbitration"
        "-framework OpenGL"
    )
endfunction()


# =========================================================
# Internal: apply iOS app bundle settings
# =========================================================

function(_reflex_configure_ios_bundle target name vendor)
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE                TRUE
        MACOSX_BUNDLE_BUNDLE_NAME    "${name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.${vendor}.${name}"
    )
    target_link_libraries(${target} PRIVATE
        "-framework UIKit"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
        "-framework UniformTypeIdentifiers"
    )
endfunction()


# =========================================================
# reflex_add_app
# =========================================================
# Creates a GUI application target.
#
# Usage:
#   reflex_add_app(MyApp
#       SOURCES  code/app.cpp code/entry.cpp code/resources.cpp code/view.cpp
#       # Note: reflex_ext.cpp is added automatically from ${REFLEX_ROOT}/src/
#       NAME     "My App"
#       VENDOR   "My Company"
#   )

function(reflex_add_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR" "SOURCES" ${ARGN})

    if(WIN32)
        add_executable(${target} WIN32 ${A_SOURCES})
    elseif(APPLE)
        add_executable(${target} MACOSX_BUNDLE ${A_SOURCES})
    else()
        add_executable(${target} ${A_SOURCES})
    endif()

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_APP)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    _reflex_apply_msvc_options(${target})
    _reflex_apply_apple_options(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::CommonUi
        Reflex::TargetApp
    )
    # Vm / VmUi are not available on all platforms (e.g. iOS)
    if(TARGET Reflex::Vm)
        target_link_libraries(${target} PRIVATE Reflex::Vm)
    endif()
    if(TARGET Reflex::VmUi)
        target_link_libraries(${target} PRIVATE Reflex::VmUi)
    endif()

    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_macos_bundle(${target} "${A_NAME}" "${A_VENDOR}")
    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_ios_bundle(${target} "${A_NAME}" "${A_VENDOR}")
    endif()

    get_filename_component(_resources_xml
        "${CMAKE_CURRENT_SOURCE_DIR}/resources.xml" ABSOLUTE)
    _reflex_add_resource_build(${target} "${_resources_xml}")

endfunction()


# =========================================================
# reflex_add_vm_app
# =========================================================
# Creates a VM-driven GUI application target (logic in C resources,
# not compiled C++).
#
# Usage:
#   reflex_add_vm_app(MyApp
#       SOURCES  code/reflex_ext.cpp code/entry.cpp code/resources.cpp
#       NAME     "My App"
#       VENDOR   "My Company"
#   )

function(reflex_add_vm_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR" "SOURCES" ${ARGN})

    if(WIN32)
        add_executable(${target} WIN32 ${A_SOURCES})
    elseif(APPLE)
        add_executable(${target} MACOSX_BUNDLE ${A_SOURCES})
    else()
        add_executable(${target} ${A_SOURCES})
    endif()

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_VM_APP)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    _reflex_apply_msvc_options(${target})
    _reflex_apply_apple_options(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::CommonUi
        Reflex::TargetApp
    )
    # Vm / VmUi are not available on all platforms (e.g. iOS)
    if(TARGET Reflex::Vm)
        target_link_libraries(${target} PRIVATE Reflex::Vm)
    endif()
    if(TARGET Reflex::VmUi)
        target_link_libraries(${target} PRIVATE Reflex::VmUi)
    endif()

    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_macos_bundle(${target} "${A_NAME}" "${A_VENDOR}")
    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_ios_bundle(${target} "${A_NAME}" "${A_VENDOR}")
    endif()

    get_filename_component(_resources_xml
        "${CMAKE_CURRENT_SOURCE_DIR}/resources.xml" ABSOLUTE)
    _reflex_add_resource_build(${target} "${_resources_xml}")

endfunction()


# =========================================================
# Internal: generate an Info.plist with resolved values
# =========================================================
# Reads the matching template plist from resources/macos/ and replaces
# Xcode variables ($(PRODUCT_NAME) etc.) with actual values, since the
# Makefile generator doesn't resolve them.

function(_reflex_generate_plist output_var target name vendor version bundle_type)
    # Optional AU parameters: pass as 7th, 8th, 9th args
    set(_au_type_4cc "${ARGV6}")
    set(_au_uid_4cc "${ARGV7}")
    set(_au_vendor_4cc "${ARGV8}")

    # Map bundle_type to template file
    if(bundle_type STREQUAL "app")
        set(_template "${REFLEX_ROOT}/resources/macos/App.plist")
    elseif(bundle_type STREQUAL "component")
        set(_template "${REFLEX_ROOT}/resources/macos/AudioUnit.plist")
    elseif(bundle_type STREQUAL "clap")
        set(_template "${REFLEX_ROOT}/resources/macos/CLAP.plist")
    else()
        # vst3, vst — all use the same VST template
        set(_template "${REFLEX_ROOT}/resources/macos/VST.plist")
    endif()

    file(READ "${_template}" _plist_content)

    # Resolve Xcode variables
    string(REPLACE " " "" _vendor_id "${vendor}")
    string(REPLACE " " "" _name_id "${name}")

    string(REPLACE "$(PRODUCT_NAME)" "${name}" _plist_content "${_plist_content}")
    string(REPLACE "$(PRODUCT_BUNDLE_IDENTIFIER)" "com.${_vendor_id}.${_name_id}.${bundle_type}" _plist_content "${_plist_content}")
    string(REPLACE "$(CURRENT_PROJECT_VERSION)" "${version}" _plist_content "${_plist_content}")

    # Version strings (templates have hardcoded "1.0.0")
    string(REPLACE ">1.0.0<" ">${version}<" _plist_content "${_plist_content}")

    # AU-specific variable replacement
    if(_au_type_4cc AND _au_uid_4cc AND _au_vendor_4cc)
        string(REPLACE "$(AU_TYPE_4CC)" "${_au_type_4cc}" _plist_content "${_plist_content}")
        string(REPLACE "$(AU_UID_4CC)" "${_au_uid_4cc}" _plist_content "${_plist_content}")
        string(REPLACE "$(AU_VENDOR_4CC)" "${_au_vendor_4cc}" _plist_content "${_plist_content}")
        string(REPLACE "$(VENDOR_NAME)" "${vendor}" _plist_content "${_plist_content}")
        # Fix description to use product name instead of generic text
        string(REPLACE "Reflex AudioUnit Plugin" "${name}" _plist_content "${_plist_content}")
    endif()

    set(_plist_path "${CMAKE_CURRENT_BINARY_DIR}/${target}_Info.plist")
    file(WRITE "${_plist_path}" "${_plist_content}")
    set(${output_var} "${_plist_path}" PARENT_SCOPE)
endfunction()


# =========================================================
# Internal: create one plugin format target
# =========================================================

function(_reflex_add_plugin_format base_target format sources name vendor version au_type_4cc au_uid_4cc au_vendor_4cc)

    set(_t "${base_target}_${format}")

    if(format STREQUAL "APP")

        if(WIN32)
            add_executable(${_t} WIN32 ${sources})
        elseif(APPLE)
            add_executable(${_t} MACOSX_BUNDLE ${sources})
        else()
            add_executable(${_t} ${sources})
        endif()

        target_link_libraries(${_t} PRIVATE Reflex::TargetAudioApp)

        if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            target_link_libraries(${_t} PRIVATE
                "-framework Cocoa"
                "-framework DiskArbitration"
                "-framework Metal"
                "-framework QuartzCore"
                "-framework OpenGL"
                "-framework CoreMIDI"
                "-framework CoreAudio"
            )
        endif()

        if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            _reflex_configure_macos_bundle(${_t} "${name}" "${vendor}")
        elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
            _reflex_configure_ios_bundle(${_t} "${name}" "${vendor}")
        endif()

    elseif(format STREQUAL "VST3")

        if(NOT TARGET Reflex::TargetVST3)
            return()
        endif()

        add_library(${_t} MODULE ${sources})
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME      "${name}"
            PREFIX           ""
            BUNDLE_EXTENSION "vst3"
        )
        target_link_libraries(${_t} PRIVATE Reflex::TargetVST3)

        if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            target_link_libraries(${_t} PRIVATE
                "-framework Cocoa"
                "-framework DiskArbitration"
                "-framework Metal"
                "-framework QuartzCore"
                "-framework OpenGL"
                "-framework CoreMIDI"
                "-framework CoreAudio"
            )
        endif()

        if(APPLE)
            _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "vst3")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            target_link_options(${_t} PRIVATE
                "SHELL:-Wl,-exported_symbols_list,${REFLEX_ROOT}/resources/macos/VST3_exports.txt"
            )
        endif()

    elseif(format STREQUAL "CLAP")

        if(NOT TARGET Reflex::TargetCLAP)
            return()
        endif()

        add_library(${_t} MODULE ${sources})
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME      "${name}"
            PREFIX           ""
            BUNDLE_EXTENSION "clap"
        )
        target_link_libraries(${_t} PRIVATE Reflex::TargetCLAP)

        if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            target_link_libraries(${_t} PRIVATE
                "-framework Cocoa"
                "-framework DiskArbitration"
                "-framework Metal"
                "-framework QuartzCore"
                "-framework OpenGL"
                "-framework CoreMIDI"
                "-framework CoreAudio"
            )
        endif()

        if(APPLE)
            _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "clap")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            target_link_options(${_t} PRIVATE
                "SHELL:-Wl,-exported_symbols_list,${REFLEX_ROOT}/resources/macos/clap_exports.txt"
            )
        endif()

    elseif(format STREQUAL "VST2")

        if(NOT TARGET Reflex::TargetVST2)
            return()
        endif()

        add_library(${_t} MODULE ${sources})
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME "${name}"
            PREFIX      ""
        )
        target_link_libraries(${_t} PRIVATE Reflex::TargetVST2)

        if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            target_link_libraries(${_t} PRIVATE
                "-framework Cocoa"
                "-framework DiskArbitration"
                "-framework Metal"
                "-framework QuartzCore"
                "-framework OpenGL"
                "-framework CoreMIDI"
                "-framework CoreAudio"
            )
        endif()

        if(APPLE)
            _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "vst")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                BUNDLE_EXTENSION         "vst"
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            target_link_options(${_t} PRIVATE
                "SHELL:-Wl,-exported_symbols_list,${REFLEX_ROOT}/resources/macos/VST2_exports.txt"
            )
        endif()

    elseif(format STREQUAL "AUV2")

        if(NOT TARGET Reflex::TargetAUV2)
            return()
        endif()

        add_library(${_t} MODULE ${sources})
        _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "component"
            "${au_type_4cc}" "${au_uid_4cc}" "${au_vendor_4cc}")
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME              "${name}"
            PREFIX                   ""
            BUNDLE                   TRUE
            BUNDLE_EXTENSION         "component"
            MACOSX_BUNDLE_INFO_PLIST "${_plist}"
        )
        target_link_libraries(${_t} PRIVATE
            Reflex::TargetAUV2
            "-framework Cocoa"
            "-framework DiskArbitration"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework OpenGL"
            "-framework AudioUnit"
            "-framework AudioToolbox"
        )
        target_link_options(${_t} PRIVATE
            "SHELL:-Wl,-exported_symbols_list,${REFLEX_ROOT}/resources/macos/audiounit_exports.txt"
        )

    else()
        message(WARNING "Reflex: unknown plugin format '${format}' — skipped")
        return()
    endif()

    # Common settings for all formats
    target_compile_definitions(${_t} PRIVATE REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
    set_target_properties(${_t} PROPERTIES
        CXX_STANDARD 20
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    _reflex_apply_msvc_options(${_t})
    _reflex_apply_apple_options(${_t})

    # Link order matters — higher-level libraries first, dependencies last
    if(TARGET Reflex::VmUi)
        target_link_libraries(${_t} PRIVATE Reflex::VmUi)
    endif()
    if(TARGET Reflex::Vm)
        target_link_libraries(${_t} PRIVATE Reflex::Vm)
    endif()
    target_link_libraries(${_t} PRIVATE
        Reflex::CommonUi
        Reflex::Common
    )

    get_filename_component(_resources_xml
        "${CMAKE_CURRENT_SOURCE_DIR}/resources.xml" ABSOLUTE)
    _reflex_add_resource_build(${_t} "${_resources_xml}")

    # Optional: copy built plugins to user plugin folders after build
    if(REFLEX_COPY_PLUGINS_AFTER_BUILD AND APPLE AND NOT format STREQUAL "APP")
        set(_user_plugins "$ENV{HOME}/Library/Audio/Plug-Ins")
        if(format STREQUAL "VST3")
            set(_dest "${_user_plugins}/VST3/${name}.vst3")
        elseif(format STREQUAL "CLAP")
            set(_dest "${_user_plugins}/CLAP/${name}.clap")
        elseif(format STREQUAL "AUV2")
            set(_dest "${_user_plugins}/Components/${name}.component")
        elseif(format STREQUAL "VST2")
            set(_dest "${_user_plugins}/VST/${name}.vst")
        else()
            set(_dest "")
        endif()
        if(_dest)
            add_custom_command(TARGET ${_t} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "$<TARGET_BUNDLE_DIR:${_t}>" "${_dest}"
                COMMENT "Copying ${format} to ${_dest}"
            )
        endif()
    endif()

endfunction()


# =========================================================
# reflex_add_audio_plugin
# =========================================================
# Creates one target per requested format. Formats not available on
# the current platform are silently skipped.
#
# Usage:
#   reflex_add_audio_plugin(MyPlugin
#       FORMATS  APP VST3 CLAP VST2 AUV2
#       SOURCES  code/entry.cpp code/instance.cpp code/resources.cpp code/view.cpp
#       # Note: reflex_ext.cpp is added automatically from ${REFLEX_ROOT}/src/
#       NAME     "My Plugin"
#       VENDOR   "My Company"
#       VERSION  "1.0.0"
#       # AU-specific (required for AUV2 format):
#       AU_TYPE_4CC    "aumf"      # aumu=instrument, aumf=MIDI processor, aufx=effect
#       AU_UID_4CC     "ES2M"      # 4-char plugin code
#       AU_VENDOR_4CC  "NdAu"      # 4-char vendor code
#   )

function(reflex_add_audio_plugin target)
    cmake_parse_arguments(A ""
        "NAME;VENDOR;VERSION;AU_TYPE_4CC;AU_UID_4CC;AU_VENDOR_4CC"
        "FORMATS;SOURCES" ${ARGN})

    if(NOT A_FORMATS)
        message(WARNING "reflex_add_audio_plugin: no FORMATS specified for '${target}'")
        return()
    endif()

    foreach(_fmt IN LISTS A_FORMATS)
        _reflex_add_plugin_format(
            ${target} ${_fmt} "${A_SOURCES}"
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
            "${A_AU_TYPE_4CC}" "${A_AU_UID_4CC}" "${A_AU_VENDOR_4CC}"
        )
    endforeach()

endfunction()


# =========================================================
# reflex_add_console_app
# =========================================================
# Creates a console (CLI) application target.
#
# Usage:
#   reflex_add_console_app(MyTool
#       SOURCES  code/entry.cpp code/main.cpp
#       NAME     "My Tool"
#       VENDOR   "My Company"
#   )

function(reflex_add_console_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR" "SOURCES" ${ARGN})

    add_executable(${target} ${A_SOURCES})

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    _reflex_apply_msvc_options(${target})
    _reflex_apply_apple_options(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::TargetConsole
    )

    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        target_link_libraries(${target} PRIVATE
            "-framework Cocoa"
            "-framework DiskArbitration"
        )
    endif()

endfunction()
