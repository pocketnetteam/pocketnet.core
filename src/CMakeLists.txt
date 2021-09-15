include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)
include(CheckLinkerFlag)

### Options

option(DISABLE_ZMQ "Disable ZMQ notifications (default is OFF)" OFF)

option(WITH_SYSTEM_UNIVALUE "Build with system UniValue (default is OFF)" OFF)

option(ENABLE_ASM "Enable assembly routines (default is NO)" ON)

set(BDB_ROOT CACHE STRING "Path to BerkleyDB 4.8")

option(DISABLE_WALLET "Disable wallet (enabled by default)")

set(WITH_MINIUPNPC CACHE STRING "enable UPNP (default is yes if libminiupnpc is found)" "auto")

option(ENABLE_UPNP_DEFAULT "If UPNP is enabled, turn it on at startup (default is no)")

option(DISABLE_TESTS "Do not compile tests (default is to compile)" OFF)
if(NOT DISABLE_TESTS)
    set(USE_TESTS ON)
endif()

option(DISABLE_GUI_TESTS "Do not compile GUI tests (default is to compile if GUI and tests enabled)" ${USE_TESTS})

option(WITH_RAPIDCHECK "Enable RapidCheck property based tests (default is yes if librapidcheck is found)" "auto")

option(DISABLE_BENCH "do not compile benchmarks (default is to compile)" OFF)
if (NOT DISABLE_BENCH)
    set(USE_BENCH ON)
endif()

option(ENABLE_EXTENDED_FUNCTIONAL_TESTS "enable expensive functional tests when using lcov (default OFF)" OFF)

option(WITH_QRENCODE "enable QR code support (default is yes if qt is enabled and libqrencode is found)" "auto")

option(DISABLE_HARDENING "do not attempt to harden the resulting executables (default is to harden when possible)" "auto")

option(ENABLE_REDUCE_EXPORTS "attempt to reduce exported symbols in the resulting executables (default is OFF)" OFF)

option(DISABLE_CCACHE "do not use ccache for building (default is to use if found)" "auto")

option(ENABLE_LCOV "enable lcov testing branch coverage (default is OFF)" OFF)

option(ENABLE_GLIBC_BACK_COMPAT "enable backwards compatibility with glibc" OFF)

option(WITH_INCOMPATIBLE_BDB "Allow using a bdb version greater than 4.8" OFF)

option(DISABLE_MAN "do not install man pages (default is to install)" OFF)
if(NOT DISABLE_MAM)
    set(ENABLE_MAN ON)
endif()

option(WITH_UTILS "build pocketcoin-cli pocketcoin-tx (default=ON)" ON)
option(WITH_DAEMON "build pocketcoind daemon (default=ON)" ON)

# BerkeleyDB
if(NOT DISABLE_WALLET)
    set(BerkeleyDB_FIND_REQUIRED ON)
    include(cmake/FindBerkeleyDB.cmake)
    set(BDB_INTERESTED_VERSION "${BerkeleyDB_VERSION_MAJOR}.${BerkeleyDB_VERSION_MINOR}")
    if(BDB_INTERESTED_VERSION VERSION_EQUAL "4.8")
        set(BDB_FOUND ON)
    elseif(BDB_INTERESTED_VERSION VERSION_GREATER "4.8")
        if(WITH_INCOMPATIBLE_BDB)
            set(BDB_FOUND ON)
            message(WARNING "Found Berkeley DB other than 4.8; wallets opened by this build will not be portable!")
        else()
            message(FATAL_ERROR "Found Berkeley DB other than 4.8, required for portable wallets (-DWITH_INCOMPATIBLE_BDB to ignore or -DDISABLE_WALLET to disable wallet functionality)")
        endif()
    else()
        message(FATAL_ERROR "BDB 4.8+ required for wallet functionality (-DDISABLE_WALLET to disable wallet functionality)")
    endif()
endif()

if(ENABLE_ASM)
    add_compile_definitions(USE_ASM=1)
endif()

