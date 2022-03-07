// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_DEFAULT_H
#define POCKETTX_DEFAULT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class Default : public Transaction
    {
    public:
        Default();
        Default(const std::shared_ptr<const CTransaction>& tx);

        void Deserialize(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        string BuildHash() override;

    };

} // namespace PocketTx

#endif // POCKETTX_DEFAULT_H
