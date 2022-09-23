// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COMPLAIN_H
#define POCKETTX_COMPLAIN_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class Complain : public Transaction
    {
    public:

        Complain();
        Complain(const std::shared_ptr<const CTransaction>& tx);

        optional<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        const optional<string>& GetAddress() const;
        void SetAddress(const string& value);

        const optional<string>& GetPostTxHash() const;
        void SetPostTxHash(const string& value);

        const optional <int64_t>& GetReason() const;
        void SetReason(int64_t value);

        string BuildHash() override;

    };

} // namespace PocketTx

#endif // POCKETTX_COMPLAIN_H
