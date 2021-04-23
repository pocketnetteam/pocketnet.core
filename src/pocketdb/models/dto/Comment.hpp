// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETTX_COMMENT_HPP
#define POCKETTX_COMMENT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Comment : public Transaction
    {
    public:
        ~Comment() = default;

        Comment(const UniValue &src) : Transaction(src)
        {
            SetTxType(PocketTxType::COMMENT_CONTENT);

            if (src.exists("lang"))
                SetLang(src["lang"].get_str());

            if (src.exists("otxid"))
                SetRootTxId(src["otxid"].get_str());

            if (src.exists("postid"))
                SetPostTxId(src["postid"].get_str());

            if (src.exists("parentid"))
                SetParentTxId(src["parentid"].get_str());

            if (src.exists("answerid"))
                SetAnswerTxId(src["answerid"].get_str());

            // TODO (brangr): set payload
        }
        

        [[nodiscard]] std::string* GetLang() const { return m_string1; }
        void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

        [[nodiscard]] std::string* GetRootTxId() const { return m_string2; }
        void SetRootTxId(std::string value) { m_string2 = new std::string(std::move(value)); }

        [[nodiscard]] std::string* GetPostTxId() const { return m_string3; }
        void SetPostTxId(std::string value) { m_string3 = new std::string(std::move(value)); }

        [[nodiscard]] std::string* GetParentTxId() const { return m_string4; }
        void SetParentTxId(std::string value) { m_string4 = new std::string(std::move(value)); }

        [[nodiscard]] std::string* GetAnswerTxId() const { return m_string5; }
        void SetAnswerTxId(std::string value) { m_string5 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_HPP
