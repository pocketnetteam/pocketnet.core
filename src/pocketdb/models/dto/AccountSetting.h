// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_ACCOUNT_SETTING_H
#define POCKETTX_ACCOUNT_SETTING_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class AccountSetting : public PocketTx::Transaction
    {
    public:

        AccountSetting(const string& hash, int64_t time);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(string value);

    protected:
        void DeserializePayload(const UniValue& src) override;
        void BuildHash() override;
    }; // class AccountSetting

} // namespace PocketTx

#endif // POCKETTX_ACCOUNT_SETTING_H
