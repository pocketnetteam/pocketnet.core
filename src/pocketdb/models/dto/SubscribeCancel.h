// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SUBSCRIBE_CANCEL_H
#define POCKETTX_SUBSCRIBE_CANCEL_H

#include "pocketdb/models/dto/Subscribe.h"

namespace PocketTx
{
    class SubscribeCancel : public Subscribe
    {
    public:
        SubscribeCancel(const string& hash, int64_t time);
        SubscribeCancel(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr<UniValue> Serialize() const override;
    };

} // namespace PocketTx

#endif //POCKETTX_SUBSCRIBE_CANCEL_H
