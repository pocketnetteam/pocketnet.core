// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_TYPESHELPER_HPP
#define POCKETHELPERS_TYPESHELPER_HPP

#include <string>
#include <key_io.h>
#include "primitives/transaction.h"

#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/PocketTypes.hpp"

namespace PocketHelpers
{
    using std::tuple;
    using std::string;
    using std::vector;
    using std::shared_ptr;

    using namespace PocketTx;

    // ================================================================================================================
    //
    //   Extended types for inner logic
    //
    // ================================================================================================================

    // Accumulate transactions in block
    typedef vector<shared_ptr<Transaction>> PocketBlock;

    // Transaction info for indexing spents and other
    struct TransactionIndexingInfo
    {
        string Hash;
        PocketTxType Type;
        map<string, int> Inputs;
    };

    // Scores data: address from, address to, type and value
    struct ScoreData
    {
        PocketTxType Type;
        string Hash;
        string From;
        string To;
        int Value;
    };
}

#endif // POCKETHELPERS_TYPESHELPER_HPP
