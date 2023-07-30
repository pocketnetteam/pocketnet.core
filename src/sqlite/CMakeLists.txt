cmake_minimum_required(VERSION 3.15)

project(sqlite VERSION 3.42.0 LANGUAGES C)

include(CheckFunctionExists)
include(CheckCSourceCompiles)


add_library(sqlite3 sqlite3.c include/sqlite3.h include/sqlite3ext.h)
target_include_directories(sqlite3 PUBLIC include/)

foreach(_FUNC fdatasync usleep localtime_r gmtime_r isnan utime)
    check_function_exists(${_FUNC} _DOES_EXISTS_${_FUNC})
    if(_DOES_EXISTS_${_FUNC})
        string(TOUPPER ${_FUNC} _FUNCNAME)
        target_compile_definitions(sqlite3 PRIVATE HAVE_${_FUNCNAME}=1)
    endif()
endforeach()

if(NOT WIN32)
    set(CMAKE_REQUIRED_FLAGS_BACKUP ${CMAKE_REQUIRED_FLAGS})
    set(CMAKE_REQUIRED_FLAGS -Werror)
    # Check for strerror_r
    check_c_source_compiles("#include <string.h>
                            #include <stdlib.h>
                            int main() {
                                const int buflen = 10;
                                char* buf = malloc(buflen);
                                strerror_r(0, buf, buflen);
                            }" _HAVE_STRERROR_R)
    if(_HAVE_STRERROR_R)
        add_compile_definitions(HAVE_STRERROR_R=1)

        # Checking if strerror_r returns char*
        check_c_source_compiles("#include <string.h>
                                #include <stdlib.h>
                                int main() {
                                    const int buflen = 10;
                                    char* buf = malloc(buflen);
                                    char* res = strerror_r(0, buf, buflen);
                                }" _STRERROR_R_CHAR_P)
        if(_STRERROR_R_CHAR_P)
            add_compile_definitions(STRERROR_R_CHAR_P=1)
        endif()
    endif()
    set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_BACKUP})
endif()
if (${CMAKE_VERSION} VERSION_LESS "3.20")
    include(TestBigEndian)
    TEST_BIG_ENDIAN(CMAKE_C_BYTE_ORDER)
    if (${CMAKE_C_BYTE_ORDER} EQUAL 1)
        set(BYTE_ORDER 4321)
    else()
        set(BYTE_ORDER 1234)
    endif()
else()
    if(${CMAKE_C_BYTE_ORDER} STREQUAL "BIG_ENDIAN")
        set(BYTE_ORDER 4321)
    else()
        set(BYTE_ORDER 1234)
    endif()
endif()

target_compile_definitions(sqlite3 PRIVATE SQLITE_BYTEORDER=${BYTE_ORDER})


option(THREAD_SAFE "build a thread-safe library [default=ON]" ON)
if(THREAD_SAFE)
    add_compile_definitions(SQLITE_THREADSAFE=1)
    find_package(Threads REQUIRED)
#    TODO does this linkage really needed on WIN32?
    target_link_libraries(sqlite3 PRIVATE Threads::Threads)
else()
    add_compile_definitions(SQLITE_THREADSAFE=0 _REENTRANT=1)
endif()

option(ENABLE_MATH "SQL math functions [default=ON]" ON)
if(ENABLE_MATH)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_MATH_FUNCTIONS=1)
endif()

option(ENABLE_FTS4 "include fts4 support [default=OFF]" OFF)
if(ENABLE_FTS4)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_FTS4=1)
endif()


option(ENABLE_FTS5 "include fts5 support [default=ON]" ON)
if(ENABLE_FTS5)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_FTS5=1)
endif()

option(ENABLE_JSON1 "include json1 support [default=ON]" ON)
if(ENABLE_JSON1)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_JSON1=1)
endif()

option(ENABLE_RTREE "include rtree support [default=ON]" ON)
if(ENABLE_RTREE)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_RTREE=1 SQLITE_ENABLE_GEOPOLY=1)
endif()

option(ENABLE_SESSION "enable the session extension [default=OFF]" OFF)
if(ENABLE_SESSION)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_SESSION=1 SQLITE_ENABLE_PREUPDATE_HOOK=1)
endif()

# TODO this is disabled due to misunderstanding how to load dynamic extensions with msvc.
target_compile_definitions(sqlite3 PRIVATE SQLITE_OMIT_LOAD_EXTENSION=1)