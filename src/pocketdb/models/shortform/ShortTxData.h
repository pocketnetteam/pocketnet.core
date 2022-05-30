// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTTXDATA_H
#define POCKETDB_SHORTTXDATA_H

#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/shortform/ShortAccount.h"
#include "pocketdb/models/shortform/ShortTxType.h"

#include <univalue.h>

#include <string>
#include <optional>


namespace PocketDb
{
    class ShortTxData
    {
    public:
        ShortTxData(std::string hash, PocketDb::ShortTxType type, std::string address,
                    std::optional<ShortAccount> account, std::optional<int> val,
                    std::optional<std::string> description);
        UniValue Serialize() const;
    private:
        std::string m_hash;
        PocketDb::ShortTxType m_type;
        std::string m_address; // Creator of tx.
        std::optional<ShortAccount> m_account; // Account data associated with address
        std::optional<int> m_val;
        std::optional<std::string> m_description; // Short description of content, e.x. first lines of post's text
    };
}

#endif // POCKETDB_SHORTTXDATA_H