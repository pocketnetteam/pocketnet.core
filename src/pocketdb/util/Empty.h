// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_UTIL_EMPTY_H
#define POCKETDB_UTIL_EMPTY_H

#include <optional>
#include <string>
#include <cstdint>

namespace PocketUtil
{
    // Check empty pointer
    bool IsEmpty(const std::optional<std::string>& ptr);

    bool IsEmpty(const std::optional<int>& ptr);

    bool IsEmpty(const std::optional<int64_t>& ptr);
}

#endif // POCKETDB_UTIL_EMPTY_H
