// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CONSENSUSREPOSITORY_H
#define POCKETDB_CONSENSUSREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/repositories/TransactionRepository.hpp"

#include <timedata.h>

namespace PocketDb
{
using std::runtime_error;
using std::string;

using namespace PocketTx;
using namespace PocketHelpers;

class ConsensusRepository : public TransactionRepository
{
public:
    explicit ConsensusRepository(SQLiteDatabase& db) : TransactionRepository(db) {}

    void Init() override;
    void Destroy() override;

    bool ExistsAnotherByName(const string& address, const string& name);
    shared_ptr<Transaction> GetLastAccountTransaction(const string& address);
    bool ExistsUserRegistrations(vector<string>& addresses);
    tuple<bool, PocketTxType> GetLastBlockingType(const string& address, const string& addressTo);
    tuple<bool, PocketTxType> GetLastSubscribeType(const string& address, const string& addressTo);
    shared_ptr<string> GetPostAddress(const string& postHash);
    bool ExistsComplain(const string& postHash, const string& address);
    shared_ptr<string> GetLastActiveCommentAddress(const string& rootHash);
    bool ExistsScore(const string& address, const string& contentHash, PocketTxType type);
    int64_t GetUserBalance(const string& address);
    int GetUserReputation(const string& addressId);
    int GetUserReputation(int addressId);
};

} // namespace PocketDb

#endif // POCKETDB_CONSENSUSREPOSITORY_H

