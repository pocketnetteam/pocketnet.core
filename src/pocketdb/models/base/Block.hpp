// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCK_H
#define POCKETTX_BLOCK_H

#include "pocketdb/models/base/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include <uint256.h>
#include <utility>

namespace PocketTx
{

    class Block : public Base
    {
    public:
        Block() = default;

        void Add(shared_ptr <Transaction> tx)
        {

        }

        shared_ptr <Transaction> Get(uint256 txId) const
        {
            // TODO (brangr):
        }
    protected:
    private:
        std::map<uint256, shared_ptr < Transaction>> vtx;
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCK_H