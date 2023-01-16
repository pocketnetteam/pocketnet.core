// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAIN_POST_PROCESSING_H
#define POCKETDB_CHAIN_POST_PROCESSING_H

#include "util/system.h"
#include "chain.h"
#include "primitives/block.h"
#include "chainparams.h"

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/pocketnet.h"

namespace PocketServices
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;
    using namespace PocketConsensus;

    using std::make_tuple;
    using std::tuple;
    using std::vector;
    using std::find;

    

    class ChainPostProcessing
    {
    public:
        static void Index(const CBlock& block, int height);
        static bool Rollback(int height);
    protected:
        static void PrepareTransactions(const CBlock& block, vector<TransactionIndexingInfo>& txs);
        static void IndexChain(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs);
        static void IndexRatings(int height, vector<TransactionIndexingInfo>& txs);
        static void IndexModeration(int height, vector<TransactionIndexingInfo>& txs);
        static void IndexBadges(int height);
    private:
        static int BadgePeriod()
        {
            switch (Params().NetworkID())
            {
            case NetworkMain:
                return 1440;
            case NetworkTest:
                return 100;
            case NetworkRegTest:
                return 5;
            default:
                throw std::runtime_error(strprintf("Not supported network"));
            }
        }
    };
} // namespace PocketServices

#endif // POCKETDB_CHAIN_POST_PROCESSING_H
