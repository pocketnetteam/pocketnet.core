// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTTXOUTPUT_H
#define POCKETDB_SHORTTXOUTPUT_H

#include "univalue.h"

#include <vector>
#include <optional>
#include <string>

namespace PocketDb
{
    class ShortTxOutput
    {
    public:
        UniValue Serialize() const;

        void SetTxHash(const std::optional<std::string>& txHash);
        const std::optional<std::string>& GetTxHash() const;
        void SetSpentTxHash(const std::optional<std::string>& spentTxHash);
        const std::optional<std::string>& GetSpentTxHash() const;
        void SetNumber(const std::optional<int>& number);
        const std::optional<int>& GetNumber() const;
        void SetAddressHash(const std::optional<std::string>& addressHash);
        const std::optional<std::string>& GetAddressHash() const;
        void SetScriptPubKey(const std::optional<std::string>& scriptPubKey);
        const std::optional<std::string>& GetcriptPubKey() const;
        void SetValue(const std::optional<int64_t>& value);
        const std::optional<int64_t>& GetValue() const;

    private:
        std::optional<std::string> m_txHash;
        std::optional<std::string> m_spentTxHash;
        std::optional<int> m_number;
        std::optional<std::string> m_addressHash;
        std::optional<std::string> m_scriptPubKey;
        std::optional<int64_t> m_value;
    };
}

#endif // POCKETDB_SHORTTXOUTPUT_H