# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = sqlite secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

POCKETCOIN_INCLUDES=-I$(builddir) $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)
POCKETCOIN_INCLUDES += -I$(srcdir)/secp256k1/include
POCKETCOIN_INCLUDES += -I$(srcdir)/sqlite/include
POCKETCOIN_INCLUDES += $(UNIVALUE_CFLAGS)

LIBPOCKETCOIN_SERVER=libpocketcoin_server.a
LIBPOCKETCOIN_COMMON=libpocketcoin_common.a
LIBPOCKETCOIN_CONSENSUS=libpocketcoin_consensus.a
LIBPOCKETCOIN_CLI=libpocketcoin_cli.a
LIBPOCKETCOIN_UTIL=libpocketcoin_util.a
LIBPOCKETCOIN_CRYPTO_BASE=crypto/libpocketcoin_crypto_base.a
LIBPOCKETCOINQT=qt/libpocketcoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBSQLITE3=sqlite/libsqlite3.la

if ENABLE_ZMQ
LIBPOCKETCOIN_ZMQ=libpocketcoin_zmq.a
endif
if BUILD_POCKETCOIN_LIBS
LIBPOCKETCOINCONSENSUS=libpocketcoinconsensus.la
endif
if ENABLE_WALLET
LIBPOCKETCOIN_WALLET=libpocketcoin_wallet.a
endif

