// Copyright (c) 2018-2021 Pocketnet developers
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
        Comment(const string& hash, int64_t time);
        Comment(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr <UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;

        void DeserializeRpc(const UniValue& src, const std::shared_ptr<const CTransaction>& tx) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(string value);

        shared_ptr <string> GetRootTxHash() const;
        void SetRootTxHash(string value);

        shared_ptr <string> GetPostTxHash() const;
        void SetPostTxHash(string value);

        shared_ptr <string> GetParentTxHash() const;
        void SetParentTxHash(string value);

        shared_ptr <string> GetAnswerTxHash() const;
        void SetAnswerTxHash(string value);

        // Payload getters
        shared_ptr <string> GetPayloadMsg() const;
        void SetPayloadMsg(string value);

    protected:
        void DeserializePayload(const UniValue& src, const std::shared_ptr<const CTransaction>& tx) override;
        void BuildHash() override;
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_H
