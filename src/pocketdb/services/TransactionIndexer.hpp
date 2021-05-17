// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONINDEXER_HPP
#define POCKETDB_TRANSACTIONINDEXER_HPP

#include "util.h"

#include "chain.h"
#include "primitives/block.h"
#include "pocketdb/pocketnet.h"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;

    using std::vector;
    using std::tuple;
    using std::make_tuple;

    class TransactionIndexer
    {
    public:
        static bool Index(const CBlock& block, int height)
        {
            auto result = true;

            result &= IndexChain(block, height);
            result &= IndexReputations(block, height);

            return result;
        }

        static bool Rollback(int height)
        {
            auto result = true;

            // TODO (brangr): implement rollback for transactions< ins/outs, and other
            //result &= BlockRepoInst.BulkRollback(height);
            result &= RollbackChain(height);

            return result;
        }

    protected:

        // =============================================================================================================
        // Delete all calculated records for this height
        static bool RollbackChain(int height)
        {
            // TODO (joni): откатиться транзакции и блок в БД
        }

        // =============================================================================================================
        // Set block height for all transactions in block
        static bool IndexChain(const CBlock& block, int height)
        {
            // TODO (brangr): записать транзакции и блок в БД
        }

        // =============================================================================================================


        static bool IndexReputations(const CBlock& block, int height)
        {
            // todo (brangr): index ratings
        }


    private:

    };

} // namespace PocketServices

#endif // POCKETDB_TRANSACTIONINDEXER_HPP
