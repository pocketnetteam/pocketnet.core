// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEBREPOSITORY_H
#define POCKETDB_WEBREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <timedata.h>
#include "core_io.h"

namespace PocketDb
{
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    class WebRepository : public BaseRepository
    {
    public:
        explicit WebRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

        UniValue GetUserAddress(string& name);
        UniValue GetAddressesRegistrationDates(vector<string>& addresses);
        UniValue GetAddressInfo(int count);
        UniValue GetAccountState(const string& address, int heightWindow);
        UniValue GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash);
        UniValue GetCommentsByIds(string& addressHash, vector<string>& commentHashes);
        UniValue GetLastComments(int count, int height, string lang = "");
        UniValue GetPostScores(vector<string>& postHashes, string& address);
        UniValue GetPageScores(vector<string>& commentHashes, string& addressHash);
        map<string, UniValue> GetUserProfile(vector<string>& addresses, bool shortForm = true, int option = 0);
        map<string, UniValue> GetSubscribesAddresses(vector<string>& addresses);
        map<string, UniValue> GetSubscribersAddresses(vector<string>& addresses);
        map<string, UniValue> GetBlockingToAddresses(vector<string>& addresses);

        map<string, UniValue> GetContentsData(vector<string>& txids);
        //map<string, UniValue> GetContents(map<string, param>& conditions, optional<int> &counttotal);
        //map<string, UniValue> GetContents(map<string, param>& conditions);
        map<string, UniValue> GetContents(int nHeight, const string& start_txid, int countOut, const string& lang,
            vector<string>& tags, vector<int>& contentTypes, vector<string>& txidsExcluded, vector<string>& adrsExcluded,
            vector<string>& tagsExcluded, const string& address);
            
        UniValue GetUnspents(vector<string>& addresses, int height);

    private:
        UniValue ParseCommentRow(sqlite3_stmt* stmt);
    };

    typedef shared_ptr<WebRepository> WebRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEBREPOSITORY_H

