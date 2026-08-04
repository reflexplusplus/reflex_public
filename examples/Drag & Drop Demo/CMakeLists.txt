cmake_minimum_required(VERSION 3.22)
project(DragDropDemo)

# ---- Reflex Path ----
# To override REFLEX_DIR, pass:
#   cmake -B build -DREFLEX_DIR=/absolute/path/to/reflex

if(NOT REFLEX_DIR)
    set(REFLEX_DIR "D:/devt/reflex")
endif()

find_package(Reflex REQUIRED CONFIG
    PATHS "${REFLEX_DIR}/cmake"
    NO_DEFAULT_PATH
)

# ---- App target ----
reflex_add_app(DragDropDemo
    SOURCES
        ${REFLEX_DIR}/src/reflex_ext.cpp
        code/app.cpp
        code/entry.cpp
        code/resources.cpp
        code/view.cpp
    NAME    "Drag & Drop Demo"
    VENDOR  "Reflex Multimedia"
)
