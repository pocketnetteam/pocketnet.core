// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONINDEXER_HPP
#define POCKETDB_TRANSACTIONINDEXER_HPP

#include "util.h"
#include "script/standard.h"
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
            result &= IndexTransactions(block, height);
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
        // Indexing outputs and inputs for transaction
        // New inputs always spent prev outs
        // Tables TxOutputs TxInputs
        static bool IndexTransactions(const CBlock& block, int height)
        {
            vector<TransactionInput> inputs;
            vector<TransactionOutput> outputs;
            vector<string> addresses;

            // Prepare data
            for (const auto& tx : block.vtx)
            {
                // indexing Outputs
                for (int i = 0; i < tx->vout.size(); i++)
                {
                    const CTxOut& txout = tx->vout[i];

                    txnouttype type;
                    std::vector<CTxDestination> vDest;
                    int nRequired;
                    if (ExtractDestinations(txout.scriptPubKey, type, vDest, nRequired))
                    {
                        TransactionOutput out;
                        out.SetTxHash(tx->GetHash().GetHex());
                        out.SetNumber(i);
                        out.SetValue(txout.nValue);

                        for (const auto& dest : vDest)
                        {
                            auto address = EncodeDestination(dest);
                            out.AddDestination(address);

                            if (std::find(std::begin(addresses), std::end(addresses), address) == std::end(addresses))
                                addresses.push_back(address);
                        }

                        outputs.push_back(out);
                    }
                }

                // Indexing inputs
                if (!tx->IsCoinBase())
                {
                    for (const auto& txin : tx->vin)
                    {
                        TransactionInput inp;

                        inp.SetTxHash(tx->GetHash().GetHex());
                        inp.SetInputTxHash(txin.prevout.hash.GetHex());
                        inp.SetInputTxNumber(txin.prevout.n);

                        inputs.push_back(inp);
                    }
                }
            }

            // Save in db
            return TransRepoInst.InsertTransactionOutputAddresses(addresses) &&
                   TransRepoInst.InsertTransactionOutputs(outputs) &&
                   TransRepoInst.InsertTransactionInputs(inputs);
        }

        static bool IndexReputations(const CBlock& block, int height)
        {
            // todo (brangr): index ratings
        }


    private:

    };

} // namespace PocketServices

#endif // POCKETDB_TRANSACTIONINDEXER_HPP
