// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SUBSCRIBE_PRIVATE_H
#define POCKETTX_SUBSCRIBE_PRIVATE_H

#include "pocketdb/models/dto/action/Subscribe.h"

namespace PocketTx
{
    class SubscribePrivate : public Subscribe
    {
    public:
        SubscribePrivate();
        SubscribePrivate(const std::shared_ptr<const CTransaction>& tx);
        optional<UniValue> Serialize() const override;
    };

} // namespace PocketTx

#endif //POCKETTX_SUBSCRIBE_PRIVATE_H
