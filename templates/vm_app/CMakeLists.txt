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

# ---- VM app target ----
reflex_add_vm_app(_PRODUCT-NAME-SYMBOL_
    SOURCES
        ${REFLEX_DIR}/src/reflex_ext.cpp
        code/entry.cpp
        code/resources.cpp
    NAME    "_PRODUCT-NAME_"
    VENDOR  "_VENDOR-NAME_"
)
