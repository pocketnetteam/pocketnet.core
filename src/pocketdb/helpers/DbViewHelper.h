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
    struct TxContextualData
    {
        optional<string> string1;
        optional<string> string2;
        optional<string> string3;
        optional<string> string4;
        optional<string> string5;
        optional<int> int1;
        optional<string> list;
    };

    class ITxDbDataTranslator
    {
    public:
        virtual bool Inject(PocketHelpers::PTransactionRef& tx, const TxContextualData& data) = 0;
        virtual bool Extract(TxContextualData& data, const PocketHelpers::PTransactionRef& tx) = 0;
        virtual ~ITxDbDataTranslator() = default;
    };

    class DbViewHelper
    {
    public:
        // TODO (optimization): rename below methods
        static bool Inject(PTransactionRef& tx, const TxContextualData& data);
        static bool Extract(TxContextualData& data, const PTransactionRef& tx);
    private:
        static const shared_ptr<ITxDbDataTranslator> m_defaultDataTranslator;
        static const map<TxType, shared_ptr<ITxDbDataTranslator>> m_translatorSelector;
    };
}

#endif // POCKETHELPERS_DBVIEWHELPER_H