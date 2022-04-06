// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_USER_H
#define POCKETTX_USER_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class User : public Transaction
    {
    public:
        User();
        User(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr <UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(const string& value);

        shared_ptr <string> GetReferrerAddress() const;
        void SetReferrerAddress(const string& value);

        // Payload getters
        shared_ptr <string> GetPayloadName() const;
        shared_ptr <string> GetPayloadAvatar() const;
        shared_ptr <string> GetPayloadUrl() const;
        shared_ptr <string> GetPayloadLang() const;
        shared_ptr <string> GetPayloadAbout() const;
        shared_ptr <string> GetPayloadDonations() const;
        shared_ptr <string> GetPayloadPubkey() const;

        string BuildHash() override;
        string BuildHash(bool includeReferrer);
        string PreBuildHash();

    }; // class User

} // namespace PocketTx

#endif // POCKETTX_USER_H
