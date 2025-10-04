# Allow user to pass a path instead of guessing
if(SQLITE_ROOT)
    set(_SQLITE_PATHS_MANUAL "${SQLITE_ROOT}")
elseif(CMAKE_SYSTEM_NAME MATCHES ".*[wW]indows.*")
    # MATCHES is used to work on any devies with windows in the name
    # Shameless copy-paste from FindOpenSSL.cmake v3.8
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
    list(APPEND _SQLITE_HINTS "${_programfiles}")

    foreach(_TARGET_SQLITE_PATH "sqlite3")
        list(APPEND _SQLITE_PATHS
                "${_programfiles}/${_TARGET_SQLITE_PATH}"
                "C:/Program Files (x86)/${_TARGET_SQLITE_PATH}"
                "C:/Program Files/${_TARGET_SQLITE_PATH}"
                "C:/${_TARGET_SQLITE_PATH}"
                )
    endforeach()
else()
    # Paths for anything other than Windows
    list(APPEND _SQLITE_PATHS
            "/usr"
            "/usr/local"
            "/opt"
            "/opt/local"
            )
endif()
if(_SQLITE_PATHS_MANUAL)
    find_path(SQlite3_INCLUDE_DIR
            NAMES "sqlite3.h"
            PATH_SUFFIXES "include" "includes"
            PATHS ${_SQLITE_PATHS_MANUAL}
            NO_DEFAULT_PATH
            )
else()
    find_path(SQlite3_INCLUDE_DIR
            NAMES "sqlite3.h"
            HINTS ${_SQLITE_HINTS}
            PATH_SUFFIXES "include" "includes"
            PATHS ${_SQLITE_PATHS}
            )
endif()
# Find includes path


# Checks if the version file exists, save the version file to a var, and fail if there's no version file
if(SQlite3_INCLUDE_DIR)
    file(STRINGS ${SQlite3_INCLUDE_DIR}/sqlite3.h _ver_line
            REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
            LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
            SQLite3_VERSION "${_ver_line}")
    unset(_ver_line)
else()
    if(SQLITE3_FIND_REQUIRED)
        message(FATAL_ERROR "Failed to find sqlite's header file \"sqlite3.h\"! Try setting \"SQLITE_ROOT\" when initiating Cmake.")
    elseif(NOT SQLITE3_FIND_QUIETLY)
        message(WARNING "Failed to find sqlite's header file \"sqlite3.h\"! Try setting \"SQLITE_ROOT\" when initiating Cmake.")
    endif()
    # Set some garbage values to the versions since we didn't find a file to read
endif()

# The actual returned/output version variable (the others can be used if needed)

# Finds the target library for sqlite, since they all follow the same naming conventions
macro(findpackage_sqlite_get_lib _SQLITE_OUTPUT_VARNAME _TARGET_SQLITE_LIB)
    if(_SQLITE_PATHS_MANUAL)
        find_library(${_SQLITE_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_SQLITE_LIB}"
                "lib${_TARGET_SQLITE_LIB}"
                "lib${_TARGET_SQLITE_LIB}${SQLite3_VERSION}"
                "lib${_TARGET_SQLITE_LIB}-${SQLite3_VERSION}"
                "lib${_TARGET_SQLITE_LIB}_${SQLite3_VERSION}"
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_SQLITE_PATHS_MANUAL}
                NO_DEFAULT_PATH
                )
    else()
        find_library(${_SQLITE_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_SQLITE_LIB}"
                "lib${_TARGET_SQLITE_LIB}"
                "lib${_TARGET_SQLITE_LIB}"
                "lib${_TARGET_SQLITE_LIB}${SQLite3_VERSION}"
                "lib${_TARGET_SQLITE_LIB}-${SQLite3_VERSION}"
                "lib${_TARGET_SQLITE_LIB}_${SQLite3_VERSION}"
                HINTS ${_SQLITE_HINTS}
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_SQLITE_PATHS}
                )
    endif()

    # If the library was found, add it to our list of libraries
    if(${_SQLITE_OUTPUT_VARNAME})
        # If found, append to our libraries variable
        # The ${{}} is because the first expands to target the real variable, the second expands the variable's contents...
        # and the real variable's contents is the path to the lib. Thus, it appends the path of the lib to SQlite3_LIBRARIES.
        list(APPEND SQlite3_LIBRARIES "${${_SQLITE_OUTPUT_VARNAME}}")
    endif()
endmacro()

# Find and set the paths of the specific library to the variable
findpackage_sqlite_get_lib(SQLITE_LIB sqlite)
findpackage_sqlite_get_lib(SQLITE3_LIB sqlite3)

if(SQLITE_LIB)
    set(SQLITE_REAL_LIB ${SQLITE_LIB})
else()
    set(SQLITE_REAL_LIB ${SQLITE3_LIB})
endif()

# Needed for find_package_handle_standard_args()
include(FindPackageHandleStandardArgs)
# Fails if required vars aren't found, or if the version doesn't meet specifications.
find_package_handle_standard_args(SQlite
        FOUND_VAR SQlite_FOUND
        REQUIRED_VARS
        SQlite3_INCLUDE_DIR
        SQLITE_REAL_LIB
        VERSION_VAR SQLite3_VERSION
        )

# Create an imported lib for easy linking by external projects
if(SQlite_FOUND AND SQlite3_LIBRARIES AND NOT TARGET SQlite::sqlite)
    add_library(SQlite::sqlite INTERFACE IMPORTED)
    set_target_properties(SQlite::sqlite PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SQlite3_INCLUDE_DIR}"
            IMPORTED_LOCATION "${SQLITE_REAL_LIB}"
            INTERFACE_LINK_LIBRARIES "${SQlite3_LIBRARIES}"
            )
endif()

include(FindPackageMessage)
# A message that tells the user what includes/libs were found, and obeys the QUIET command.
find_package_message(SQlite
        "Found SQlite3 libraries: ${SQlite3_LIBRARIES}"
        "[${SQlite3_LIBRARIES}[${SQlite3_INCLUDE_DIR}]]"
        )