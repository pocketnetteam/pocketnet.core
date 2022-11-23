// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SUBSCRIBE_H
#define POCKETTX_SUBSCRIBE_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class Subscribe : public Transaction
    {
    public:

        Subscribe();
        Subscribe(const std::shared_ptr<const CTransaction>& tx);

        optional<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        const optional <string>& GetAddress() const;
        void SetAddress(const string& value);

        const optional <string>& GetAddressTo() const;
        void SetAddressTo(const string& value);

        string BuildHash() override;

    };

} // namespace PocketTx

#endif //POCKETTX_SUBSCRIBE_H
