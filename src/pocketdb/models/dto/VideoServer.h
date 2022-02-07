// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_VIDEO_SERVER_H
#define POCKETTX_VIDEO_SERVER_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class VideoServer : public Transaction
    {
    public:
        VideoServer();
        VideoServer(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr <UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(const string& value) override;

        // Payload getters
        shared_ptr <string> GetPayloadName() const;
        shared_ptr <string> GetPayloadIP() const;
        shared_ptr <string> GetPayloadDomain() const;
        shared_ptr <string> GetPayloadAbout() const;

        string BuildHash() override;
        string PreBuildHash();

    }; // class VideoServer

} // namespace PocketTx

#endif // POCKETTX_VIDEO_SERVER_H
