// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONINDEXER_HPP
#define POCKETTX_TRANSACTIONINDEXER_HPP

#include "chain.h"
#include "primitives/block.h"
#include "pocketdb/pocketnet.h"

namespace PocketServices
{
    //using namespace PocketTx;
    //using namespace PocketDb;

    class BlockIndexer
    {
    public:
        static void Index(const CBlock &block, CBlockIndex *pindex)
        {
            // for block.vtx
            // проставить транзакциям высоту и оут
            // расчитали рейтинги
            // проставить утхо чреез BlockRepository
            //PocketDb::
            //BlockRepository.UtxoSpent(height, vector<tuple<unit256, int>> txs)
        }

    protected:
    private:
    };

} // namespace PocketServices

#endif //POCKETTX_TRANSACTIONINDEXER_HPP
