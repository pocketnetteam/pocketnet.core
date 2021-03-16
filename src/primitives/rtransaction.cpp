#include "rtransaction.h"

RTransaction::RTransaction() { }
RTransaction::RTransaction(CTransaction tx) : CTransactionRef(MakeTransactionRef(std::move(tx)))
{ }

RTransaction::RTransaction(CMutableTransaction mtx) : CTransactionRef(MakeTransactionRef(std::move(mtx)))
{ }

RTransaction::~RTransaction()
{ }
