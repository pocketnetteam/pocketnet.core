# Allow user to pass a path instead of guessing
if(ZMQ_ROOT)
    set(_ZMQ_PATHS_MANUAL "${ZMQ_ROOT}")
elseif(CMAKE_SYSTEM_NAME MATCHES ".*[wW]indows.*")
    # MATCHES is used to work on any devies with windows in the name
    # Shameless copy-paste from FindOpenSSL.cmake v3.8
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
    list(APPEND _ZMQ_HINTS "${_programfiles}")

    # TODO probably other default paths?
    foreach(_TARGET_ZMQ_PATH "libzmq" "zmq" "zeromq")
        list(APPEND _ZMQ_PATHS
                "${_programfiles}/${_TARGET_ZMQ_PATH}"
                "C:/Program Files (x86)/${_TARGET_ZMQ_PATH}"
                "C:/Program Files/${_TARGET_ZMQ_PATH}"
                "C:/${_TARGET_ZMQ_PATH}"
                )
    endforeach()
else()
    # Paths for anything other than Windows
    list(APPEND _ZMQ_PATHS
            "/usr"
            "/usr/local"
            "/opt"
            "/opt/local"
            "${CMAKE_PREFIX_PATH}"
            )
endif()
if(_ZMQ_PATHS_MANUAL)
    find_path(Zmq_INCLUDE_DIRS
            NAMES "zmq.h"
            PATH_SUFFIXES "include" "includes"
            PATHS ${_ZMQ_PATHS_MANUAL}
            NO_DEFAULT_PATH
            )
else()
    find_path(Zmq_INCLUDE_DIRS
            NAMES "zmq.h"
            HINTS ${_ZMQ_HINTS}
            PATH_SUFFIXES "include" "includes"
            PATHS ${_ZMQ_PATHS}
            )
endif()
# Find includes path


# Checks if the version file exists, save the version file to a var, and fail if there's no version file
if(Zmq_INCLUDE_DIRS)
    # Read the version file db.h into a variable
    file(READ "${Zmq_INCLUDE_DIRS}/zmq.h" _ZMQ_CONFIG)
    # Parse the DB version into variables to be used in the lib names
    string(REGEX REPLACE ".*ZMQ_VERSION_MAJOR ([0-9]+).*" "\\1" Zmq_VERSION_MAJOR "${_ZMQ_CONFIG}")
    string(REGEX REPLACE ".*ZMQ_VERSION_MINOR ([0-9]+).*" "\\1" Zmq_VERSION_MINOR "${_ZMQ_CONFIG}")
    # Patch version example on non-crypto installs: x.x.xNC
    string(REGEX REPLACE ".*ZMQ_VERSION_PATCH ([0-9]+).*" "\\1" Zmq_VERSION_PATCH "${_ZMQ_CONFIG}")
else()
    if(ZMQ_FIND_REQUIRED)
        message(FATAL_ERROR "Failed to find libzmq's header file \"zmq.h\"! Try setting \"ZMQ_ROOT\" when initiating Cmake.")
    elseif(NOT ZMQ_FIND_QUIETLY)
        message(WARNING "Failed to find libzmq's header file \"zmq.h\"! Try setting \"ZMQ_ROOT\" when initiating Cmake.")
    endif()
    # Set some garbage values to the versions since we didn't find a file to read
    set(Zmq_VERSION_MAJOR "0")
    set(Zmq_VERSION_MINOR "0")
    set(Zmq_VERSION_PATCH "0")
endif()

# The actual returned/output version variable (the others can be used if needed)
set(Zmq_VERSION "${Zmq_VERSION_MAJOR}.${Zmq_VERSION_MINOR}.${Zmq_VERSION_PATCH}")
set(_zmq_VERSION_UNDERSCORED "${Zmq_VERSION_MAJOR}_${Zmq_VERSION_MINOR}_${Zmq_VERSION_PATCH}")


if(MSVC)
    #add in all the names it can have on windows
    if(CMAKE_GENERATOR_TOOLSET MATCHES "v140" OR MSVC14)
        set(_zmq_TOOLSET "-v140")
    elseif(CMAKE_GENERATOR_TOOLSET MATCHES "v120" OR MSVC12)
        set(_zmq_TOOLSET "-v120")
    elseif(CMAKE_GENERATOR_TOOLSET MATCHES "v110_xp")
        set(_zmq_TOOLSET "-v110_xp")
    elseif(CMAKE_GENERATOR_TOOLSET MATCHES "v110" OR MSVC11)
        set(_zmq_TOOLSET "-v110")
    elseif(CMAKE_GENERATOR_TOOLSET MATCHES "v100" OR MSVC10)
        set(_zmq_TOOLSET "-v100")
    elseif(CMAKE_GENERATOR_TOOLSET MATCHES "v90" OR MSVC90)
        set(_zmq_TOOLSET "-v90")
    endif()

    set(_zmq_SEARCH_NAMES
            zmq libzmq "libzmq-mt-sgd-${_zmq_VERSION_UNDERSCORED}" "libzmq${_zmq_TOOLSET}-mt-sgd-${_zmq_VERSION_UNDERSCORED}")
else()
    set(_zmq_SEARCH_NAMES
            zmq libzmq)
endif()

if(_ZMQ_PATHS_MANUAL)
    find_library(ZMQ_LIBRARY
            NAMES ${_zmq_SEARCH_NAMES}
            PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
            PATHS ${_ZMQ_PATHS_MANUAL}
            NO_DEFAULT_PATH)
else()
    find_library(ZMQ_LIBRARY
            NAMES ${_zmq_SEARCH_NAMES}
            PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
            HINTS ${_ZMQ_HINTS}
            PATHS ${_ZMQ_PATHS})
endif()


# Needed for find_package_handle_standard_args()
include(FindPackageHandleStandardArgs)
# Fails if required vars aren't found, or if the version doesn't meet specifications.
find_package_handle_standard_args(ZMQ
        FOUND_VAR ZMQ_FOUND
        REQUIRED_VARS
            Zmq_INCLUDE_DIRS
            ZMQ_LIBRARY
        VERSION_VAR Zmq_VERSION
        )

# Create an imported lib for easy linking by external projects
if(ZMQ_FOUND AND ZMQ_LIBRARY AND NOT TARGET ZMQ::zmq)
    add_library(ZMQ::zmq INTERFACE IMPORTED)
    set_target_properties(ZMQ::zmq PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Zmq_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${ZMQ_LIBRARY}"
            )
endif()

include(FindPackageMessage)
# A message that tells the user what includes/libs were found, and obeys the QUIET command.
find_package_message(ZMQ
        "Found ZMQ libraries: ${ZMQ_LIBRARY}"
        "[${ZMQ_LIBRARY}[${Zmq_INCLUDE_DIRS}]]"
        )