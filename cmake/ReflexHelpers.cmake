# ReflexHelpers.cmake
# Included automatically by ReflexConfig.cmake.
#
# Public API:
#   reflex_add_app(target SOURCES ... NAME ... VENDOR ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)
#   reflex_add_vm_app(target SOURCES ... NAME ... VENDOR ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)
#   reflex_add_audio_plugin(target SOURCES ... FORMATS ... NAME ... VENDOR ... VERSION ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)
#   reflex_add_console_app(target SOURCES ... NAME ... VENDOR ... PACKAGE_ID_VENDOR ... PACKAGE_ID_PRODUCT ...)

# =========================================================
# Internal: add the required Reflex Windows application resource
# =========================================================

function(_reflex_add_windows_resource target)
    if(NOT REFLEX_PLATFORM_WINDOWS)
        return()
    endif()

    set(_resource "${REFLEX_ROOT}/resources/win/reflex_system.res")
    if(NOT EXISTS "${_resource}")
        message(FATAL_ERROR
            "Reflex: required Windows application resource not found: ${_resource}")
    endif()
    target_sources(${target} PRIVATE "${_resource}")
endfunction()


# =========================================================
# Internal: resource compilation step
# =========================================================
# Builds resources during configuration so generated sources exist before
# CMake validates them, then registers the equivalent project.cfg pre-build
# action through ReflexBuild.cmake. If the CLI is absent, resource generation
# is skipped to preserve the existing optional-tool behaviour.

function(_reflex_add_resource_build target resources_xml)

    if(NOT (REFLEX_PLATFORM_WINDOWS OR REFLEX_PLATFORM_MACOS OR
            REFLEX_PLATFORM_IOS OR REFLEX_PLATFORM_LINUX))
        return()
    endif()

    reflex_get_cli(_tool)

    if(NOT EXISTS "${_tool}")
        return()  # tool not in this distribution, skip silently
    endif()

    execute_process(
        COMMAND "${_tool}" build-resources --path "${resources_xml}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    reflex_target_add_build_action(${target}
        PHASE PRE_BUILD
        NAME "[Reflex] Building resources: ${resources_xml}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND "${_tool}" build-resources --path "${resources_xml}"
        INPUTS "${resources_xml}")

endfunction()


# =========================================================
# Internal: common target properties/options
# =========================================================

