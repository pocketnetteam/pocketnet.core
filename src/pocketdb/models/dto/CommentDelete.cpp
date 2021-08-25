// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/CommentDelete.h"

namespace PocketTx
{
    CommentDelete::CommentDelete(const string& hash, int64_t time) : Comment(hash, time)
    {
        SetType(PocketTxType::CONTENT_COMMENT_DELETE);
    }

    void CommentDelete::DeserializeRpc(const UniValue& src)
    {
        Comment::DeserializeRpc(src);
        ClearPayload();
    }

    void CommentDelete::DeserializePayload(const UniValue& src)
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