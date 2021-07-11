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

#include "pocketdb/consensus.h"
#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/pocketnet.h"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;

    using std::make_tuple;
    using std::tuple;
    using std::vector;
    using std::find;

    static bool GetBlock(const CBlock& block, string& data)
    {
        try
        {
            std::vector<std::string> txs;
            for (const auto& tx : block.vtx)
            {
                if (PocketHelpers::IsPocketTransaction(tx))
                    txs.push_back(tx->GetHash().GetHex());
            }

            if (txs.empty())
                return true;

            auto pocketBlock = PocketDb::TransRepoInst.GetList(txs, true);
            data = PocketServices::SerializeBlock(pocketBlock)->write();

            return pocketBlock->size() == txs.size();
        }
        catch (const std::exception& e)
        {
            LogPrintf("Error: PocketServices::GetBlock (%s) - %s\n", block.GetHash().GetHex(), e.what());
            return false;
        }
    }

    static bool GetTransaction(const CTransaction& tx, string& data)
    {
        auto ptx = PocketDb::TransRepoInst.GetByHash(tx.GetHash().GetHex(), true);
        if (ptx)
            data = PocketServices::SerializeBlock(*ptx)->write();
        
        return ptx != nullptr;
    }


} // namespace PocketServices

#endif // POCKETDB_TRANSACTIONINDEXER_HPP