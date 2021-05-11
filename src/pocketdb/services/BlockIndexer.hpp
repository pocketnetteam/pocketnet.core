// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BLOCKINDEXER_HPP
#define POCKETDB_BLOCKINDEXER_HPP

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

    class BlockIndexer
    {
    public:
        static bool Index(const CBlock &block, int height)
        {
            auto result = true;
// TODO (brangr): implement
//            vector<shared_ptr<Utxo>> utxoNew;
//            vector<shared_ptr<Utxo>> utxoSpent;

            // Build all models for sql inserts
            for (const auto &tx : block.vtx)
            {
//                IndexOuts(tx, height, utxoNew, utxoSpent);
                // проставить транзакциям высоту и оут
                // расчитали рейтинги
            }

//            result &= TransRepoInst.BulkInsert(utxoNew);
//            result &= TransRepoInst.BulkSpent(utxoSpent);

            // For explorer database is optional
            if (gArgs.GetBoolArg("-explorer", false))
                result &= IndexChain(block, height);

            return result;
        }

        static bool Rollback(int height)
        {
            auto result = BlockRepoInst.BulkRollback(height);

            if (gArgs.GetBoolArg("-explorer", false))
                result &= RollbackChain(height);

            return result;
        }

    protected:

        static bool IndexChain(const CBlock &block, int height)
        {
            // TODO (joni): записать транзакции и блок в БД
        }

        static bool RollbackChain(int height) {
            // TODO (joni): откатиться транзакции и блок в БД
        }

//        static void IndexOuts(const CTransactionRef &tx, int height,
//            vector<shared_ptr<Utxo>> &vUtxoNew,
//            vector<shared_ptr<Utxo>> &utxoSpent)
//        {
//            // New utxos
//            for (int i = 0; i < tx->vout.size(); i++)
//            {
//                const CTxOut &txout = tx->vout[i];
//                string txOutAddress;
//
//                if (TryGetOutAddress(txout, txOutAddress))
//                {
//                    Utxo utxo;
//                    utxo.SetTxId(tx->GetHash().GetHex());
//                    utxo.SetBlock(height);
//                    utxo.SetTxOut(i);
//                    utxo.SetTxTime(tx->nTime);
//                    utxo.SetAddress(txOutAddress);
//                    utxo.SetAmount(txout.nValue);
//
//                    vUtxoNew.push_back(make_shared<Utxo>(utxo));
//                }
//            }
//
//            // Spent exists utxo
//            if (!tx->IsCoinBase())
//            {
//                for (const auto &txin : tx->vin)
//                {
//                    Utxo utxo;
//                    utxo.SetTxId(txin.prevout.hash.GetHex());
//                    utxo.SetTxOut(txin.prevout.n);
//                    utxo.SetBlockSpent(height);
//
//                    utxoSpent.push_back(make_shared<Utxo>(utxo));
//                }
//            }
//        }

        // todo (brangr): index ratings

    private:
        static bool TryGetOutAddress(const CTxOut &txout, std::string &address)
        {
            CTxDestination destAddress;
            bool fValidAddress = ExtractDestination(txout.scriptPubKey, destAddress);
            if (fValidAddress)
                address = EncodeDestination(destAddress);

            return fValidAddress;
        }
    };

} // namespace PocketServices

#endif // POCKETDB_BLOCKINDEXER_HPP
