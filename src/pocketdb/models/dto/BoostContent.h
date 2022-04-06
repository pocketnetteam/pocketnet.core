// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_BOOSTCONTENT_H
#define SRC_BOOSTCONTENT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class BoostContent : public Transaction
    {
    public:

        BoostContent();
        BoostContent(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(const string& value);

        shared_ptr<string> GetContentTxHash() const;
        void SetContentTxHash(const string& value);

        string BuildHash() override;
    };
}


#endif //SRC_BOOSTCONTENT_H
