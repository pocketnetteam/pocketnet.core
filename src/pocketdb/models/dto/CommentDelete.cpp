// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/CommentDelete.h"

namespace PocketTx
{
    CommentDelete::CommentDelete() : Comment()
    {
        SetType(PocketTxType::CONTENT_COMMENT_DELETE);
    }

    CommentDelete::CommentDelete(const std::shared_ptr<const CTransaction>& tx) : Comment(tx)
    {
        SetType(PocketTxType::CONTENT_COMMENT_DELETE);
    }

    void CommentDelete::DeserializeRpc(const UniValue& src, const std::shared_ptr<const CTransaction>& tx)
    {
        Comment::DeserializeRpc(src, tx);
        ClearPayload();
    }

    void CommentDelete::DeserializePayload(const UniValue& src, const std::shared_ptr<const CTransaction>& tx)
    {

    }

    void CommentDelete::BuildHash()
    {
        std::string data;

        data += GetPostTxHash() ? *GetPostTxHash() : "";
        data += ""; // Skip Message for deleted comment
        data += GetParentTxHash() ? *GetParentTxHash() : "";
        data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

        Transaction::GenerateHash(data);
    }
} // namespace PocketTx