cmake_minimum_required(VERSION 3.20)

# TODO (release): set version and actual year
set(CLIENT_NAME "Pocketnet Core")
set(CLIENT_VERSION_MAJOR 0)
set(CLIENT_VERSION_MINOR 22)
set(CLIENT_VERSION_REVISION 19)
set(CLIENT_VERSION_BUILD 0)
set(CLIENT_VERSION_IS_RELEASE false)
set(COPYRIGHT_YEAR 2025)

project (
        PocketnetCore 
        VERSION ${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_REVISION}
        HOMEPAGE_URL "https://github.com/pocketnetteam/"
        )

if(WIN32)
    set(COPYRIGHT_HOLDERS "The %%s developers")
else()
    set(COPYRIGHT_HOLDERS "The %s developers")
endif()
set(COPYRIGHT_HOLDERS_SUBSTITUTION "${CLIENT_NAME}")
set(COPYRIGHT_HOLDERS_FINAL "The ${COPYRIGHT_HOLDERS_SUBSTITUTION} developers")
set(PACKAGE_BUGREPORT "https://github.com/pocketnetteam/pocketnet.core/issues")
set(PACKAGE_TARNAME "pocketnetcore")

#[[
include(init.cmake)
init("${CLIENT_NAME}" "${PROJECT_VERSION}" "${_PACKAGE_BUGREPORT}" "${_PACKAGE_TARNAME}" "${PROJECT_HOMEPAGE_URL}")

add_compile_definitions(CLIENT_VERSION_MAJOR=${_CLIENT_VERSION_MAJOR}
                        CLIENT_VERSION_MINOR=${_CLIENT_VERSION_MINOR}
                        CLIENT_VERSION_REVISION=${_CLIENT_VERSION_REVISION}
                        CLIENT_VERSION_BUILD=${_CLIENT_VERSION_BUILD}
                        CLIENT_VERSION_IS_RELEASE=${_CLIENT_VERSION_IS_RELEASE}
                        COPYRIGHT_YEAR=${_COPYRIGHT_YEAR}
                        COPYRIGHT_HOLDERS="${_COPYRIGHT_HOLDERS}"
                        COPYRIGHT_HOLDERS_SUBSTITUTION="${_COPYRIGHT_HOLDERS_SUBSTITUTION}"
                        COPYRIGHT_HOLDERS_FINAL="${_COPYRIGHT_HOLDERS_FINAL}"
                        HOMEPAGE="${PROJECT_HOMEPAGE_URL}")
]]

add_subdirectory(src/)