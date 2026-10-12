cmake_minimum_required(VERSION 3.22)
project(StatusItem)

# ---- Reflex Path ----
# To override REFLEX_DIR, pass:
#   cmake -B build -DREFLEX_DIR=/absolute/path/to/reflex

if(NOT REFLEX_DIR)
    get_filename_component(REFLEX_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

find_package(Reflex REQUIRED CONFIG
    PATHS "${REFLEX_DIR}/cmake"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
)

# ---- App target ----
reflex_add_app(StatusItem
    SOURCES
        ${REFLEX_DIR}/src/reflex_ext.cpp
        code/app.cpp
        code/entry.cpp
        code/resources.cpp
    NAME               "Status Item"
    VENDOR             "Reflex++"
    PACKAGE_ID_VENDOR  "reflexplusplus"
    PACKAGE_ID_PRODUCT "statusitem"
)
