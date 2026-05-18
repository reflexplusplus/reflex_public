cmake_minimum_required(VERSION 3.22)

project(reflex LANGUAGES CXX)

add_library(reflex STATIC)

# Public headers
target_include_directories(reflex PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/glx/ext/freetype-2.9.1/include"
)

# Sources
target_sources(reflex PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/core.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/debug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/vec.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/data.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/file.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/glx.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/system/win_library.cpp"

    "${CMAKE_CURRENT_LIST_DIR}/src/reflex_ext/reflex_ext_file.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex_ext/reflex_ext_glx.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/reflex_ext/reflex_ext_ide.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/reflex_ext/reflex_ext_bootstrap.cpp"
)

set_source_files_properties(
    "${CMAKE_CURRENT_LIST_DIR}/src/reflex/glx.cpp"
    PROPERTIES
        COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/bigobj>"
)

# ---- Language standard ----
set_target_properties(reflex PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)


# ---- Disable RTTI (target-local) ----
set_property(TARGET reflex PROPERTY CXX_RTTI OFF)

# (Optional) If you need this for Windows builds
if (MSVC)
    target_compile_definitions(reflex PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()
