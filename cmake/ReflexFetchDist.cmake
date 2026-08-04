# Fetches a self-contained reflex-dist-<os>.zip release package and points
# REFLEX_DIR at the extracted tree, so a consumer project needs nothing but:
#
#     include(cmake/ReflexFetchDist.cmake)
#     reflex_fetch_dist(VERSION v0.4.0)                     # public releases
#     reflex_fetch_dist(VERSION v0.4.0 REPO owner/repo PRIVATE)
#
# Consumers vendor a copy of this file next to CPM.cmake (the same bootstrap
# pattern as CPM itself); the canonical version lives in the Reflex SDK at
# cmake/ReflexFetchDist.cmake and ships inside every dist package.
#
# Requires CPM (include cmake/CPM.cmake first) — the dist is cached in
# CPM_SOURCE_CACHE like any other dependency. Needs CMake >= 3.19 (string(JSON)).
#
# Behaviour:
#   - An explicit -DREFLEX_DIR=... or REFLEX_DIR environment variable always
#     wins (SDK-development workflow) — no download happens. If that checkout
#     is a git repo not at the requested tag, a non-fatal warning is issued.
#   - Otherwise the dist package for VERSION is fetched:
#       * public repos:  plain releases/download/ URL, no credentials.
#       * PRIVATE repos: GitHub serves release assets only through the API
#         asset endpoint, so the asset id is resolved from the release
#         metadata first (cached across reconfigures), authenticated via the
#         REFLEX_GITHUB_TOKEN environment variable or `gh auth token`.
#   - The extracted tree's version.txt is checked against VERSION to catch a
#     poisoned/stale cache.

