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

        optional <UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        const optional <string>& GetAddress() const;
        void SetAddress(const string& value);

        const optional <string>& GetReferrerAddress() const;
        void SetReferrerAddress(const string& value);

        // Payload getters
        optional <string> GetPayloadName() const;
        optional <string> GetPayloadAvatar() const;
        optional <string> GetPayloadUrl() const;
        optional <string> GetPayloadLang() const;
        optional <string> GetPayloadAbout() const;
        optional <string> GetPayloadDonations() const;
        optional <string> GetPayloadPubkey() const;

        string BuildHash() override;
        string BuildHash(bool includeReferrer);
        string PreBuildHash();

        size_t PayloadSize() override;

    }; // class User

} // namespace PocketTx

#endif // POCKETTX_USER_H