LIBPOCKETCOIN_CRYPTO= $(LIBPOCKETCOIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBPOCKETCOIN_CRYPTO_SSE41 = crypto/libpocketcoin_crypto_sse41.a
LIBPOCKETCOIN_CRYPTO += $(LIBPOCKETCOIN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBPOCKETCOIN_CRYPTO_AVX2 = crypto/libpocketcoin_crypto_avx2.a
LIBPOCKETCOIN_CRYPTO += $(LIBPOCKETCOIN_CRYPTO_AVX2)
endif
if ENABLE_SHANI
LIBPOCKETCOIN_CRYPTO_SHANI = crypto/libpocketcoin_crypto_shani.a
LIBPOCKETCOIN_CRYPTO += $(LIBPOCKETCOIN_CRYPTO_SHANI)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

$(LIBSQLITE3): $(wildcard sqlite/include/*.h) sqlite/sqlite3.c sqlite/shell.c
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBPOCKETCOIN_CRYPTO) \
  $(LIBPOCKETCOIN_UTIL) \
  $(LIBPOCKETCOIN_COMMON) \
  $(LIBPOCKETCOIN_CONSENSUS) \
  $(LIBPOCKETCOIN_SERVER) \
  $(LIBPOCKETCOIN_CLI) \
  $(LIBPOCKETCOIN_WALLET) \
  $(LIBPOCKETCOIN_ZMQ)  

lib_LTLIBRARIES = $(LIBPOCKETCOINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_POCKETCOIND
  bin_PROGRAMS += pocketcoind
endif

if BUILD_POCKETCOIN_UTILS
  bin_PROGRAMS += pocketcoin-cli pocketcoin-tx
endif

.PHONY: FORCE check-symbols check-security

# PocketDB
POCKETDB_H = \
    pocketdb/pocketnet.h \
    pocketdb/SQLiteDatabase.h \
    pocketdb/SQLiteConnection.h \
    \
    pocketdb/migrations/base.h \
    pocketdb/migrations/main.h \
    pocketdb/migrations/web.h \
    \
    pocketdb/helpers/PocketnetHelper.h \
    pocketdb/helpers/TransactionHelper.h \
    pocketdb/helpers/ShortFormHelper.h \
    \
    pocketdb/web/PocketContentRpc.h \
    pocketdb/web/PocketCommentsRpc.h \
    pocketdb/web/PocketSystemRpc.h \
    pocketdb/web/PocketScoresRpc.h \
    pocketdb/web/PocketAccountRpc.h \
    pocketdb/web/PocketFrontend.h \
    pocketdb/web/PocketExplorerRpc.h \
    pocketdb/web/PocketTransactionRpc.h \
    pocketdb/web/WebSocketRpc.h \
    pocketdb/web/SearchRpc.h \
    pocketdb/web/PocketTagsRpc.h \
    pocketdb/web/PocketRpc.h \
    pocketdb/web/WebRpcUtils.h \
    \
    pocketdb/repositories/BaseRepository.h \
    pocketdb/repositories/TransactionRepository.h \
    pocketdb/repositories/ChainRepository.h \
    pocketdb/repositories/ConsensusRepository.h \
    pocketdb/repositories/RatingsRepository.h \
    pocketdb/repositories/CheckpointRepository.h \
    pocketdb/repositories/SystemRepository.h \
    pocketdb/repositories/MigrationRepository.h \
    pocketdb/repositories/web/WebRepository.h \
    pocketdb/repositories/web/WebRpcRepository.h \
    pocketdb/repositories/web/NotifierRepository.h \
    pocketdb/repositories/web/ExplorerRepository.h \
    pocketdb/repositories/web/SearchRepository.h \
    \
    pocketdb/services/WsNotifier.h \
    pocketdb/services/b/services/Serializer.h \
    pocketdb/services/b/services/ChainPostProcessing.h \
    pocketdb/services/b/services/WebPostProcessing.h \
    pocketdb/services/Accessor.h \
    \
    pocketdb/consensus/Base.h \
    pocketdb/consensus/Helper.h \
    pocketdb/consensus/Social.h \
    pocketdb/consensus/Lottery.h \
    pocketdb/consensus/Reputation.h \
    \
    pocketdb/consensus/social/Blocking.hpp \
    pocketdb/consensus/social/BlockingCancel.hpp \
    pocketdb/consensus/social/Comment.hpp \
    pocketdb/consensus/social/CommentEdit.hpp \
    pocketdb/consensus/social/CommentDelete.hpp \
    pocketdb/consensus/social/Complain.hpp \
    pocketdb/consensus/social/Post.hpp \
    pocketdb/consensus/social/Video.hpp \
    pocketdb/consensus/social/Article.hpp \
    pocketdb/consensus/social/BoostContent.hpp \
    pocketdb/consensus/social/ScoreComment.hpp \
    pocketdb/consensus/social/ScoreContent.hpp \
    pocketdb/consensus/social/Subscribe.hpp \
    pocketdb/consensus/social/SubscribeCancel.hpp \
    pocketdb/consensus/social/SubscribePrivate.hpp \
    pocketdb/consensus/social/AccountUser.hpp \
    pocketdb/consensus/social/AccountSetting.hpp \
    pocketdb/consensus/social/ContentDelete.hpp \
    \
    pocketdb/consensus/moderation/Flag.hpp \
    \
    pocketdb/models/base/PocketTypes.h \
    pocketdb/models/base/Base.h \
    pocketdb/models/base/Payload.h \
    pocketdb/models/base/Transaction.h \
    pocketdb/models/base/TransactionInput.h \
    pocketdb/models/base/TransactionOutput.h \
    pocketdb/models/base/SocialTransaction.h \
    pocketdb/models/base/Rating.h \
    pocketdb/models/base/Payload.h \
    pocketdb/models/base/DtoModels.h \
    \
    pocketdb/models/dto/money/Default.h \
    pocketdb/models/dto/money/Coinbase.h \
    pocketdb/models/dto/money/Coinstake.h \
    pocketdb/models/dto/content/Content.h \
    pocketdb/models/dto/content/Post.h \
    pocketdb/models/dto/content/ContentDelete.h \
    pocketdb/models/dto/action/BoostContent.h \
    pocketdb/models/dto/account/Delete.h \
    pocketdb/models/dto/account/Setting.h \
    pocketdb/models/dto/account/User.h \
    pocketdb/models/dto/action/ScoreContent.h \
    pocketdb/models/dto/action/ScoreComment.h \
    pocketdb/models/dto/action/Blocking.h \
    pocketdb/models/dto/action/BlockingCancel.h \
    pocketdb/models/dto/content/Comment.h \
    pocketdb/models/dto/content/CommentEdit.h \
    pocketdb/models/dto/content/CommentDelete.h \
    pocketdb/models/dto/action/Subscribe.h \
    pocketdb/models/dto/action/SubscribePrivate.h \
    pocketdb/models/dto/action/SubscribeCancel.h \
    pocketdb/models/dto/action/Complain.h \
    pocketdb/models/dto/content/Video.h \
    pocketdb/models/dto/content/Article.h \
    pocketdb/models/dto/moderation/Flag.h \
    \
    pocketdb/models/web/WebTag.h \
    pocketdb/models/web/WebContent.h \
    pocketdb/models/web/SearchRequest.h \
    pocketdb/models/shortform/ShortForm.h \
    pocketdb/models/shortform/ShortAccount.h \
    pocketdb/models/shortform/ShortTxData.h \
    pocketdb/models/shortform/ShortTxType.h \
    pocketdb/models/shortform/ShortTxOutput.h

# PocketDb CPP
POCKETDB_CPP = \
    pocketdb/SQLiteDatabase.cpp \
    pocketdb/SQLiteConnection.cpp \
    pocketdb/pocketnet.cpp \
    \
    pocketdb/migrations/main.cpp \
    pocketdb/migrations/web.cpp \
    \
    pocketdb/helpers/TransactionHelper.cpp \
    pocketdb/helpers/ShortFormHelper.cpp \
    \
    pocketdb/services/WsNotifier.cpp \
    pocketdb/services/Serializer.cpp \
    pocketdb/services/ChainPostProcessing.cpp \
    pocketdb/services/WebPostProcessing.cpp \
    pocketdb/services/Accessor.cpp \
    \
    pocketdb/repositories/ConsensusRepository.cpp \
    pocketdb/repositories/ChainRepository.cpp \
    pocketdb/repositories/TransactionRepository.cpp \
    pocketdb/repositories/RatingsRepository.cpp \
    pocketdb/repositories/CheckpointRepository.cpp \
    pocketdb/repositories/SystemRepository.cpp \
    pocketdb/repositories/MigrationRepository.cpp \
    pocketdb/repositories/web/WebRepository.cpp \
    pocketdb/repositories/web/WebRpcRepository.cpp \
    pocketdb/repositories/web/NotifierRepository.cpp \
    pocketdb/repositories/web/ExplorerRepository.cpp \
    pocketdb/repositories/web/SearchRepository.cpp \
    \
    pocketdb/consensus/Helper.cpp \
    pocketdb/consensus/Base.cpp \
    pocketdb/consensus/Lottery.cpp \
    \
    pocketdb/models/base/Base.cpp \
    pocketdb/models/base/Payload.cpp \
    pocketdb/models/base/Transaction.cpp \
    pocketdb/models/base/TransactionInput.cpp \
    pocketdb/models/base/TransactionOutput.cpp \
    pocketdb/models/base/SocialTransaction.cpp \
    pocketdb/models/base/Rating.cpp \
    pocketdb/models/base/Payload.cpp \
    \
    pocketdb/web/PocketContentRpc.cpp \
    pocketdb/web/PocketCommentsRpc.cpp \
    pocketdb/web/PocketSystemRpc.cpp \
    pocketdb/web/PocketScoresRpc.cpp \
    pocketdb/web/PocketAccountRpc.cpp \
    pocketdb/web/PocketFrontend.cpp \
    pocketdb/web/PocketExplorerRpc.cpp \
    pocketdb/web/PocketTransactionRpc.cpp \
    pocketdb/web/WebSocketRpc.cpp \
    pocketdb/web/SearchRpc.cpp \
    pocketdb/web/PocketTagsRpc.cpp \
    pocketdb/web/PocketRpc.cpp \
    pocketdb/web/WebRpcUtils.cpp \
    \
    pocketdb/models/dto/money/Default.cpp \
    pocketdb/models/dto/money/Coinbase.cpp \
    pocketdb/models/dto/money/Coinstake.cpp \
    pocketdb/models/dto/content/Content.cpp \
    pocketdb/models/dto/content/Post.cpp \
    pocketdb/models/dto/content/ContentDelete.cpp \
    pocketdb/models/dto/action/BoostContent.cpp \
    pocketdb/models/dto/account/Setting.cpp \
    pocketdb/models/dto/account/Delete.cpp \
    pocketdb/models/dto/account/User.cpp \
    pocketdb/models/dto/action/ScoreContent.cpp \
    pocketdb/models/dto/action/ScoreComment.cpp \
    pocketdb/models/dto/action/Blocking.cpp \
    pocketdb/models/dto/action/BlockingCancel.cpp \
    pocketdb/models/dto/content/Comment.cpp \
    pocketdb/models/dto/content/CommentEdit.cpp \
    pocketdb/models/dto/content/CommentDelete.cpp \
    pocketdb/models/dto/action/Subscribe.cpp \
    pocketdb/models/dto/action/SubscribePrivate.cpp \
    pocketdb/models/dto/action/SubscribeCancel.cpp \
    pocketdb/models/dto/action/Complain.cpp \
    pocketdb/models/dto/content/Video.cpp \
    pocketdb/models/dto/action/BoostContent.cpp \
    pocketdb/models/dto/content/Article.cpp \
    pocketdb/models/dto/moderation/Flag.cpp \
    pocketdb/models/shortform/ShortForm.cpp \
    pocketdb/models/shortform/ShortAccount.cpp \
    pocketdb/models/shortform/ShortTxData.cpp \
    pocketdb/models/shortform/ShortTxOutput.cpp

# CORE
POCKETCOIN_CORE_H = \
    addrdb.h \
    addrman.h \
    attributes.h \
    base58.h \
    bech32.h \
    bloom.h \
    blockencodings.h \
    blockfilter.h \
    chain.h \
    chainparams.h \
    chainparamsbase.h \
    chainparamsseeds.h \
    checkpoints.h \
    checkqueue.h \
    clientversion.h \
    coins.h \
    compat.h \
    compat/byteswap.h \
    compat/endian.h \
    compat/sanity.h \
    compressor.h \
    consensus/consensus.h \
    consensus/tx_verify.h \
    core_io.h \
    core_memusage.h \
    cuckoocache.h \
    eventloop.h \
    fs.h \
    httprpc.h \
    httpserver.h \
    index/base.h \
    index/txindex.h \
    indirectmap.h \
    init.h \
    interfaces/handler.h \
    interfaces/node.h \
    interfaces/wallet.h \
    key.h \
    key_io.h \
    keystore.h \
    dbwrapper.h \
    ldb/ldb.h \
    limitedmap.h \
    logging.h \
    memusage.h \
    merkleblock.h \
    miner.h \
    net.h \
    net_processing.h \
    netaddress.h \
    netbase.h \
    netmessagemaker.h \
    noui.h \
    outputtype.h \
    policy/feerate.h \
    policy/fees.h \
    policy/policy.h \
    policy/rbf.h \
    pos.h \
    pow.h \
    protectedmap.h \
    protocol.h \
    random.h \
    reverse_iterator.h \
    reverselock.h \
    rpc/blockchain.h \
    rpc/cache.h \
    rpc/client.h \
    rpc/mining.h \
    rpc/protocol.h \
    rpc/server.h \
    rpc/rawtransaction.h \
    rpc/register.h \
    rpc/util.h \
    statistic.hpp \
    scheduler.h \
    script/descriptor.h \
    script/ismine.h \
    script/sigcache.h \
    script/sign.h \
    script/standard.h \
    shutdown.h \
    staker.h \
    streams.h \
    support/allocators/secure.h \
    support/allocators/zeroafterfree.h \
    support/cleanse.h \
    support/events.h \
    support/lockedpool.h \
    sync.h \
    threadsafety.h \
    threadinterrupt.h \
    timedata.h \
    torcontrol.h \
    txdb.h \
    txmempool.h \
    ui_interface.h \
    undo.h \
    util.h \
    utilmemory.h \
    utilmoneystr.h \
    utiltime.h \
    validation.h \
    validationinterface.h \
    versionbits.h \
    walletinitinterface.h \
    wallet/coincontrol.h \
    wallet/crypter.h \
    wallet/db.h \
    wallet/feebumper.h \
    wallet/fees.h \
    wallet/rpcwallet.h \
    wallet/wallet.h \
    wallet/walletdb.h \
    wallet/walletutil.h \
    wallet/coinselection.h \
    warnings.h \
    zmq/zmqabstractnotifier.h \
    zmq/zmqconfig.h\
    zmq/zmqnotificationinterface.h \
    zmq/zmqpublishnotifier.h \
    zmq/zmqrpc.h \
    websocket/ws.h \
    websocket/notifyprocessor.h \
    utils/html.h \
    $(POCKETDB_H)


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libpocketcoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between pocketcoind and pocketcoin-qt
libpocketcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libpocketcoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_server_a_SOURCES = \
    addrdb.cpp \
    addrman.cpp \
    bloom.cpp \
    blockencodings.cpp \
    blockfilter.cpp \
    chain.cpp \
    checkpoints.cpp \
    consensus/tx_verify.cpp \
    httprpc.cpp \
    httpserver.cpp \
    index/base.cpp \
    index/txindex.cpp \
    interfaces/handler.cpp \
    interfaces/node.cpp \
    init.cpp  \
    ldb/ldb.cpp \
    dbwrapper.cpp \
    merkleblock.cpp \
    miner.cpp \
    net.cpp \
    net_processing.cpp \
    noui.cpp \
    outputtype.cpp \
    policy/fees.cpp \
    policy/policy.cpp \
    policy/rbf.cpp \
    pos.cpp \
    pow.cpp \
    rest.cpp \
    rpc/blockchain.cpp \
    rpc/cache.cpp \
    rpc/mining.cpp \
    rpc/misc.cpp \
    rpc/net.cpp \
    rpc/rawtransaction.cpp \
    rpc/server.cpp \
    rpc/util.cpp \
    script/sigcache.cpp \
    shutdown.cpp \
    staker.cpp \
    timedata.cpp \
    torcontrol.cpp \
    txdb.cpp \
    txmempool.cpp \
    ui_interface.cpp \
    validation.cpp \
    validationinterface.cpp \
    versionbits.cpp \
    websocket/ws.cpp \
    websocket/notifyprocessor.cpp \
    utils/html.cpp \
    $(POCKETDB_CPP) \
    $(POCKETCOIN_CORE_H)

if !ENABLE_WALLET
libpocketcoin_server_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libpocketcoin_zmq_a_CPPFLAGS = $(POCKETCOIN_INCLUDES) $(ZMQ_CFLAGS)
libpocketcoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp
endif


# wallet: shared between pocketcoind and pocketcoin-qt, but only linked
# when wallet enabled
libpocketcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
libpocketcoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_wallet_a_SOURCES = \
  interfaces/wallet.cpp \
  wallet/coincontrol.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/init.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(POCKETCOIN_CORE_H)

# crypto primitives library
crypto_libpocketcoin_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libpocketcoin_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libpocketcoin_crypto_base_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h

if USE_ASM
crypto_libpocketcoin_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_libpocketcoin_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libpocketcoin_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libpocketcoin_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libpocketcoin_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_libpocketcoin_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_libpocketcoin_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libpocketcoin_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libpocketcoin_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libpocketcoin_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_libpocketcoin_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

crypto_libpocketcoin_crypto_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libpocketcoin_crypto_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libpocketcoin_crypto_shani_a_CXXFLAGS += $(SHANI_CXXFLAGS)
crypto_libpocketcoin_crypto_shani_a_CPPFLAGS += -DENABLE_SHANI
crypto_libpocketcoin_crypto_shani_a_SOURCES = crypto/sha256_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
libpocketcoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
libpocketcoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/pocketcoinconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  span.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between pocketcoind, and pocketcoin-qt and non-server tools
libpocketcoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
libpocketcoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  key_io.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  policy/feerate.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/ismine.cpp \
  script/sign.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(POCKETCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libpocketcoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
libpocketcoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  logging.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(POCKETCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libpocketcoin_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between pocketcoin-cli and pocketcoin-qt
libpocketcoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
libpocketcoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libpocketcoin_cli_a_SOURCES = \
  rpc/client.cpp \
  $(POCKETCOIN_CORE_H)

nodist_libpocketcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# pocketcoind binary #
pocketcoind_SOURCES = pocketcoind.cpp
pocketcoind_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
pocketcoind_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
pocketcoind_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
pocketcoind_SOURCES += pocketcoind-res.rc
endif

pocketcoind_LDADD = \
  $(LIBPOCKETCOIN_SERVER) \
  $(LIBPOCKETCOIN_WALLET) \
  $(LIBPOCKETCOIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBPOCKETCOIN_UTIL) \
  $(LIBPOCKETCOIN_ZMQ) \
  $(LIBPOCKETCOIN_CONSENSUS) \
  $(LIBPOCKETCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1) \
  $(LIBSQLITE3)

pocketcoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS)

# pocketcoin-cli binary #
pocketcoin_cli_SOURCES = pocketcoin-cli.cpp
pocketcoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES) $(EVENT_CFLAGS)
pocketcoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
pocketcoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
pocketcoin_cli_SOURCES += pocketcoin-cli-res.rc
endif

pocketcoin_cli_LDADD = \
  $(LIBPOCKETCOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBPOCKETCOIN_UTIL) \
  $(LIBPOCKETCOIN_CRYPTO)

pocketcoin_cli_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)

# pocketcoin-tx binary #
pocketcoin_tx_SOURCES = pocketcoin-tx.cpp
pocketcoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(POCKETCOIN_INCLUDES)
pocketcoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
pocketcoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
pocketcoin_tx_SOURCES += pocketcoin-tx-res.rc
endif

pocketcoin_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBPOCKETCOIN_COMMON) \
  $(LIBPOCKETCOIN_UTIL) \
  $(LIBPOCKETCOIN_CONSENSUS) \
  $(LIBPOCKETCOIN_CRYPTO) \
  $(LIBSECP256K1)

pocketcoin_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# pocketcoinconsensus library #
if BUILD_POCKETCOIN_LIBS
include_HEADERS = script/pocketcoinconsensus.h
libpocketcoinconsensus_la_SOURCES = $(crypto_libpocketcoin_crypto_base_a_SOURCES) $(libpocketcoin_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libpocketcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libpocketcoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libpocketcoinconsensus_la_LIBADD = $(LIBSECP256K1)
libpocketcoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_POCKETCOIN_INTERNAL
libpocketcoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/pocketcoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/pocketcoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/pocketcoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/pocketcoin-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

# if ENABLE_QT_TESTS
# include Makefile.qttest.include
# endif
