// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCK_HPP
#define POCKETTX_BLOCK_HPP

#include <string>
#include <univalue.h>
#include <utility>
#include <utilstrencodings.h>
#include <crypto/sha256.h>
#include "primitives/block.h"

namespace PocketTx
{
    class Block : public CBlock
    {
    public:
        Block() = default;

    protected:

    private:

    };

} // namespace PocketTx

#endif // POCKETTX_BLOCK_HPP