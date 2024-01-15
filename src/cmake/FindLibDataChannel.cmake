# Allow user to pass a path instead of guessing
if(LIBDATACHANNEL_ROOT)
    set(_LIBDATACHANNEL_PATHS_MANUAL "${LIBDATACHANNEL_ROOT}")
elseif(CMAKE_SYSTEM_NAME MATCHES ".*[wW]indows.*")
    # MATCHES is used to work on any devies with windows in the name
    # Shameless copy-paste from FindOpenSSL.cmake v3.8
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
    list(APPEND _LIBDATACHANNEL_HINTS "${_programfiles}")

    # TODO probably other default paths?
    foreach(_TARGET_LIBDATACHANNEL_PATH "libdatachannel")
        list(APPEND _LIBDATACHANNEL_PATHS
                "${_programfiles}/${_TARGET_LIBDATACHANNEL_PATH}"
                "C:/Program Files (x86)/${_TARGET_LIBDATACHANNEL_PATH}"
                "C:/Program Files/${_TARGET_LIBDATACHANNEL_PATH}"
                "C:/${_TARGET_LIBDATACHANNEL_PATH}"
                )
    endforeach()
else()
    # Paths for anything other than Windows
    list(APPEND _LIBDATACHANNEL_PATHS
            "/usr"
            "/usr/local"
            #            "/usr/local/Cellar/libdatachannel" TODO: mac os?
            "/opt"
            "/opt/local"
            )
endif()
if(_LIBDATACHANNEL_PATHS_MANUAL)
    find_path(LibDataChannel_INCLUDE_DIR
            NAMES "rtc/rtc.h"
            PATH_SUFFIXES "include" "includes"
            PATHS ${_LIBDATACHANNEL_PATHS_MANUAL}
            NO_DEFAULT_PATH
            )
else()
    find_path(LibDataChannel_INCLUDE_DIR
            NAMES "rtc/rtc.h"
            HINTS ${_LIBDATACHANNEL_HINTS}
            PATH_SUFFIXES "include" "includes"
            PATHS ${_LIBDATACHANNEL_PATHS}
            )
endif()
# Find includes path


# Checks if the version file exists, save the version file to a var, and fail if there's no version file
if(LibDataChannel_INCLUDE_DIR)
    # file(STRINGS ${LibDataChannel_INCLUDE_DIR}/rtc/rtc.h _ver_line
    #         REGEX "^#define LIBDATACHANNEL_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
    #         LIMIT_COUNT 1)
    # string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
    #         LibDataChannel_VERSION "${_ver_line}")
    # unset(_ver_line)
    set (LibDataChannel_VERSION 1) # TODO (losty): hardcoded
else()
    if(LIBDATACHANNEL_FIND_REQUIRED)
        message(FATAL_ERROR "Failed to find libdatachannel's header file \"rtc/rtc.h\"! Try setting \"LIBDATACHANNEL_ROOT\" when initiating Cmake.")
    elseif(NOT LIBDATACHANNEL_FIND_QUIETLY)
        message(WARNING "Failed to find libdatachannel's header file \"rtc/rtc.h\"! Try setting \"LIBDATACHANNEL_ROOT\" when initiating Cmake.")
    endif()
    # Set some garbage values to the versions since we didn't find a file to read
endif()

# The actual returned/output version variable (the others can be used if needed)

# Finds the target library for libdatachannel, since they all follow the same naming conventions
macro(findpackage_libdatachannel_get_lib _LIBDATACHANNEL_OUTPUT_VARNAME _TARGET_LIBDATACHANNEL_LIB)
    if(_LIBDATACHANNEL_PATHS_MANUAL)
        find_library(${_LIBDATACHANNEL_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_LIBDATACHANNEL_LIB}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}${LibDataChannel_VERSION}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}-${LibDataChannel_VERSION}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}_${LibDataChannel_VERSION}"
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_LIBDATACHANNEL_PATHS_MANUAL}
                NO_DEFAULT_PATH
                )
    else()
        find_library(${_LIBDATACHANNEL_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_LIBDATACHANNEL_LIB}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}${LibDataChannel_VERSION}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}-${LibDataChannel_VERSION}"
                "lib${_TARGET_LIBDATACHANNEL_LIB}_${LibDataChannel_VERSION}"
                HINTS ${_LIBDATACHANNEL_HINTS}
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_LIBDATACHANNEL_PATHS}
                )
    endif()

    # If the library was found, add it to our list of libraries
    if(${_LIBDATACHANNEL_OUTPUT_VARNAME})
        # If found, append to our libraries variable
        # The ${{}} is because the first expands to target the real variable, the second expands the variable's contents...
        # and the real variable's contents is the path to the lib. Thus, it appends the path of the lib to LibDataChannel_LIBRARIES.
        list(APPEND LibDataChannel_LIBRARIES "${${_LIBDATACHANNEL_OUTPUT_VARNAME}}")
    endif()
endmacro()

# Find and set the paths of the specific library to the variable
findpackage_libdatachannel_get_lib(LIBDATACHANNEL_LIB datachannel)
findpackage_libdatachannel_get_lib(USRSCTP_LIB usrsctp)
findpackage_libdatachannel_get_lib(JUICE_LIB juice)

# Needed for find_package_handle_standard_args()
include(FindPackageHandleStandardArgs)
# Fails if required vars aren't found, or if the version doesn't meet specifications.
find_package_handle_standard_args(LibDataChannel
        FOUND_VAR LibDataChannel_FOUND
        REQUIRED_VARS
        LibDataChannel_INCLUDE_DIR
        LIBDATACHANNEL_LIB
        VERSION_VAR LibDataChannel_VERSION
        )

# Create an imported lib for easy linking by external projects
if(LibDataChannel_FOUND AND LibDataChannel_LIBRARIES AND NOT TARGET LibDataChannel::libdatachannel)
    add_library(LibDataChannel::libdatachannel INTERFACE IMPORTED)
    set_target_properties(LibDataChannel::libdatachannel PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LibDataChannel_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${LibDataChannel_LIBRARIES}"
            )
endif()

include(FindPackageMessage)
# A message that tells the user what includes/libs were found, and obeys the QUIET command.
find_package_message(LibDataChannel
        "Found LibDataChannel libraries: ${LibDataChannel_LIBRARIES}"
        "[${LibDataChannel_LIBRARIES}[${LibDataChannel_INCLUDE_DIR}]]"
        )
