# ReflexConfig.cmake
# find_package(Reflex REQUIRED CONFIG PATHS "${REFLEX_DIR}/cmake" NO_DEFAULT_PATH)
#
# Exposes imported targets:
#   Reflex::Common             ReflexCommon
#   Reflex::CommonUi           ReflexCommonUi
#   Reflex::Vm                 ReflexCommonVm
#   Reflex::TargetApp          ReflexTargetApp
#   Reflex::TargetAudioApp     ReflexTargetAudioApp
#   Reflex::TargetConsole      ReflexTargetConsole
#   Reflex::TargetLibrary      ReflexTargetLibrary
#   Reflex::TargetDynamicLibrary
#   Reflex::TargetVST3         (Windows + macOS)
#   Reflex::TargetCLAP         (Windows + macOS)
#   Reflex::TargetVST2         (Windows + macOS)
#   Reflex::TargetAU           (macOS only)
#
# Helper functions (from ReflexHelpers.cmake):
#   reflex_add_app(target ...)
#   reflex_add_vm_app(target ...)
#   reflex_add_audio_plugin(target ...)
#   reflex_add_console_app(target ...)

cmake_minimum_required(VERSION 3.22)

get_filename_component(REFLEX_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Cache REFLEX_ROOT so the reflex_add_* helpers (which read it at call time)
# resolve it even when find_package ran in a subproject scope — e.g. when the
# SDK is pulled in via FetchContent/CPM and consumed from the parent project.
set(REFLEX_ROOT "${REFLEX_ROOT}" CACHE INTERNAL "Reflex SDK root directory" FORCE)

# =========================================================
# Validate installation
# =========================================================

if(NOT EXISTS "${REFLEX_ROOT}/include")
    message(FATAL_ERROR
        "Reflex: include directory not found at '${REFLEX_ROOT}/include'.\n"
        "Is REFLEX_DIR pointing to the correct Reflex installation?")
endif()

# =========================================================
# Platform detection + library path resolution
# =========================================================

# Cleared rather than defaulted, so a nested find_package against a different
# SDK root does not inherit the enclosing scope's probe directories.
unset(_REFLEX_LIB_PROBE_DIRS)

if(WIN32)

    set(_REFLEX_PLATFORM    "win")
    set(_REFLEX_LIB_PREFIX  "")
    set(_REFLEX_LIB_SUFFIX  ".lib")

    # Resolve architecture subfolder (Visual Studio generator sets
    # CMAKE_GENERATOR_PLATFORM; Ninja/NMake use CMAKE_SIZEOF_VOID_P)
    if(CMAKE_GENERATOR_PLATFORM)
        set(_REFLEX_ARCH "${CMAKE_GENERATOR_PLATFORM}")  # "x64" or "Win32"
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_REFLEX_ARCH "x64")
    else()
        set(_REFLEX_ARCH "Win32")
    endif()

    set(_REFLEX_LIB_DIR_DBG "${REFLEX_ROOT}/bin/lib/win/Debug/${_REFLEX_ARCH}")
    set(_REFLEX_LIB_DIR_REL "${REFLEX_ROOT}/bin/lib/win/Release/${_REFLEX_ARCH}")

