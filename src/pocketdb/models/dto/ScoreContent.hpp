// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef SRC_SCORECONTENT_HPP
#define SRC_SCORECONTENT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScoreContent : public Transaction
    {
    public:

        ScoreContent(string& hash, int64_t time, string& opReturn) : Transaction(hash, time, opReturn)
        {
            SetType(PocketTxType::ACTION_SCORE_CONTENT);
        }

        shared_ptr<UniValue> Serialize() const override
        {
            auto result = Transaction::Serialize();

            if (GetAddress()) result->pushKV("address", *GetAddress());
            if (GetContentTxHash()) result->pushKV("posttxid", *GetContentTxHash());
            if (GetValue()) result->pushKV("value", *GetValue());

            return result;
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) SetContentTxHash(val);
            if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
        }

        shared_ptr <string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetContentTxHash() const { return m_string2; }
        void SetContentTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr<int64_t> GetValue() const { return m_int1; }
        void SetValue(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    protected:

        void DeserializePayload(const UniValue& src) override
        {
        }

        void BuildHash() override
        {
            std::string data;
            data += GetContentTxHash() ? *GetContentTxHash() : "";
            data += GetValue() ? std::to_string(*GetValue()) : "";
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //SRC_SCORECONTENT_HPP
