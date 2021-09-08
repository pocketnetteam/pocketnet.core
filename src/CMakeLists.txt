include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckSymbolExists)

### Options

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
include(check_endian.cmake)

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
find_package(Boost 1.66 REQUIRED COMPONENTS System Thread Chrono)

# Univalue library
# TODO do this only if EMBEDDED_UNIVALUE defined (see root configure)
add_subdirectory(univalue/)

set(POCKETCOIN_COMMON pocketcoin_common)
add_library(${POCKETCOIN_COMMON} index/base.h
                                 index/base.cpp
                                 index/txindex.h
                                 index/txindex.cpp
                                 addrdb.h addrdb.cpp
                                 addrman.h addrman.cpp
                                 base58.h base58.cpp
                                 bech32.h bech32.cpp
                                 bloom.h bloom.cpp
                                 blockencodings.h blockencodings.cpp
                                 blockfilter.h blockfilter.cpp
                                 chainparams.h chainparams.cpp
                                 chainparamsseeds.h
                                 checkpoints.h checkpoints.cpp
                                 checkqueue.h
                                 clientversion.h clientversion.cpp
                                 coins.h coins.cpp
                                 compat.h
                                 compat/byteswap.h
                                 compat/endian.h
                                 compat/sanity.h
                                 compressor.h compressor.cpp
                                 consensus/consensus.h
                                 consensus/tx_verify.h consensus/tx_verify.cpp
                                 core_io.h
                                 core_memusage.h
                                 cuckoocache.h
                                 httprpc.h httprpc.cpp
                                 httpserver.h httpserver.cpp
                                 indirectmap.h
                                 init.h init.cpp
                                 interfaces/handler.h interfaces/handler.cpp
                                 interfaces/node.h interfaces/node.cpp
                                 interfaces/wallet.h interfaces/wallet.cpp
                                 key.h key.cpp
                                 key_io.h key_io.cpp
                                 keystore.h keystore.cpp
                                 dbwrapper.h dbwrapper.cpp
                                 ldb/ldb.h ldb/ldb.cpp
                                 limitedmap.h
                                 memusage.h
                                 merkleblock.h merkleblock.cpp
                                 miner.h miner.cpp
                                 net.h net.cpp
                                 net_processing.h net_processing.cpp
                                 netaddress.h netaddress.cpp
                                 netbase.h netbase.cpp
                                 netmessagemaker.h
                                 noui.h noui.cpp
                                 outputtype.h outputtype.cpp
                                 policy/feerate.h policy/feerate.cpp
                                 policy/fees.h policy/fees.cpp
                                 policy/policy.h policy/policy.cpp
                                 policy/rbf.h policy/rbf.cpp
                                 pos.h pos.cpp
                                 pow.h pow.cpp
                                 protocol.h protocol.cpp
                                 reverse_iterator.h
                                 reverselock.h
                                 rpc/blockchain.h rpc/blockchain.cpp
                                 rpc/client.h rpc/client.cpp
                                 rpc/mining.h rpc/mining.cpp
                                 rpc/protocol.h rpc/protocol.cpp
                                 rpc/server.h rpc/server.cpp
                                 rpc/rawtransaction.h rpc/rawtransaction.cpp
                                 rpc/register.h
                                 rpc/util.h rpc/util.cpp
                                 statistic.hpp
                                 scheduler.h scheduler.cpp
                                 script/descriptor.h script/descriptor.cpp
                                 script/ismine.h script/ismine.cpp
                                 script/sigcache.h script/sigcache.cpp
                                 script/sign.h script/sign.cpp
                                 script/standard.h script/standard.cpp
                                 shutdown.h shutdown.cpp
                                 staker.h staker.cpp
                                 streams.h
                                 timedata.h timedata.cpp
                                 torcontrol.h torcontrol.cpp
                                 txdb.h txdb.cpp
                                 txmempool.h txmempool.cpp
                                 ui_interface.h ui_interface.cpp
                                 undo.h
                                 validation.h validation.cpp
                                 validationinterface.h validationinterface.cpp
                                 versionbits.h versionbits.cpp
                                 walletinitinterface.h
                                 wallet/coincontrol.h wallet/coincontrol.cpp
                                 wallet/crypter.h wallet/crypter.cpp
                                 wallet/db.h wallet/db.cpp
                                 wallet/feebumper.h wallet/feebumper.cpp
                                 wallet/fees.h wallet/fees.cpp
                                 wallet/rpcwallet.h wallet/rpcwallet.cpp
                                 wallet/wallet.h wallet/wallet.cpp
                                 wallet/walletdb.h wallet/walletdb.cpp
                                 wallet/walletutil.h wallet/walletutil.cpp
                                 wallet/coinselection.h wallet/coinselection.cpp
                                 warnings.h warnings.cpp
                                 zmq/zmqconfig.h
                                 zmq/zmqrpc.h zmq/zmqrpc.cpp
                                 websocket/ws.h websocket/ws.cpp
                                 utils/html.h utils/html.cpp)