elseif(APPLE)

    set(_REFLEX_LIB_PREFIX "lib")
    set(_REFLEX_LIB_SUFFIX ".a")

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(_REFLEX_PLATFORM "ios")
        # Only Xcode substitutes $(EFFECTIVE_PLATFORM_NAME); under any other
        # generator the link paths below reach the build system verbatim and the
        # first link fails on a path containing a literal '$('. Refuse here
        # rather than emitting a project that cannot build.
        if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
            message(FATAL_ERROR
                "Reflex: iOS requires the Xcode generator (got '${CMAKE_GENERATOR}').\n"
                "The iOS libraries are laid out per-sdk and are selected with Xcode's "
                "$(EFFECTIVE_PLATFORM_NAME), which other generators do not substitute.\n"
                "Re-run with -G Xcode.")
        endif()
        # iOS libs are built per-config-per-sdk: bin/lib/ios/<Config>-<sdk>.
        # Use Xcode's native $(EFFECTIVE_PLATFORM_NAME) (-iphoneos /
        # -iphonesimulator) so the right slice is linked for the active
        # destination at build time, without re-running CMake when toggling
        # device <-> simulator in Xcode.
        set(_REFLEX_LIB_DIR_DBG "${REFLEX_ROOT}/bin/lib/ios/Debug$(EFFECTIVE_PLATFORM_NAME)")
        set(_REFLEX_LIB_DIR_REL "${REFLEX_ROOT}/bin/lib/ios/Release$(EFFECTIVE_PLATFORM_NAME)")
        # $(EFFECTIVE_PLATFORM_NAME) is substituted by Xcode at build time, so the
        # paths above never resolve during configuration. Probe the concrete
        # per-sdk directories instead when testing whether a library exists.
        set(_REFLEX_LIB_PROBE_DIRS
            "${REFLEX_ROOT}/bin/lib/ios/Debug-iphoneos"
            "${REFLEX_ROOT}/bin/lib/ios/Debug-iphonesimulator"
            "${REFLEX_ROOT}/bin/lib/ios/Release-iphoneos"
            "${REFLEX_ROOT}/bin/lib/ios/Release-iphonesimulator")
    else()
        set(_REFLEX_PLATFORM "macos")
        set(_REFLEX_LIB_DIR_DBG "${REFLEX_ROOT}/bin/lib/macos/Debug")
        set(_REFLEX_LIB_DIR_REL "${REFLEX_ROOT}/bin/lib/macos/Release")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

    set(_REFLEX_PLATFORM    "linux")
    set(_REFLEX_LIB_PREFIX  "lib")
    set(_REFLEX_LIB_SUFFIX  ".a")
    set(_REFLEX_LIB_DIR_DBG "${REFLEX_ROOT}/bin/lib/linux/Debug")
    set(_REFLEX_LIB_DIR_REL "${REFLEX_ROOT}/bin/lib/linux/Release")
    find_package(Threads REQUIRED)
    find_package(CURL REQUIRED)
    set(_REFLEX_LINUX_COMMON_LIBS "Threads::Threads;CURL::libcurl")
    set(_REFLEX_LINUX_UI_LIBS "wayland-client;wayland-egl;EGL;GLESv2")

elseif(EMSCRIPTEN)

    set(_REFLEX_PLATFORM    "webasm")
    set(_REFLEX_LIB_PREFIX  "lib")
    set(_REFLEX_LIB_SUFFIX  ".a")
    set(_REFLEX_LIB_DIR_DBG "${REFLEX_ROOT}/bin/lib/webasm/Debug")
    set(_REFLEX_LIB_DIR_REL "${REFLEX_ROOT}/bin/lib/webasm/Release")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")

    # Android ships a prebuilt AAR consumed via Gradle prefab, not loose static
    # libs imported by find_package. Point the consumer at the AAR rather than
    # importing libraries that aren't laid out for CMake.
    message(FATAL_ERROR
        "Reflex: on Android, consume the prebuilt AAR via Gradle (prefab), not "
        "find_package(Reflex). See bin/lib/android/{debug,release}/reflex.aar.")

else()
    message(FATAL_ERROR "Reflex: unsupported platform '${CMAKE_SYSTEM_NAME}'")
endif()

# Directories to test for the presence of a library. These are the link paths
# themselves on every platform whose link paths are fully resolved at configure
# time; platforms using generator-time substitution set this above.
if(NOT _REFLEX_LIB_PROBE_DIRS)
    set(_REFLEX_LIB_PROBE_DIRS "${_REFLEX_LIB_DIR_DBG}" "${_REFLEX_LIB_DIR_REL}")
endif()

# Sets <out_var> to TRUE when <name> is present in any probe directory.
function(_reflex_lib_exists out_var name)
    foreach(_dir IN LISTS _REFLEX_LIB_PROBE_DIRS)
        if(EXISTS "${_dir}/${_REFLEX_LIB_PREFIX}${name}${_REFLEX_LIB_SUFFIX}")
            set(${out_var} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} FALSE PARENT_SCOPE)
endfunction()

# =========================================================
# Fetch prebuilt binaries (exported consumer repos)
# =========================================================
# Populates bin/lib (+ bin/tools) from the repo's GitHub Release before the
# imported targets below resolve. No-op for local source builds (libs present).

