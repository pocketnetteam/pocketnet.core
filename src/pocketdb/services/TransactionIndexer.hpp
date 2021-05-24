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
#include "pocketdb/helpers/TypesHelper.hpp"
#include "pocketdb/helpers/TransactionHelper.hpp"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;

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
            result &= IndexRatings(block, height);

            return result;
        }

        static bool Rollback(int height)
        {
            auto result = true;
            result &= RollbackChain(height);
            return result;
        }

    protected:

        // Delete all calculated records for this height
        static bool RollbackChain(int height)
        {
            return PocketDb::ChainRepoInst.RollbackBlock(height);
        }

        // Set block height for all transactions in block
        static bool IndexChain(const CBlock& block, int height)
        {
            // transaction with all inputs
            vector<TransactionIndexingInfo> txs;
            for (const auto& tx : block.vtx)
            {
                TransactionIndexingInfo txInfo;
                txInfo.Hash = tx->GetHash().GetHex();
                txInfo.Type = ParseType(tx);

                if (!tx->IsCoinBase())
                {
                    for (const auto& inp : tx->vin)
                        txInfo.Inputs.emplace(inp.prevout.hash.GetHex(), inp.prevout.n);
                }
                
                txs.push_back(txInfo);
            }

            return PocketDb::ChainRepoInst.UpdateHeight(block.GetHash().GetHex(), height, txs);
        }

        static bool IndexRatings(const CBlock& block, int height)
        {
            // todo (brangr): implement
            return true;
        }


    private:

    };

} // namespace PocketServices

#endif // POCKETDB_TRANSACTIONINDEXER_HPP
