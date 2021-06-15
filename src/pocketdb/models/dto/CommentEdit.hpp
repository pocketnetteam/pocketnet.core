// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETTX_COMMENT_EDIT_HPP
#define POCKETTX_COMMENT_EDIT_HPP

#include "pocketdb/models/dto/Comment.hpp"

namespace PocketTx
{

    class CommentEdit : public Comment
    {
    public:

        CommentEdit(string& hash, int64_t time) : Comment(hash, time)
        {
            SetType(PocketTxType::CONTENT_COMMENT_EDIT);
        }

    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_EDIT_HPP
