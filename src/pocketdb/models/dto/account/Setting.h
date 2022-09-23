// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_ACCOUNT_SETTING_H
#define POCKETTX_ACCOUNT_SETTING_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class AccountSetting : public Transaction
    {
    public:
        AccountSetting();
        AccountSetting(const CTransactionRef& tx);

        optional<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        const optional <string>& GetAddress() const;
        void SetAddress(const string& value);

        optional <string> GetPayloadData() const;

        string BuildHash() override;

    }; // class AccountSetting

} // namespace PocketTx

#endif // POCKETTX_ACCOUNT_SETTING_H
