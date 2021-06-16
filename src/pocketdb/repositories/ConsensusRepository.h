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
    bool ExistsUserRegistrations(vector<string>& addresses, int height = 0);
    tuple<bool, PocketTxType> GetLastBlockingType(string& address, string& addressTo, int height);
    tuple<bool, PocketTxType> GetLastSubscribeType(string& address, string& addressTo, int height);
    shared_ptr<string> GetPostAddress(string& postHash, int height);
    bool ExistsComplain(string& postHash, string& address, int height);
    shared_ptr<string> GetLastActiveCommentAddress(string& rootHash, int height);
    bool ExistsScore(string& address, string& contentHash, PocketTxType type, int height);
};

} // namespace PocketDb

#endif // POCKETDB_CONSENSUSREPOSITORY_H

