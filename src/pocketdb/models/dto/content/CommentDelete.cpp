// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/CommentDelete.h"

namespace PocketTx
{
    CommentDelete::CommentDelete() : Comment()
    {
        SetType(TxType::CONTENT_COMMENT_DELETE);
    }

    CommentDelete::CommentDelete(const std::shared_ptr<const CTransaction>& tx) : Comment(tx)
    {
        SetType(TxType::CONTENT_COMMENT_DELETE);
    }

    void CommentDelete::DeserializeRpc(const UniValue& src)
    {
        Comment::DeserializeRpc(src);
        ClearPayload();
    }

    void CommentDelete::DeserializePayload(const UniValue& src)
    {
    }

    string CommentDelete::BuildHash()
    {
        std::string data;

        data += GetPostTxHash() ? *GetPostTxHash() : "";
        data += ""; // Skip Message for deleted comment
        data += GetParentTxHash() ? *GetParentTxHash() : "";
        data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx