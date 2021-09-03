// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COMMENT_DELETE_H
#define POCKETTX_COMMENT_DELETE_H

#include "pocketdb/models/dto/Comment.h"

namespace PocketTx
{
    class CommentDelete : public Comment
    {
    public:
        CommentDelete(const string& hash, int64_t time);
        CommentDelete(const std::shared_ptr<const CTransaction>& tx);

        void DeserializeRpc(const UniValue& src) override;
    protected:
        void DeserializePayload(const UniValue& src) override;
        void BuildHash() override;
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_DELETE_H
