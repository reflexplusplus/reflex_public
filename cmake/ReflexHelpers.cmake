# ReflexHelpers.cmake
# Included automatically by ReflexConfig.cmake.
#
# Public API:
#   reflex_add_app(target SOURCES ... NAME ... VENDOR ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)
#   reflex_add_vm_app(target SOURCES ... NAME ... VENDOR ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)
#   reflex_add_audio_plugin(target SOURCES ... FORMATS ... NAME ... VENDOR ... VERSION ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)
#   reflex_add_console_app(target SOURCES ... NAME ... VENDOR ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)

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
# Registers a PRE_BUILD command to run the Reflex CLI resource compiler on
# resources.xml. Silently skips if the CLI tool is not present so configure
# succeeds on machines where tools haven't been installed yet.

function(_reflex_add_resource_build target resources_xml)

    if(WIN32)

        set(_tool "${REFLEX_ROOT}/bin/tools/win/reflex.exe")
        if(NOT EXISTS "${_tool}")
            return()  # tool not in this distribution, skip silently
        endif()
        add_custom_command(TARGET ${target} PRE_BUILD
            COMMAND "${_tool}" build-resources --path "${resources_xml}"
            COMMENT "[Reflex] Building resources: ${resources_xml}"
            VERBATIM
        )

    elseif(APPLE)

		set(_tool "${REFLEX_ROOT}/bin/tools/macos/reflex")
		if(NOT EXISTS "${_tool}")
			return()
		endif()

		if(CMAKE_GENERATOR STREQUAL "Xcode")
			# Xcode supports PRE_BUILD and renders it as an explicit build phase.
			add_custom_command(TARGET ${target} PRE_BUILD
				COMMAND "${_tool}" build-resources --path "${resources_xml}"
				COMMENT "[Reflex] Building resources: ${resources_xml}"
				VERBATIM
			)
		else()
			set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_resources.stamp")
			set(_script "${CMAKE_CURRENT_BINARY_DIR}/${target}_build_resources.sh")

			# Write the build script at configure time to avoid shell quoting
			# issues with VERBATIM + argument forwarding through custom commands.
			file(WRITE "${_script}"
"#!/bin/sh
set -euo pipefail
\"${_tool}\" build-resources --path \"$1\"
")

			add_custom_command(
				OUTPUT "${_stamp}"
				COMMAND /bin/sh "${_script}" "${resources_xml}"
				COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
				DEPENDS "${resources_xml}" "${_tool}"
				COMMENT "[Reflex] Building resources: ${resources_xml}"
				VERBATIM
			)

			add_custom_target(${target}__reflex_resources DEPENDS "${_stamp}")
			add_dependencies(${target} ${target}__reflex_resources)
		endif()

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

        set(_tool "${REFLEX_ROOT}/bin/tools/linux/reflex")
        if(NOT EXISTS "${_tool}")
            return()  # tool not in this distribution, skip silently
        endif()
        add_custom_command(TARGET ${target} PRE_BUILD
            COMMAND "${_tool}" build-resources --path "${resources_xml}"
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
# Internal: create an executable target with platform-appropriate flags
# =========================================================

function(_reflex_add_executable_target target gui)
    if("${gui}" STREQUAL "TRUE")
        if(WIN32)
            add_executable(${target} WIN32 ${ARGN})
        elseif(APPLE)
            add_executable(${target} MACOSX_BUNDLE ${ARGN})
        else()
            add_executable(${target} ${ARGN})
        endif()
    else()
        add_executable(${target} ${ARGN})
    endif()
endfunction()


# =========================================================
# Internal: common target properties/options
# =========================================================

function(_reflex_init_target target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    _reflex_apply_msvc_options(${target})
    _reflex_apply_apple_options(${target})
endfunction()


# =========================================================
# Internal: resolve package identifier
# =========================================================

function(_reflex_resolve_package_id output_var vendor name package_id_vendor package_id_product)
    if(package_id_vendor AND package_id_product)
        set(_package_id "com.${package_id_vendor}.${package_id_product}")
    else()
        string(REPLACE " " "" _vendor_id "${vendor}")
        string(REPLACE " " "" _name_id "${name}")
        set(_package_id "com.${_vendor_id}.${_name_id}")
    endif()

    set(${output_var} "${_package_id}" PARENT_SCOPE)
endfunction()


# =========================================================
# Internal: apply bundle identifier to Apple bundle targets
# =========================================================

function(_reflex_set_bundle_identifier target package_id)
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "${package_id}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${package_id}"
    )
endfunction()


# =========================================================
# Internal: make Xcode macOS bundle signing behave like native app projects
# =========================================================

function(_reflex_configure_xcode_bundle_signing target)
    if(APPLE AND CMAKE_GENERATOR STREQUAL "Xcode" AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set_target_properties(${target} PROPERTIES
            XCODE_ATTRIBUTE_AD_HOC_CODE_SIGNING_ALLOWED "YES"
        )
    endif()
endfunction()


# =========================================================
# Internal: optional VM runtime linkage
# =========================================================

function(_reflex_link_optional_vm_libraries target)
    if(TARGET Reflex::Vm)
        target_link_libraries(${target} PRIVATE Reflex::Vm)
    endif()
    if(TARGET Reflex::VmUi)
        target_link_libraries(${target} PRIVATE Reflex::VmUi)
    endif()
endfunction()


# =========================================================
# Internal: Apple framework linkage helpers
# =========================================================

function(_reflex_link_apple_app_frameworks target)
    target_link_libraries(${target} PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework DiskArbitration"
    )
endfunction()

function(_reflex_link_apple_ios_frameworks target)
    target_link_libraries(${target} PRIVATE
        "-framework UIKit"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
        "-framework UniformTypeIdentifiers"
    )
endfunction()

function(_reflex_link_apple_audio_frameworks target)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        # iOS audio-app container: AVFoundation/AVFAudio (AVAudioEngine,
        # AVAudioSession), AudioToolbox + CoreAudioKit (AUAudioUnit hosting).
        target_link_libraries(${target} PRIVATE
            "-framework AVFoundation"
            "-framework AudioToolbox"
            "-framework CoreAudioKit"
            "-framework CoreMIDI"
            "-framework CoreAudio"
        )
    else()
        target_link_libraries(${target} PRIVATE
            "-framework Cocoa"
            "-framework DiskArbitration"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework CoreMIDI"
            "-framework CoreAudio"
        )
    endif()
endfunction()

function(_reflex_link_apple_console_frameworks target)
    target_link_libraries(${target} PRIVATE
        "-framework Cocoa"
        "-framework DiskArbitration"
    )
endfunction()


# =========================================================
# Internal: apply macOS app bundle settings
# =========================================================

function(_reflex_configure_macos_bundle target name vendor package_id_vendor package_id_product)
    _reflex_generate_plist(_plist ${target} "${name}" "${vendor}" "1.0.0" "app"
        "${package_id_vendor}" "${package_id_product}")
    _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
    _reflex_set_bundle_identifier(${target} "${_package_id}")
    _reflex_configure_xcode_bundle_signing(${target})

    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE                TRUE
        MACOSX_BUNDLE_BUNDLE_NAME    "${name}"
        MACOSX_BUNDLE_INFO_PLIST     "${_plist}"
    )
endfunction()


# =========================================================
# Internal: apply iOS app bundle settings
# =========================================================

function(_reflex_configure_ios_bundle target name vendor package_id_vendor package_id_product)
    _reflex_generate_plist(_plist ${target} "${name}" "${vendor}" "1.0.0" "app"
        "${package_id_vendor}" "${package_id_product}")
    _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
    _reflex_set_bundle_identifier(${target} "${_package_id}")

    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE                TRUE
        MACOSX_BUNDLE_BUNDLE_NAME    "${name}"
        # iOS App.plist carries the UIApplicationSceneManifest — required or the
        # scene-based app launches with no scene and hangs at a blank screen.
        MACOSX_BUNDLE_INFO_PLIST     "${_plist}"
        # Generate a shared scheme for the host app. The AUv3 extension scheme
        # (wasCreatedForAppExtension) resolves its run destinations against the
        # host app's scheme — without it Xcode shows "No Destinations".
        XCODE_GENERATE_SCHEME        ON
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
#       PACKAGE_ID_VENDOR  "mycompany"
#       PACKAGE_ID_PRODUCT "myapp"
#   )

function(reflex_add_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT" "SOURCES" ${ARGN})

    _reflex_add_executable_target(${target} TRUE ${A_SOURCES})

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_APP)
    _reflex_init_target(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::CommonUi
        Reflex::TargetApp
    )
    _reflex_link_optional_vm_libraries(${target})

    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_macos_bundle(${target}
            "${A_NAME}" "${A_VENDOR}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_app_frameworks(${target})
    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_ios_bundle(${target}
            "${A_NAME}" "${A_VENDOR}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_ios_frameworks(${target})
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
#       PACKAGE_ID_VENDOR  "mycompany"
#       PACKAGE_ID_PRODUCT "myapp"
#   )

function(reflex_add_vm_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT" "SOURCES" ${ARGN})

    _reflex_add_executable_target(${target} TRUE ${A_SOURCES})

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_VM_APP)
    _reflex_init_target(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::CommonUi
        Reflex::TargetApp
    )
    _reflex_link_optional_vm_libraries(${target})

    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_macos_bundle(${target}
            "${A_NAME}" "${A_VENDOR}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_app_frameworks(${target})
    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_configure_ios_bundle(${target}
            "${A_NAME}" "${A_VENDOR}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_ios_frameworks(${target})
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
    # Optional package/AU parameters:
    # pass package id vendor/product as 7th/8th args,
    # then AU values as 9th/10th/11th args
    set(_package_id_vendor "${ARGV6}")
    set(_package_id_product "${ARGV7}")
    set(_au_type_4cc "${ARGV8}")
    set(_au_uid_4cc "${ARGV9}")
    set(_au_vendor_4cc "${ARGV10}")
    set(_au_tag "${ARGV11}")

    # Map bundle_type to template file
    if(bundle_type STREQUAL "app")
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            # iOS needs the UIApplicationSceneManifest (UISceneDelegate) plist,
            # else the scene-based app launches with no scene and hangs.
            set(_template "${REFLEX_ROOT}/resources/ios/App.plist")
        else()
            set(_template "${REFLEX_ROOT}/resources/macos/App.plist")
        endif()
    elseif(bundle_type STREQUAL "component")
        set(_template "${REFLEX_ROOT}/resources/macos/AudioUnit.plist")
    elseif(bundle_type STREQUAL "clap")
        set(_template "${REFLEX_ROOT}/resources/macos/CLAP.plist")
    elseif(bundle_type STREQUAL "auv3")
        set(_template "${REFLEX_ROOT}/resources/macos/AUv3.plist")
    else()
        # vst3, vst — all use the same VST template
        set(_template "${REFLEX_ROOT}/resources/macos/VST.plist")
    endif()

    file(READ "${_template}" _plist_content)

    # Resolve Xcode variables
    _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${_package_id_vendor}" "${_package_id_product}")

    if(bundle_type STREQUAL "app")
        set(_bundle_id "${_package_id}")
    else()
        set(_bundle_id "${_package_id}.${bundle_type}")
    endif()

    string(REPLACE "$(PRODUCT_NAME)" "${name}" _plist_content "${_plist_content}")
    string(REPLACE "$(PRODUCT_BUNDLE_IDENTIFIER)" "${_bundle_id}" _plist_content "${_plist_content}")
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

    # AUv3 AudioComponents tag (MIDI / Synthesizer / Effects)
    if(_au_tag)
        string(REPLACE "$(AU_TAG)" "${_au_tag}" _plist_content "${_plist_content}")
    endif()

    set(_plist_path "${CMAKE_CURRENT_BINARY_DIR}/${target}_Info.plist")
    file(WRITE "${_plist_path}" "${_plist_content}")
    set(${output_var} "${_plist_path}" PARENT_SCOPE)
endfunction()


# =========================================================
# Internal: create one plugin format target
# =========================================================

function(_reflex_add_plugin_format base_target format sources name vendor version package_id_vendor package_id_product au_type_4cc au_uid_4cc au_vendor_4cc)

    set(_t "${base_target}_${format}")

    if(format STREQUAL "Standalone")

        if(WIN32)
            add_executable(${_t} WIN32 ${sources})
        elseif(APPLE)
            add_executable(${_t} MACOSX_BUNDLE ${sources})
        else()
            add_executable(${_t} ${sources})
        endif()

        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME "${name}"
        )
        target_link_libraries(${_t} PRIVATE Reflex::TargetAudioApp)

        # Audio frameworks on both macOS and iOS (the helper is platform-aware).
        if(APPLE)
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            _reflex_configure_macos_bundle(${_t}
                "${name}" "${vendor}"
                "${package_id_vendor}" "${package_id_product}"
            )
        elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
            _reflex_configure_ios_bundle(${_t}
                "${name}" "${vendor}"
                "${package_id_vendor}" "${package_id_product}"
            )
            _reflex_link_apple_ios_frameworks(${_t})
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
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(APPLE)
            _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
            _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "vst3"
                "${package_id_vendor}" "${package_id_product}")
            _reflex_set_bundle_identifier(${_t} "${_package_id}.vst3")
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
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(APPLE)
            _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
            _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "clap"
                "${package_id_vendor}" "${package_id_product}")
            _reflex_set_bundle_identifier(${_t} "${_package_id}.clap")
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
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(APPLE)
            _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
            _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "vst"
                "${package_id_vendor}" "${package_id_product}")
            _reflex_set_bundle_identifier(${_t} "${_package_id}.vst")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                BUNDLE_EXTENSION         "vst"
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            target_link_options(${_t} PRIVATE
                "SHELL:-Wl,-exported_symbols_list,${REFLEX_ROOT}/resources/macos/VST2_exports.txt"
            )
        endif()

    elseif(format STREQUAL "AU")

        if(NOT TARGET Reflex::TargetAU)
            return()
        endif()

        add_library(${_t} MODULE ${sources})
        _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
        _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "component"
            "${package_id_vendor}" "${package_id_product}"
            "${au_type_4cc}" "${au_uid_4cc}" "${au_vendor_4cc}")
        _reflex_set_bundle_identifier(${_t} "${_package_id}.component")
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME              "${name}"
            PREFIX                   ""
            BUNDLE                   TRUE
            BUNDLE_EXTENSION         "component"
            MACOSX_BUNDLE_INFO_PLIST "${_plist}"
        )
        target_link_libraries(${_t} PRIVATE
            Reflex::TargetAU
            "-framework AudioUnit"
            "-framework AudioToolbox"
        )
        _reflex_link_apple_audio_frameworks(${_t})
        target_link_options(${_t} PRIVATE
            "SHELL:-Wl,-exported_symbols_list,${REFLEX_ROOT}/resources/macos/audiounit_exports.txt"
        )

    elseif(format STREQUAL "AUv3")

        if(NOT APPLE)
            return()
        endif()

        # No prebuilt .a exists for AUv3 — build the platform-specific unity
        # TU inline. osx_auv3.mm pulls in Cocoa/AppKit; ios_auv3.mm pulls in
        # UIKit. They are mutually exclusive — building the wrong one yields
        # missing-header errors.
        if(NOT TARGET Reflex::TargetAUv3)
            if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
                set(_auv3_unity "${REFLEX_ROOT}/src/reflex/system/ios_auv3.mm")
            else()
                set(_auv3_unity "${REFLEX_ROOT}/src/reflex/system/osx_auv3.mm")
            endif()
            add_library(_ReflexSrc_TargetAUv3 STATIC "${_auv3_unity}")
            target_include_directories(_ReflexSrc_TargetAUv3 PRIVATE
                "${REFLEX_ROOT}/include"
                "${REFLEX_ROOT}/src"
            )
            set_target_properties(_ReflexSrc_TargetAUv3 PROPERTIES
                CXX_STANDARD 20
            )
            set_source_files_properties("${_auv3_unity}" PROPERTIES
                COMPILE_FLAGS "-fobjc-arc -Wno-deprecated-declarations"
            )
            _reflex_apply_apple_options(_ReflexSrc_TargetAUv3)
            add_library(Reflex::TargetAUv3 ALIAS _ReflexSrc_TargetAUv3)
        endif()

        # iOS AUv3 extensions must be MH_EXECUTE binaries (launchd spawns them
        # as independent processes via NSExtensionMain), not MH_BUNDLE. An
        # add_library(MODULE) produces MH_BUNDLE, on which codesign silently
        # drops entitlements during signing. Emit an executable and apply
        # MACOSX_BUNDLE so CMake wraps it in a .appex directory (JUCE does the
        # same). macOS AUv3 keeps using MODULE; only iOS strictly requires
        # MH_EXECUTE because of amfi.
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            add_executable(${_t} MACOSX_BUNDLE ${sources})
            target_link_options(${_t} PRIVATE
                "-e" "_NSExtensionMain"
                "-fapplication-extension"
            )
        else()
            add_library(${_t} MODULE ${sources})
        endif()

        # Derive AU tag from type
        if(au_type_4cc STREQUAL "aumi")
            set(_au_tag "MIDI")
        elseif(au_type_4cc STREQUAL "aumu")
            set(_au_tag "Synthesizer")
        else()
            set(_au_tag "Effects")
        endif()

        _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
        _reflex_generate_plist(_plist ${_t} "${name}" "${vendor}" "${version}" "auv3"
            "${package_id_vendor}" "${package_id_product}"
            "${au_type_4cc}" "${au_uid_4cc}" "${au_vendor_4cc}" "${_au_tag}")
        _reflex_set_bundle_identifier(${_t} "${_package_id}.auv3")

        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME      "${name} AUv3"
            PREFIX           ""
            BUNDLE           TRUE
            BUNDLE_EXTENSION "appex"
            MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            # Mark as an app extension so Xcode (a) embeds the provisioning
            # profile, (b) applies entitlements from the dev team, and (c)
            # links with -e _NSExtensionMain. Without this the .appex ships as
            # a generic loadable bundle with no entitlements and iOS amfi
            # rejects it at launch with ENOEXEC.
            XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
            # Do NOT let CMake generate a scheme: it emits a direct-.appex run
            # scheme Xcode refuses to launch, and ZERO_CHECK re-emits it on
            # every reconfigure, clobbering the host-app "Ask on Launch" scheme.
            XCODE_GENERATE_SCHEME OFF
        )

        # On iOS, point CODE_SIGN_ENTITLEMENTS at an explicit entitlements file
        # so Xcode attaches an entitlements blob to the signature. The
        # XCODE_PRODUCT_TYPE override sets the product type but doesn't inherit
        # its default build settings, so an explicit file is required to
        # unblock Xcode's profile-derived entitlements merge at sign time.
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            set_target_properties(${_t} PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
                    "${REFLEX_ROOT}/resources/ios/AUv3.entitlements"
            )
        endif()

        target_link_libraries(${_t} PRIVATE Reflex::TargetAUv3)
        target_link_options(${_t} PRIVATE -ObjC)

        if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            target_link_libraries(${_t} PRIVATE
                "-framework Cocoa" "-framework Metal" "-framework QuartzCore"
                "-framework DiskArbitration" "-framework OpenGL"
                "-framework CoreMIDI" "-framework CoreAudio"
                "-framework AudioToolbox" "-framework AVFoundation"
            )
        else()
            target_link_libraries(${_t} PRIVATE
                "-framework UIKit" "-framework Metal" "-framework QuartzCore"
                "-framework Foundation" "-framework UniformTypeIdentifiers"
                "-framework QuickLook"
                "-framework CoreMIDI" "-framework CoreAudio"
                "-framework CoreAudioKit"
                "-framework AudioToolbox" "-framework AVFoundation"
            )
        endif()

        # Ad-hoc sign the .appex so macOS AU host registration picks up dev
        # builds. Skipped on iOS: Xcode signs the .appex with the dev team as
        # part of the app-extension product type, and re-signing ad-hoc
        # afterwards invalidates the container .app's signature.
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            add_custom_command(TARGET ${_t} POST_BUILD
                COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:${_t}>"
                COMMENT "Ad-hoc signing AUv3 extension"
            )
        endif()

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
    if(REFLEX_COPY_PLUGINS_AFTER_BUILD AND APPLE AND NOT format STREQUAL "Standalone")
        set(_user_plugins "$ENV{HOME}/Library/Audio/Plug-Ins")
        if(format STREQUAL "VST3")
            set(_dest "${_user_plugins}/VST3/${name}.vst3")
        elseif(format STREQUAL "CLAP")
            set(_dest "${_user_plugins}/CLAP/${name}.clap")
        elseif(format STREQUAL "AU")
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
# Internal: emit a post-configure script that writes the AUv3 debug scheme
# =========================================================
# The AUv3 target has XCODE_GENERATE_SCHEME OFF (so reconfigures don't clobber
# the scheme). Xcode would otherwise auto-create a direct-.appex run scheme that
# it refuses to launch ("Direct installation of an App Extension is not
# supported"). This writes a shared scheme with the APP as the runnable and
# "Ask on Launch", mirroring the JUCE AUv3 debug workflow.

