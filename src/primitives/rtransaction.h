#pragma once

#include "transaction.h"
#include "pocketdb/pocketdb.h"

class RTransaction : public CTransactionRef
{
public:
    RTransaction();
    RTransaction(CTransactionRef tx);
    RTransaction(CMutableTransaction tx);
    RTransaction(CTransaction tx);
    ~RTransaction();

    // reindexer part of transaction
    reindexer::Item pTransaction;
    std::string TxType;
    std::string pTable;
    std::string Address;
};