target_link_libraries(${POCKETCOIN_COMMON} PUBLIC univalue leveldb secp256k1)

# ZMQ
# TODO allow specify zmq by user
# TODO probably rework this with pkgconfig to allow checking minimal version
set(POCKETCOIN_ZMQ pocketcoin_zmq)
find_library(ZMQ_FOUND zmq)
if(ZMQ_FOUND)
        add_compile_definitions(ENABLE_ZMQ)
        message(STATUS "hehe")
        add_library(${POCKETCOIN_ZMQ}   zmq/zmqabstractnotifier.h
                                        zmq/zmqabstractnotifier.cpp
                                        zmq/zmqnotificationinterface.h
                                        zmq/zmqnotificationinterface.cpp
                                        zmq/zmqpublishnotifier.h
                                        zmq/zmqconfig.h
                                        zmq/zmqpublishnotifier.cpp
                                        zmq/zmqrpc.h
                                        zmq/zmqrpc.cpp)
        target_link_libraries(${POCKETCOIN_ZMQ} ${POCKETCOIN_COMMON})
endif(ZMQ_FOUND)

# Pocket DB library
set(POCKETDB pocketdb)
add_library(${POCKETDB} pocketdb/pocketnet.h 
                        pocketdb/SQLiteDatabase.h 
                        pocketdb/SQLiteConnection.h 

                        pocketdb/migrations/main.h 

                        pocketdb/helpers/TransactionHelper.h 
                        pocketdb/helpers/CheckpointHelper.h 

                        pocketdb/web/PocketContentRpc.h 
                        pocketdb/web/PocketCommentsRpc.h 
                        pocketdb/web/PocketSystemRpc.h 
                        pocketdb/web/PocketScoresRpc.h 
                        pocketdb/web/PocketAccountRpc.h 
                        pocketdb/web/PocketFrontend.h 
                        pocketdb/web/PocketExplorerRpc.h 
                        pocketdb/web/PocketTransactionRpc.h 
                        pocketdb/web/PocketRpc.h 

                        pocketdb/repositories/BaseRepository.hpp 
                        pocketdb/repositories/TransactionRepository.hpp 
                        pocketdb/repositories/ChainRepository.h 
                        pocketdb/repositories/ConsensusRepository.h 
                        pocketdb/repositories/web/WebRepository.h 
                        pocketdb/repositories/web/ExplorerRepository.h 

                        pocketdb/services/TransactionSerializer.h 
                        pocketdb/services/TransactionIndexer.h 
                        pocketdb/services/Accessor.h 

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

                        pocketdb/models/base/PocketTypes.h 
                        pocketdb/models/base/Base.h 
                        pocketdb/models/base/Payload.h 
                        pocketdb/models/base/Transaction.h 
                        pocketdb/models/base/TransactionOutput.h 
                        pocketdb/models/base/Rating.h 
                        pocketdb/models/base/Payload.h
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
                        pocketdb/SQLiteDatabase.cpp 
                        pocketdb/SQLiteConnection.cpp 
                        pocketdb/pocketnet.cpp 
                        
                        pocketdb/migrations/main.cpp 
                        
                        pocketdb/helpers/TransactionHelper.cpp 
                        pocketdb/helpers/LotteryCheckpointHelper.cpp 
                        pocketdb/helpers/SocialCheckpointHelper.cpp 
                        pocketdb/helpers/OpReturnCheckpointHelper.cpp
                        
                        pocketdb/services/TransactionSerializer.cpp 
                        pocketdb/services/TransactionIndexer.cpp 
                        pocketdb/services/Accessor.cpp 
                        
                        pocketdb/repositories/web/WebRepository.cpp 
                        pocketdb/repositories/web/ExplorerRepository.cpp 
                        pocketdb/repositories/ConsensusRepository.cpp 
                        pocketdb/repositories/ChainRepository.cpp 
                        
                        pocketdb/consensus/Helper.cpp 
                        pocketdb/consensus/Base.cpp 
                        pocketdb/consensus/Lottery.cpp 
                        pocketdb/consensus/Reputation.cpp 
                        
                        pocketdb/models/base/Base.cpp 
                        pocketdb/models/base/Payload.cpp
                        pocketdb/models/base/Transaction.cpp 
                        pocketdb/models/base/TransactionOutput.cpp 
                        pocketdb/models/base/Rating.cpp 
                        pocketdb/models/base/Payload.cpp 
                        
                        pocketdb/web/PocketContentRpc.cpp 
                        pocketdb/web/PocketCommentsRpc.cpp 
                        pocketdb/web/PocketSystemRpc.cpp 
                        pocketdb/web/PocketScoresRpc.cpp 
                        pocketdb/web/PocketAccountRpc.cpp 
                        pocketdb/web/PocketFrontend.cpp 
                        pocketdb/web/PocketExplorerRpc.cpp 
                        pocketdb/web/PocketTransactionRpc.cpp 
                        pocketdb/web/PocketRpc.cpp 
                        
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
                        pocketdb/models/dto/Video.cpp)

