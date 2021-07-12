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


    shared_ptr<Transaction> GetLastAccountTransaction(const string& address);
    tuple<bool, int64_t> GetLastAccountHeight(const string& address) { return {true, 0}; } // TODO (brangr): implement

    tuple<bool, PocketTxType> GetLastBlockingType(const string& address, const string& addressTo);
    tuple<bool, PocketTxType> GetLastSubscribeType(const string& address, const string& addressTo);
    shared_ptr<string> GetLastActiveCommentAddress(const string& rootHash);

    shared_ptr<string> GetPostAddress(const string& postHash);
    int64_t GetUserBalance(const string& address);
    int GetUserReputation(const string& addressId);
    int GetUserReputation(int addressId);

    shared_ptr<ScoreDataDto> GetScoreData(const string& txHash);
    shared_ptr<map<string, string>> GetReferrers(const vector<string>& addresses, int minHeight);
    shared_ptr<string> GetReferrer(const string& address, int minTime);
    int GetUserLikersCount(int addressId);
    int GetScoreContentCount(PocketTxType scoreType, PocketTxType contentType,
                              const string& scoreAddress, const string& contentAddress,
                              int height, const CTransactionRef& tx,
                              const std::vector<int>& values,
                              int64_t scoresOneToOneDepth);

    // Exists
    bool ExistsComplain(const string& txHash, const string& postHash, const string& address);
    bool ExistsScore(const string& address, const string& contentHash, PocketTxType type);
    bool ExistsUserRegistrations(vector<string>& addresses);
    bool ExistsAnotherByName(const string& address, const string& name);

    // get counts in "mempool" - Height is null
    int CountMempoolBlocking(const string& address, const string& addressTo) {return 0;} // TODO (brangr): implement
    int CountMempoolSubscribe(const string& address, const string& addressTo) {return 0;} // TODO (brangr): implement
    int CountMempoolComplain(const string& address) {return 0;} // TODO (brangr): implement
    int CountMempoolAccount(const string& address) {return 0;} // TODO (brangr): implement
    int CountMempoolContent(const string& address, PocketTxType txType) {return 0;} // TODO (brangr): implement
    int CountMempoolContentEdit(const string& rootTxHash) {return 0;} // TODO (brangr): implement

    // get counts in "chain" - Height is not null
    int CountChainComplain(const string& address, int64_t time) // TODO (brangr): implement
    {
     /*    reindexer::Query("Complains")
             .Where("address", CondEq, _address)
             .Where("time", CondGe, _time - 86400)
             .Where("block", CondLt, height),
         complainsRes)*/
     return 0;
    }
    int CountChainContent(const string& address, int64_t time, PocketTxType txType) {return 0;} // TODO (brangr): implement as complains root != hash
    int CountChainContentEdit(const string& rootTxHash) {return 0;} // TODO (brangr): implement find all edited instance of post






};

} // namespace PocketDb

#endif // POCKETDB_CONSENSUSREPOSITORY_H

