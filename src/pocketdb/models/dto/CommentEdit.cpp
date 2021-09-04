// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/CommentEdit.h"

namespace PocketTx
{
    CommentEdit::CommentEdit() : Comment()
    {
        SetType(PocketTxType::CONTENT_COMMENT_EDIT);
    }

    CommentEdit::CommentEdit(const std::shared_ptr<const CTransaction>& tx) : Comment(tx)
    {
        SetType(PocketTxType::CONTENT_COMMENT_EDIT);
    }
} // namespace PocketTx