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
        AccountSetting();
        AccountSetting(const CTransactionRef& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src, const CTransactionRef& tx) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(const string& value);

        shared_ptr <string> GetPayloadData() const;

    protected:
        void DeserializePayload(const UniValue& src, const CTransactionRef& tx) override;
        void BuildHash() override;
    }; // class AccountSetting

} // namespace PocketTx

#endif // POCKETTX_ACCOUNT_SETTING_H
