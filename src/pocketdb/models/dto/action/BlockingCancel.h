// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCKING_CANCEL_H
#define POCKETTX_BLOCKING_CANCEL_H

#include "pocketdb/models/dto/action/Blocking.h"

namespace PocketTx
{

    class BlockingCancel : public Blocking
    {
    public:
        BlockingCancel();
        BlockingCancel(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr<UniValue> Serialize() const override;
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCKING_CANCEL_H
