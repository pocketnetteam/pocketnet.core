// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETSERVICES_ACCESSOR_H
#define POCKETSERVICES_ACCESSOR_H

#include "primitives/transaction.h"
#include "primitives/block.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/services/Serializer.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;

    using std::make_tuple;
    using std::tuple;
    using std::vector;
    using std::find;

    class Accessor
    {
    public:
        static bool GetBlock(const CBlock& block, PocketBlockRef& pocketBlock);
        static bool GetBlock(const CBlock& block, string& data);
        static bool GetTransaction(const CTransaction& tx, PTransactionRef& pocketTx);
        static bool GetTransaction(const CTransaction& tx, string& data);
    };
} // namespace PocketServices

#endif // POCKETSERVICES_ACCESSOR_H