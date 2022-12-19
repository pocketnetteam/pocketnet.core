// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/DbViewHelper.h"

#include "pocketdb/pocketnet.h"


class BlockingDataTransaclator : public PocketHelpers::ITxDbDataTranslator
{
public:
    // TODO (losty-db): using PocketDb::TransRepoInst here is a dirty hack to avoid header cycle dependency
    // because TransactionRepository requires to use this helper class. Consider extract required methods to
    // some kind of database accessor.
    bool Inject(PocketHelpers::PTransactionRef& tx, const PocketHelpers::TxContextualData& data) override
    {
        if (data.list)
            tx->SetString3(*data.list);
        else 
            return false;

        return true;
    };

    bool Extract(PocketHelpers::TxContextualData& data, const PocketHelpers::PTransactionRef& tx) override
    {
        data.list = tx->GetString3();
        return true;
    }
};



bool PocketHelpers::DbViewHelper::Inject(PTransactionRef& tx, const TxContextualData& data)
{
    if (auto translator = m_translatorSelector.find(*tx->GetType()); translator != m_translatorSelector.end()) {
        return translator->second->Inject(tx, data);
    }

    return false;
}

bool PocketHelpers::DbViewHelper::Extract(TxContextualData& data, const PTransactionRef& tx)
{
    if (auto translator = m_translatorSelector.find(*tx->GetType()); translator != m_translatorSelector.end()) {
        return translator->second->Extract(data, tx);
    }

    return false;
}

#define TRANSLATOR(txType, TranslatorName) \
    {PocketTx::TxType::txType, std::make_shared<TranslatorName>()}

// TODO (losty-db): !!! fill for all other models
const std::map<PocketDb::TxType, std::shared_ptr<PocketHelpers::ITxDbDataTranslator>> PocketHelpers::DbViewHelper::m_translatorSelector = {
    TRANSLATOR(ACTION_BLOCKING, BlockingDataTransaclator),
};

