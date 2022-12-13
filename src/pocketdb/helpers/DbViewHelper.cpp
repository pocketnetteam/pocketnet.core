// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/DbViewHelper.h"

#include "pocketdb/pocketnet.h"


class AccountDataTranslator : public PocketHelpers::ITxDbDataTranslator
{
public:
    // TODO (losty-db): using PocketDb::TransRepoInst here is a dirty hack to avoid header cycle dependency
    // because TransactionRepository requires to use this helper class. Consider extract required methods to
    // some kind of database accessor.
    bool ToModel(PocketHelpers::PTransactionRef& tx, const PocketHelpers::TxData& data) override
    {
        if (data.int1)
            if (auto val = PocketDb::TransRepoInst.TxIdToHash(*data.int1); val)
                tx->SetString1(*val);

        return true;
    };

    bool FromModel(PocketHelpers::TxData& data, const PocketHelpers::PTransactionRef& tx) override
    {
        if (tx->GetString1()) data.int1 = PocketDb::TransRepoInst.TxHashToId(*tx->GetString1());

        return true;
    }
};



bool PocketHelpers::DbViewHelper::DbViewToModel(PTransactionRef& tx, const TxData& data)
{
    if (auto translator = m_translatorSelector.find(*tx->GetType()); translator != m_translatorSelector.end()) {
        return translator->second->ToModel(tx, data);
    }

    return false;
}

bool PocketHelpers::DbViewHelper::ModelToDbView(TxData& data, const PTransactionRef& tx)
{
    if (auto translator = m_translatorSelector.find(*tx->GetType()); translator != m_translatorSelector.end()) {
        return translator->second->FromModel(data, tx);
    }

    return false;
}

#define TRANSLATOR(txType, TranslatorName) \
    {PocketTx::TxType::txType, std::make_shared<TranslatorName>()}

// TODO (losty-db): !!! fill for all other models
const std::map<PocketDb::TxType, std::shared_ptr<PocketHelpers::ITxDbDataTranslator>> PocketHelpers::DbViewHelper::m_translatorSelector = {
    TRANSLATOR(ACCOUNT_USER, AccountDataTranslator),
};

