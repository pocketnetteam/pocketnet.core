// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/DbViewHelper.h"

#include "pocketdb/pocketnet.h"

#include <algorithm>

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

class BoostDataTranslator : public PocketHelpers::ITxDbDataTranslator
{
public:
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
        data.int1 = tx->GetInt1().value_or(calculateIntField(tx->Outputs(), tx->Inputs()));

        return true;
    }
protected:
    // TODO (optimization): find a better place for this maybe (calculate on tx deserialization or smth)
    int64_t calculateIntField(const std::vector<PocketTx::TransactionOutput>& outputs, const std::vector<PocketTx::TransactionInput>& inputs)
    {
        return 
            std::accumulate(
                outputs.begin(),
                outputs.end(),
                (int64_t)0,
                [](const int64_t& sum, const PocketTx::TransactionOutput& elem) { return sum + elem.GetValue().value_or(0); }
            ) - 
            std::accumulate(
                inputs.begin(),
                inputs.end(),
                (int64_t)0,
                [](const int64_t& sum, const PocketTx::TransactionInput& elem) { return sum + elem.GetValue().value_or(0); });
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

const std::map<PocketDb::TxType, std::shared_ptr<PocketHelpers::ITxDbDataTranslator>> PocketHelpers::DbViewHelper::m_translatorSelector = {
    TRANSLATOR(ACTION_BLOCKING, BlockingDataTranslator),
    TRANSLATOR(BOOST_CONTENT, BoostDataTranslator),
};

std::shared_ptr<PocketHelpers::ITxDbDataTranslator> const PocketHelpers::DbViewHelper::m_defaultDataTranslator = std::make_shared<CommonDataTranslator>();