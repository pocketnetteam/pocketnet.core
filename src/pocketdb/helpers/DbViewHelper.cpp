// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/DbViewHelper.h"

#include "pocketdb/pocketnet.h"


class CommonDataTranslator : public PocketHelpers::ITxDbDataTranslator
{
public:
    bool Inject(PocketHelpers::PTransactionRef& tx, const PocketHelpers::TxContextualData& data) override
    {
        if (data.string1) tx->SetString1(*data.string1);
        if (data.string2) tx->SetString2(*data.string2);
        if (data.string3) tx->SetString3(*data.string3);
        if (data.string4) tx->SetString4(*data.string4);
        if (data.string5) tx->SetString5(*data.string5);
        if (data.int1) tx->SetInt1(*data.int1);

        return true;
    }

    bool Extract(PocketHelpers::TxContextualData& data, const PocketHelpers::PTransactionRef& tx) override
    {
        data.string1 = tx->GetString1();
        data.string2 = tx->GetString2();
        data.string3 = tx->GetString3();
        data.string4 = tx->GetString4();
        data.string5 = tx->GetString5();
        data.int1 = tx->GetInt1();

        return true;
    }
};

class BlockingDataTranslator : public PocketHelpers::ITxDbDataTranslator
{
public:
    // TODO (losty-db): using PocketDb::TransRepoInst here is a dirty hack to avoid header cycle dependency
    // because TransactionRepository requires to use this helper class. Consider extract required methods to
    // some kind of database accessor.
    bool Inject(PocketHelpers::PTransactionRef& tx, const PocketHelpers::TxContextualData& data) override
    {
        if (data.string1) tx->SetString1(*data.string1);
        if (data.string2) tx->SetString1(*data.string2);
        if (data.string4) tx->SetString1(*data.string4);
        if (data.string5) tx->SetString1(*data.string5);
        if (data.list) tx->SetString3(*data.list);
        if (data.int1) tx->SetInt1(*data.int1);

        return true;
    };

    bool Extract(PocketHelpers::TxContextualData& data, const PocketHelpers::PTransactionRef& tx) override
    {
        data.string1 = tx->GetString1();
        data.string2 = tx->GetString2();
        data.list = tx->GetString3();
        data.string4 = tx->GetString4();
        data.string5 = tx->GetString5();
        data.int1 = tx->GetInt1();

        return true;
    }
};

bool PocketHelpers::DbViewHelper::Inject(PTransactionRef& tx, const TxContextualData& data)
{
    if (auto translator = m_translatorSelector.find(*tx->GetType()); translator != m_translatorSelector.end()) {
        return translator->second->Inject(tx, data);
    }

    return m_defaultDataTranslator->Inject(tx, data);
}

bool PocketHelpers::DbViewHelper::Extract(TxContextualData& data, const PTransactionRef& tx)
{
    if (auto translator = m_translatorSelector.find(*tx->GetType()); translator != m_translatorSelector.end()) {
        return translator->second->Extract(data, tx);
    }

    return m_defaultDataTranslator->Extract(data, tx);
}

#define TRANSLATOR(txType, TranslatorName) \
    {PocketTx::TxType::txType, std::make_shared<TranslatorName>()}

// TODO (losty-db): !!! fill for all other models
const std::map<PocketDb::TxType, std::shared_ptr<PocketHelpers::ITxDbDataTranslator>> PocketHelpers::DbViewHelper::m_translatorSelector = {
    TRANSLATOR(ACTION_BLOCKING, BlockingDataTranslator),
};

std::shared_ptr<PocketHelpers::ITxDbDataTranslator> const PocketHelpers::DbViewHelper::m_defaultDataTranslator = std::make_shared<CommonDataTranslator>();