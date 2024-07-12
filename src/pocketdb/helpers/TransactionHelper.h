// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_TRANSACTIONHELPER_H
#define POCKETHELPERS_TRANSACTIONHELPER_H

#include <string>
#include <key_io.h>
#include <boost/algorithm/string.hpp>
#include <numeric>
#include "script/standard.h"
#include "primitives/transaction.h"
#include "util/strencodings.h"

#include "pocketdb/models/base/DtoModels.h"
#include "pocketdb/models/base/PocketTypes.h"

#include "pocketdb/models/dto/money/Coinbase.h"
#include "pocketdb/models/dto/money/Coinstake.h"
#include "pocketdb/models/dto/money/Default.h"

#include "pocketdb/models/dto/content/Post.h"
#include "pocketdb/models/dto/content/Video.h"
#include "pocketdb/models/dto/content/Article.h"
#include "pocketdb/models/dto/content/Stream.h"
#include "pocketdb/models/dto/content/Audio.h"
#include "pocketdb/models/dto/content/Collection.h"
#include "pocketdb/models/dto/content/App.h"
#include "pocketdb/models/dto/content/Comment.h"
#include "pocketdb/models/dto/content/CommentEdit.h"
#include "pocketdb/models/dto/content/CommentDelete.h"
#include "pocketdb/models/dto/content/ContentDelete.h"

#include "pocketdb/models/dto/action/Subscribe.h"
#include "pocketdb/models/dto/action/SubscribeCancel.h"
#include "pocketdb/models/dto/action/SubscribePrivate.h"
#include "pocketdb/models/dto/action/Complain.h"
#include "pocketdb/models/dto/action/ScoreContent.h"
#include "pocketdb/models/dto/action/ScoreComment.h"
#include "pocketdb/models/dto/action/Blocking.h"
#include "pocketdb/models/dto/action/BlockingCancel.h"
#include "pocketdb/models/dto/action/BoostContent.h"

#include "pocketdb/models/dto/account/Setting.h"
#include "pocketdb/models/dto/account/User.h"
#include "pocketdb/models/dto/account/Delete.h"

#include "pocketdb/models/dto/moderation/Flag.h"
#include "pocketdb/models/dto/moderation/Vote.h"

#include "pocketdb/models/dto/barteron/Account.h"
#include "pocketdb/models/dto/barteron/Offer.h"

namespace PocketHelpers
{
    using namespace std;
    using namespace PocketTx;

    // Accumulate transactions in block
    typedef PocketTx::Transaction PTransaction;
    typedef shared_ptr<PocketTx::Transaction> PTransactionRef;
    typedef shared_ptr<PocketTx::TransactionInput> PTransactionInputRef;
    typedef shared_ptr<PocketTx::TransactionOutput> PTransactionOutputRef;
    typedef vector<PTransactionRef> PocketBlock;
    typedef shared_ptr<PocketBlock> PocketBlockRef;

    class TransactionHelper
    {
    public:
        static TxoutType ScriptType(const CScript& scriptPubKey);
        static std::string ExtractDestination(const CScript& scriptPubKey);
        static tuple<bool, string> GetPocketAuthorAddress(const CTransactionRef& tx);
        static TxType ConvertOpReturnToType(const string& op);
        static string ParseAsmType(const CTransactionRef& tx, vector<string>& vasm);
        static TxType ParseType(const CTransactionRef& tx, vector<string>& vasm);
        static TxType ParseType(const CTransactionRef& tx);
        static string ConvertToReindexerTable(const Transaction& transaction);
        static string ExtractOpReturnHash(const CTransactionRef& tx);
        static tuple<bool, string> ExtractOpReturnPayload(const CTransactionRef& tx);
        static bool IsPocketSupportedTransaction(const CTransactionRef& tx, TxType& txType);
        static bool IsPocketSupportedTransaction(const CTransactionRef& tx);
        static bool IsPocketSupportedTransaction(const CTransaction& tx);
        static bool IsPocketTransaction(TxType& txType);
        static bool IsPocketTransaction(const CTransactionRef& tx, TxType& txType);
        static bool IsPocketTransaction(const CTransactionRef& tx);
        static bool IsPocketTransaction(const CTransaction& tx);
        static bool IsPocketNeededPaymentTransaction(const CTransactionRef& tx);
        static tuple<bool, shared_ptr<ScoreDataDto>> ParseScore(const CTransactionRef& tx);
        static PTransactionRef CreateInstance(TxType txType);
        static PTransactionRef CreateInstance(TxType txType, const CTransactionRef& tx);
        static bool IsIn(TxType txType, const vector<TxType>& inTypes);
        static string TxStringType(TxType type);
        static TxType TxIntType(const string& type);
    };
}

#endif // POCKETHELPERS_TRANSACTIONHELPER_H
