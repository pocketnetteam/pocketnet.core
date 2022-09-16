// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COMMENT_DELETE_H
#define POCKETTX_COMMENT_DELETE_H

#include "pocketdb/models/dto/content/Comment.h"

namespace PocketTx
{
    class CommentDelete : public Comment
    {
    public:
        CommentDelete();
        CommentDelete(const std::shared_ptr<const CTransaction>& tx);

        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        string BuildHash() override;

    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_DELETE_H
