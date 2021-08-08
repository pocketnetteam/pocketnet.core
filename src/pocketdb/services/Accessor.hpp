// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDSERVICESTACCESSORHPP
#define POCKETDSERVICESTACCESSORHPP

#include "primitives/transaction.h"
#include "primitives/block.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/services/TransactionSerializer.hpp"
#include "pocketdb/helpers/TransactionHelper.hpp"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;

    using std::make_tuple;
    using std::tuple;
    using std::vector;
    using std::find;


    static bool GetBlock(const CBlock& block, PocketBlockRef& pocketBlock, bool onlyPocket = false)
    {
        try
        {
            std::vector<std::string> txs;
            for (const auto& tx : block.vtx)
            {
                if (!IsPocketSupportedTransaction(tx))
                    continue;

                if (onlyPocket && !IsPocketTransaction(tx))
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

    static bool GetBlock(const CBlock& block, string& data)
    {
        PocketBlockRef pocketBlock;
        if (!GetBlock(block, pocketBlock, true))
            return false;

        if (!pocketBlock)
            return true;

        data = PocketServices::TransactionSerializer::SerializeBlock(*pocketBlock)->write();
        return true;
    }

    static bool GetTransaction(const CTransaction& tx, PTransactionRef& pocketTx)
    {
        pocketTx = PocketDb::TransRepoInst.GetByHash(tx.GetHash().GetHex(), true);
        return pocketTx != nullptr;
    }

    static bool ExistsTransaction(const CTransaction& tx)
    {
        return PocketDb::TransRepoInst.ExistsByHash(tx.GetHash().GetHex());
    }

    static bool GetTransaction(const CTransaction& tx, string& data)
    {
        PTransactionRef pocketTx;
        if (GetTransaction(tx, pocketTx) && pocketTx)
        {
            data = PocketServices::TransactionSerializer::SerializeTransaction(*pocketTx)->write();
            return true;
        }

        return false;
    }


} // namespace PocketServices

#endif // POCKETSERVICES_ACCESSOR_HPP