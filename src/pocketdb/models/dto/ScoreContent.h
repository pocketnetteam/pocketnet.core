// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_SCORECONTENT_H
#define SRC_SCORECONTENT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class ScoreContent : public Transaction
    {
    public:

        ScoreContent(const string& hash, int64_t time);
        ScoreContent(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        
        void DeserializeRpc(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(string value);

        shared_ptr<string> GetContentTxHash() const;
        void SetContentTxHash(string value);

        shared_ptr<int64_t> GetValue() const;
        void SetValue(int64_t value);

    protected:
        void DeserializePayload(const UniValue& src) override;
        void BuildHash() override;
    };

} // namespace PocketTx

#endif //SRC_SCORECONTENT_H
