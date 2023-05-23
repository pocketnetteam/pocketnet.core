// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Stream.h"

namespace PocketTx
{
    Stream::Stream() : Post()
    {
        SetType(TxType::CONTENT_STREAM);
    }

    Stream::Stream(const CTransactionRef& tx) : Post(tx)
    {
        SetType(TxType::CONTENT_STREAM);
    }

    size_t Stream::PayloadSize() const
    {
        size_t dataSize =
            (GetPayloadUrl() ? GetPayloadUrl()->size() : 0) +
            (GetPayloadCaption() ? GetPayloadCaption()->size() : 0) +
            (GetPayloadMessage() ? GetPayloadMessage()->size() : 0) +
            (GetRelayTxHash() ? GetRelayTxHash()->size() : 0) +
            (GetPayloadSettings() ? GetPayloadSettings()->size() : 0) +
            (GetPayloadLang() ? GetPayloadLang()->size() : 0);

        if (GetRootTxHash() && *GetRootTxHash() != *GetHash())
            dataSize += GetRootTxHash()->size();

        if (GetPayloadTags() && !(*GetPayloadTags()).empty())
        {
            UniValue tags(UniValue::VARR);
            tags.read(*GetPayloadTags());
            for (size_t i = 0; i < tags.size(); ++i)
                dataSize += tags[i].get_str().size();
        }

        if (GetPayloadImages() && !(*GetPayloadImages()).empty())
        {
            UniValue images(UniValue::VARR);
            images.read(*GetPayloadImages());
            for (size_t i = 0; i < images.size(); ++i)
                dataSize += images[i].get_str().size();
        }

        return dataSize;
    }

} // namespace PocketTx