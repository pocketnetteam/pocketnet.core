// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_HELPER_H
#define POCKETCONSENSUS_HELPER_H

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/SocialConsensus.h"

#include "pocketdb/consensus/social/PostT.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    // This helper need for hide selector Consensus rules
    class SocialConsensusHelper
    {
    public:
        static bool Validate(const PocketBlockRef& block, int height);
        static tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, int height);
        static tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, PocketBlockRef& block, int height);
        // Проверяет блок транзакций без привязки к цепи
        static bool Check(const CBlock& block, const PocketBlockRef& pBlock);
        // Проверяет транзакцию без привязки к цепи
        static tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const PTransactionRef& ptx);
    protected:
        static tuple<bool, SocialConsensusResult> validate(const PTransactionRef& ptx, const PocketBlockRef& block, int height);
        static tuple<bool, SocialConsensusResult> check(const CTransactionRef& tx, const PTransactionRef& ptx);
        static bool isConsensusable(PocketTxType txType);
        static shared_ptr<SocialConsensus> getConsensus(PocketTxType txType, int height = 0);
        static tuple<bool, SocialConsensusResult> validateT(const PTransactionRef& ptx, const PocketBlockRef& block, int height);
    };
}

#endif // POCKETCONSENSUS_HELPER_H