function(_reflex_generate_auv3_scheme_script name app_target auv3_target)
    set(_template "${REFLEX_ROOT}/resources/ios/AUv3Debug.xcscheme.in")
    set(_script "${CMAKE_BINARY_DIR}/generate_auv3_scheme.sh")
    file(WRITE "${_script}" "#!/bin/bash
# Auto-generated by ReflexHelpers.cmake — creates AUv3 debug scheme.
set -e
build_dir=\"\$(cd \"\$(dirname \"\$0\")\" && pwd)\"
pbx=\"\$build_dir/${CMAKE_PROJECT_NAME}.xcodeproj/project.pbxproj\"
template=\"${_template}\"

if [ ! -f \"\$pbx\" ]; then
    echo \"error: \$pbx not found — run cmake first\" >&2
    exit 1
fi
if [ ! -f \"\$template\" ]; then
    echo \"error: \$template not found\" >&2
    exit 1
fi

# Target definitions in pbxproj: '<GUID> /* TargetName */ = {' on the line
# before 'isa = PBXNativeTarget'. Extract the hex GUID from that line.
app_guid=\$(grep -B1 'isa = PBXNativeTarget' \"\$pbx\" | grep '${app_target}' | sed 's|^[[:space:]]*||; s| .*||')
auv3_guid=\$(grep -B1 'isa = PBXNativeTarget' \"\$pbx\" | grep '${auv3_target}' | sed 's|^[[:space:]]*||; s| .*||')

if [ -z \"\$app_guid\" ] || [ -z \"\$auv3_guid\" ]; then
    echo \"warning: could not find target GUIDs in pbxproj — skipping scheme\" >&2
    exit 0
fi

# Derive the container path from a CMake-generated scheme so Xcode resolves
# target references correctly. The AUv3 target has XCODE_GENERATE_SCHEME OFF
# (so reconfigures don't clobber this script's output), so read it from the APP
# scheme — which CMake still generates — rather than the (now absent) AUv3 one.
scheme_dir=\"\$build_dir/${CMAKE_PROJECT_NAME}.xcodeproj/xcshareddata/xcschemes\"
cmake_scheme=\"\$scheme_dir/${app_target}.xcscheme\"
if [ -f \"\$cmake_scheme\" ]; then
    container_path=\$(grep 'ReferencedContainer' \"\$cmake_scheme\" | head -1 | sed 's/.*container://; s/\".*//')
else
    container_path=\"${CMAKE_PROJECT_NAME}.xcodeproj\"
fi

mkdir -p \"\$scheme_dir\"
sed -e \"s|@APP_GUID@|\$app_guid|g\" \\
    -e \"s|@AUV3_GUID@|\$auv3_guid|g\" \\
    -e \"s|@APP_TARGET@|${app_target}|g\" \\
    -e \"s|@AUV3_TARGET@|${auv3_target}|g\" \\
    -e \"s|@APP_BUILDABLE_NAME@|${name}.app|g\" \\
    -e \"s|@AUV3_BUILDABLE_NAME@|${name} AUv3.appex|g\" \\
    -e \"s|@CONTAINER_PATH@|\$container_path|g\" \\
    \"\$template\" > \"\$scheme_dir/${auv3_target}.xcscheme\"
echo \"Generated AUv3 debug scheme at \$scheme_dir/${auv3_target}.xcscheme\"
")
endfunction()


# =========================================================
# reflex_add_audio_plugin
# =========================================================
# Creates one target per requested format. Formats not available on
# the current platform are silently skipped.
#
# Usage:
#   reflex_add_audio_plugin(MyPlugin
#       FORMATS  Standalone VST3 CLAP VST2 AU
#       SOURCES  code/entry.cpp code/instance.cpp code/resources.cpp code/view.cpp
#       # Note: reflex_ext.cpp is added automatically from ${REFLEX_ROOT}/src/
#       NAME     "My Plugin"
#       VENDOR   "My Company"
#       VERSION  "1.0.0"
#       PACKAGE_ID_VENDOR  "mycompany"
#       PACKAGE_ID_PRODUCT "myplugin"
#       # AU-specific (required for AU format):
#       AU_TYPE_4CC    "aumf"      # aumu=instrument, aumf=MIDI processor, aufx=effect
#       AU_UID_4CC     "ES2M"      # 4-char plugin code
#       AU_VENDOR_4CC  "NdAu"      # 4-char vendor code
#   )

function(reflex_add_audio_plugin target)
    cmake_parse_arguments(A ""
        "NAME;VENDOR;VERSION;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT;AU_TYPE_4CC;AU_UID_4CC;AU_VENDOR_4CC"
        "FORMATS;SOURCES" ${ARGN})

    if(NOT A_FORMATS)
        message(WARNING "reflex_add_audio_plugin: no FORMATS specified for '${target}'")
        return()
    endif()

    foreach(_fmt IN LISTS A_FORMATS)
        _reflex_add_plugin_format(
            ${target} ${_fmt} "${A_SOURCES}"
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
            "${A_AU_TYPE_4CC}" "${A_AU_UID_4CC}" "${A_AU_VENDOR_4CC}"
        )
    endforeach()

    # Embed the AUv3 .appex inside the standalone app bundle.
    # XCODE_EMBED_APP_EXTENSIONS (CMake 3.21+) generates a native Xcode
    # "Embed App Extensions" build phase that runs in the correct order:
    # sign extension → copy into PlugIns/ → sign container. A POST_BUILD
    # copy_directory breaks iOS because it modifies the container after Xcode
    # seals it, invalidating the code signature. JUCE embeds AUv3 the same way.
    set(_app_t  "${target}_Standalone")
    set(_auv3_t "${target}_AUv3")
    if(TARGET ${_app_t} AND TARGET ${_auv3_t})
        set_target_properties(${_app_t} PROPERTIES
            XCODE_EMBED_APP_EXTENSIONS "${_auv3_t}"
            XCODE_EMBED_APP_EXTENSIONS_CODE_SIGN_ON_COPY ON
            XCODE_EMBED_APP_EXTENSIONS_REMOVE_HEADERS_ON_COPY ON
        )

        # Emit (and, once the pbxproj exists, auto-run) a helper that writes the
        # AUv3 debug scheme with "Ask on Launch" + APP as the runnable. The AUv3
        # target has XCODE_GENERATE_SCHEME OFF, so without this Xcode's
        # auto-created direct-.appex scheme triggers "Direct installation of an
        # App Extension is not supported".
        if(CMAKE_GENERATOR STREQUAL "Xcode")
            _reflex_generate_auv3_scheme_script("${A_NAME}" ${_app_t} ${_auv3_t})
            set(_auv3_scheme_script "${CMAKE_BINARY_DIR}/generate_auv3_scheme.sh")
            if(EXISTS "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.xcodeproj/project.pbxproj")
                execute_process(COMMAND bash "${_auv3_scheme_script}")
            else()
                message(STATUS "AUv3 scheme: run 'bash ${_auv3_scheme_script}' after first configure")
            endif()
        endif()
    endif()

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
#       PACKAGE_ID_VENDOR  "mycompany"
#       PACKAGE_ID_PRODUCT "mytool"
#   )

function(reflex_add_console_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT" "SOURCES" ${ARGN})

    _reflex_add_executable_target(${target} FALSE ${A_SOURCES})

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP)
    _reflex_init_target(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::TargetConsole
    )

    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        _reflex_link_apple_console_frameworks(${target})
    endif()

endfunction()