function(reflex_fetch_dist)
    cmake_parse_arguments(ARG "PRIVATE" "VERSION;REPO" "" ${ARGN})
    if(NOT ARG_VERSION)
        message(FATAL_ERROR "reflex_fetch_dist: VERSION is required (e.g. VERSION v0.4.0)")
    endif()
    if(NOT ARG_REPO)
        set(ARG_REPO "reflexplusplus/reflex_public")
    endif()
    if(NOT COMMAND CPMAddPackage)
        message(FATAL_ERROR "reflex_fetch_dist: CPM is not loaded — include(cmake/CPM.cmake) first")
    endif()

    # 1. Explicit SDK location wins (command line, then environment).
    if(NOT DEFINED REFLEX_DIR OR REFLEX_DIR STREQUAL "")
        if(DEFINED ENV{REFLEX_DIR} AND NOT "$ENV{REFLEX_DIR}" STREQUAL "")
            set(REFLEX_DIR "$ENV{REFLEX_DIR}")
        endif()
    endif()
    if(DEFINED REFLEX_DIR AND NOT REFLEX_DIR STREQUAL "")
        # SDK devs routinely run a branch ahead of the pinned tag — warn, don't fail.
        if(EXISTS "${REFLEX_DIR}/.git")
            execute_process(
                COMMAND git -C "${REFLEX_DIR}" describe --tags --always --dirty
                OUTPUT_VARIABLE _reflex_git_desc
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            execute_process(
                COMMAND git -C "${REFLEX_DIR}" describe --tags --exact-match HEAD
                OUTPUT_VARIABLE _reflex_exact_tag
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE _reflex_exact_result
                ERROR_QUIET
            )
            if(NOT _reflex_exact_result EQUAL 0 OR NOT "${_reflex_exact_tag}" STREQUAL "${ARG_VERSION}")
                message(WARNING
                    "Reflex SDK at ${REFLEX_DIR} is at '${_reflex_git_desc}' but this project "
                    "expects '${ARG_VERSION}'. Resync with:\n"
                    "    git -C ${REFLEX_DIR} fetch --tags && git -C ${REFLEX_DIR} checkout ${ARG_VERSION}\n"
                    "(ignore if you're developing on the SDK side).")
            endif()
        endif()
        set(REFLEX_DIR "${REFLEX_DIR}" PARENT_SCOPE)
        return()
    endif()

    # 2. Fetch the dist package for this platform.
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(_reflex_dist_os "macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_reflex_dist_os "windows")
    else()
        set(_reflex_dist_os "linux")
    endif()
    set(_reflex_dist_asset "reflex-dist-${_reflex_dist_os}.zip")

    if(NOT ARG_PRIVATE)
        CPMAddPackage(
            NAME reflex
            VERSION ${ARG_VERSION}
            URL "https://github.com/${ARG_REPO}/releases/download/${ARG_VERSION}/${_reflex_dist_asset}"
            DOWNLOAD_ONLY YES
        )
    else()
        # Auth: CI variable first, then the developer's gh CLI login.
        set(_reflex_token "$ENV{REFLEX_GITHUB_TOKEN}")
        if(_reflex_token STREQUAL "")
            execute_process(
                COMMAND gh auth token
                OUTPUT_VARIABLE _reflex_token
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
        if(_reflex_token STREQUAL "")
            message(FATAL_ERROR
                "No credentials to fetch the Reflex SDK dist package from ${ARG_REPO}. "
                "Either set REFLEX_GITHUB_TOKEN (a PAT with read access), log in with `gh auth login`, "
                "or point -DREFLEX_DIR at a local SDK checkout.")
        endif()

        # Private-repo release assets are only served through the API asset
        # endpoint (the plain releases/download/ URL 404s even with a token):
        # resolve the asset id from the release metadata, cached per tag+os so
        # reconfigures don't re-hit the API. If a tag's assets are re-uploaded
        # (new asset ids, same names) an existing build dir fails its download
        # with a 404 — reconfigure with -UREFLEX_DIST_ASSET_KEY to re-resolve.
        set(_reflex_asset_key "${ARG_VERSION}-${_reflex_dist_os}")
        if(NOT "${REFLEX_DIST_ASSET_KEY}" STREQUAL "${_reflex_asset_key}")
            # Bearer scheme: required by fine-grained PATs (the legacy "token"
            # scheme only works reliably for classic PATs).
            set(_reflex_rel_json "${CMAKE_BINARY_DIR}/reflex-release.json")
            file(DOWNLOAD
                "https://api.github.com/repos/${ARG_REPO}/releases/tags/${ARG_VERSION}"
                "${_reflex_rel_json}"
                HTTPHEADER "Authorization: Bearer ${_reflex_token}"
                HTTPHEADER "Accept: application/vnd.github+json"
                STATUS _reflex_dl_status
            )
            list(GET _reflex_dl_status 0 _reflex_dl_code)
            if(NOT _reflex_dl_code EQUAL 0)
                set(_reflex_dl_body "")
                if(EXISTS "${_reflex_rel_json}")
                    file(READ "${_reflex_rel_json}" _reflex_dl_body)
                    file(REMOVE "${_reflex_rel_json}")
                endif()
                message(FATAL_ERROR
                    "Failed to query ${ARG_REPO} release ${ARG_VERSION}: ${_reflex_dl_status}\n"
                    "Response: ${_reflex_dl_body}\n"
                    "(check that the token grants Contents read on ${ARG_REPO} and the tag's release exists)")
            endif()
            file(READ "${_reflex_rel_json}" _reflex_rel)
            file(REMOVE "${_reflex_rel_json}")
            string(JSON _reflex_asset_count LENGTH "${_reflex_rel}" assets)
            set(_reflex_asset_id "")
            math(EXPR _reflex_asset_last "${_reflex_asset_count} - 1")
            foreach(_reflex_i RANGE ${_reflex_asset_last})
                string(JSON _reflex_asset_name GET "${_reflex_rel}" assets ${_reflex_i} name)
                if(_reflex_asset_name STREQUAL "${_reflex_dist_asset}")
                    string(JSON _reflex_asset_id GET "${_reflex_rel}" assets ${_reflex_i} id)
                    break()
                endif()
            endforeach()
            if(_reflex_asset_id STREQUAL "")
                message(FATAL_ERROR
                    "${_reflex_dist_asset} is not published on the ${ARG_REPO} ${ARG_VERSION} release.")
            endif()
            set(REFLEX_DIST_ASSET_ID "${_reflex_asset_id}" CACHE INTERNAL "release asset id of ${_reflex_dist_asset}")
            set(REFLEX_DIST_ASSET_KEY "${_reflex_asset_key}" CACHE INTERNAL "tag+os the cached asset id belongs to")
        endif()

        CPMAddPackage(
            NAME reflex
            VERSION ${ARG_VERSION}
            URL "https://api.github.com/repos/${ARG_REPO}/releases/assets/${REFLEX_DIST_ASSET_ID}"
            HTTP_HEADER "Authorization: Bearer ${_reflex_token}" "Accept: application/octet-stream"
            DOWNLOAD_ONLY YES
        )
    endif()

    # The dist stamps its tag into version.txt — catch a poisoned/stale cache.
    file(READ "${reflex_SOURCE_DIR}/version.txt" _reflex_dist_version)
    string(STRIP "${_reflex_dist_version}" _reflex_dist_version)
    if(NOT _reflex_dist_version STREQUAL "${ARG_VERSION}")
        message(FATAL_ERROR
            "Reflex dist at ${reflex_SOURCE_DIR} is '${_reflex_dist_version}' but ${ARG_VERSION} "
            "was requested — clear the CPM cache entry and reconfigure.")
    endif()

    set(REFLEX_DIR "${reflex_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
