// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/web/Content.h"

namespace PocketDbWeb
{
    Content::Content(int64_t contentId, ContentFieldType fieldType, string value)
    {
        ContentId = contentId;
        FieldType = fieldType;
        Value = value;
    }
}