cmake_minimum_required(VERSION 3.22)
project(Widgets)

if(NOT REFLEX_DIR)
    get_filename_component(REFLEX_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

find_package(Reflex REQUIRED CONFIG
    PATHS "${REFLEX_DIR}/cmake"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
)

reflex_add_app(Widgets
    SOURCES
        ${REFLEX_DIR}/src/reflex_ext.cpp
        code/app.cpp
        code/entry.cpp
        code/resources.cpp
        code/view.cpp
        code/widgets/button.cpp
        code/widgets/rotaryslider.cpp
        code/widgets/dragedit.cpp
        code/widgets/rangebar.cpp
        code/widgets/zone.cpp
    NAME               "Widgets"
    VENDOR             "Reflex++"
    PACKAGE_ID_VENDOR  "reflexplusplus"
    PACKAGE_ID_PRODUCT "widgets"
)
