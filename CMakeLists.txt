cmake_minimum_required(VERSION 3.15)

set(_CLIENT_VERSION_MAJOR 0)
set(_CLIENT_VERSION_MINOR 22)
set(_CLIENT_VERSION_REVISION 4)
set(_CLIENT_VERSION_BUILD 0)
set(_CLIENT_VERSION_IS_RELEASE false)
set(_COPYRIGHT_YEAR 2024)

if(WIN32)
    set(_COPYRIGHT_HOLDERS "\"The %%s developers\"")
else()
    set(_COPYRIGHT_HOLDERS "\"The %s developers\"")
endif()

set(_COPYRIGHT_HOLDERS_SUBSTITUTION "\"Pocketnet\"")
set(_HOMEPAGE "\"https://github.com/pocketnetteam/\"")
string(REPLACE "%s" ${_COPYRIGHT_HOLDERS_SUBSTITUTION} _COPYRIGHT_HOLDERS_FINAL ${_COPYRIGHT_HOLDERS_SUBSTITUTION})
include(init.cmake)
init("\"Pocketnet Core\"" "\"${_CLIENT_VERSION_MAJOR}.${_CLIENT_VERSION_MINOR}.${_CLIENT_VERSION_REVISION}\"" "\"https://github.com/pocketnetteam/pocketnet.core/issues\"" "\"pocketnetcore\"" "${_HOMEPAGE}")
add_compile_definitions(CLIENT_VERSION_MAJOR=${_CLIENT_VERSION_MAJOR}
                        CLIENT_VERSION_MINOR=${_CLIENT_VERSION_MINOR}
                        CLIENT_VERSION_REVISION=${_CLIENT_VERSION_REVISION}
                        CLIENT_VERSION_BUILD=${_CLIENT_VERSION_BUILD}
                        CLIENT_VERSION_IS_RELEASE=${_CLIENT_VERSION_IS_RELEASE}
                        COPYRIGHT_YEAR=${_COPYRIGHT_YEAR}
                        COPYRIGHT_HOLDERS=${_COPYRIGHT_HOLDERS}
                        COPYRIGHT_HOLDERS_SUBSTITUTION=${_COPYRIGHT_HOLDERS_SUBSTITUTION}
                        COPYRIGHT_HOLDERS_FINAL=${_COPYRIGHT_HOLDERS_FINAL}
                        HOMEPAGE=${_HOMEPAGE})

# TODO (build): missing bug report url
project (
        Pocketnet-Core 
        VERSION ${_CLIENT_VERSION_MAJOR}.${_CLIENT_VERSION_MINOR}.${_CLIENT_VERSION_REVISION}
        HOMEPAGE_URL ${_HOMEPAGE}
        )

add_subdirectory(src/)