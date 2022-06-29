// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTTXDATA_H
#define POCKETDB_SHORTTXDATA_H

#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/shortform/ShortAccount.h"

#include <univalue.h>

#include <string>
#include <optional>


namespace PocketDb
{
    class ShortTxData
    {
    public:
        ShortTxData(std::string hash, PocketTx::TxType txType, std::optional<std::string> address, std::optional<int64_t> height,
                    std::optional<int64_t> blockNum, std::optional<ShortAccount> account, std::optional<int> val,
                    std::optional<std::string> description);
        ShortTxData(std::string hash, PocketTx::TxType txType);

        UniValue Serialize() const;

        const std::string& GetHash() const;
        const std::optional<std::string>& GetAddress() const;
        void SetAddress(const std::optional<std::string>& address);
        PocketTx::TxType GetTxType() const;
        const std::optional<int64_t>& GetHeight() const;
        void SetHeight(const std::optional<int64_t>& height);
        const std::optional<int64_t>& GetBlockNum() const;
        void SetBlockNum(const std::optional<int64_t>& height);
        const std::optional<ShortAccount>& GetAccount() const;
        void SetAccount(const std::optional<ShortAccount>& account);
        const std::optional<int>& GetVal() const;
        void SetVal(const std::optional<int>& val);
        const std::optional<std::string>& GetDescription() const;
        void SetDescription(const std::optional<std::string>& description);
    private:
        std::string m_hash;
        PocketTx::TxType m_txType;
        std::optional<std::string> m_address; // Creator of tx. // This field is optional if we are requesting a lot of txs for one address and want to not duplicate meaningless data
        std::optional<int64_t> m_height; // This field is optional if we are requesting a lot of txs for one height and want to not duplicate meaningless data 
        std::optional<int64_t> m_blockNum; // TODO (losty): probably some filters for these fields 
        std::optional<ShortAccount> m_account; // Account data associated with address
        std::optional<int> m_val;
        std::optional<std::string> m_description; // Short description of content, e.x. first lines of post's text
    };
}

#endif // POCKETDB_SHORTTXDATA_H