function(_reflex_init_target target)
    cmake_parse_arguments(ARG "" "RELEASE_OPTIMIZATION;FLOATING_POINT;APPLE_DEPLOYMENT_TARGET;OBJC_ARC" "" ${ARGN})
    if(NOT ARG_RELEASE_OPTIMIZATION)
        set(ARG_RELEASE_OPTIMIZATION full)
    endif()
    if(NOT ARG_FLOATING_POINT)
        set(ARG_FLOATING_POINT default)
    endif()

    reflex_target_set_cpp_standard(${target} cxx20)
    reflex_target_set_runtime_library(${target} static DEBUG_CONFIGS Debug)
    reflex_target_enable_string_pooling(${target})
    reflex_target_set_warning_level(${target} standard)
    reflex_target_set_optimization(${target} none CONFIG Debug)
    reflex_target_set_optimization(${target} ${ARG_RELEASE_OPTIMIZATION} CONFIG Release)
    if(REFLEX_PLATFORM_MACOS)
        reflex_target_set_rtti(${target} ON)
    else()
        reflex_target_set_rtti(${target} OFF)
    endif()
    reflex_target_set_floating_point(${target} ${ARG_FLOATING_POINT})
    reflex_target_set_debug_information(${target} ON CONFIG Debug)
    reflex_target_set_dead_strip(${target} ON CONFIG Release)
    if(REFLEX_PLATFORM_MACOS OR REFLEX_PLATFORM_IOS)
        # Explicit arg > consumer's CMAKE_OSX_DEPLOYMENT_TARGET > SDK default
        if(NOT ARG_APPLE_DEPLOYMENT_TARGET)
            if(NOT "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "")
                set(ARG_APPLE_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}")
            elseif(REFLEX_PLATFORM_IOS)
                set(ARG_APPLE_DEPLOYMENT_TARGET 14.0)
            else()
                set(ARG_APPLE_DEPLOYMENT_TARGET 11.0)
            endif()
        endif()
        if("${ARG_OBJC_ARC}" STREQUAL "")
            if(REFLEX_PLATFORM_IOS)
                set(ARG_OBJC_ARC ON)
            else()
                set(ARG_OBJC_ARC OFF)
            endif()
        endif()
        reflex_target_set_apple_deployment_target(${target} "${ARG_APPLE_DEPLOYMENT_TARGET}")
        reflex_target_set_objc_arc(${target} ${ARG_OBJC_ARC})
    endif()
endfunction()


# =========================================================
# Internal: derive a reverse-DNS-safe identifier segment
# =========================================================
# Mirrors the CLI's PackageIdentifier generator: underscores to hyphens,
# drop anything but alphanumerics and hyphens, lowercase.

function(_reflex_sanitize_id output_var value)
    string(REPLACE "_" "-" _s "${value}")
    string(REGEX REPLACE "[^A-Za-z0-9-]" "" _s "${_s}")
    string(TOLOWER "${_s}" _s)
    set(${output_var} "${_s}" PARENT_SCOPE)
endfunction()


# =========================================================
# Internal: resolve package identifier
# =========================================================

function(_reflex_resolve_package_id output_var vendor name package_id_vendor package_id_product)
    if(package_id_vendor AND package_id_product)
        set(_package_id "com.${package_id_vendor}.${package_id_product}")
    else()
        _reflex_sanitize_id(_vendor_id "${vendor}")
        _reflex_sanitize_id(_name_id "${name}")
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
    if(REFLEX_PLATFORM_MACOS AND CMAKE_GENERATOR STREQUAL "Xcode")
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
    if(REFLEX_PLATFORM_IOS)
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

function(_reflex_configure_macos_bundle target name vendor version package_id_vendor package_id_product)
    _reflex_generate_plist(_plist ${target} "${name}" "${name}" "${vendor}" "${version}" "app"
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

function(_reflex_configure_macos_audio_bundle target name vendor version package_id_vendor package_id_product)
    _reflex_generate_plist(_plist ${target} "${name}" "${name}" "${vendor}" "${version}" "audioapp"
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

function(_reflex_configure_ios_bundle target name vendor version package_id_vendor package_id_product)
    _reflex_generate_plist(_plist ${target} "${name}" "${name}" "${vendor}" "${version}" "ios_app"
        "${package_id_vendor}" "${package_id_product}")
    _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
    _reflex_set_bundle_identifier(${target} "${_package_id}")

    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE                TRUE
        MACOSX_BUNDLE_BUNDLE_NAME    "${name}"
        # iOS app.plist carries the UIApplicationSceneManifest — required or the
        # scene-based app launches with no scene and hangs at a blank screen.
        MACOSX_BUNDLE_INFO_PLIST     "${_plist}"
        # Generate a shared scheme for the host app. The AUv3 extension scheme
        # (wasCreatedForAppExtension) resolves its run destinations against the
        # host app's scheme — without it Xcode shows "No Destinations".
        XCODE_GENERATE_SCHEME        ON
    )
endfunction()

function(_reflex_configure_ios_audio_bundle target name vendor version package_id_vendor package_id_product)
    _reflex_generate_plist(_plist ${target} "${name}" "${name}" "${vendor}" "${version}" "ios_audioapp"
        "${package_id_vendor}" "${package_id_product}")
    _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
    _reflex_set_bundle_identifier(${target} "${_package_id}")

    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE                TRUE
        MACOSX_BUNDLE_BUNDLE_NAME    "${name}"
        MACOSX_BUNDLE_INFO_PLIST     "${_plist}"
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
#       APPLE_DEPLOYMENT_TARGET "10.15"   # optional; defaults to CMAKE_OSX_DEPLOYMENT_TARGET
#   )

function(reflex_add_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR;VERSION;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT;APPLE_DEPLOYMENT_TARGET" "SOURCES" ${ARGN})

    if(NOT A_VERSION)
        set(A_VERSION "1.0.0")
    endif()

    reflex_add_target(${target} TYPE APP SOURCES ${A_SOURCES})

    # Name the binary after NAME so it matches the plist's CFBundleExecutable.
    # Without this the executable takes the CMake target name; when NAME differs
    # (e.g. contains spaces) macOS cannot find the executable and refuses to launch.
    if(A_NAME)
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "${A_NAME}")
    endif()

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_APP)
    _reflex_init_target(${target} APPLE_DEPLOYMENT_TARGET "${A_APPLE_DEPLOYMENT_TARGET}")
    _reflex_add_windows_resource(${target})

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::CommonUi
        Reflex::TargetApp
    )
    _reflex_link_optional_vm_libraries(${target})

    if(REFLEX_PLATFORM_MACOS)
        _reflex_configure_macos_bundle(${target}
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_app_frameworks(${target})
    elseif(REFLEX_PLATFORM_IOS)
        _reflex_configure_ios_bundle(${target}
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
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
#       APPLE_DEPLOYMENT_TARGET "10.15"   # optional; defaults to CMAKE_OSX_DEPLOYMENT_TARGET
#   )

function(reflex_add_vm_app target)
    # The public SDK ships without the VM, so fail here with an explanation
    # rather than deep inside the link step on a missing Reflex::Vm target.
    if(NOT TARGET Reflex::Vm)
        message(FATAL_ERROR
            "reflex_add_vm_app(${target}) requires the Reflex VM, which is not "
            "present in this SDK distribution.")
    endif()

    cmake_parse_arguments(A "" "NAME;VENDOR;VERSION;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT;APPLE_DEPLOYMENT_TARGET" "SOURCES" ${ARGN})

    if(NOT A_VERSION)
        set(A_VERSION "1.0.0")
    endif()

    reflex_add_target(${target} TYPE APP SOURCES ${A_SOURCES})

    # Name the binary after NAME so it matches the plist's CFBundleExecutable.
    # Without this the executable takes the CMake target name; when NAME differs
    # (e.g. contains spaces) macOS cannot find the executable and refuses to launch.
    if(A_NAME)
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "${A_NAME}")
    endif()

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_VM_APP)
    _reflex_init_target(${target} APPLE_DEPLOYMENT_TARGET "${A_APPLE_DEPLOYMENT_TARGET}")
    _reflex_add_windows_resource(${target})

    if(MSVC)
        target_compile_options(${target} PRIVATE /bigobj)
    endif()

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::CommonUi
        Reflex::TargetApp
    )
    _reflex_link_optional_vm_libraries(${target})

    if(REFLEX_PLATFORM_MACOS)
        _reflex_configure_macos_bundle(${target}
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_app_frameworks(${target})
    elseif(REFLEX_PLATFORM_IOS)
        _reflex_configure_ios_bundle(${target}
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
        )
        _reflex_link_apple_ios_frameworks(${target})
    endif()

    get_filename_component(_resources_xml
        "${CMAKE_CURRENT_SOURCE_DIR}/resources.xml" ABSOLUTE)
    _reflex_add_resource_build(${target} "${_resources_xml}")

endfunction()


# =========================================================
# Internal: locate the prebuilt `reflex` CLI tool
# =========================================================
# Sets out_var to the host-platform tool path, or empty if it is not present
# in this distribution (matching _reflex_add_resource_build's silent skip).

function(_reflex_find_tool out_var)
    if(REFLEX_PLATFORM_WINDOWS)
        set(_t "${REFLEX_ROOT}/bin/tools/win/reflex.exe")
    elseif(REFLEX_PLATFORM_MACOS OR REFLEX_PLATFORM_IOS)
        set(_t "${REFLEX_ROOT}/bin/tools/macos/reflex")
    elseif(REFLEX_PLATFORM_LINUX)
        set(_t "${REFLEX_ROOT}/bin/tools/linux/reflex")
    else()
        set(_t "")
    endif()
    if(_t AND EXISTS "${_t}")
        set(${out_var} "${_t}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()


# =========================================================
# Internal: generate an Info.plist with resolved values
# =========================================================
# Uses the 'reflex' cli tool with 'build-plist' task
# the tool derives the AU AudioComponents integer version and AUv3 tag).

function(_reflex_generate_plist output_var target name executable vendor version bundle_type)
    # Optional package/AU parameters:
    # pass package id vendor/product as 8th/9th args,
    # then the AU component list and manufacturer as 10th/11th args
    set(_package_id_vendor "${ARGV7}")
    set(_package_id_product "${ARGV8}")
    set(_au_components "${ARGV9}")
    set(_au_vendor_4cc "${ARGV10}")

    set(_plist_path "${CMAKE_CURRENT_BINARY_DIR}/${target}_Info.plist")

    # Bundle identifier (shared by both generation paths)
    _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${_package_id_vendor}" "${_package_id_product}")
    if(bundle_type STREQUAL "app"
       OR bundle_type STREQUAL "audioapp"
       OR bundle_type STREQUAL "ios_app"
       OR bundle_type STREQUAL "ios_audioapp")
        set(_bundle_id "${_package_id}")
    else()
        set(_bundle_id "${_package_id}.${bundle_type}")
    endif()

    _reflex_find_tool(_tool)
    if(NOT _tool)
        message(FATAL_ERROR
            "Reflex: 'reflex build-plist' tool not found. "
            "Expected a host tool in the Reflex distribution for plist generation.")
    endif()

    # Map CMake bundle_type -> build-plist --target
    if(bundle_type STREQUAL "component")
        set(_tool_target "au")
    elseif(bundle_type STREQUAL "vst")
        set(_tool_target "vst2")
    else()
        # app/audioapp/ios_app/ios_audioapp/clap/vst3/auv3 map 1:1
        set(_tool_target "${bundle_type}")
    endif()

    set(_args
        build-plist
        --target    "${_tool_target}"
        --output    "${_plist_path}"
        --product   "${name}"
        --executable "${executable}"
        --bundle_id "${_bundle_id}"
        --version   "${version}"
    )
    # AU / AUv3 need vendor, a shared manufacturer 4CC, and component records.
    if(_tool_target STREQUAL "au" OR _tool_target STREQUAL "auv3")
        list(APPEND _args
            --vendor          "${vendor}"
            --au_components   "${_au_components}"
            --au_manufacturer "${_au_vendor_4cc}"
        )
    endif()

    execute_process(
        COMMAND "${_tool}" ${_args}
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE  _err
    )
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR
            "Reflex: 'reflex build-plist' failed for ${target} "
            "(${_tool_target}):\n${_out}${_err}")
    endif()
    set(${output_var} "${_plist_path}" PARENT_SCOPE)
endfunction()


# =========================================================
# Internal: sign a macOS bundle after build
# =========================================================
function(_reflex_codesign_bundle target label)
    if(NOT REFLEX_PLATFORM_MACOS OR NOT REFLEX_CODESIGN)
        return()
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND codesign --force --sign "${REFLEX_CODESIGN_IDENTITY}" "$<TARGET_BUNDLE_DIR:${target}>"
        COMMENT "Signing ${label}"
    )
