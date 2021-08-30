// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/Accessor.h"

namespace PocketServices
{
    bool Accessor::GetBlock(const CBlock& block, PocketBlockRef& pocketBlock, bool onlyPocket)
    {
        try
        {
            std::vector<std::string> txs;
            for (const auto& tx : block.vtx)
            {
                if (!PocketHelpers::TransactionHelper::IsPocketSupportedTransaction(tx))
                    continue;

                if (onlyPocket && !PocketHelpers::TransactionHelper::IsPocketTransaction(tx))
                    continue;

                txs.push_back(tx->GetHash().GetHex());
            }

            if (txs.empty())
                return true;

            pocketBlock = PocketDb::TransRepoInst.GetList(txs, true);
            return pocketBlock->size() == txs.size();
        }
        catch (const std::exception& e)
        {
            LogPrintf("Error: PocketServices::GetBlock (%s) - %s\n", block.GetHash().GetHex(), e.what());
            return false;
        }
    }

    bool Accessor::GetBlock(const CBlock& block, string& data)
    {
        PocketBlockRef pocketBlock;
        if (!GetBlock(block, pocketBlock, true))
            return false;

        if (!pocketBlock)
            return true;

        data = PocketServices::TransactionSerializer::SerializeBlock(*pocketBlock)->write();
        return true;
    }

    bool Accessor::GetTransaction(const CTransaction& tx, PTransactionRef& pocketTx)
    {
        pocketTx = PocketDb::TransRepoInst.GetByHash(tx.GetHash().GetHex(), true);
        return pocketTx != nullptr;
    }

    bool Accessor::GetTransaction(const CTransaction& tx, string& data)
    {
        PTransactionRef pocketTx;
        if (GetTransaction(tx, pocketTx) && pocketTx && PocketHelpers::TransactionHelper::IsPocketTransaction(*pocketTx->GetType()))
            data = PocketServices::TransactionSerializer::SerializeTransaction(*pocketTx)->write();

        return true;
    }

    bool Accessor::ExistsTransaction(const CTransaction& tx)
    {
        return PocketDb::TransRepoInst.ExistsByHash(tx.GetHash().GetHex());
    }

} // namespace PocketServices