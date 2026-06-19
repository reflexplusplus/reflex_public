# FetchReflexLibs.cmake
# Downloads prebuilt Reflex SDK binaries from the consumer repo's GitHub Release.
#
# Hosting model: release assets are mirrored to each target repo's Releases, so a
# consumer fetches from its own origin remote:
#   - reflex_public (public)  : anonymous download, no token needed
#   - reflex (licensed/private): set REFLEX_GITHUB_TOKEN
#
# Target-awareness (REFLEX_DISTRIBUTION = public | licensed):
#   public   -> fetch libs + tools + docs   (reflex_public has no src/ or build/)
#   licensed -> fetch libs + docs           (reflex builds tools locally)
# Auto-detected from the presence of build/tools/ unless set explicitly.
#
# Version source (first that resolves): REFLEX_VERSION, version.txt, "latest".
# tools/docs assets exist for macOS + Windows only.
#
# This is a no-op when libraries are already present without a fetch marker
# (i.e. a local source build), so it never clobbers locally built libs.

# Map the CMake platform to the release-asset platform name.
function(_reflex_asset_platform _out)
    if(WIN32)
        set(${_out} "windows" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")   # must precede APPLE (iOS is APPLE)
        set(${_out} "ios" PARENT_SCOPE)
    elseif(APPLE)
        set(${_out} "macos" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(${_out} "linux" PARENT_SCOPE)
    else()
        set(${_out} "" PARENT_SCOPE)  # Android (AAR via Gradle) / WebAssembly: no fetch
    endif()
endfunction()

# Find an asset by name in the release JSON; returns its API url and browser url.
function(_reflex_find_asset _json _name _api_url_out _browser_url_out)
    set(${_api_url_out} "" PARENT_SCOPE)
    set(${_browser_url_out} "" PARENT_SCOPE)

    string(JSON _assets ERROR_VARIABLE _err GET "${_json}" assets)
    if(_err)
        return()
    endif()
    string(JSON _count ERROR_VARIABLE _err LENGTH "${_assets}")
    if(_err OR _count EQUAL 0)
        return()
    endif()

    math(EXPR _last "${_count} - 1")
    foreach(_i RANGE ${_last})
        string(JSON _asset GET "${_assets}" ${_i})
        string(JSON _aname GET "${_asset}" name)
        if("${_aname}" STREQUAL "${_name}")
            string(JSON _aurl GET "${_asset}" url)
            string(JSON _burl GET "${_asset}" browser_download_url)
            set(${_api_url_out} "${_aurl}" PARENT_SCOPE)
            set(${_browser_url_out} "${_burl}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# Download one asset (token-optional) and extract it into _dest_dir.
# Returns 0 on success in _result.
function(_reflex_fetch_asset _json _token _asset_name _dest_dir _result)
    set(${_result} 1 PARENT_SCOPE)

    _reflex_find_asset("${_json}" "${_asset_name}" _api_url _browser_url)
    if("${_api_url}" STREQUAL "")
        message(STATUS "Reflex: asset not found in release: ${_asset_name}")
        return()
    endif()

    set(_tmp "${CMAKE_CURRENT_BINARY_DIR}/_reflex_${_asset_name}")
    if(NOT "${_token}" STREQUAL "")
        # Private repo: asset API endpoint requires the octet-stream Accept header.
        file(DOWNLOAD "${_api_url}" "${_tmp}"
            HTTPHEADER "Authorization: Bearer ${_token}"
            HTTPHEADER "Accept: application/octet-stream"
            STATUS _dl_status)
    else()
        # Public repo: anonymous download via the browser URL.
        file(DOWNLOAD "${_browser_url}" "${_tmp}" STATUS _dl_status)
    endif()

    list(GET _dl_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        list(GET _dl_status 1 _dl_msg)
        message(WARNING "Reflex: failed to download ${_asset_name}: ${_dl_msg}")
        file(REMOVE "${_tmp}")
        return()
    endif()

    file(MAKE_DIRECTORY "${_dest_dir}")
    file(ARCHIVE_EXTRACT INPUT "${_tmp}" DESTINATION "${_dest_dir}")
    file(REMOVE "${_tmp}")
    set(${_result} 0 PARENT_SCOPE)
endfunction()

function(reflex_fetch_libs)
    _reflex_asset_platform(_platform)
    if("${_platform}" STREQUAL "")
        message(STATUS "Reflex: no prebuilt assets for this platform; skipping fetch")
        return()
    endif()

    # Distribution flavour.
    if(NOT DEFINED REFLEX_DISTRIBUTION)
        if(EXISTS "${REFLEX_ROOT}/build/tools")
            set(REFLEX_DISTRIBUTION "licensed")
        else()
            set(REFLEX_DISTRIBUTION "public")
        endif()
    endif()

    # Resolve version. version.txt is generated from the git tag and baked into
    # exported/distributed trees (the only place this fetch runs), so there is no
    # git-tag fallback: a downloaded zip has no .git to describe.
    if(DEFINED REFLEX_VERSION AND NOT "${REFLEX_VERSION}" STREQUAL "")
        set(_version "${REFLEX_VERSION}")
    elseif(EXISTS "${REFLEX_ROOT}/version.txt")
        file(READ "${REFLEX_ROOT}/version.txt" _version)
        string(STRIP "${_version}" _version)
    else()
        set(_version "latest")
    endif()

    # Decide whether a fetch is needed (and never clobber a local source build).
    set(_lib_dir "${REFLEX_ROOT}/bin/lib")
    set(_marker "${_lib_dir}/.reflex-libs-version")
    set(_wanted "${_version}-${_platform}-${REFLEX_DISTRIBUTION}")
    set(_core_lib "${_REFLEX_LIB_DIR_REL}/${_REFLEX_LIB_PREFIX}ReflexCommon${_REFLEX_LIB_SUFFIX}")

    if(EXISTS "${_marker}")
        file(READ "${_marker}" _have)
        string(STRIP "${_have}" _have)
        if("${_have}" STREQUAL "${_wanted}")
            message(STATUS "Reflex: prebuilt binaries up to date (${_version})")
            return()
        endif()
    elseif(EXISTS "${_core_lib}")
        # Libraries present with no fetch marker: a local build. Leave it alone.
        return()
    endif()

    # Resolve the repo (REFLEX_GITHUB_REPO, else the origin remote).
    set(_repo "$ENV{REFLEX_GITHUB_REPO}")
    if("${_repo}" STREQUAL "")
        execute_process(
            COMMAND git -C "${REFLEX_ROOT}" remote get-url origin
            OUTPUT_VARIABLE _remote OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        string(REGEX MATCH "[:/]([^/]+/[^/.]+)(\\.git)?$" _m "${_remote}")
        set(_repo "${CMAKE_MATCH_1}")
    endif()
    if("${_repo}" STREQUAL "")
        message(WARNING "Reflex: cannot determine GitHub repo; skipping binary fetch")
        return()
    endif()

    set(_token "$ENV{REFLEX_GITHUB_TOKEN}")  # optional (public repos need none)

    message(STATUS "Reflex: fetching prebuilt binaries (${_version}, ${_platform}, ${REFLEX_DISTRIBUTION})")

    # Query release metadata for this version.
    if("${_version}" STREQUAL "latest")
        set(_api "https://api.github.com/repos/${_repo}/releases/latest")
    else()
        set(_api "https://api.github.com/repos/${_repo}/releases/tags/${_version}")
    endif()

    set(_json_tmp "${CMAKE_CURRENT_BINARY_DIR}/_reflex_release.json")
    if(NOT "${_token}" STREQUAL "")
        file(DOWNLOAD "${_api}" "${_json_tmp}"
            HTTPHEADER "Authorization: Bearer ${_token}"
            HTTPHEADER "Accept: application/vnd.github+json"
            STATUS _api_status)
    else()
        file(DOWNLOAD "${_api}" "${_json_tmp}"
            HTTPHEADER "Accept: application/vnd.github+json"
            STATUS _api_status)
    endif()
    list(GET _api_status 0 _api_code)
    if(NOT _api_code EQUAL 0)
        list(GET _api_status 1 _api_msg)
        message(WARNING "Reflex: release lookup failed for ${_version}: ${_api_msg}")
        return()
    endif()
    file(READ "${_json_tmp}" _json)

    # file(DOWNLOAD) only reports transport errors, not HTTP status. A 404
    # (no release for tag), 401/403 (bad token / private repo), or rate-limit
    # returns a JSON error body with no "assets". Detect that and explain it,
    # rather than later failing with a misleading "asset not found".
    string(JSON _release_assets ERROR_VARIABLE _json_err GET "${_json}" assets)
    if(_json_err)
        message(WARNING
            "Reflex: no usable release for '${_version}' on ${_repo}. "
            "Likely the release is missing, the token is invalid, or the GitHub "
            "API rate limit was hit. For private repos (or to lift the anonymous "
            "rate limit) set REFLEX_GITHUB_TOKEN.")
        file(REMOVE "${_json_tmp}")
        return()
    endif()

    # Libraries (all platforms) -> bin/lib.
    _reflex_fetch_asset("${_json}" "${_token}" "reflex-libs-${_platform}.zip" "${_lib_dir}" _libs_rc)
    if(NOT _libs_rc EQUAL 0)
        message(WARNING "Reflex: could not fetch libraries; aborting")
        file(REMOVE "${_json_tmp}")
        return()
    endif()

    set(_tools_dir "${REFLEX_ROOT}/bin/tools")

    # Tools (public flavour only; macOS/Windows only) -> bin/tools.
    if("${REFLEX_DISTRIBUTION}" STREQUAL "public" AND NOT "${_platform}" STREQUAL "linux")
        _reflex_fetch_asset("${_json}" "${_token}" "reflex-tools-${_platform}.zip" "${_tools_dir}" _tools_rc)
    endif()

    # ReflexDocumentation (both flavours; macOS/Windows only) -> bin/tools.
    if(NOT "${_platform}" STREQUAL "linux")
        _reflex_fetch_asset("${_json}" "${_token}" "reflex-docs-${_platform}.zip" "${_tools_dir}" _docs_rc)
    endif()

    file(REMOVE "${_json_tmp}")
    file(WRITE "${_marker}" "${_wanted}\n")
    message(STATUS "Reflex: prebuilt binaries installed (${_version})")
endfunction()
