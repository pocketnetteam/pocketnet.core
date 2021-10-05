// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/web/Tag.h"

namespace PocketDbWeb
{
    Tag::Tag(int64_t contentId, const string& value)
    {
        ContentId = contentId;
        Value = value;
    }
}