include("${CMAKE_CURRENT_LIST_DIR}/FetchReflexLibs.cmake")
reflex_fetch_libs()

# =========================================================
# Helper: create a single imported static library target
# =========================================================

function(_reflex_import_lib alias name)
    if(TARGET Reflex::${alias})
        return()
    endif()

    set(_dbg_lib  "${_REFLEX_LIB_DIR_DBG}/${_REFLEX_LIB_PREFIX}${name}${_REFLEX_LIB_SUFFIX}")
    set(_rel_lib  "${_REFLEX_LIB_DIR_REL}/${_REFLEX_LIB_PREFIX}${name}${_REFLEX_LIB_SUFFIX}")

    # Not every distribution ships every library (the public SDK has no VM), so
    # only define the target when the library is actually present. Consumers can
    # test with if(TARGET Reflex::Vm); defining it unconditionally would instead
    # fail much later with a confusing missing-input link error.
    _reflex_lib_exists(_present "${name}")
    if(NOT _present)
        return()
    endif()

    add_library(Reflex::${alias} STATIC IMPORTED GLOBAL)

    set_target_properties(Reflex::${alias} PROPERTIES
        IMPORTED_CONFIGURATIONS       "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG       "${_dbg_lib}"
        IMPORTED_LOCATION_RELEASE     "${_rel_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${REFLEX_ROOT}/include"
    )
endfunction()

# =========================================================
# Core libraries — all platforms
# =========================================================

_reflex_import_lib(Common             ReflexCommon)
_reflex_import_lib(CommonUi           ReflexCommonUi)
_reflex_import_lib(Vm                 ReflexCommonVm)

# =========================================================
# Target libraries — available on all platforms
# =========================================================

_reflex_import_lib(TargetApp              ReflexTargetApp)
_reflex_import_lib(TargetAudioApp         ReflexTargetAudioApp)
_reflex_import_lib(TargetConsole          ReflexTargetConsole)
_reflex_import_lib(TargetLibrary          ReflexTargetLibrary)
_reflex_import_lib(TargetDynamicLibrary   ReflexTargetDynamicLibrary)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    foreach(_linux_target TargetConsole TargetApp TargetAudioApp TargetLibrary TargetDynamicLibrary)
        if(TARGET Reflex::${_linux_target})
            set_property(TARGET Reflex::${_linux_target} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${_REFLEX_LINUX_COMMON_LIBS}"
            )
        endif()
    endforeach()

    foreach(_linux_ui_target TargetApp TargetAudioApp)
        if(TARGET Reflex::${_linux_ui_target})
            set_property(TARGET Reflex::${_linux_ui_target} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${_REFLEX_LINUX_UI_LIBS}"
            )
        endif()
    endforeach()
endif()

# =========================================================
# Plugin format targets — Windows + macOS
# =========================================================

if(WIN32 OR (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS"))
    _reflex_import_lib(TargetVST3  ReflexTargetVST3)
    _reflex_import_lib(TargetCLAP  ReflexTargetCLAP)
    _reflex_import_lib(TargetVST2  ReflexTargetVST2)
endif()

# =========================================================
# Plugin format targets — macOS only
# =========================================================

if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    _reflex_import_lib(TargetAU  ReflexTargetAU)
endif()

# =========================================================
# Source-build toggle
# =========================================================
# When ON, plugin format targets compile their platform system-entry TU
# (src/reflex/system/<plat>_<fmt>) from source instead of linking the prebuilt
# Reflex::Target* lib — so SDK devs editing system code rebuild without a lib
# refresh. Defaults ON when the source tree is present (full checkout /
# reflex_master), OFF for prebuilt-only distributions (reflex_public). A
# consumer may set it explicitly before find_package(Reflex). The helper falls
# back to the prebuilt lib if a unity source is absent, so ON is always safe.
if(EXISTS "${REFLEX_ROOT}/src/reflex/system")
    option(REFLEX_BUILD_TARGETS_FROM_SOURCE "Build Reflex target libs from source" ON)
else()
    option(REFLEX_BUILD_TARGETS_FROM_SOURCE "Build Reflex target libs from source" OFF)
endif()

# =========================================================
# Helper functions
# =========================================================

include("${CMAKE_CURRENT_LIST_DIR}/ReflexHelpers.cmake")

set(Reflex_FOUND TRUE)
