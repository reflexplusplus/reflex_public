cmake_minimum_required(VERSION 3.22)
project(DragandDrop)

# ---- Reflex Path ----
# To override REFLEX_DIR, pass:
#   cmake -B build -DREFLEX_DIR=/absolute/path/to/reflex

if(NOT REFLEX_DIR)
    set(REFLEX_DIR "/Volumes/USB_HDD/devt/reflex")
endif()

find_package(Reflex REQUIRED CONFIG
    PATHS "${REFLEX_DIR}/cmake"
    NO_DEFAULT_PATH
)

# ---- App target ----
reflex_add_app(DragandDrop
    SOURCES
        ${REFLEX_DIR}/src/reflex_ext.cpp
        code/app.cpp
        code/entry.cpp
        code/resources.cpp
        code/view.cpp
    NAME    "Drag and Drop"
    VENDOR  "Reflex++"
)