# Univalue defined in its cmake
# message(STATUS ${UNIVALUE})
# add_dependencies(${POCKETDB} ${UNIVALUE})
target_link_libraries(${POCKETDB} PRIVATE univalue leveldb ${POCKETCOIN_COMMON})

# Pocketcoin support library
add_subdirectory(support/)
# Pocketcoin util library
set (POCKETCOIN_UTIL pocketcoin_util)
add_library(${POCKETCOIN_UTIL}  chainparamsbase.h
                                chainparamsbase.cpp
                                clientversion.h
                                clientversion.cpp
                                compat/glibc_sanity.cpp
                                compat/glibcxx_sanity.cpp
                                compat/strnlen.cpp
                                fs.h
                                fs.cpp
                                logging.h
                                logging.cpp
                                random.h
                                random.cpp
                                sync.h
                                sync.cpp
                                threadsafety.h
                                threadinterrupt.h
                                threadinterrupt.cpp
                                util.h
                                util.cpp
                                utilmemory.h
                                utilmoneystr.h
                                utilmoneystr.cpp
                                utilstrencodings.h
                                utilstrencodings.cpp
                                utiltime.h
                                utiltime.cpp)
target_link_libraries(${POCKETCOIN_UTIL} PRIVATE ${POCKETDB})
check_function_exists(strnlen HAVE_DECL_STRNLEN)
if(HAVE_DECL_STRNLEN)
        add_compile_definitions(HAVE_DECL_STRNLEN=1)
else(HAVE_DECL_STRNLEN)
        add_compile_definitions(HAVE_DECL_STRNLEN=0)
endif(HAVE_DECL_STRNLEN)
target_link_libraries(${POCKETCOIN_UTIL} PRIVATE Boost::thread Boost::chrono)



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
                        
set(CMAKE_REQUIRED_LIBRARIES Boost::thread Boost::system Boost::chrono)
check_cxx_source_compiles("${CHECK_BOOST_SLEEP}" HAVE_WORKING_BOOST_SLEEP_FOR )
if(HAVE_WORKING_BOOST_SLEEP_FOR)
        add_compile_definitions(HAVE_WORKING_BOOST_SLEEP_FOR=1)
else(HAVE_WORKING_BOOST_SLEEP_FOR)
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
        else(HAVE_WORKING_BOOST_SLEEP)
                message(FATAL_ERROR "No working boost sleep implementation found.")
        endif(HAVE_WORKING_BOOST_SLEEP)
endif(HAVE_WORKING_BOOST_SLEEP_FOR)
