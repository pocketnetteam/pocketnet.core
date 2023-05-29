// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/util/Empty.h"

bool PocketUtil::IsEmpty(const std::optional<std::string>& ptr)
{
    return !ptr || (*ptr).empty();
}

bool PocketUtil::IsEmpty(const std::optional<int>& ptr)
{
    return !ptr;
}

bool PocketUtil::IsEmpty(const std::optional<int64_t>& ptr)
{
    return !ptr;
}

