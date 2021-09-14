include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckSymbolExists)

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

option(WITH_SYSTEM_UNIVALUE "Build with system UniValue (default is OFF)" OFF)

option(WITH_INCOMPATIBLE_BDB "Allow using a bdb version greater than 4.8" OFF)

option(DISABLE_MAN "do not install man pages (default is to install)" OFF)
if(NOT DISABLE_MAM)
    set(ENABLE_MAN ON)
endif()

# BerkeleyDB
if(NOT DISABLE_WALLET)
    set(BerkeleyDB_FIND_REQUIRED ON)
    include(FindBerkeleyDB.cmake)
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
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_BACKUP})

# Looking for required system libraries
find_library(REQUIRED event)
find_library(REQUIRED event_pthreads)
find_library(REQUIRED sqlite3)
find_package(OpenSSL REQUIRED)


# Some configuration
check_function_exists(strnlen HAVE_DECL_STRNLEN)
if(HAVE_DECL_STRNLEN)
        add_compile_definitions(HAVE_DECL_STRNLEN=1)
else(HAVE_DECL_STRNLEN)
        add_compile_definitions(HAVE_DECL_STRNLEN=0)
endif(HAVE_DECL_STRNLEN)


# Univalue library
# TODO do this only if EMBEDDED_UNIVALUE defined (see root configure)
if(WITH_SYSTEM_UNIVALUE)
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
target_link_libraries(${POCKETCOIN_UTIL} PRIVATE ${POCKETCOIN_SUPPORT} ${POCKETCOIN_COMMON} ${POCKETCOIN_CRYPTO} OpenSSL::Crypto Boost::thread Boost::chrono Boost::filesystem)


######## Crypto
set(POCKETCOIN_CRYPTO pocketcoin_crypto)
add_library(${POCKETCOIN_CRYPTO} 
                crypto/sha256_sse4.cpp
                crypto/sha256.cpp
                crypto/chacha20.cpp
                crypto/aes.h
                crypto/sha256_sse41.cpp
                crypto/hmac_sha256.cpp
                crypto/sha512.h
                crypto/hmac_sha512.cpp
                crypto/sha256_shani.cpp
                crypto/ripemd160.cpp
                crypto/sha1.h
                crypto/ripemd160.h
                crypto/hmac_sha512.h
                crypto/chacha20.h
                crypto/sha512.cpp
                crypto/sha1.cpp
                crypto/sha256_avx2.cpp
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
target_link_libraries(${POCKETCOIN_COMMON_RPC} PRIVATE ${POCKETCOIN_CRYPTO})


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
                staker.h
                staker.cpp
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
target_link_libraries(${POCKETCOIN_SERVER} PRIVATE ${POCKETCOIN_COMMON_RPC} ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON} ${POCKETCOIN_SYSTEM} ${POCKETCOIN_CONSENSUS} ${POCKETCOIN_CRYPTO} leveldb univalue sqlite3)
if(NOT DISABLE_WALLET)
    target_sources(pocketcoin_server PRIVATE
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
                    )
    target_link_libraries(${POCKETCOIN_SERVER} PRIVATE Oracle::BerkeleyDB)
endif()


# ZMQ
if (NOT DISABLE_ZMQ)
    find_library(ZMQ zmq)
    # TODO allow specify zmq by user
    # TODO probably rework this with pkgconfig to allow checking minimal version
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
target_link_libraries(${POCKETCOIND} PRIVATE ${POCKETCOIN_SERVER} ${POCKETCOIN_COMMON_RPC} ${POCKETDB} ${POCKETCOIN_UTIL} ${POCKETCOIN_CONSENSUS} ${POCKETCOIN_SYSTEM} event event_pthreads memenv)
add_compile_definitions(ENABLE_WALLET=1)

set(POCKETCOIN_CLI pocketcoin_cli)
add_executable(${POCKETCOIN_CLI} pocketcoin-cli.cpp rpc/client.cpp)
target_link_libraries(${POCKETCOIN_CLI} PRIVATE ${POCKETCOIN_UTIL} ${POCKETCOIN_COMMON_RPC} univalue event)

set(POCKETCOIN_TX pocketcoin_tx)
add_executable(${POCKETCOIN_TX} pocketcoin-tx.cpp)
target_link_libraries(${POCKETCOIN_TX} PRIVATE ${POCKETCOIN_SERVER} ${POCKETCOIN_UTIL} ${POCKETDB} ${POCKETCOIN_CRYPTO} ${POCKETCOIN_CONSENSUS} univalue)