endfunction()


# =========================================================
# Internal: create one plugin format target
# =========================================================

# =========================================================
# Internal: map a plugin format to its platform system-entry TU
# =========================================================
# Returns in <out> the absolute path to the per-platform unity source for
# <basename> (e.g. "vst3" -> src/reflex/system/osx_vst3.mm on macOS), or empty
# if no such source exists for the current platform / source tree.
function(_reflex_platform_unity_source basename out)
    set(${out} "" PARENT_SCOPE)
    if(REFLEX_PLATFORM_WINDOWS)
        set(_cand "${REFLEX_ROOT}/src/reflex/system/win_${basename}.cpp")
    elseif(REFLEX_PLATFORM_IOS)
        set(_cand "${REFLEX_ROOT}/src/reflex/system/ios_${basename}.mm")
    elseif(REFLEX_PLATFORM_MACOS)
        set(_cand "${REFLEX_ROOT}/src/reflex/system/osx_${basename}.mm")
    elseif(REFLEX_PLATFORM_LINUX)
        set(_cand "${REFLEX_ROOT}/src/reflex/system/linux_${basename}.cpp")
    else()
        return()
    endif()
    if(EXISTS "${_cand}")
        set(${out} "${_cand}" PARENT_SCOPE)
    endif()
endfunction()


