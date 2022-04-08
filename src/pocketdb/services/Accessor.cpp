// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/Accessor.h"

namespace PocketServices
{
    bool Accessor::GetBlock(const CBlock& block, PocketBlockRef& pocketBlock)
    {
        try
        {
            std::vector<std::string> txs;
            for (const auto& tx : block.vtx)
                txs.push_back(tx->GetHash().GetHex());

            if (txs.empty())
                return true;

            pocketBlock = PocketDb::TransRepoInst.List(txs, true);
            return pocketBlock && pocketBlock->size() == txs.size();
        }
        catch (const std::exception& e)
        {
            LogPrintf("Error: PocketServices::GetBlock (%s) - %s\n", block.GetHash().GetHex(), e.what());
            return false;
        }
    }

    // Read block data for send via network
    bool Accessor::GetBlock(const CBlock& block, string& data)
    {
        PocketBlockRef pocketBlock;
        if (!GetBlock(block, pocketBlock))
            return false;

        auto dataPtr = PocketServices::Serializer::SerializeBlock(*pocketBlock);
        if (dataPtr)
            data = dataPtr->write();

        return true;
    }

    bool Accessor::GetTransaction(const CTransaction& tx, PTransactionRef& pocketTx)
    {
        pocketTx = PocketDb::TransRepoInst.Get(tx.GetHash().GetHex(), true);
        return pocketTx != nullptr;
    }

    // Read transaction data for send via network
    bool Accessor::GetTransaction(const CTransaction& tx, string& data)
    {
        PTransactionRef pocketTx;
        if (!GetTransaction(tx, pocketTx) || !pocketTx)
            return false;
            
        auto dataPtr = PocketServices::Serializer::SerializeTransaction(*pocketTx);
        if (dataPtr)
            data = dataPtr->write();

        return true;
    }

} // namespace PocketServices