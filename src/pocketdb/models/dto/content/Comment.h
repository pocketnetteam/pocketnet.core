// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COMMENT_H
#define POCKETTX_COMMENT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class Comment : public Transaction
    {
    public:
        Comment();
        Comment(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr <UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(const string& value);

        shared_ptr <string> GetRootTxHash() const;
        void SetRootTxHash(const string& value);

        shared_ptr <string> GetPostTxHash() const;
        void SetPostTxHash(const string& value);

        shared_ptr <string> GetParentTxHash() const;
        void SetParentTxHash(const string& value);

        shared_ptr <string> GetAnswerTxHash() const;
        void SetAnswerTxHash(const string& value);

        // Payload getters
        shared_ptr <string> GetPayloadMsg() const;
        void SetPayloadMsg(const string& value);

        string BuildHash() override;

    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_H
