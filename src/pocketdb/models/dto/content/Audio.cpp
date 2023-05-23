// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Audio.h"

namespace PocketTx
{
    Audio::Audio() : Post()
    {
        SetType(TxType::CONTENT_AUDIO);
    }

    Audio::Audio(const CTransactionRef& tx) : Post(tx)
    {
        SetType(TxType::CONTENT_AUDIO);
    }

} // namespace PocketTx