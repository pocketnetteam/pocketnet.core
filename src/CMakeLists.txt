include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(CheckIncludeFileCXX)

set(CMAKE_CXX_STANDARD 17)

### Options

option(DISABLE_GUI "Disable building qt gui (Default is OFF)" OFF)

option(DISABLE_ZMQ "Disable ZMQ notifications (default is OFF)" OFF)

option(WITH_SYSTEM_UNIVALUE "Build with system UniValue (default is OFF)" OFF)

option(ENABLE_ASM "Enable assembly routines (default is ON)" ON)

set(BDB_ROOT CACHE STRING "Path to BerkleyDB 4.8")

option(DISABLE_WALLET "Disable wallet (enabled by default)")

set(WITH_MINIUPNPC CACHE STRING "enable UPNP (default is yes if libminiupnpc is found)" "auto")

option(ENABLE_UPNP_DEFAULT "If UPNP is enabled, turn it on at startup (default is no)")

option(DISABLE_TESTS "Do not compile tests (default is to compile)" OFF)
if (NOT DISABLE_TESTS)
    set(USE_TESTS ON)
endif ()

option(DISABLE_GUI_TESTS "Do not compile GUI tests (default is to compile if GUI and tests enabled)" ${USE_TESTS})

option(WITH_RAPIDCHECK "Enable RapidCheck property based tests (default is yes if librapidcheck is found)" "auto")

option(DISABLE_BENCH "do not compile benchmarks (default is to compile)" OFF)
if (NOT DISABLE_BENCH)
    set(USE_BENCH ON)
endif ()

option(ENABLE_EXTENDED_FUNCTIONAL_TESTS "enable expensive functional tests when using lcov (default OFF)" OFF)

option(WITH_QRENCODE "enable QR code support (default is yes if qt is enabled and libqrencode is found)" "auto")

option(DISABLE_HARDENING "do not attempt to harden the resulting executables (default is to harden when possible)" "auto")

option(ENABLE_REDUCE_EXPORTS "attempt to reduce exported symbols in the resulting executables (default is OFF)" OFF)

option(DISABLE_CCACHE "do not use ccache for building (default is to use if found)" "auto")

option(ENABLE_LCOV "enable lcov testing branch coverage (default is OFF)" OFF)

option(ENABLE_GLIBC_BACK_COMPAT "enable backwards compatibility with glibc" OFF)

option(WITH_INCOMPATIBLE_BDB "Allow using a bdb version greater than 4.8" OFF)

option(DISABLE_MAN "do not install man pages (default is to install)" OFF)
if (NOT DISABLE_MAM)
    #    TODO (build): build man
    set(ENABLE_MAN ON)
endif ()

option(WITH_UTILS "build pocketcoin-cli pocketcoin-tx (default=ON)" ON)
option(WITH_DAEMON "build pocketcoind daemon (default=ON)" ON)
option(MSVC_FORCE_STATIC "Build with MTd runtime linking. Use this if you want to statically link internal libraries. Ignored for non-MSVC build (default=ON)" ON)

# Disable warnings
if(NOT WIN32)
    add_compile_options(-Wall -Wno-unused-function -Wno-unused-parameter -Wno-deprecated)
endif()

# Specific msvc options
if(MSVC)
    add_compile_options(/bigobj)
    if (MSVC_FORCE_STATIC)
        set(Protobuf_USE_STATIC_LIBS ON)
        set(Boost_USE_STATIC_LIBS ON)
        set(Boost_USE_STATIC_RUNTIME ON)
        set(Boost_USE_DEBUG_RUNTIME ON)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# BerkeleyDB
if (NOT DISABLE_WALLET)
    set(BerkeleyDB_FIND_REQUIRED ON)
    include(cmake/FindBerkeleyDB.cmake)
    set(BDB_INTERESTED_VERSION "${BerkeleyDB_VERSION_MAJOR}.${BerkeleyDB_VERSION_MINOR}")
    if (BDB_INTERESTED_VERSION VERSION_EQUAL "4.8")
        set(BDB_FOUND ON)
    elseif (BDB_INTERESTED_VERSION VERSION_GREATER "4.8")
        if (WITH_INCOMPATIBLE_BDB)
            set(BDB_FOUND ON)
            message(WARNING "Found Berkeley DB other than 4.8; wallets opened by this build will not be portable!")
        else ()
            message(FATAL_ERROR "Found Berkeley DB other than 4.8, required for portable wallets (-DWITH_INCOMPATIBLE_BDB to ignore or -DDISABLE_WALLET to disable wallet functionality)")
        endif ()
    else ()
        message(FATAL_ERROR "BDB 4.8+ required for wallet functionality (-DDISABLE_WALLET to disable wallet functionality)")
    endif ()
endif ()

if (ENABLE_ASM)
    add_compile_definitions(USE_ASM=1)
endif ()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Check endian headers
include(CmakeCheckEndian.cmake)

# LevelDB
# Disable leveldb benchmarks because they break building
# pocketnet.core with clang
option(LEVELDB_BUILD_BENCHMARKS "" OFF)
add_subdirectory(leveldb/)


# secp256k1

