// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SCORECOMMENT_H
#define POCKETTX_SCORECOMMENT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class ScoreComment : public Transaction
    {
    public:

        ScoreComment();
        ScoreComment(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        
        void DeserializeRpc(const UniValue& src, const std::shared_ptr<const CTransaction>& tx) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(string value);

        shared_ptr <string> GetCommentTxHash() const;
        void SetCommentTxHash(string value);

        shared_ptr <int64_t> GetValue() const;
        void SetValue(int64_t value);

        string BuildHash() override;

    protected:
        void DeserializePayload(const UniValue& src, const std::shared_ptr<const CTransaction>& tx) override;
    };

} // namespace PocketTx

#endif // POCKETTX_SCORECOMMENT_H
