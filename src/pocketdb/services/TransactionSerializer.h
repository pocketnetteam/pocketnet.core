// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_H
#define POCKETTX_TRANSACTIONSERIALIZER_H

#include "pocketdb/helpers/TransactionHelper.h"

#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/models/base/TransactionOutput.h"
#include "pocketdb/models/dto/Coinbase.h"
#include "pocketdb/models/dto/Coinstake.h"
#include "pocketdb/models/dto/Default.h"
#include "pocketdb/models/dto/User.h"
#include "pocketdb/models/dto/Post.h"
#include "pocketdb/models/dto/Video.h"
#include "pocketdb/models/dto/Blocking.h"
#include "pocketdb/models/dto/BlockingCancel.h"
#include "pocketdb/models/dto/Comment.h"
#include "pocketdb/models/dto/ScoreContent.h"
#include "pocketdb/models/dto/ScoreComment.h"
#include "pocketdb/models/dto/Subscribe.h"
#include "pocketdb/models/dto/SubscribePrivate.h"
#include "pocketdb/models/dto/SubscribeCancel.h"
#include "pocketdb/models/dto/Complain.h"

#include "key_io.h"
#include "streams.h"
#include "logging.h"

#include <utilstrencodings.h>

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketHelpers;

    class TransactionSerializer
    {
    public:
        static tuple<bool, PocketBlock> DeserializeBlock(CBlock& block, CDataStream& stream);
        static tuple<bool, PocketBlock> DeserializeBlock(CBlock& block);

        static tuple<bool, PTransactionRef> DeserializeTransactionRpc(const CTransactionRef& tx, const UniValue& pocketData);
        static tuple<bool, PTransactionRef> DeserializeTransaction(const CTransactionRef& tx, CDataStream& stream);
        static tuple<bool, PTransactionRef> DeserializeTransaction(const CTransactionRef& tx);

        static shared_ptr<UniValue> SerializeBlock(const PocketBlock& block);
        static shared_ptr<UniValue> SerializeTransaction(const Transaction& transaction);

    private:
        static shared_ptr<Transaction> buildInstance(const CTransactionRef& tx, const UniValue& src);
        static shared_ptr<Transaction> buildInstanceRpc(const CTransactionRef& tx, const UniValue& src);
        static bool buildOutputs(const CTransactionRef& tx, shared_ptr<Transaction>& ptx);
        static UniValue parseStream(CDataStream& stream);
        static tuple<bool, PocketBlock> deserializeBlock(CBlock& block, UniValue& pocketData);
        static tuple<bool, shared_ptr<Transaction>> deserializeTransaction(const CTransactionRef& tx, UniValue& pocketData);
    };

}

#endif // POCKETTX_TRANSACTIONSERIALIZER_H