# Enabling options for sepc
set(ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
# Force fpic to ON for sepc (as it has done in autotools)
set(WITH_PIC_BACKUP ${WITH_PIC})
set(WITH_PIC ON CACHE BOOL "Enable fPIC" FORCE)
add_subdirectory(secp256k1/)
# Return original fpic
set(WITH_PIC ${WITH_PIC_BACKUP} CACHE BOOL "Enable fPIC" FORCE)

# Finding boost
find_package(Boost 1.66 REQUIRED COMPONENTS system thread filesystem)
#[[
######## Boost sleep functions validation
set(CHECK_BOOST_SLEEP "#include <boost/thread/thread.hpp>
                        #include <boost/version.hpp>
                        int main(){
                        #if BOOST_VERSION >= 105000 && (!defined(BOOST_HAS_NANOSLEEP) || BOOST_VERSION >= 105200)
                        boost::this_thread::sleep_for(boost::chrono::milliseconds(0));
                        #else
                                choke me
                        #endif

                        return 0;}")

set(CMAKE_REQUIRED_LIBRARIES_BACKUP ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_FLAGS_BACKUP ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_LIBRARIES Boost::thread Boost::system Boost::chrono Boost::date_time)
check_cxx_source_compiles("${CHECK_BOOST_SLEEP}" HAVE_WORKING_BOOST_SLEEP_FOR )
if(HAVE_WORKING_BOOST_SLEEP_FOR)
    add_compile_definitions(HAVE_WORKING_BOOST_SLEEP_FOR=1)
else ()
    set(CHECK_BOOST_SLEEP "  #include <boost/version.hpp>
        #include <boost/thread.hpp>
        #include <boost/date_time/posix_time/posix_time_types.hpp>
        int main(){
        #if BOOST_VERSION <= 105600
        boost::this_thread::sleep(boost::posix_time::milliseconds(0));
        #else
        choke me
        #endif
        }")
    check_cxx_source_compiles("${CHECK_BOOST_SLEEP}" HAVE_WORKING_BOOST_SLEEP)
    if (HAVE_WORKING_BOOST_SLEEP)
        add_compile_definitions(HAVE_WORKING_BOOST_SLEEP=1)
    else ()
        message(FATAL_ERROR "No working boost sleep implementation found.")
    endif ()
endif ()
]]

# Common checks

# AC_CHECK_HEADERS analog
foreach(_HEADER endian.h sys/endian.h byteswap.h stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h sys/select.h sys/prctl.h sys/sysctl.h vm/vm_param.h sys/vmmeter.h sys/resources.h)
    check_include_file_cxx(${_HEADER} _RES_${_HEADER})
    if (_RES_${_HEADER})
        # Replace all non letter and number symbols with "_"
        string (REGEX REPLACE "[^a-zA-Z0-9]" "_" _HEADER ${_HEADER})
        # All letters to upper
        string (TOUPPER ${_HEADER} _HEADER)
        string (CONCAT _TODECLARE "HAVE_" ${_HEADER})
        add_compile_definitions(${_TODECLARE}=1)
    endif()
endforeach()

# Checking for getifaddrs and freeifaddrs
set(_ADDRS_STUFF_COUNT 0)
foreach(_SYMBOL getifaddrs freeifaddrs)
    check_cxx_symbol_exists(${_SYMBOL} "sys/types.h;ifaddrs.h" _RES_${_SYMBOL})
    if (_RES_${_SYMBOL})
        # Replace all non letter and number symbols with "_"
        string (REGEX REPLACE "[^a-zA-Z0-9]" "_" _SYMBOL ${_SYMBOL})
        # All letters to upper
        string (TOUPPER ${_SYMBOL} _SYMBOL)
        string (CONCAT _TODECLARE "HAVE_DECL_" ${_SYMBOL})
        add_compile_definitions(${_TODECLARE}=1)
        MATH(EXPR _ADDRS_STUFF_COUNT "${_ADDRS_STUFF_COUNT}+1")
    endif()
endforeach()

# Checking for bswap functions
foreach(_SYMBOL bswap_16 bswap_32 bswap_64)
    check_cxx_symbol_exists(${_SYMBOL} byteswap.h _RES_${_SYMBOL})
    # Replace all non letter and number symbols with "_"
    string (REGEX REPLACE "[^a-zA-Z0-9]" "_" _TODECLARE ${_SYMBOL})
    # All letters to upper
    string (TOUPPER ${_TODECLARE} _TODECLARE)
    string (CONCAT _TODECLARE "HAVE_DECL_" ${_TODECLARE})
    if (_RES_${_SYMBOL})
        add_compile_definitions(${_TODECLARE}=1)
    else()
        add_compile_definitions(${_TODECLARE}=0)
    endif()
endforeach()

if(${_ADDRS_STUFF_COUNT} EQUAL 2) # Means we succeed with both getifaddrs and freeifaddrs functions checking
    # Trying to use noticed functions without linking "socket" lib
    set(_CHECK_SOCKET_testbody "#include <sys/types.h>
                                #include <ifaddrs.h>

                                int main() {
                                    struct ifaddrs *ifaddr;
                                    getifaddrs(&ifaddr);
                                    freeifaddrs(ifaddr);
                                }")
    check_cxx_source_runs("${_CHECK_SOCKET_testbody}" _CHECK_SOCKET_RES_WITHOUT_LIBSOCKET)
    if(_CHECK_SOCKET_RES_WITHOUT_LIBSOCKET)
        set(REQUIRED_SOCKET_LIB OFF)
    else()
        set(CMAKE_REQUIRED_LIBS_BACKUP ${CMAKE_REQUIRED_LIBS})
        set(CMAKE_REQUIRED_LIBS ${CMAKE_REQUIRED_LIBS} socket)
        check_cxx_source_runs("${_CHECK_SOCKET_testbody}" _CHECK_SOCKET_RES_WITH_LIBSOCKET)
        if(_CHECK_SOCKET_RES_WITH_LIBSOCKET)
            set(REQUIRED_SOCKET_LIB ON)
        else()
            message(FATAL_ERROR "cannot figure out how to use getifaddrs")
        endif()
        set (CMAKE_REQUIRED_LIBS ${CMAKE_REQUIRED_LIBS_BACKUP})
    endif()
endif()

# Non-posix method from bsd and macos that should be enabled by specific option.
# Checking for daemon function
check_cxx_symbol_exists(daemon unistd.h _RES_DAEMON)
if (_RES_DAEMON)
    add_compile_definitions(HAVE_DECL_DAEMON=1)
endif()

# Checking for __builtin_clzll
check_cxx_source_compiles("int main()
                           {
                               (void) __builtin_clzll(0);
                               return 0;
                           }" HAVE_BUILTIN_CLZLL)
if(HAVE_BUILTIN_CLZLL)
    add_compile_definitions(HAVE_BUILTIN_CLZLL=1)
endif()

# Checking for posix_fallocate
check_cxx_source_compiles("#ifdef __linux__
                           #ifdef _POSIX_C_SOURCE
                           #undef _POSIX_C_SOURCE
                           #endif
                           #define _POSIX_C_SOURCE 200112L
                           #endif // __linux__
                           #include <fcntl.h>
                           int main()
                           {
                               int f = posix_fallocate(0, 0, 0);
                               return 0;
                           }" HAVE_POSIX_FALLOCATE)
if(HAVE_POSIX_FALLOCATE)
    add_compile_definitions(HAVE_POSIX_FALLOCATE=1)
endif()

# Checking for sysctl
check_cxx_source_compiles("#include <sys/types.h>
                           #include <sys/sysctl.h>
                           int main()
                           {
                               #ifdef __linux__
                               #error \"Don't use sysctl on Linux, it's deprecated even when it works\"
                               #endif
                               sysctl(nullptr, 2, nullptr, nullptr, nullptr, 0);
                               return 0;
                           }" HAVE_SYSCTL)
if(HAVE_SYSCTL)
    add_compile_definitions(HAVE_SYSCTL=1)
endif()

# Checking for sysctl
check_cxx_source_compiles("#include <sys/types.h>
                           #include <sys/sysctl.h>
                           int main()
                           {
                               #ifdef __linux__
                               #error \"Don't use sysctl on Linux, it's deprecated even when it works\"
                               #endif
                               static int name[2] = {CTL_KERN, KERN_ARND};
                               sysctl(name, 2, nullptr, nullptr, nullptr, 0);
                               return 0;
                           }" HAVE_SYSCTL_ARND)
if(HAVE_SYSCTL_ARND)
    add_compile_definitions(HAVE_SYSCTL_ARND=1)
endif()

# Checking for fdatasync
check_cxx_source_compiles("#include <unistd.h>
                           int main()
                           {
                               fdatasync(0);
                               return 0;
                           }" HAVE_FDATASYNC)
if(HAVE_FDATASYNC)
    add_compile_definitions(HAVE_FDATASYNC=1)
else()
    add_compile_definitions(HAVE_FDATASYNC=0)
endif()

# Checking for std::system
check_cxx_source_compiles("#include <cstdlib>
                           int main()
                           {
                               int nErr = std::system(\"\");
                               return 0;
                           }" HAVE_STD__SYSTEM)
if(HAVE_STD__SYSTEM)
    add_compile_definitions(HAVE_STD__SYSTEM=1)
endif()

# Checking for ::_wsystem
check_cxx_source_compiles("int main()
                           {
                               int nErr = ::_wsystem(\"\");
                               return 0;
                           }" HAVE_WSYSTEM)
if(HAVE_WSYSTEM)
    add_compile_definitions(HAVE_WSYSTEM=1)
endif()

if(HAVE_STD__SYSTEM OR HAVE_WSYSTEM)
    add_compile_definitions(HAVE_SYSTEM=1)
endif()

set(CMAKE_REQUIRED_FLAGS -msse4.1)
check_cxx_source_runs( "#include <stdint.h>
                            #include <immintrin.h>
                            int main() {
                                __m128i l = _mm_set1_epi32(0);
                                return _mm_extract_epi32(l, 3);
                            }" ENABLE_SSE41)

set(CMAKE_REQUIRED_FLAGS -mavx -mavx2)
check_cxx_source_runs( "#include <stdint.h>
                            #include <immintrin.h>
                            int main() {
                                __m256i l = _mm256_set1_epi32(0);
                                return _mm256_extract_epi32(l, 7);
                            }" ENABLE_AVX2)

set(CMAKE_REQUIRED_FLAGS -msse4 -msha)
check_cxx_source_runs( "#include <stdint.h>
                            #include <immintrin.h>
                            int main() {
                                __m128i i = _mm_set1_epi32(0);
                                __m128i j = _mm_set1_epi32(1);
                                __m128i k = _mm_set1_epi32(2);
                                return _mm_extract_epi32(_mm_sha256rnds2_epu32(i, i, k), 0);
                            }" ENABLE_SHANI)


if (ENABLE_GLIBC_BACK_COMPAT)
    find_library(REQUIRED rt)
    set(CMAKE_REQUIRED_LIBRARIES rt)
    check_function_exists(clock_gettime CLOCK_GETTIME_EXISTS)
    if (NOT CLOCK_GETTIME_EXISTS)
        message(FATAL_ERROR "librt missing")
    endif ()
    check_cxx_source_compiles("#ifdef _FORTIFY_SOURCE
                                    #undef _FORTIFY_SOURCE
                                #endif
                                #define _FORTIFY_SOURCE 2
                                #include <sys/select.h>
                                extern \"C\" long unsigned int __fdelt_warn(long unsigned int);
                                int main() {return 0;}
                                " FDELT_TYPE)
    if (FDELT_TYPE)
        add_compile_definitions(FDELT_TYPE="long unsigned int")
    else ()
        add_compile_definitions(FDELT_TYPE="long int")
    endif ()
endif ()

check_cxx_source_compiles(" #include <malloc.h>
                            int main(){
                                int f = malloc_info(0, NULL);
                            }" HAVE_MALLOC_INFO)
if (HAVE_MALLOC_INFO)
    add_compile_definitions(HAVE_MALLOC_INFO=1)
endif ()

check_cxx_source_compiles(" #include <malloc.h>
                            int main(){
                                mallopt(M_ARENA_MAX, 1);
                            }" HAVE_MALLOPT_ARENA_MAX)
if (HAVE_MALLOPT_ARENA_MAX)
    add_compile_definitions(HAVE_MALLOPT_ARENA_MAX=1)
endif ()

check_cxx_source_compiles(" int foo_def( void ) __attribute__((visibility(\"default\")));
                            int main(){}" HAVE_VISIBILITY_ATTRIBUTE)
if (HAVE_VISIBILITY_ATTRIBUTE)
    add_compile_definitions(HAVE_VISIBILITY_ATTRIBUTE=1)
elseif (ENABLE_REDUCE_EXPORTS)
    message(FATAL_ERROR "Cannot find a working visibility attribute. Use -DENABLE_REDUCE_EXPORTS=OFF.")
endif ()

check_cxx_source_compiles("#include <ctime>
                            int main() {
                                gmtime_r((const time_t *) nullptr, (struct tm *) nullptr);
                            }" HAVE_GMTIME_R)
if(HAVE_GMTIME_R)
    add_compile_definitions(HAVE_GMTIME_R=1)
else()
    check_cxx_source_compiles("#include <ctime>
                                int main() {
                                    gmtime_s((struct tm *) nullptr, (const time_t *) nullptr);
                                }" HAVE_GMTIME_S)
    if(NOT HAVE_GMTIME_S)
        message(FATAL_ERROR "Both gmtime_r and gmtime_s are unavailable")
    endif()
endif()

# TODO (build): make an option to allow disabling this
if (NOT MINGW) # TODO (build): also this is required for freebsd but probably we do not support it
    check_cxx_source_compiles(" #include <thread>
                                static thread_local int foo = 0;
                                static void run_thread() { foo++;}
                                int main(){
                                    for(int i = 0; i < 10; i++) { std::thread(run_thread).detach();}
                                    return foo;
                                }" HAVE_THREAD_LOCAL)
    if (HAVE_THREAD_LOCAL)
        add_compile_definitions(HAVE_THREAD_LOCAL=1)
    endif ()
endif()

# TODO (build): add libmultiprocess stuff

check_cxx_source_compiles(" #include <sys/syscall.h>
                            #include <linux/random.h>
                            int main(){
                                syscall(SYS_getrandom, nullptr, 32, 0);
                            }" HAVE_SYS_GETRANDOM)
if (HAVE_SYS_GETRANDOM)
    add_compile_definitions(HAVE_SYS_GETRANDOM=1)
endif ()

check_cxx_source_compiles(" #include <unistd.h>
                            int main(){
                                getentropy(nullptr, 32);
                            }" HAVE_GETENTROPY)
if (HAVE_GETENTROPY)
    add_compile_definitions(HAVE_GETENTROPY=1)
endif ()

check_cxx_source_compiles(" #include <unistd.h>
                            #include <sys/random.h
                            int main(){
                                getentropy(nullptr, 32) ;
                            }" HAVE_GETENTROPY_RAND)
if (HAVE_GETENTROPY_RAND)
    add_compile_definitions(HAVE_GETENTROPY_RAND=1)
endif ()

check_cxx_source_compiles(" #include <stdint.h>
                            #include <type_traits>
                            int main(){
                                static_assert(std::is_same<int8_t, char>::value, \"\");
                            }" CHAR_EQUALS_INT8)
if (CHAR_EQUALS_INT8)
    add_compile_definitions(CHAR_EQUALS_INT8=1)
endif ()

if (ENABLE_REDUCE_EXPORTS)
    #    TODO (build): It doesn't go anywhere
    check_cxx_compiler_flag(-fvisibility=hidden VISIBILITY_HIDDEN)
    if (NOT VISIBILITY_HIDDEN)
        message(FATAL_ERROR "Cannot set default symbol visibility. Use -DENABLE_REDUCE_EXPORTS=OFF")
    endif ()
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_BACKUP})
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_BACKUP})

# Looking for required system libraries
find_package(Event 2.0 REQUIRED)

add_subdirectory(sqlite/)
add_compile_definitions(USE_SQLITE=1)

find_package(OpenSSL REQUIRED)

# Some configuration
check_function_exists(strnlen HAVE_DECL_STRNLEN)
if (HAVE_DECL_STRNLEN)
    add_compile_definitions(HAVE_DECL_STRNLEN=1)
else (HAVE_DECL_STRNLEN)
    add_compile_definitions(HAVE_DECL_STRNLEN=0)
endif (HAVE_DECL_STRNLEN)


# Univalue library
if (WITH_SYSTEM_UNIVALUE)
    # TODO (build): write find_package script
    find_library(UNIVALUE univalue)
    if (NOT UNIVALUE)
        message(FATAL_ERROR "System univalue requested but not found")
    endif ()
else ()
    add_subdirectory(univalue/)
endif ()


# Common library
set(POCKETCOIN_COMMON pocketcoin_common)
add_library(${POCKETCOIN_COMMON}
        attributes.h
        cuckoocache.h
        eventloop.h
        indirectmap.h
        # limitedmap.h
        memusage.h
        version.h
        reverse_iterator.h
        # reverselock.h
        streams.h
        undo.h
        span.h
        prevector.h
        protectedmap.h
        compat.h
        compat/assumptions.h
        compat/byteswap.h
        compat/cpuid.h
        compat/endian.h
        compat/sanity.h
        compat/glibc_sanity.cpp
        compat/glibcxx_sanity.cpp
        compat/strnlen.cpp
        compat/stdin.h
        compat/stdin.cpp
        optional.h
        amount.h
        tinyformat.h
        )
target_include_directories(${POCKETCOIN_COMMON} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set (POCKET_UTIL pocket_util)
add_library(${POCKET_UTIL}
        pocketdb/util/Empty.h
        pocketdb/util/Empty.cpp
)
target_link_libraries(${POCKET_UTIL} PRIVATE ${POCKETCOIN_COMMON})

# Pocket DB functional library.
# It is divided to pocketdb-server part and this functional part
set(POCKETDB pocketdb)
add_library(${POCKETDB}
        pocketdb/migrations/base.h
        pocketdb/migrations/main.h
        pocketdb/migrations/main.cpp
        pocketdb/migrations/web.h
        pocketdb/migrations/web.cpp
        pocketdb/migrations/old_minimal.h
        pocketdb/migrations/old_minimal.cpp

        pocketdb/models/base/PocketTypes.h
        pocketdb/models/base/DtoModels.h
        pocketdb/models/base/Base.h
        pocketdb/models/base/Base.cpp
        pocketdb/models/base/Payload.h
        pocketdb/models/base/Payload.cpp
        pocketdb/models/base/Transaction.h
        pocketdb/models/base/Transaction.cpp
        pocketdb/models/base/TransactionInput.h
        pocketdb/models/base/TransactionInput.cpp
        pocketdb/models/base/TransactionOutput.h
        pocketdb/models/base/TransactionOutput.cpp
        pocketdb/models/base/SocialTransaction.h
        pocketdb/models/base/SocialTransaction.cpp
        pocketdb/models/base/SocialEditableTransaction.h
        pocketdb/models/base/SocialEditableTransaction.cpp
        pocketdb/models/base/Rating.h
        pocketdb/models/base/Rating.cpp

        pocketdb/models/dto/money/Default.h
        pocketdb/models/dto/money/Default.cpp
        pocketdb/models/dto/money/Coinbase.h
        pocketdb/models/dto/money/Coinbase.cpp
        pocketdb/models/dto/money/Coinstake.h
        pocketdb/models/dto/money/Coinstake.cpp

        pocketdb/models/dto/account/User.h
        pocketdb/models/dto/account/User.cpp
        pocketdb/models/dto/account/Setting.h
        pocketdb/models/dto/account/Setting.cpp
        pocketdb/models/dto/account/Delete.h
        pocketdb/models/dto/account/Delete.cpp

        pocketdb/models/dto/content/Content.h
        pocketdb/models/dto/content/Content.cpp
        pocketdb/models/dto/content/ContentDelete.h
        pocketdb/models/dto/content/ContentDelete.cpp
        pocketdb/models/dto/content/Post.h
        pocketdb/models/dto/content/Post.cpp
        pocketdb/models/dto/content/Video.h
        pocketdb/models/dto/content/Video.cpp
        pocketdb/models/dto/content/Article.h
        pocketdb/models/dto/content/Article.cpp
        pocketdb/models/dto/content/Stream.h
        pocketdb/models/dto/content/Stream.cpp
        pocketdb/models/dto/content/Audio.h
        pocketdb/models/dto/content/Audio.cpp
        pocketdb/models/dto/content/Collection.h
        pocketdb/models/dto/content/Collection.cpp
        pocketdb/models/dto/content/Comment.h
        pocketdb/models/dto/content/Comment.cpp
        pocketdb/models/dto/content/CommentEdit.h
        pocketdb/models/dto/content/CommentEdit.cpp
        pocketdb/models/dto/content/CommentDelete.h
        pocketdb/models/dto/content/CommentDelete.cpp
        pocketdb/models/dto/content/App.h
        pocketdb/models/dto/content/App.cpp

        pocketdb/models/dto/barteron/Account.h
        pocketdb/models/dto/barteron/Account.cpp
        pocketdb/models/dto/barteron/Offer.h
        pocketdb/models/dto/barteron/Offer.cpp

        pocketdb/models/dto/action/ScoreContent.h
        pocketdb/models/dto/action/ScoreContent.cpp
        pocketdb/models/dto/action/ScoreComment.h
        pocketdb/models/dto/action/ScoreComment.cpp
        pocketdb/models/dto/action/Blocking.h
        pocketdb/models/dto/action/Blocking.cpp
        pocketdb/models/dto/action/BlockingCancel.h
        pocketdb/models/dto/action/BlockingCancel.cpp
        pocketdb/models/dto/action/Subscribe.h
        pocketdb/models/dto/action/Subscribe.cpp
        pocketdb/models/dto/action/SubscribePrivate.h
        pocketdb/models/dto/action/SubscribePrivate.cpp
        pocketdb/models/dto/action/SubscribeCancel.h
        pocketdb/models/dto/action/SubscribeCancel.cpp
        pocketdb/models/dto/action/Complain.h
        pocketdb/models/dto/action/Complain.cpp
        pocketdb/models/dto/action/BoostContent.h
        pocketdb/models/dto/action/BoostContent.cpp

        pocketdb/models/dto/moderation/Flag.h
        pocketdb/models/dto/moderation/Flag.cpp
        pocketdb/models/dto/moderation/Vote.h
        pocketdb/models/dto/moderation/Vote.cpp
        # pocketdb/models/dto/moderation/Moderator.h
        # pocketdb/models/dto/moderation/Moderator.cpp
        # pocketdb/models/dto/moderation/RequestCoin.h
        # pocketdb/models/dto/moderation/RequestCoin.cpp
        # pocketdb/models/dto/moderation/RequestSubs.h
        # pocketdb/models/dto/moderation/RequestSubs.cpp
        # pocketdb/models/dto/moderation/RequestCancel.h
        # pocketdb/models/dto/moderation/RequestCancel.cpp
        # pocketdb/models/dto/moderation/RegisterSelf.h
        # pocketdb/models/dto/moderation/RegisterSelf.cpp
        # pocketdb/models/dto/moderation/RegisterRequest.h
        # pocketdb/models/dto/moderation/RegisterRequest.cpp
        # pocketdb/models/dto/moderation/RegisterCancel.h
        # pocketdb/models/dto/moderation/RegisterCancel.cpp
        
        pocketdb/models/web/WebTag.h
        pocketdb/models/web/WebContent.h
        pocketdb/models/web/SearchRequest.h

        pocketdb/models/shortform/ShortForm.h
        pocketdb/models/shortform/ShortForm.cpp
        pocketdb/models/shortform/ShortTxData.h
        pocketdb/models/shortform/ShortTxData.cpp
        pocketdb/models/shortform/ShortAccount.h
        pocketdb/models/shortform/ShortAccount.cpp
        pocketdb/models/shortform/ShortTxType.h
        pocketdb/models/shortform/ShortTxOutput.h
        pocketdb/models/shortform/ShortTxOutput.cpp
        pocketdb/helpers/ShortFormModelsHelper.h
        pocketdb/helpers/ShortFormModelsHelper.cpp
        )
    target_link_libraries(${POCKETDB} PRIVATE ${POCKETCOIN_COMMON} ${POCKETCOIN_UTIL} ${POCKETCOIN_CRYPTO} ${POCKET_UTIL} univalue leveldb)


# Pocketcoin support library
set(POCKETCOIN_SUPPORT pocketcoin_support)
add_library(${POCKETCOIN_SUPPORT}
        support/cleanse.h
        support/cleanse.cpp
        support/events.h
        support/lockedpool.h
        support/lockedpool.cpp
        support/allocators/secure.h
        support/allocators/zeroafterfree.h
        )
target_include_directories(${POCKETCOIN_SUPPORT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Pocketcoin util library
set(POCKETCOIN_UTIL pocketcoin_util)
add_library(${POCKETCOIN_UTIL}
                randomenv.h
                randomenv.cpp
                clientversion.h
                clientversion.cpp
                flatfile.h
                flatfile.cpp
                fs.h
                fs.cpp
                logging.h
                logging.cpp
                logging/timer.h
                sync.h
                sync.cpp
                threadsafety.h
                threadinterrupt.h
                threadinterrupt.cpp
                util/memory.h
                util/moneystr.h
                util/moneystr.cpp
                util/time.h
                util/time.cpp
                util/html.h
                util/html.cpp
                util/asmap.cpp
                util/asmap.h
                util/bip32.cpp
                util/bip32.h
                util/bytevectorhash.cpp
                util/bytevectorhash.h
                util/check.h
                util/error.cpp
                util/error.h
                util/fees.cpp
                util/fees.h
                util/golombrice.h
                util/macros.h
                util/message.cpp
                util/message.h
                util/rbf.cpp
                util/rbf.h
                util/readwritefile.h
                util/readwritefile.cpp
                util/ref.h
                util/settings.cpp
                util/settings.h
                util/sock.h
                util/sock.cpp
                util/spanparsing.cpp
                util/spanparsing.h
                util/string.cpp
                util/string.h
                util/syserror.h
                util/syserror.cpp
                util/threadnames.cpp
                util/threadnames.h
                util/translation.h
                util/ui_change_type.h
                util/url.cpp
                util/url.h
                util/vector.h
                random.h
                random.cpp
                timedata.h
                timedata.cpp
                netaddress.h
                netaddress.cpp
                node/ui_interface.h
                node/ui_interface.cpp
                node/utxo_snapshot.h
                util/system.h
                util/system.cpp
                chainparamsbase.h
                chainparamsbase.cpp
                chainparamsseeds.h
                warnings.h
                warnings.cpp
            )
target_link_libraries(${POCKETCOIN_UTIL} PRIVATE ${POCKETCOIN_SUPPORT} ${POCKETCOIN_COMMON} ${POCKETCOIN_CRYPTO} ${CRYPT32} Boost::thread Event::event univalue)
target_link_libraries(${POCKETCOIN_UTIL} PUBLIC Boost::filesystem)
if(WIN32)
    target_link_libraries(${POCKETCOIN_UTIL} PRIVATE ws2_32 crypt32 wsock32)
    add_compile_definitions(NOMINMAX)
endif ()
if (ENABLE_GLIBC_BACK_COMPAT)
    target_sources(${POCKETCOIN_UTIL} PRIVATE compat/glibc_compat.cpp)
endif ()
if (REQUIRED_SOCKET_LIB)
    # Linking to a socket library if getifaddrs freeifaddrs functions require it (see _CHECK_SOCKET_RES_WITHOUT_LIBSOCKET) 
    target_link_libraries(${POCKETCOIN_UTIL} PRIVATE socket)
endif()
target_include_directories(${POCKETCOIN_UTIL} PUBLIC ${Boost_INCLUDE_DIRS})

######## Crypto
set(POCKETCOIN_CRYPTO pocketcoin_crypto)
add_library(${POCKETCOIN_CRYPTO}
        crypto/sha256_sse4.cpp
        crypto/sha256.cpp
        crypto/sha3.h
        crypto/sha3.cpp
        crypto/chacha20.cpp
        crypto/aes.h
        crypto/hmac_sha256.cpp
        crypto/sha512.h
        crypto/hmac_sha512.cpp
        crypto/ripemd160.cpp
        crypto/sha1.h
        crypto/ripemd160.h
        crypto/hmac_sha512.h
        crypto/chacha20.h
        crypto/chacha_poly_aead.h
        crypto/chacha_poly_aead.cpp
        crypto/hkdf_sha256_32.h
        crypto/hkdf_sha256_32.cpp
        crypto/sha512.cpp
        crypto/sha1.cpp
        crypto/common.h
        crypto/poly1305.h
        crypto/poly1305.cpp
        crypto/hmac_sha256.h
        crypto/sha256.h
        crypto/siphash.h
        crypto/siphash.cpp
        crypto/aes.cpp
        crypto/x509.h
        crypto/x509.cpp
        uint256.h
        uint256.cpp
        hash.h
        hash.cpp
        base58.h
        base58.cpp
        bech32.h
        bech32.cpp
        serialize.h
        arith_uint256.cpp
        arith_uint256.h
        util/strencodings.h # TODO (build): maybe move to utils
        util/strencodings.cpp
        )
target_include_directories(${POCKETCOIN_CRYPTO} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${POCKETCOIN_CRYPTO} PRIVATE ${POCKETCOIN_COMMON} ${POCKETCOIN_SUPPORT} secp256k1)
if (ENABLE_SSE41)
    target_sources(${POCKETCOIN_CRYPTO} PRIVATE crypto/sha256_sse41.cpp)
    if (MSVC)
        # TODO (build): it seems like windows fails with everything exept sse2 that is enabled by default
        #               Futhermore there are cmake scripts to find sse4.1 or avx support where they are always hardcoded as false for Windows:
        #               https://github.com/magic-sph/magic/blob/master/cmake/FindSSE.cmake
        # target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE /arch:SSE4.1)
    else ()
        target_compile_definitions(${POCKETCOIN_CRYPTO} PRIVATE ENABLE_SSE41)
        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE -msse4.1)
    endif ()
endif ()
if (ENABLE_AVX2)
    target_sources(${POCKETCOIN_CRYPTO} PRIVATE crypto/sha256_avx2.cpp)
    if (MSVC)
        #        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE /arch:AVX /arch:AVX2)
    else ()
        target_compile_definitions(${POCKETCOIN_CRYPTO} PRIVATE ENABLE_AVX2)
        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE -mavx -mavx2)
    endif ()
endif ()
if (ENABLE_SHANI)
    target_sources(${POCKETCOIN_CRYPTO} PRIVATE crypto/sha256_shani.cpp)
    if (MSVC)
        #        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE /arch:SSE4 /arch:SHA)
    else ()
        target_compile_definitions(${POCKETCOIN_CRYPTO} PRIVATE ENABLE_SHANI)
        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE -msse4 -msha)
    endif ()
endif ()


# System
set(POCKETCOIN_SYSTEM pocketcoin_system)
add_library(${POCKETCOIN_SYSTEM} scheduler.cpp
        scheduler.h
        shutdown.h
        shutdown.cpp
        )
target_link_libraries(${POCKETCOIN_SYSTEM} PRIVATE ${POCKETCOIN_UTIL} ${POCKETCOIN_CRYPTO})
target_include_directories(${POCKETCOIN_SYSTEM} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Consensus
# TODO (build): probably completely move this to server
set(POCKETCOIN_CONSENSUS pocketcoin_consensus)
add_library(${POCKETCOIN_CONSENSUS}
        protocol.h
        protocol.cpp
        pubkey.h
        pubkey.cpp
        key.h
        key.cpp
        )
target_link_libraries(${POCKETCOIN_CONSENSUS} PRIVATE ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON} ${POCKETCOIN_CRYPTO} ${POCKETDB} univalue secp256k1)
# TODO (build): hadrdcoded because we are always building consensus
add_compile_definitions(HAVE_CONSENSUS_LIB=1)


# Common rpc that used by server and cli
set(POCKETCOIN_COMMON_RPC pocketcoin_common_rpc)
add_library(${POCKETCOIN_COMMON_RPC}
        rpc/protocol.h
        rpc/requestutils.h
        rpc/requestutils.cpp
        )
target_link_libraries(${POCKETCOIN_COMMON_RPC} PRIVATE ${POCKETCOIN_CRYPTO} ${POCKETCOIN_UTIL} univalue)
target_link_libraries(${POCKETCOIN_COMMON_RPC} PUBLIC sqlite3)
target_include_directories(${POCKETCOIN_COMMON_RPC} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


########### Server
set(POCKETCOIN_SERVER pocketcoin_server)
add_library(${POCKETCOIN_SERVER}

        node/coin.h
        node/coin.cpp
        node/coinstats.h
        node/coinstats.cpp
        node/connection_types.h
        node/connection_types.cpp
        node/context.h
        node/context.cpp
        node/psbt.h
        node/psbt.cpp
        node/transaction.h
        node/transaction.cpp
        interfaces/chain.h
        interfaces/chain.cpp
        interfaces/node.h
        interfaces/node.cpp
        chainparams.h
        chainparams.cpp
        consensus/tx_verify.h
        consensus/tx_verify.cpp
        consensus/tx_check.h
        consensus/tx_check.cpp
        consensus/merkle.h
        consensus/merkle.cpp
        chain.h
        chain.cpp
        consensus/consensus.h
        consensus/params.h
        consensus/validation.h
        core_io.h
        core_read.cpp
        core_write.cpp
        coins.h
        coins.cpp
        rest.cpp
        core_memusage.h
        index/base.h
        index/base.cpp
        index/txindex.h
        index/txindex.cpp
        index/blockfilterindex.h
        index/blockfilterindex.cpp
        index/disktxpos.h
        checkpoints.h
        checkpoints.cpp
        checkqueue.h
        banman.h
        banman.cpp
        blockencodings.h
        blockencodings.cpp
        blockfilter.h
        blockfilter.cpp
        httprpc.h
        httprpc.cpp
        httpserver.h
        httpserver.cpp
        i2p.h
        i2p.cpp
        init.h
        init.cpp
        interfaces/handler.h
        interfaces/handler.cpp
        dbwrapper.h
        dbwrapper.cpp
        ldb/ldb.h
        ldb/ldb.cpp
        merkleblock.h
        merkleblock.cpp
        miner.h
        miner.cpp
        noui.h
        noui.cpp
        outputtype.h
        outputtype.cpp
        pos.h
        pos.cpp
        pow.h
        pow.cpp
        statistic.hpp
        txdb.h
        txdb.cpp
        versionbits.h
        versionbits.cpp
        versionbitsinfo.h
        versionbitsinfo.cpp
        # keystore.h
        # keystore.cpp
        compressor.h
        compressor.cpp
        key_io.h
        key_io.cpp
        websocket/ws.h
        websocket/ws.cpp
        websocket/notifyprocessor.h
        websocket/notifyprocessor.cpp
        websocket/wshandlerprovider.h
        websocket/wshandlerprovider.cpp
        validation.h
        validation.cpp
        validationinterface.h
        validationinterface.cpp
        policy/feerate.h
        policy/feerate.cpp
        policy/fees.h
        policy/fees.cpp
        policy/policy.h
        policy/policy.cpp
        policy/rbf.h
        policy/rbf.cpp
        policy/settings.h
        policy/settings.cpp
        signet.h
        signet.cpp
        script/keyorigin.h
        script/descriptor.h
        script/descriptor.cpp
        script/signingprovider.h
        script/signingprovider.cpp
        script/sigcache.h
        script/sigcache.cpp
        script/sign.h
        script/sign.cpp
        script/standard.h
        script/standard.cpp
        script/interpreter.h
        script/interpreter.cpp
        script/pocketcoinconsensus.cpp
        script/pocketcoinconsensus.h
        script/script_error.cpp
        script/script_error.h
        script/script.cpp
        script/script.h
        txmempool.h
        txmempool.cpp
        txrequest.h
        txrequest.cpp
        net.h
        net.cpp
        primitives/block.h
        primitives/block.cpp
        primitives/transaction.h
        primitives/transaction.cpp
        netbase.h
        netbase.cpp
        net_processing.h
        net_processing.cpp
        net_permissions.h
        net_permissions.cpp
        netmessagemaker.h
        net_types.h
        addrdb.h
        addrdb.cpp
        addrman.h
        addrman.cpp
        torcontrol.h
        torcontrol.cpp
        bloom.h
        bloom.cpp
        rpc/blockchain.h
        rpc/blockchain.cpp
        rpc/mining.h
        rpc/mining.cpp
        rpc/misc.cpp
        rpc/net.cpp
        # rpc/rawtransaction.h
        rpc/rawtransaction.cpp
        rpc/rawtransaction_util.h
        rpc/rawtransaction_util.cpp
        rpc/register.h
        rpc/request.h
        rpc/request.cpp
        rpc/server.cpp
        rpc/server.h
        rpc/util.cpp
        rpc/util.h
        psbt.h
        psbt.cpp
        rpc/cache.h
        rpc/cache.cpp
        walletinitinterface.h
        pocketdb/helpers/PocketnetHelper.h
        pocketdb/helpers/TransactionHelper.h
        pocketdb/helpers/TransactionHelper.cpp
        pocketdb/helpers/ShortFormRepositoryHelper.h
        pocketdb/helpers/ShortFormRepositoryHelper.cpp
        pocketdb/helpers/DbViewHelper.h
        pocketdb/helpers/DbViewHelper.cpp
        pocketdb/SQLiteDatabase.h
        pocketdb/SQLiteConnection.h
        pocketdb/SQLiteDatabase.cpp
        pocketdb/SQLiteConnection.cpp
        pocketdb/stmt.h
        pocketdb/stmt.cpp
        pocketdb/web/PocketContentRpc.cpp
        pocketdb/web/PocketCommentsRpc.cpp
        pocketdb/web/PocketSystemRpc.cpp
        pocketdb/web/PocketScoresRpc.cpp
        pocketdb/web/PocketAccountRpc.cpp
        pocketdb/web/PocketFrontend.cpp
        pocketdb/web/PocketExplorerRpc.cpp
        pocketdb/web/PocketTransactionRpc.cpp
        pocketdb/web/PocketContentRpc.h
        pocketdb/web/PocketCommentsRpc.h
        pocketdb/web/PocketSystemRpc.h
        pocketdb/web/PocketScoresRpc.h
        pocketdb/web/PocketAccountRpc.h
        pocketdb/web/PocketFrontend.h
        pocketdb/web/PocketExplorerRpc.h
        pocketdb/web/PocketTransactionRpc.h
        pocketdb/web/WebSocketRpc.h
        pocketdb/web/WebSocketRpc.cpp
        pocketdb/web/SearchRpc.h
        pocketdb/web/SearchRpc.cpp
        pocketdb/web/PocketRpc.h
        pocketdb/web/PocketRpc.cpp
        pocketdb/web/PocketTagsRpc.h
        pocketdb/web/PocketTagsRpc.cpp
        pocketdb/web/WebRpcUtils.h
        pocketdb/web/WebRpcUtils.cpp
        pocketdb/web/ModerationRpc.h
        pocketdb/web/ModerationRpc.cpp
        pocketdb/web/BarteronRpc.h
        pocketdb/web/BarteronRpc.cpp
        pocketdb/web/AppRpc.h
        pocketdb/web/AppRpc.cpp
        pocketdb/pocketnet.h
        pocketdb/pocketnet.cpp
        pocketdb/services/WsNotifier.cpp
        pocketdb/services/WsNotifier.h
        pocketdb/services/Serializer.cpp
        pocketdb/services/ChainPostProcessing.cpp
        pocketdb/services/WebPostProcessing.cpp
        pocketdb/services/Accessor.cpp
        pocketdb/services/Serializer.h
        pocketdb/services/ChainPostProcessing.h
        pocketdb/services/WebPostProcessing.h
        pocketdb/services/Accessor.h
        pocketdb/services/WalController.h
        pocketdb/services/WalController.cpp
        pocketdb/repositories/BaseRepository.h
        pocketdb/repositories/BaseRepository.cpp
        pocketdb/repositories/TransactionRepository.h
        pocketdb/repositories/TransactionRepository.cpp
        pocketdb/repositories/RatingsRepository.h
        pocketdb/repositories/RatingsRepository.cpp
        pocketdb/repositories/ChainRepository.h
        pocketdb/repositories/ChainRepository.cpp
        pocketdb/repositories/ConsensusRepository.h
        pocketdb/repositories/ConsensusRepository.cpp
        pocketdb/repositories/CheckpointRepository.h
        pocketdb/repositories/CheckpointRepository.cpp
        pocketdb/repositories/SystemRepository.h
        pocketdb/repositories/SystemRepository.cpp
        pocketdb/repositories/MigrationRepository.h
        pocketdb/repositories/MigrationRepository.cpp
        pocketdb/repositories/web/NotifierRepository.h
        pocketdb/repositories/web/NotifierRepository.cpp
        pocketdb/repositories/web/WebRepository.h
        pocketdb/repositories/web/WebRpcRepository.h
        pocketdb/repositories/web/WebRepository.cpp
        pocketdb/repositories/web/WebRpcRepository.cpp
        pocketdb/repositories/web/ExplorerRepository.h
        pocketdb/repositories/web/ExplorerRepository.cpp
        pocketdb/repositories/web/SearchRepository.h
        pocketdb/repositories/web/SearchRepository.cpp
        pocketdb/repositories/web/ModerationRepository.h
        pocketdb/repositories/web/ModerationRepository.cpp
        pocketdb/repositories/web/BarteronRepository.h
        pocketdb/repositories/web/BarteronRepository.cpp
        pocketdb/repositories/web/AppRepository.h
        pocketdb/repositories/web/AppRepository.cpp
        pocketdb/consensus/Base.h
        pocketdb/consensus/Helper.h
        pocketdb/consensus/Social.h
        pocketdb/consensus/Lottery.h
        pocketdb/consensus/Reputation.h
        pocketdb/consensus/social/Blocking.hpp
        pocketdb/consensus/social/BlockingCancel.hpp
        pocketdb/consensus/social/Comment.hpp
        pocketdb/consensus/social/CommentEdit.hpp
        pocketdb/consensus/social/CommentDelete.hpp
        pocketdb/consensus/social/Complain.hpp
        pocketdb/consensus/social/Post.hpp
        pocketdb/consensus/social/Video.hpp
        pocketdb/consensus/social/Stream.hpp
        pocketdb/consensus/social/Audio.hpp
        pocketdb/consensus/social/Collection.hpp
        pocketdb/consensus/social/App.hpp
        pocketdb/consensus/social/ScoreComment.hpp
        pocketdb/consensus/social/ScoreContent.hpp
        pocketdb/consensus/social/Subscribe.hpp
        pocketdb/consensus/social/SubscribeCancel.hpp
        pocketdb/consensus/social/SubscribePrivate.hpp
        pocketdb/consensus/social/account/Account.hpp
        pocketdb/consensus/social/account/AccountUser.hpp
        pocketdb/consensus/social/account/AccountSetting.hpp
        pocketdb/consensus/social/account/AccountDelete.hpp
        pocketdb/consensus/social/ContentDelete.hpp
        pocketdb/consensus/social/BoostContent.hpp
        pocketdb/consensus/moderation/Flag.hpp
        pocketdb/consensus/moderation/Vote.hpp
        pocketdb/consensus/barteron/Offer.hpp
        pocketdb/consensus/barteron/Account.hpp
        pocketdb/consensus/Helper.cpp
        )
    target_link_libraries(${POCKETCOIN_SERVER} PRIVATE ${POCKETCOIN_COMMON_RPC} ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON} ${POCKETCOIN_SYSTEM} ${POCKETCOIN_CONSENSUS} ${POCKETCOIN_CRYPTO} ${POCKET_UTIL} Event::event ${CRYPT32} Boost::boost)
target_include_directories(${POCKETCOIN_SERVER} PRIVATE ${OPENSSL_INCLUDE_DIR} ${Event_INCLUDE_DIRS})
target_link_libraries(${POCKETCOIN_SERVER} PUBLIC sqlite3 univalue leveldb)
target_include_directories(${POCKETCOIN_SERVER} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT DISABLE_WALLET)
    target_sources(${POCKETCOIN_SERVER} PRIVATE
            wallet/bdb.h
            wallet/bdb.cpp
            wallet/coincontrol.h
            wallet/coincontrol.cpp
            wallet/context.h
            wallet/context.cpp
            wallet/crypter.h
            wallet/crypter.cpp
            wallet/db.h
            wallet/db.cpp
            wallet/feebumper.h
            wallet/feebumper.cpp
            wallet/fees.h
            wallet/fees.cpp
            wallet/init.cpp
            wallet/ismine.h
            wallet/load.h
            wallet/load.cpp
            wallet/rpcdump.cpp
            wallet/rpcwallet.h
            wallet/rpcwallet.cpp
            wallet/wallet.h
            wallet/wallet.cpp
            wallet/walletdb.h
            wallet/walletdb.cpp
            wallet/wallettool.h
            wallet/walletutil.h
            wallet/walletutil.cpp
            wallet/coinselection.h
            wallet/coinselection.cpp
            wallet/salvage.h
            wallet/scriptpubkeyman.h
            wallet/scriptpubkeyman.cpp

            wallet/sqlite.h
            wallet/sqlite.cpp
            interfaces/wallet.h
            interfaces/wallet.cpp
            staker.h
            staker.cpp
            )
    target_link_libraries(${POCKETCOIN_SERVER} PRIVATE Oracle::BerkeleyDB)
    add_compile_definitions(ENABLE_WALLET=1)
else ()
    target_sources(${POCKETCOIN_SERVER} PRIVATE dummywallet.cpp)
endif ()

if(REQUIRED_SOCKET_LIB)
    # same as for above for POCKETCOIN_UTIL
    target_link_libraries(${POCKETCOIN_SERVER} PRIVATE socket)
endif()

# TODO (build): USE_UPNP

# ZMQ
if (NOT DISABLE_ZMQ)
    find_package(ZMQ)
    if (ZMQ_FOUND)
        if (MSVC AND MSVC_FORCE_STATIC)
            target_compile_definitions(${POCKETCOIN_SERVER} PRIVATE ZMQ_STATIC)
        endif ()
        add_compile_definitions(ENABLE_ZMQ=1)
        set(POCKETCOIN_ZMQ_SOURCES
                zmq/zmqabstractnotifier.h
                zmq/zmqabstractnotifier.cpp
                zmq/zmqnotificationinterface.h
                zmq/zmqnotificationinterface.cpp
                zmq/zmqpublishnotifier.h
                # zmq/zmqconfig.h
                zmq/zmqpublishnotifier.cpp
                zmq/zmqrpc.h
                zmq/zmqrpc.cpp
                zmq/zmqutil.h
                zmq/zmqutil.cpp
                )
        target_sources(${POCKETCOIN_SERVER} PRIVATE ${POCKETCOIN_ZMQ_SOURCES})
        target_link_libraries(${POCKETCOIN_SERVER} PRIVATE ZMQ::zmq)
        target_include_directories(${POCKETCOIN_SERVER} PRIVATE ${Zmq_INCLUDE_DIRS})
    else ()
        message(WARNING "ZMQ not found, disabling")
    endif ()
else()
    add_compile_definitions(ENABLE_ZMQ=0)
endif ()

set(POCKETCOIND pocketcoind)
add_executable(${POCKETCOIND} pocketcoind.cpp)
target_link_libraries(${POCKETCOIND} PRIVATE ${POCKETCOIN_SERVER} ${POCKETCOIN_COMMON_RPC} ${POCKETDB} ${POCKETCOIN_UTIL} ${POCKETCOIN_CONSENSUS} ${POCKETCOIN_SYSTEM} OpenSSL::Crypto OpenSSL::SSL ${CRYPT32} Event::event sqlite3 univalue secp256k1 leveldb)
target_include_directories(${POCKETCOIND} PRIVATE ${OPENSSL_INCLUDE_DIR} ${Event_INCLUDE_DIRS})

add_library(libpocketcoin_cli rpc/client.h rpc/client.cpp)
target_link_libraries(libpocketcoin_cli PRIVATE univalue ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON_RPC})
target_include_directories(libpocketcoin_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set(POCKETCOIN_CLI pocketcoin-cli)
add_executable(${POCKETCOIN_CLI} pocketcoin-cli.cpp )
# TODO (build): remove leveldb after header fix
target_link_libraries(${POCKETCOIN_CLI} PRIVATE ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON_RPC} univalue Event::event libpocketcoin_cli leveldb)
target_include_directories(${POCKETCOIN_CLI} PRIVATE ${OPENSSL_INCLUDE_DIR} ${Event_INCLUDE_DIRS})

set(POCKETCOIN_TX pocketcoin-tx)
add_executable(${POCKETCOIN_TX} pocketcoin-tx.cpp)
target_link_libraries(${POCKETCOIN_TX} PRIVATE ${POCKETCOIN_SERVER} ${POCKETCOIN_UTIL} ${POCKETDB} ${POCKETCOIN_CRYPTO} ${POCKETCOIN_CONSENSUS} univalue sqlite3)

if(NOT DISABLE_GUI)
    add_subdirectory(qt/)
endif()