# Hack to use default cmake's fpic settings if not specified
if(DEFINED WITH_PIC)
    if(WITH_PIC)
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    else()
        set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Check endian headers
include(CmakeCheckEndian.cmake)

# LevelDB
# TODO only if EMBEDDED_LVLDB defined
add_subdirectory(leveldb/)


# secp256k1

# Enabling options for sepc
set(ENABLE_MODULE_RECOVERY ON)
# Force fpic to ON for sepc (as it has done in autotools)
set(WITH_PIC_BACKUP ${WITH_PIC})
set(WITH_PIC ON CACHE BOOL "Enable fPIC" FORCE)
add_subdirectory(secp256k1/)
# Return original fpic
set(WITH_PIC ${WITH_PIC_BACKUP} CACHE BOOL "Enable fPIC" FORCE)

# Finding boost
# TODO minimal boost version is increased to 1.66 because websockets (websocket/ws.h) are using lambdas as WriteHandler. This functional was implemented in 1.66.
find_package(Boost 1.66 REQUIRED COMPONENTS System Thread Chrono filesystem)
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
set(CMAKE_REQUIRED_LIBRARIES Boost::thread Boost::system Boost::chrono)
check_cxx_source_compiles("${CHECK_BOOST_SLEEP}" HAVE_WORKING_BOOST_SLEEP_FOR )
if(HAVE_WORKING_BOOST_SLEEP_FOR)
    add_compile_definitions(HAVE_WORKING_BOOST_SLEEP_FOR=1)
else()
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
        else()
                message(FATAL_ERROR "No working boost sleep implementation found.")
        endif()
endif()

