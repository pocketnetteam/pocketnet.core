// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef SRC_SCOREPOST_HPP
#define SRC_SCOREPOST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScorePost : public Transaction
    {
    public:
        ~ScorePost() = default;

        ScorePost(const UniValue &src) : Transaction(src)
        {
            SetTxType(PocketTxType::SCORE_POST_ACTION);

            assert(src.exists("value") && src["value"].isNum());
            SetValue(src["value"].get_int());

            assert(src.exists("posttxid") && src["posttxid"].isStr());
            SetPostTxId(src["posttxid"].get_str());
        }
        
        [[nodiscard]] int64_t *GetValue() const { return m_int1; }
        void SetValue(int64_t value) { m_int1 = new int64_t(value); }

        [[nodiscard]] std::string* GetPostTxId() const { return m_string1; }
        void SetPostTxId(std::string value) { m_string1 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif //SRC_SCOREPOST_HPP
