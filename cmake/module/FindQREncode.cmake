# Allow user to pass a path instead of guessing
if(QRENCODE_ROOT)
    set(_QRENCODE_PATHS_MANUAL "${QRENCODE_ROOT}")
elseif(CMAKE_SYSTEM_NAME MATCHES ".*[wW]indows.*")
    # MATCHES is used to work on any devies with windows in the name
    # Shameless copy-paste from FindOpenSSL.cmake v3.8
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
    list(APPEND _QRENCODE_HINTS "${_programfiles}")

    # TODO probably other default paths?
    foreach(_TARGET_QRENCODE_PATH "libqrencode")
        list(APPEND _QRENCODE_PATHS
                "${_programfiles}/${_TARGET_QRENCODE_PATH}"
                "C:/Program Files (x86)/${_TARGET_QRENCODE_PATH}"
                "C:/Program Files/${_TARGET_QRENCODE_PATH}"
                "C:/${_TARGET_QRENCODE_PATH}"
                )
    endforeach()
else()
    # Paths for anything other than Windows
    list(APPEND _QRENCODE_PATHS
            "/usr"
            "/usr/local"
            #            "/usr/local/Cellar/libqrencode" TODO: mac os?
            "/opt"
            "/opt/local"
            )
endif()
if(_QRENCODE_PATHS_MANUAL)
    find_path(QREncode_INCLUDE_DIRS
            NAMES "qrencode.h"
            PATH_SUFFIXES "include" "includes"
            PATHS ${_QRENCODE_PATHS_MANUAL}
            NO_DEFAULT_PATH
            )
else()
    find_path(QREncode_INCLUDE_DIRS
            NAMES "qrencode.h"
            HINTS ${_QRENCODE_HINTS}
            PATH_SUFFIXES "include" "includes"
            PATHS ${_QRENCODE_PATHS}
            )
endif()
# Find includes path


# Checks if the version file exists, save the version file to a var, and fail if there's no version file
if(NOT QREncode_INCLUDE_DIRS)
    if(QRENCODE_FIND_REQUIRED)
        message(FATAL_ERROR "Failed to find libqrencode's header file \"qrencode.h\"! Try setting \"QRENCODE_ROOT\" when initiating Cmake.")
    elseif(NOT QRENCODE_FIND_QUIETLY)
        message(WARNING "Failed to find libqrencode's header file \"qrencode.h\"! Try setting \"QRENCODE_ROOT\" when initiating Cmake.")
    endif()
    # Set some garbage values to the versions since we didn't find a file to read
endif()

# Finds the target library for qrencode, since they all follow the same naming conventions
macro(findpackage_qrencode_get_lib _QRENCODE_OUTPUT_VARNAME _TARGET_QRENCODE_LIB)
    # Different systems sometimes have a version in the lib name...
    # and some have a dash or underscore before the versions.
    # CMake recommends to put unversioned names before versioned names
    if(_QRENCODE_PATHS_MANUAL)
        find_library(${_QRENCODE_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_QRENCODE_LIB}"
                "lib${_TARGET_QRENCODE_LIB}"
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_QRENCODE_PATHS_MANUAL}
                NO_DEFAULT_PATH
                )
    else()
        find_library(${_QRENCODE_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_QRENCODE_LIB}"
                "lib${_TARGET_QRENCODE_LIB}"
                HINTS ${_QRENCODE_HINTS}
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_QRENCODE_PATHS}
                )
    endif()

    # If the library was found, add it to our list of libraries
    if(${_QRENCODE_OUTPUT_VARNAME})
        # If found, append to our libraries variable
        # The ${{}} is because the first expands to target the real variable, the second expands the variable's contents...
        # and the real variable's contents is the path to the lib. Thus, it appends the path of the lib to QREncode_LIBRARIES.
        list(APPEND QREncode_LIBRARIES "${${_QRENCODE_OUTPUT_VARNAME}}")
    endif()
endmacro()

# Find and set the paths of the specific library to the variable
if (MSVC)
# dirty hack. MSVC requires all dependencies to be built with same configurations as target project.
# Because of pocketcoin supports only debug builds due to runtime assertions, all external dependencies for MSVC build should be built with debug too.
# Because of libqrencode has different names for debug and release versions (at least with vcpkg and build from source), we just hardcode it here to use debug version.
    findpackage_qrencode_get_lib(QRENCODE_LIB "qrencoded")
else()
# Linux doesn't care for build configurations of external dependencies and even doesn't provide debug versions with package managers. So just use find libqrencode as common
    findpackage_qrencode_get_lib(QRENCODE_LIB "qrencode")
endif()

# Needed for find_package_handle_standard_args()
include(FindPackageHandleStandardArgs)
# Fails if required vars aren't found, or if the version doesn't meet specifications.
find_package_handle_standard_args(QREncode
    FOUND_VAR QREncode_FOUND
    REQUIRED_VARS
    QREncode_INCLUDE_DIRS
    QRENCODE_LIB
    QREncode_LIBRARIES
)

# Create an imported lib for easy linking by external projects
if(QREncode_FOUND AND QREncode_LIBRARIES AND NOT TARGET QREncode::qrencode)
    add_library(QREncode::qrencode INTERFACE IMPORTED)
    set_target_properties(QREncode::qrencode PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${QREncode_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${QRENCODE_LIB}"
        INTERFACE_LINK_LIBRARIES "${QREncode_LIBRARIES}"
    )
endif()

include(FindPackageMessage)
# A message that tells the user what includes/libs were found, and obeys the QUIET command.
find_package_message(QREncode
    "Found qrencode libraries: ${QREncode_LIBRARIES}"
    "[${QREncode_LIBRARIES}[${QREncode_INCLUDE_DIRS}]]"
)