# =========================================================
# Internal: resolve the library a plugin format links against
# =========================================================
# With REFLEX_BUILD_TARGETS_FROM_SOURCE ON and the source tree present, compiles
# the platform system-entry TU into a static lib once (Reflex::<alias>), so edits
# to system/<plat>_<basename> rebuild without refreshing a prebuilt lib. Otherwise
# returns the prebuilt imported target. Returns empty in <out> when the format is
# unavailable on this platform (the caller skips it).
function(_reflex_resolve_target_lib alias basename out)
    set(${out} "" PARENT_SCOPE)

    if(REFLEX_BUILD_TARGETS_FROM_SOURCE)
        set(_srctgt "_ReflexSrc_${alias}")
        if(NOT TARGET ${_srctgt})
            _reflex_platform_unity_source("${basename}" _unity)
            if(_unity)
                reflex_add_target(${_srctgt} TYPE STATIC_LIBRARY SOURCES "${_unity}")
                target_include_directories(${_srctgt} PRIVATE
                    "${REFLEX_ROOT}/include"
                    "${REFLEX_ROOT}/src")
                if(REFLEX_PLATFORM_MACOS)
                    _reflex_init_target(${_srctgt} APPLE_DEPLOYMENT_TARGET 10.15)
                else()
                    _reflex_init_target(${_srctgt})
                endif()
                # ObjC++ entry TUs (.mm) are MRC, matching the prebuilt libs
                # (no CLANG_ENABLE_OBJC_ARC → MRC default). The macOS entry
                # unities have no ARC-only constructs, so they stay MRC. The iOS
                # unities use __weak self-captures and ARC-heap-copied block
                # returns (hard MRC errors), so iOS builds with ARC. iOS has no
                # AudioUnit-v2 (au) target, so no au-under-ARC concern here.
                # Silence SDK-deprecation/nullability noise.
                get_filename_component(_ext "${_unity}" EXT)
                if(_ext STREQUAL ".mm")
                    set_source_files_properties("${_unity}" PROPERTIES
                        COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-nullability-completeness")
                    # The system entry sources call Apple APIs the latest SDK marks
                    # obsoleted at recent deployment targets (e.g. CGWindowListCreateImage,
                    # obsoleted 15.0 — a hard error there, only a deprecation below it).
                    # Build them against the same SDK floor the prebuilt libs use so those
                    # APIs stay available (and the deprecation is silenced above). A static
                    # lib with a lower min-version links cleanly into a newer-target app.
                endif()
            endif()
        endif()
        if(TARGET ${_srctgt})
            set(${out} "${_srctgt}" PARENT_SCOPE)
            return()
        endif()
    endif()

    if(TARGET Reflex::${alias})
        set(${out} "Reflex::${alias}" PARENT_SCOPE)
    endif()
endfunction()


function(_reflex_add_plugin_format base_target format sources name vendor version package_id_vendor package_id_product au_components au_vendor_4cc)

    set(_apple_deployment_target "${ARGV10}")   # optional

    set(_t "${base_target}_${format}")

    # Normalize source paths to CMake form (forward slashes). Consumers on Windows
    # may pass native backslash paths (e.g. paths derived from a backslash
    # REFLEX_DIR); reaching add_library()/add_executable() as string literals, CMake
    # parses backslash sequences as invalid character escapes (e.g. "\G" from a
    # "...\GitLab-Runner\..." path), failing configuration. A direct backslash
    # replace (not file(TO_CMAKE_PATH), which splits drive letters on ":") is the
    # safe, platform-uniform normalization.
    set(_normalized_sources "")
    foreach(_src IN LISTS sources)
        string(REPLACE "\\" "/" _src "${_src}")
        list(APPEND _normalized_sources "${_src}")
    endforeach()
    set(sources "${_normalized_sources}")

    if(format STREQUAL "Standalone")

        _reflex_resolve_target_lib(TargetAudioApp audioapp _fmtlib)
        if(NOT _fmtlib)
            return()
        endif()

        reflex_add_target(${_t} TYPE APP SOURCES ${sources})

        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME "${name}"
        )
        target_link_libraries(${_t} PRIVATE ${_fmtlib})

        # Audio frameworks on both macOS and iOS (the helper is platform-aware).
        if(REFLEX_PLATFORM_MACOS OR REFLEX_PLATFORM_IOS)
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(REFLEX_PLATFORM_MACOS)
            _reflex_configure_macos_audio_bundle(${_t}
                "${name}" "${vendor}" "${version}"
                "${package_id_vendor}" "${package_id_product}"
            )
        elseif(REFLEX_PLATFORM_IOS)
            _reflex_configure_ios_audio_bundle(${_t}
                "${name}" "${vendor}" "${version}"
                "${package_id_vendor}" "${package_id_product}"
            )
            _reflex_link_apple_ios_frameworks(${_t})
        endif()

    elseif(format STREQUAL "VST3")

        _reflex_resolve_target_lib(TargetVST3 vst3 _fmtlib)
        if(NOT _fmtlib)
            return()
        endif()

        reflex_add_target(${_t} TYPE MODULE_LIBRARY SOURCES ${sources})
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME      "${name}"
            PREFIX           ""
            BUNDLE_EXTENSION "vst3"
        )
        target_link_libraries(${_t} PRIVATE ${_fmtlib})

        # BUNDLE_EXTENSION only applies under BUNDLE (Apple). On Windows match the
        # canonical VS project (templates/cpp_audioplugin/projects/win/vcxproj/
        # VST3.vcxproj: TargetExt=.vst3): emit a flat <name>.vst3 module. Without
        # this the module is <name>.dll and collides with the CLAP target's .dll.
        if(REFLEX_PLATFORM_WINDOWS)
            set_target_properties(${_t} PROPERTIES SUFFIX ".vst3")
        endif()

        if(REFLEX_PLATFORM_MACOS)
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(REFLEX_PLATFORM_MACOS OR REFLEX_PLATFORM_IOS)
            _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
            _reflex_generate_plist(_plist ${_t} "${name}" "${name}" "${vendor}" "${version}" "vst3"
                "${package_id_vendor}" "${package_id_product}")
            _reflex_set_bundle_identifier(${_t} "${_package_id}.vst3")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            reflex_target_set_macos_exported_symbols(${_t}
                "${REFLEX_ROOT}/resources/macos/VST3_exports.txt")
        endif()

    elseif(format STREQUAL "CLAP")

        _reflex_resolve_target_lib(TargetCLAP clap _fmtlib)
        if(NOT _fmtlib)
            return()
        endif()

        reflex_add_target(${_t} TYPE MODULE_LIBRARY SOURCES ${sources})
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME      "${name}"
            PREFIX           ""
            BUNDLE_EXTENSION "clap"
        )
        target_link_libraries(${_t} PRIVATE ${_fmtlib})

        # See VST3 above: match the canonical VS project (CLAP.vcxproj
        # TargetExt=.clap) and emit a flat <name>.clap module on Windows.
        if(REFLEX_PLATFORM_WINDOWS)
            set_target_properties(${_t} PROPERTIES SUFFIX ".clap")
        endif()

        if(REFLEX_PLATFORM_MACOS)
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(REFLEX_PLATFORM_MACOS OR REFLEX_PLATFORM_IOS)
            _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
            _reflex_generate_plist(_plist ${_t} "${name}" "${name}" "${vendor}" "${version}" "clap"
                "${package_id_vendor}" "${package_id_product}")
            _reflex_set_bundle_identifier(${_t} "${_package_id}.clap")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            reflex_target_set_macos_exported_symbols(${_t}
                "${REFLEX_ROOT}/resources/macos/clap_exports.txt")
        endif()

    elseif(format STREQUAL "VST2")

        _reflex_resolve_target_lib(TargetVST2 vst2 _fmtlib)
        if(NOT _fmtlib)
            return()
        endif()

        reflex_add_target(${_t} TYPE MODULE_LIBRARY SOURCES ${sources})
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME "${name}"
            PREFIX      ""
        )
        target_link_libraries(${_t} PRIVATE ${_fmtlib})

        if(REFLEX_PLATFORM_MACOS)
            _reflex_link_apple_audio_frameworks(${_t})
        endif()

        if(REFLEX_PLATFORM_MACOS OR REFLEX_PLATFORM_IOS)
            _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
            _reflex_generate_plist(_plist ${_t} "${name}" "${name}" "${vendor}" "${version}" "vst"
                "${package_id_vendor}" "${package_id_product}")
            _reflex_set_bundle_identifier(${_t} "${_package_id}.vst")
            set_target_properties(${_t} PROPERTIES
                BUNDLE                   TRUE
                BUNDLE_EXTENSION         "vst"
                MACOSX_BUNDLE_INFO_PLIST "${_plist}"
            )
            reflex_target_set_macos_exported_symbols(${_t}
                "${REFLEX_ROOT}/resources/macos/VST2_exports.txt")
        endif()

    elseif(format STREQUAL "AU")

        _reflex_resolve_target_lib(TargetAU au _fmtlib)
        if(NOT _fmtlib)
            return()
        endif()

        reflex_add_target(${_t} TYPE MODULE_LIBRARY SOURCES ${sources})
        _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
        _reflex_generate_plist(_plist ${_t} "${name}" "${name}" "${vendor}" "${version}" "component"
            "${package_id_vendor}" "${package_id_product}"
            "${au_components}" "${au_vendor_4cc}")
        _reflex_set_bundle_identifier(${_t} "${_package_id}.component")
        set_target_properties(${_t} PROPERTIES
            OUTPUT_NAME              "${name}"
            PREFIX                   ""
            BUNDLE                   TRUE
            BUNDLE_EXTENSION         "component"
            MACOSX_BUNDLE_INFO_PLIST "${_plist}"
        )
        target_link_libraries(${_t} PRIVATE
            ${_fmtlib}
            "-framework AudioUnit"
            "-framework AudioToolbox"
        )
        _reflex_link_apple_audio_frameworks(${_t})
        reflex_target_set_macos_exported_symbols(${_t}
            "${REFLEX_ROOT}/resources/macos/audiounit_exports.txt")

    elseif(format STREQUAL "AUv3")

        if(NOT REFLEX_PLATFORM_MACOS AND NOT REFLEX_PLATFORM_IOS)
            return()
        endif()

        # No prebuilt .a exists for AUv3 — build the platform-specific unity
        # TU inline. osx_auv3.mm pulls in Cocoa/AppKit; ios_auv3.mm pulls in
        # UIKit. They are mutually exclusive — building the wrong one yields
        # missing-header errors.
        if(NOT TARGET Reflex::TargetAUv3)
            if(REFLEX_PLATFORM_IOS)
                set(_auv3_unity "${REFLEX_ROOT}/src/reflex/system/ios_auv3.mm")
            else()
                set(_auv3_unity "${REFLEX_ROOT}/src/reflex/system/osx_auv3.mm")
            endif()
            reflex_add_target(_ReflexSrc_TargetAUv3 TYPE STATIC_LIBRARY SOURCES "${_auv3_unity}")
            target_include_directories(_ReflexSrc_TargetAUv3 PRIVATE
                "${REFLEX_ROOT}/include"
                "${REFLEX_ROOT}/src"
            )
            _reflex_init_target(_ReflexSrc_TargetAUv3 OBJC_ARC ON)
            # ARC on both platforms — the AUv3 exception to the otherwise-MRC
            # macOS build. common/instance/auv3.mm uses __weak self-captures and
            # returns blocks that ARC heap-copies on its macOS (#else) path as
            # well as iOS — both hard MRC errors — so macOS AUv3 cannot build
            # under MRC. build.mm in the same unity has no MRC constructs, so
            # ARC is safe for the whole TU.
            set_source_files_properties("${_auv3_unity}" PROPERTIES
                COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-nullability-completeness"
            )
            add_library(Reflex::TargetAUv3 ALIAS _ReflexSrc_TargetAUv3)
        endif()

        # iOS AUv3 extensions must be MH_EXECUTE binaries (launchd spawns them
        # as independent processes via NSExtensionMain), not MH_BUNDLE. An
        # add_library(MODULE) produces MH_BUNDLE, on which codesign silently
        # drops entitlements during signing. Emit an executable and apply
        # MACOSX_BUNDLE so CMake wraps it in a .appex directory (JUCE does the
        # same). macOS AUv3 keeps using MODULE; only iOS strictly requires
        # MH_EXECUTE because of amfi.
        if(REFLEX_PLATFORM_IOS)
            reflex_add_target(${_t} TYPE APP SOURCES ${sources})
            target_link_options(${_t} PRIVATE
                "-e" "_NSExtensionMain"
                "-fapplication-extension"
            )
        else()
            reflex_add_target(${_t} TYPE MODULE_LIBRARY SOURCES ${sources})
        endif()

        _reflex_resolve_package_id(_package_id "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
        _reflex_generate_plist(_plist ${_t} "${name}" "${name} AUv3" "${vendor}" "${version}" "auv3"
            "${package_id_vendor}" "${package_id_product}"
            "${au_components}" "${au_vendor_4cc}")

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
        if(REFLEX_PLATFORM_IOS)
            set_target_properties(${_t} PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
                    "${REFLEX_ROOT}/resources/ios/AUv3.entitlements"
            )
        endif()

        target_link_libraries(${_t} PRIVATE Reflex::TargetAUv3)
        target_link_options(${_t} PRIVATE -ObjC)

        if(NOT REFLEX_PLATFORM_IOS)
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

        # Skipped on iOS: Xcode signs the .appex with the dev team as part of
        # the app-extension product type, and re-signing afterwards invalidates
        # the container .app's signature.
        _reflex_codesign_bundle(${_t} "AUv3 extension")

    else()
        message(WARNING "Reflex: unknown plugin format '${format}' — skipped")
        return()
    endif()

    # Common settings for all formats
    target_compile_definitions(${_t} PRIVATE REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
    # Single source of truth for the version: the same value drives the
    # Info.plist (above) and the run-time class metadata. instance.cpp reads
    # this via REFLEX_STRINGIFY(PRODUCT_VERSION); matches the Xcode templates.
    target_compile_definitions(${_t} PRIVATE PRODUCT_VERSION=${version})

    _reflex_resolve_package_id(_product_package_identifier
        "${vendor}" "${name}" "${package_id_vendor}" "${package_id_product}")
    target_compile_definitions(${_t} PRIVATE
        "PRODUCT_PACKAGE_IDENTIFIER=${_product_package_identifier}"
        "AU_COMPONENTS=${au_components}"
        "AU_VENDOR_4CC=${au_vendor_4cc}")
    _reflex_init_target(${_t} FLOATING_POINT fast
        APPLE_DEPLOYMENT_TARGET "${_apple_deployment_target}")
    _reflex_add_windows_resource(${_t})

    # Link order matters — higher-level libraries first, dependencies last
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

    # After every step that writes into the bundle, and before the copy below.
    if(NOT format STREQUAL "AUv3")
        _reflex_codesign_bundle(${_t} "${format}")
    endif()

    # Optional: copy built plugins to user plugin folders after build
    if(REFLEX_COPY_PLUGINS_AFTER_BUILD AND REFLEX_PLATFORM_MACOS AND NOT format STREQUAL "Standalone")
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
#       AU_COMPONENTS  "ES2M:aumu:Example Synth,ES2F:aufx:Example FX"
#       AU_VENDOR_4CC  "NdAu"      # 4-char vendor code
#       APPLE_DEPLOYMENT_TARGET "10.15"   # optional; defaults to CMAKE_OSX_DEPLOYMENT_TARGET
#   )

function(reflex_add_audio_plugin target)
    cmake_parse_arguments(A ""
        "NAME;VENDOR;VERSION;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT;AU_COMPONENTS;AU_VENDOR_4CC;APPLE_DEPLOYMENT_TARGET"
        "FORMATS;SOURCES" ${ARGN})

    if(NOT A_FORMATS)
        message(WARNING "reflex_add_audio_plugin: no FORMATS specified for '${target}'")
        return()
    endif()

    # Version is the single source of truth for both the Info.plist and the
    # run-time class metadata. Default it so omitting VERSION never yields an
    # empty plist version (the tool also requires x.y.z to pack the AU integer).
    if(NOT A_VERSION)
        set(A_VERSION "1.0.0")
    endif()

    foreach(_fmt IN LISTS A_FORMATS)
        _reflex_add_plugin_format(
            ${target} ${_fmt} "${A_SOURCES}"
            "${A_NAME}" "${A_VENDOR}" "${A_VERSION}"
            "${A_PACKAGE_ID_VENDOR}" "${A_PACKAGE_ID_PRODUCT}"
            "${A_AU_COMPONENTS}" "${A_AU_VENDOR_4CC}"
            "${A_APPLE_DEPLOYMENT_TARGET}"
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
#       APPLE_DEPLOYMENT_TARGET "10.15"   # optional; defaults to CMAKE_OSX_DEPLOYMENT_TARGET
#   )

function(reflex_add_console_app target)
    cmake_parse_arguments(A "" "NAME;VENDOR;PACKAGE_ID_VENDOR;PACKAGE_ID_PRODUCT;APPLE_DEPLOYMENT_TARGET" "SOURCES" ${ARGN})

    reflex_add_target(${target} TYPE CONSOLE SOURCES ${A_SOURCES})

    target_compile_definitions(${target} PRIVATE REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP)
    _reflex_init_target(${target} RELEASE_OPTIMIZATION size
        APPLE_DEPLOYMENT_TARGET "${A_APPLE_DEPLOYMENT_TARGET}")

    target_link_libraries(${target} PRIVATE
        Reflex::Common
        Reflex::TargetConsole
    )

    if(REFLEX_PLATFORM_MACOS)
        _reflex_link_apple_console_frameworks(${target})
    endif()

endfunction()
