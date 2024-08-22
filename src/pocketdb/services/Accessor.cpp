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
            {
                if (!PocketHelpers::TransactionHelper::IsPocketSupportedTransaction(tx))
                    continue;

                txs.push_back(tx->GetHash().GetHex());
            }

            if (txs.empty())
                return true;

            pocketBlock = PocketDb::TransRepoInst.List(txs, true);
            return pocketBlock && pocketBlock->size() == txs.size();
        }
        catch (const std::exception& e)
        {
            LogPrintf("Error: Accessor::GetBlock (%s) - %s\n", block.GetHash().GetHex(), e.what());
            return false;
        }
    }

    // Read block data for send via network
    bool Accessor::GetBlock(const CBlock& block, string& data)
    {
        PocketBlockRef pocketBlock;
        if (!GetBlock(block, pocketBlock))
            return false;

        if (!pocketBlock)
            return true;

        auto dataPtr = PocketServices::Serializer::SerializeBlock(*pocketBlock);
        if (dataPtr)
            data = dataPtr->write();

        return true;
    }

    bool Accessor::GetTransaction(const CTransaction& tx, PTransactionRef& pocketTx)
    {
        if (!PocketHelpers::TransactionHelper::IsPocketSupportedTransaction(tx))
            return true;

        try
        {    
            pocketTx = PocketDb::TransRepoInst.Get(tx.GetHash().GetHex(), true);
        }
        catch (const std::exception& e)
        {
            LogPrintf("Error: Accessor::GetTransaction (%s) - %s\n", tx.GetHash().GetHex(), e.what());
            return false;
        }

        return pocketTx != nullptr;
    }

    // Read transaction data for send via network
    bool Accessor::GetTransaction(const CTransaction& tx, string& data)
    {
        PTransactionRef pocketTx;
        if (!GetTransaction(tx, pocketTx))
            return false;

        if (!pocketTx)
            return true;
            
        auto dataPtr = PocketServices::Serializer::SerializeTransaction(*pocketTx);
        if (dataPtr)
            data = dataPtr->write();

        return true;
    }

    bool Accessor::ExistsTransaction(const string& hash)
    {
        return PocketDb::TransRepoInst.Exists(hash);
    }

    bool Accessor::ExistsTransactions(vector<string>& txHashes)
    {
        return PocketDb::TransRepoInst.Exists(txHashes);
    }

} // namespace PocketServices