# Common checks
set(CMAKE_REQUIRED_FLAGS -msse4.1)
check_cxx_source_compiles( "#include <stdint.h>
                            #include <immintrin.h>
                            int main() {
                                __m128i l = _mm_set1_epi32(0);
                                return _mm_extract_epi32(l, 3);
                            }" ENABLE_SSE41)

set(CMAKE_REQUIRED_FLAGS -mavx -mavx2)
check_cxx_source_compiles( "#include <stdint.h>
                            #include <immintrin.h>
                            int main() {
                                __m256i l = _mm256_set1_epi32(0);
                                return _mm256_extract_epi32(l, 7);
                            }" ENABLE_AVX2)

set(CMAKE_REQUIRED_FLAGS -msse4 -msha)
check_cxx_source_compiles( "#include <stdint.h>
                            #include <immintrin.h>
                            int main() {
                                __m128i i = _mm_set1_epi32(0);
                                __m128i j = _mm_set1_epi32(1);
                                __m128i k = _mm_set1_epi32(2);
                                return _mm_extract_epi32(_mm_sha256rnds2_epu32(i, i, k), 0);
                            }" ENABLE_SHANI)


if(ENABLE_GLIBC_BACK_COMPAT)
    find_library(REQUIRED rt)
    set(CMAKE_REQUIRED_LIBRARIES rt)
    check_function_exists(clock_gettime CLOCK_GETTIME_EXISTS)
    if(NOT CLOCK_GETTIME_EXISTS)
        message(FATAL_ERROR "librt missing")
    endif()
    check_cxx_source_compiles( "#ifdef _FORTIFY_SOURCE
                                    #undef _FORTIFY_SOURCE
                                #endif
                                #define _FORTIFY_SOURCE 2
                                #include <sys/select.h>
                                extern \"C\" long unsigned int __fdelt_warn(long unsigned int);
                                int main() {return 0;}
                                " FDELT_TYPE)
    if(FDELT_TYPE)
        add_compile_definitions(FDELT_TYPE="long unsigned int")
    else()
        add_compile_definitions(FDELT_TYPE="long int")
    endif()
endif()

check_cxx_source_compiles(" #include <malloc.h>
                            int main(){
                                int f = malloc_info(0, NULL);
                            }" HAVE_MALLOC_INFO)
if(HAVE_MALLOC_INFO)
    add_compile_definitions(HAVE_MALLOC_INFO=1)
endif()

check_cxx_source_compiles(" #include <malloc.h>
                            int main(){
                                mallopt(M_ARENA_MAX, 1);
                            }" HAVE_MALLOPT_ARENA_MAX)
if(HAVE_MALLOPT_ARENA_MAX)
    add_compile_definitions(HAVE_MALLOPT_ARENA_MAX=1)
endif()

check_cxx_source_compiles(" int foo_def( void ) __attribute__((visibility(\"default\")));
                            int main(){}" HAVE_VISIBILITY_ATTRIBUTE)
if(HAVE_VISIBILITY_ATTRIBUTE)
    add_compile_definitions(HAVE_VISIBILITY_ATTRIBUTE=1)
elseif(ENABLE_REDUCE_EXPORTS)
    message(FATAL_ERROR "Cannot find a working visibility attribute. Use -DENABLE_REDUCE_EXPORTS=OFF.")
endif()

check_cxx_source_compiles(" #include <thread>
                            static thread_local int foo = 0;
                            static void run_thread() { foo++;}
                            int main(){
                                for(int i = 0; i < 10; i++) { std::thread(run_thread).detach();}
                                return foo;
                            }" HAVE_THREAD_LOCAL)
if(HAVE_THREAD_LOCAL)
    add_compile_definitions(HAVE_THREAD_LOCAL=1)
endif()

check_cxx_source_compiles(" #include <sys/syscall.h>
                            #include <linux/random.h>
                            int main(){
                                syscall(SYS_getrandom, nullptr, 32, 0);
                            }" HAVE_SYS_GETRANDOM)
if(HAVE_SYS_GETRANDOM)
    add_compile_definitions(HAVE_SYS_GETRANDOM=1)
endif()

check_cxx_source_compiles(" #include <unistd.h>
                            int main(){
                                getentropy(nullptr, 32);
                            }" HAVE_GETENTROPY)
if(HAVE_GETENTROPY)
    add_compile_definitions(HAVE_GETENTROPY=1)
endif()

check_cxx_source_compiles(" #include <unistd.h>
                            #include <sys/random.h
                            int main(){
                                getentropy(nullptr, 32) ;
                            }" HAVE_GETENTROPY_RAND)
if(HAVE_GETENTROPY_RAND)
    add_compile_definitions(HAVE_GETENTROPY_RAND=1)
endif()

check_cxx_source_compiles(" #include <stdint.h>
                            #include <type_traits>
                            int main(){
                                static_assert(std::is_same<int8_t, char>::value, \"\");
                            }" CHAR_EQUALS_INT8)
if(CHAR_EQUALS_INT8)
    add_compile_definitions(CHAR_EQUALS_INT8=1)
endif()

if(ENABLE_REDUCE_EXPORTS)
    check_cxx_compiler_flag(-fvisibility=hidden VISIBILITY_HIDDEN)
    if(NOT VISIBILITY_HIDDEN)
        message(FATAL_ERROR "Cannot set default symbol visibility. Use -DENABLE_REDUCE_EXPORTS=OFF")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_BACKUP})
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_BACKUP})

# Looking for required system libraries
find_package(Event 2.0 REQUIRED)

find_package(SQlite REQUIRED)

find_package(OpenSSL REQUIRED)
if(WIN32)
    find_library(WS2_32_LIB ws2_32 REQUIRED)
    add_compile_definitions(NOMINMAX)
endif()

list(APPEND CMAKE_REQUIRED_FLAGS ${COMPAT_LDFLAGS})

# Some configuration
check_function_exists(strnlen HAVE_DECL_STRNLEN)
if(HAVE_DECL_STRNLEN)
        add_compile_definitions(HAVE_DECL_STRNLEN=1)
else(HAVE_DECL_STRNLEN)
        add_compile_definitions(HAVE_DECL_STRNLEN=0)
endif(HAVE_DECL_STRNLEN)


# Univalue library
if(WITH_SYSTEM_UNIVALUE)
    # TODO write find_package script
    find_library(UNIVALUE univalue)
    if(NOT UNIVALUE)
        message(FATAL_ERROR "System univalue requested but not found")
    endif()
else()
    add_subdirectory(univalue/)
endif()


# Common library
set(POCKETCOIN_COMMON pocketcoin_common)
add_library(${POCKETCOIN_COMMON}
                cuckoocache.h
                indirectmap.h
                limitedmap.h
                memusage.h
                version.h
                reverse_iterator.h
                reverselock.h
                streams.h
                undo.h
                span.h
                prevector.h
                compat.h
                compat/byteswap.h
                compat/endian.h
                compat/sanity.h
                compat/glibc_sanity.cpp
                compat/glibcxx_sanity.cpp
                compat/strnlen.cpp
                amount.h
                tinyformat.h
            )


# Pocket DB functional library.
# It is divided to pocketdb-server part and this functional part
set(POCKETDB pocketdb)
add_library(${POCKETDB}
                pocketdb/migrations/main.h
                pocketdb/models/base/PocketTypes.h
                pocketdb/models/base/Base.h
                pocketdb/models/base/Payload.h
                pocketdb/models/base/Transaction.h
                pocketdb/models/base/TransactionOutput.h
                pocketdb/models/base/Rating.h
                pocketdb/models/base/ReturnDtoModels.h
                pocketdb/models/dto/Default.h
                pocketdb/models/dto/Coinbase.h
                pocketdb/models/dto/Coinstake.h
                pocketdb/models/dto/Post.h
                pocketdb/models/dto/User.h
                pocketdb/models/dto/ScoreContent.h
                pocketdb/models/dto/ScoreComment.h
                pocketdb/models/dto/Blocking.h
                pocketdb/models/dto/BlockingCancel.h
                pocketdb/models/dto/Comment.h
                pocketdb/models/dto/CommentEdit.h
                pocketdb/models/dto/CommentDelete.h
                pocketdb/models/dto/Subscribe.h
                pocketdb/models/dto/SubscribePrivate.h
                pocketdb/models/dto/SubscribeCancel.h
                pocketdb/models/dto/Complain.h
                pocketdb/models/dto/Video.h
                pocketdb/migrations/main.cpp
                pocketdb/models/base/Base.cpp
                pocketdb/models/base/Payload.cpp
                pocketdb/models/base/Transaction.cpp
                pocketdb/models/base/TransactionOutput.cpp
                pocketdb/models/base/Rating.cpp
                pocketdb/models/dto/Default.cpp
                pocketdb/models/dto/Coinbase.cpp
                pocketdb/models/dto/Coinstake.cpp
                pocketdb/models/dto/Post.cpp
                pocketdb/models/dto/User.cpp
                pocketdb/models/dto/ScoreContent.cpp
                pocketdb/models/dto/ScoreComment.cpp
                pocketdb/models/dto/Blocking.cpp
                pocketdb/models/dto/BlockingCancel.cpp
                pocketdb/models/dto/Comment.cpp
                pocketdb/models/dto/CommentEdit.cpp
                pocketdb/models/dto/CommentDelete.cpp
                pocketdb/models/dto/Subscribe.cpp
                pocketdb/models/dto/SubscribePrivate.cpp
                pocketdb/models/dto/SubscribeCancel.cpp
                pocketdb/models/dto/Complain.cpp
                pocketdb/models/dto/Video.cpp
            )
target_link_libraries(${POCKETDB} PRIVATE ${POCKETCOIN_COMMON} ${POCKETCOIN_UTIL} ${POCKETCOIN_CRYPTO} univalue leveldb)


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


# Pocketcoin util library
set(POCKETCOIN_UTIL pocketcoin_util)
add_library(${POCKETCOIN_UTIL}
                clientversion.h
                clientversion.cpp
                fs.h
                fs.cpp
                logging.h
                logging.cpp
                sync.h
                sync.cpp
                threadsafety.h
                threadinterrupt.h
                threadinterrupt.cpp
                utilmemory.h
                utilmoneystr.h
                utilmoneystr.cpp
                utiltime.h
                utiltime.cpp
                utils/html.h
                utils/html.cpp
                random.h
                random.cpp
                timedata.h
                timedata.cpp
                netaddress.h
                netaddress.cpp
                ui_interface.h
                ui_interface.cpp
                util.h
                util.cpp
                chainparamsbase.h
                chainparamsbase.cpp
                chainparamsseeds.h
                warnings.h
                warnings.cpp
            )
target_link_libraries(${POCKETCOIN_UTIL} PRIVATE ${POCKETCOIN_SUPPORT} ${POCKETCOIN_COMMON} ${POCKETCOIN_CRYPTO} OpenSSL::Crypto Boost::thread Boost::chrono Boost::filesystem Event::event)
if(WIN32)
    target_link_libraries(${POCKETCOIN_UTIL} PRIVATE ${WS2_32_LIB})
endif()
if(ENABLE_GLIBC_BACK_COMPAT)
    target_sources(${POCKETCOIN_UTIL} PRIVATE compat/glibc_compat.cpp)
endif()
target_include_directories(${POCKETCOIN_UTIL} PUBLIC ${Boost_INCLUDE_DIRS})

######## Crypto
set(POCKETCOIN_CRYPTO pocketcoin_crypto)
add_library(${POCKETCOIN_CRYPTO}
                crypto/sha256_sse4.cpp
                crypto/sha256.cpp
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
                crypto/sha512.cpp
                crypto/sha1.cpp
                crypto/common.h
                crypto/hmac_sha256.h
                crypto/sha256.h
                crypto/aes.cpp
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
                utilstrencodings.h
                utilstrencodings.cpp
            )
target_link_libraries(${POCKETCOIN_CRYPTO} PRIVATE ${POCKETCOIN_COMMON} ${POCKETCOIN_SUPPORT} secp256k1)
if(ENABLE_SSE41)
    target_sources(${POCKETCOIN_CRYPTO} PRIVATE crypto/sha256_sse41.cpp)
    if(MSVC)
        #        TODO it seems like windows fails with everything exept sse2 that is enabled by default
        #        Futhermore there are cmake scripts to find sse4.1 or avx support where they are always hardcoded as false for Windows
        #        https://github.com/magic-sph/magic/blob/master/cmake/FindSSE.cmake
        #        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE /arch:SSE4.1)
    else()
        target_compile_definitions(${POCKETCOIN_CRYPTO} PRIVATE ENABLE_SSE41)
        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE -msse4.1)
    endif()
endif()
if(ENABLE_AVX2)
    target_sources(${POCKETCOIN_CRYPTO} PRIVATE crypto/sha256_avx2.cpp)
    if(MSVC)
        #        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE /arch:AVX /arch:AVX2)
    else()
        target_compile_definitions(${POCKETCOIN_CRYPTO} PRIVATE ENABLE_AVX2)
        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE -mavx -mavx2)
    endif()
endif()
if(ENABLE_SHANI)
    target_sources(${POCKETCOIN_CRYPTO} PRIVATE crypto/sha256_shani.cpp)
    if(MSVC)
        #        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE /arch:SSE4 /arch:SHA)
    else()
        target_compile_definitions(${POCKETCOIN_CRYPTO} PRIVATE ENABLE_SHANI)
        target_compile_options(${POCKETCOIN_CRYPTO} PRIVATE -msse4 -msha)
    endif()
endif()


# System
set(POCKETCOIN_SYSTEM pocketcoin_system)
add_library(${POCKETCOIN_SYSTEM} scheduler.cpp
                scheduler.h
                shutdown.h
                shutdown.cpp
            )
target_link_libraries(${POCKETCOIN_SYSTEM} PRIVATE ${POCKETCOIN_UTIL} ${POCKETCOIN_CRYPTO})

# Consensus
# TODO probably completely move this to server
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


# Common rpc that used by server and cli
set(POCKETCOIN_COMMON_RPC pocketcoin_common_rpc)
add_library(${POCKETCOIN_COMMON_RPC}
                rpc/protocol.h
                rpc/protocol.cpp)
target_link_libraries(${POCKETCOIN_COMMON_RPC} PRIVATE ${POCKETCOIN_CRYPTO} ${POCKETCOIN_UTIL})


########### Server
set(POCKETCOIN_SERVER pocketcoin_server)
add_library(${POCKETCOIN_SERVER}
                interfaces/node.h
                interfaces/node.cpp
                chainparams.h
                chainparams.cpp
                consensus/tx_verify.h
                consensus/tx_verify.cpp
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
                checkpoints.h
                checkpoints.cpp
                checkqueue.h
                blockencodings.h
                blockencodings.cpp
                blockfilter.h
                blockfilter.cpp
                httprpc.h
                httprpc.cpp
                httpserver.h
                httpserver.cpp
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
                keystore.h
                keystore.cpp
                compressor.h
                compressor.cpp
                key_io.h
                key_io.cpp
                websocket/ws.h
                websocket/ws.cpp
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
                script/descriptor.h
                script/descriptor.cpp
                script/ismine.h
                script/ismine.cpp
                script/sigcache.h
                script/sigcache.cpp
                script/sign.h
                script/sign.cpp
                script/standard.h
                script/standard.cpp
                script/interpreter.h
                script/interpreter.cpp
                script/pocketcoinconsensus.cpp # TODO What to do with this
                script/pocketcoinconsensus.h
                script/script_error.cpp
                script/script_error.h
                script/script.cpp
                script/script.h
                txmempool.h
                txmempool.cpp
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
                netmessagemaker.h
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
                rpc/rawtransaction.h
                rpc/rawtransaction.cpp
                rpc/register.h
                rpc/server.cpp
                rpc/server.h
                rpc/util.cpp
                rpc/util.h
                walletinitinterface.h
                pocketdb/helpers/TransactionHelper.h
                pocketdb/helpers/CheckpointHelper.h
                pocketdb/helpers/TransactionHelper.cpp
                pocketdb/helpers/LotteryCheckpointHelper.cpp
                pocketdb/helpers/SocialCheckpointHelper.cpp
                pocketdb/helpers/OpReturnCheckpointHelper.cpp
                pocketdb/SQLiteDatabase.h
                pocketdb/SQLiteConnection.h
                pocketdb/SQLiteDatabase.cpp
                pocketdb/SQLiteConnection.cpp
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
                pocketdb/web/PocketRpc.h
                pocketdb/web/PocketRpc.cpp
                pocketdb/pocketnet.h
                pocketdb/pocketnet.cpp
                pocketdb/services/TransactionSerializer.cpp
                pocketdb/services/TransactionIndexer.cpp
                pocketdb/services/Accessor.cpp
                pocketdb/services/TransactionSerializer.h
                pocketdb/services/TransactionIndexer.h
                pocketdb/services/Accessor.h
                pocketdb/repositories/BaseRepository.hpp
                pocketdb/repositories/TransactionRepository.hpp
                pocketdb/repositories/ChainRepository.h
                pocketdb/repositories/ConsensusRepository.h
                pocketdb/repositories/web/WebRepository.h
                pocketdb/repositories/web/ExplorerRepository.h
                pocketdb/repositories/web/WebRepository.cpp
                pocketdb/repositories/web/ExplorerRepository.cpp
                pocketdb/repositories/ConsensusRepository.cpp
                pocketdb/repositories/ChainRepository.cpp
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
                pocketdb/consensus/social/ScoreComment.hpp
                pocketdb/consensus/social/ScoreContent.hpp
                pocketdb/consensus/social/Subscribe.hpp
                pocketdb/consensus/social/SubscribeCancel.hpp
                pocketdb/consensus/social/SubscribePrivate.hpp
                pocketdb/consensus/social/User.hpp
                pocketdb/consensus/Helper.cpp
                pocketdb/consensus/Base.cpp
                pocketdb/consensus/Lottery.cpp
                pocketdb/consensus/Reputation.cpp
            )
target_link_libraries(${POCKETCOIN_SERVER} PRIVATE ${POCKETCOIN_COMMON_RPC} ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON} ${POCKETCOIN_SYSTEM} ${POCKETCOIN_CONSENSUS} ${POCKETCOIN_CRYPTO} Event::event leveldb univalue SQlite::sqlite OpenSSL::Crypto Boost::boost)
target_include_directories(${POCKETCOIN_SERVER} PRIVATE ${SQlite3_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${Event_INCLUDE_DIRS})

if(NOT DISABLE_WALLET)
    target_sources(${POCKETCOIN_SERVER} PRIVATE
                        wallet/coincontrol.h
                        wallet/coincontrol.cpp
                        wallet/crypter.h
                        wallet/crypter.cpp
                        wallet/db.h
                        wallet/db.cpp
                        wallet/feebumper.h
                        wallet/feebumper.cpp
                        wallet/fees.h
                        wallet/fees.cpp
                        wallet/init.cpp
                        wallet/rpcdump.cpp
                        wallet/rpcwallet.h
                        wallet/rpcwallet.cpp
                        wallet/wallet.h
                        wallet/wallet.cpp
                        wallet/walletdb.h
                        wallet/walletdb.cpp
                        wallet/walletutil.h
                        wallet/walletutil.cpp
                        wallet/coinselection.h
                        wallet/coinselection.cpp
                        interfaces/wallet.h
                        interfaces/wallet.cpp
                        staker.h
                        staker.cpp
                    )
    target_link_libraries(${POCKETCOIN_SERVER} PRIVATE Oracle::BerkeleyDB)
    add_compile_definitions(ENABLE_WALLET=1)
else()
    target_sources(${POCKETCOIN_SERVER} PRIVATE dummywallet.cpp)
endif()

# TODO USE_UPNP

# ZMQ
if (NOT DISABLE_ZMQ)
    find_library(ZMQ zmq)
    if(ZMQ)
        add_compile_definitions(ENABLE_ZMQ)
        set(POCKETCOIN_ZMQ_SOURCES   
                zmq/zmqabstractnotifier.h
                zmq/zmqabstractnotifier.cpp
                zmq/zmqnotificationinterface.h
                zmq/zmqnotificationinterface.cpp
                zmq/zmqpublishnotifier.h
                zmq/zmqconfig.h
                zmq/zmqpublishnotifier.cpp
                zmq/zmqrpc.h
                zmq/zmqrpc.cpp
            )
        target_sources(${POCKETCOIN_SERVER} PRIVATE ${POCKETCOIN_ZMQ_SOURCES})
        target_link_libraries(${POCKETCOIN_SERVER} PRIVATE ${ZMQ})
    else()
        message(WARNING "ZMQ not found, disabling")
    endif()
endif()

set(POCKETCOIND pocketcoind)
add_executable(${POCKETCOIND} pocketcoind.cpp)
target_link_libraries(${POCKETCOIND} PRIVATE ${POCKETCOIN_SERVER} ${POCKETCOIN_COMMON_RPC} ${POCKETDB} ${POCKETCOIN_UTIL} ${POCKETCOIN_CONSENSUS} ${POCKETCOIN_SYSTEM} OpenSSL::Crypto Event::event SQlite::sqlite univalue memenv)
target_include_directories(${POCKETCOIND} PRIVATE ${OPENSSL_INCLUDE_DIR} ${Event_INCLUDE_DIRS} ${SQlite3_INCLUDE_DIR})

set(POCKETCOIN_CLI pocketcoin_cli)
add_executable(${POCKETCOIN_CLI} pocketcoin-cli.cpp rpc/client.cpp)
target_link_libraries(${POCKETCOIN_CLI} PRIVATE ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON_RPC} univalue Event::event)
target_include_directories(${POCKETCOIN_CLI} PRIVATE ${Event_INCLUDE_DIRS})

set(POCKETCOIN_TX pocketcoin_tx)
add_executable(${POCKETCOIN_TX} pocketcoin-tx.cpp)
target_link_libraries(${POCKETCOIN_TX} PRIVATE ${POCKETCOIN_SERVER} ${POCKETCOIN_UTIL} ${POCKETDB} ${POCKETCOIN_CRYPTO} ${POCKETCOIN_CONSENSUS} univalue)