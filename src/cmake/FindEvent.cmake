# Allow user to pass a path instead of guessing
if(EVENT_ROOT)
    set(_EVENT_PATHS_MANUAL "${EVENT_ROOT}")
elseif(CMAKE_SYSTEM_NAME MATCHES ".*[wW]indows.*")
    # MATCHES is used to work on any devies with windows in the name
    # Shameless copy-paste from FindOpenSSL.cmake v3.8
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
    list(APPEND _EVENT_HINTS "${_programfiles}")

    # TODO probably other default paths?
    foreach(_TARGET_EVENT_PATH "libevent")
        list(APPEND _EVENT_PATHS
                "${_programfiles}/${_TARGET_EVENT_PATH}"
                "C:/Program Files (x86)/${_TARGET_EVENT_PATH}"
                "C:/Program Files/${_TARGET_EVENT_PATH}"
                "C:/${_TARGET_EVENT_PATH}"
                )
    endforeach()
else()
    # Paths for anything other than Windows
    list(APPEND _EVENT_PATHS
            "/usr"
            "/usr/local"
            #            "/usr/local/Cellar/libevent" TODO: mac os?
            "/opt"
            "/opt/local"
            )
endif()
if(_EVENT_PATHS_MANUAL)
    find_path(Event_INCLUDE_DIRS
            NAMES "event.h"
            PATH_SUFFIXES "include" "includes"
            PATHS ${_EVENT_PATHS_MANUAL}
            NO_DEFAULT_PATH
            )
else()
    find_path(Event_INCLUDE_DIRS
            NAMES "event.h"
            HINTS ${_EVENT_HINTS}
            PATH_SUFFIXES "include" "includes"
            PATHS ${_EVENT_PATHS}
            )
endif()
# Find includes path


# Checks if the version file exists, save the version file to a var, and fail if there's no version file
if(Event_INCLUDE_DIRS)
    # Read the version file db.h into a variable
    file(READ "${Event_INCLUDE_DIRS}/event2/event-config.h" _EVENT_CONFIG)
    # Parse the DB version into variables to be used in the lib names
    string(REGEX REPLACE ".*EVENT__VERSION_MAJOR ([0-9]+).*" "\\1" Event_VERSION_MAJOR "${_EVENT_CONFIG}")
    string(REGEX REPLACE ".*EVENT__VERSION_MINOR ([0-9]+).*" "\\1" Event_VERSION_MINOR "${_EVENT_CONFIG}")
    # Patch version example on non-crypto installs: x.x.xNC
    string(REGEX REPLACE ".*EVENT__VERSION_PATCH ([0-9]+).*" "\\1" Event_VERSION_PATCH "${_EVENT_CONFIG}")
else()
    if(EVENT_FIND_REQUIRED)
        message(FATAL_ERROR "Failed to find libevent's header file \"event.h\"! Try setting \"EVENT_ROOT\" when initiating Cmake.")
    elseif(NOT EVENT_FIND_QUIETLY)
        message(WARNING "Failed to find libevent's header file \"event.h\"! Try setting \"EVENT_ROOT\" when initiating Cmake.")
    endif()
    # Set some garbage values to the versions since we didn't find a file to read
    set(Event_VERSION_MAJOR "0")
    set(Event_VERSION_MINOR "0")
    set(Event_VERSION_PATCH "0")
endif()

# The actual returned/output version variable (the others can be used if needed)
set(Event_VERSION "${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}.${Event_VERSION_PATCH}")

# Finds the target library for event, since they all follow the same naming conventions
macro(findpackage_event_get_lib _EVENT_OUTPUT_VARNAME _TARGET_EVENT_LIB)
    # Different systems sometimes have a version in the lib name...
    # and some have a dash or underscore before the versions.
    # CMake recommends to put unversioned names before versioned names
    if(_EVENT_PATHS_MANUAL)
        find_library(${_EVENT_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_EVENT_LIB}"
                "lib${_TARGET_EVENT_LIB}"
                "lib${_TARGET_EVENT_LIB}${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}-${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}_${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}${Event_VERSION_MAJOR}${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}-${Event_VERSION_MAJOR}${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}_${Event_VERSION_MAJOR}${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}${Event_VERSION_MAJOR}"
                "lib${_TARGET_EVENT_LIB}-${Event_VERSION_MAJOR}"
                "lib${_TARGET_EVENT_LIB}_${Event_VERSION_MAJOR}"
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_EVENT_PATHS_MANUAL}
                NO_DEFAULT_PATH
                )
    else()
        find_library(${_EVENT_OUTPUT_VARNAME}
                NAMES
                "${_TARGET_EVENT_LIB}"
                "lib${_TARGET_EVENT_LIB}"
                "lib${_TARGET_EVENT_LIB}${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}-${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}_${Event_VERSION_MAJOR}.${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}${Event_VERSION_MAJOR}${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}-${Event_VERSION_MAJOR}${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}_${Event_VERSION_MAJOR}${Event_VERSION_MINOR}"
                "lib${_TARGET_EVENT_LIB}${Event_VERSION_MAJOR}"
                "lib${_TARGET_EVENT_LIB}-${Event_VERSION_MAJOR}"
                "lib${_TARGET_EVENT_LIB}_${Event_VERSION_MAJOR}"
                HINTS ${_EVENT_HINTS}
                PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
                PATHS ${_EVENT_PATHS}
                )
    endif()

    # If the library was found, add it to our list of libraries
    if(${_EVENT_OUTPUT_VARNAME})
        # If found, append to our libraries variable
        # The ${{}} is because the first expands to target the real variable, the second expands the variable's contents...
        # and the real variable's contents is the path to the lib. Thus, it appends the path of the lib to Event_LIBRARIES.
        list(APPEND Event_LIBRARIES "${${_EVENT_OUTPUT_VARNAME}}")
    endif()
endmacro()

# Find and set the paths of the specific library to the variable
findpackage_event_get_lib(EVENT_LIB "event")
# NOTE: Windows doesn't have an event_pthreads lib, but instead compiles the cxx code into the core lib
findpackage_event_get_lib(EVENT_PTHREADS_LIB "event_pthreads")
findpackage_event_get_lib(EVENT_CORE_LIB "event_core")
findpackage_event_get_lib(EVENT_EXTRA_LIB "event_extra")

if(COPY_DLL)
    foreach(LIB ${Event_LIBRARIES})
        string(REGEX_REPLACE)
        #        configure_file(${LIB})
    endforeach()
endif()

# Needed for find_package_handle_standard_args()
include(FindPackageHandleStandardArgs)
# Fails if required vars aren't found, or if the version doesn't meet specifications.
find_package_handle_standard_args(Event
        FOUND_VAR Event_FOUND
        REQUIRED_VARS
        Event_INCLUDE_DIRS
        EVENT_LIB
        VERSION_VAR Event_VERSION
        )

# Create an imported lib for easy linking by external projects
if(Event_FOUND AND Event_LIBRARIES AND NOT TARGET Event::event)
    add_library(Event::event INTERFACE IMPORTED)
    set_target_properties(Event::event PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Event_INCLUDE_DIRS}"
            IMPORTED_LOCATION "${EVENT_LIB}"
            INTERFACE_LINK_LIBRARIES "${Event_LIBRARIES}"
            )
endif()

include(FindPackageMessage)
# A message that tells the user what includes/libs were found, and obeys the QUIET command.
find_package_message(Event
        "Found Event libraries: ${Event_LIBRARIES}"
        "[${Event_LIBRARIES}[${Event_INCLUDE_DIRS}]]"
        )