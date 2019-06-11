#pragma once

#include "transaction.h"
#include "pocketdb/pocketdb.h"

class RTransaction : public CTransactionRef {
public:
    RTransaction();
    RTransaction(CTransaction tx);
    RTransaction(CMutableTransaction mtx);
    ~RTransaction();

    // reindexer part of transaction
    reindexer::Item pTransaction;
    std::string pTable;
};