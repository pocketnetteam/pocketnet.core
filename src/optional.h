// Copyright (c) 2017-2019 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_OPTIONAL_H
#define POCKETCOIN_OPTIONAL_H

// This file existed for providing "optional" functional for c++11 compiler that is used by bitcoin.
// Pocketnet is using c++17 so this functional is not even needed.
// However, these functions are still here to provide more similarity with bitcoin code but adapted to use STL's optional

#include <utility>

#include <optional>

template <typename T>
using Optional = std::optional<T>;

// Here boolean condition is removed because it causes conflicts with STL's make_optional.
// Optional is initialized empty by default or use "= nullopt" to explicitly initialize empty
template <typename T>
Optional<T> MakeOptional(T&& value)
{
    return std::make_optional(std::forward<T>(value));
}

using std::nullopt;

#endif // POCKETCOIN_OPTIONAL_H
