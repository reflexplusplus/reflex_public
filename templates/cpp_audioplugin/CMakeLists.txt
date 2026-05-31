cmake_minimum_required(VERSION 3.22)
project(_PRODUCT-NAME-SYMBOL_)

# ---- Reflex Path ----
# To override REFLEX_DIR, pass:
#   cmake -B build -DREFLEX_DIR=/absolute/path/to/reflex

if(NOT REFLEX_DIR)
    set(REFLEX_DIR "_REFLEX-PATH_")
endif()

find_package(Reflex REQUIRED CONFIG
    PATHS "${REFLEX_DIR}/cmake"
    NO_DEFAULT_PATH
)

# ---- Format selection ----
# All formats are ON by default; disable any you don't need.
# Formats not available on the current platform are skipped automatically.

option(REFLEX_BUILD_APP  "Build standalone audio app" ON)
option(REFLEX_BUILD_VST3 "Build VST3 plugin"          ON)
option(REFLEX_BUILD_CLAP "Build CLAP plugin"          ON)
option(REFLEX_BUILD_VST2 "Build VST2 plugin"          OFF)
option(REFLEX_BUILD_AUV2 "Build AUV2 plugin"          ON)

set(_FORMATS)
if(REFLEX_BUILD_APP)
    list(APPEND _FORMATS APP)
endif()
if(REFLEX_BUILD_VST3)
    list(APPEND _FORMATS VST3)
endif()
if(REFLEX_BUILD_CLAP)
    list(APPEND _FORMATS CLAP)
endif()
if(REFLEX_BUILD_VST2)
    list(APPEND _FORMATS VST2)
endif()
if(REFLEX_BUILD_AUV2)
    list(APPEND _FORMATS AUV2)
endif()

# ---- Plugin targets ----
# Each format becomes its own target: _PRODUCT-NAME-SYMBOL__APP,
# _PRODUCT-NAME-SYMBOL__VST3, _PRODUCT-NAME-SYMBOL__CLAP, etc.

reflex_add_audio_plugin(_PRODUCT-NAME-SYMBOL_
    FORMATS  ${_FORMATS}
    SOURCES
        ${REFLEX_DIR}/src/reflex_ext.cpp
        code/entry.cpp
        code/instance.cpp
        code/resources.cpp
        code/view.cpp
    NAME     "_PRODUCT-NAME_"
    VENDOR   "_VENDOR-NAME_"
    VERSION  "1.0.0"
)
