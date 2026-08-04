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

# ---- Console app target ----
reflex_add_console_app(_PRODUCT-NAME-SYMBOL_
    SOURCES
        code/entry.cpp
        code/main.cpp
    NAME               "_PRODUCT-NAME_"
    VENDOR             "_VENDOR-NAME_"
    PACKAGE_ID_VENDOR  "_PACKAGE-ID-VENDOR_"
    PACKAGE_ID_PRODUCT "_PACKAGE-ID-PRODUCT_"
)
