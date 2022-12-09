// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_DBVIEWHELPER_H
#define POCKETHELPERS_DBVIEWHELPER_H

#include "pocketdb/helpers/TransactionHelper.h"
#include <cstdint>
#include <string>

namespace PocketHelpers
{
    struct TxData
    {
        optional<int64_t> int1;
        optional<int64_t> int2;
        optional<int64_t> int3;
        optional<int64_t> int4;
        optional<int64_t> int5;
        optional<int64_t> int6;
        optional<string> string1;
    };

    class ITxDbDataTranslator
    {
    public:
        virtual bool ToModel(PocketHelpers::PTransactionRef& tx, const TxData& data) = 0;
        virtual bool FromModel(TxData& data, const PocketHelpers::PTransactionRef& tx) = 0;
    };

    class DbViewHelper
    {
    public:
        static bool DbViewToModel(PTransactionRef& tx, const TxData& data);
        static bool ModelToDbView(TxData& data, const PTransactionRef& tx);
    private:
        static const map<TxType, shared_ptr<ITxDbDataTranslator>> m_translatorSelector;
    };
}

#endif // POCKETHELPERS_DBVIEWHELPER_H