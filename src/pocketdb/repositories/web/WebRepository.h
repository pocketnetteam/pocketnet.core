// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEBREPOSITORY_H
#define POCKETDB_WEBREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"

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

        UniValue GetAddressId(const string& address);
        UniValue GetAddressId(int64_t id);
        UniValue GetUserAddress(const string& name);
        UniValue GetAddressesRegistrationDates(const vector<string>& addresses);
        UniValue GetTopAddresses(int count);
        UniValue GetAccountState(const string& address, int heightWindow);
        UniValue GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash);
        UniValue GetCommentsByIds(const string& addressHash, const vector<string>& commentHashes);
        UniValue GetLastComments(int count, int height, const string& lang = "");
        UniValue GetPostScores(const vector<string>& postHashes, const string& address);
        UniValue GetPageScores(const vector<string>& commentHashes, const string& addressHash);
        UniValue GetAddressScores(const vector<string>& postHashes, const string& address);
        map<string, UniValue> GetUserProfile(const vector<string>& addresses, bool shortForm = true, int option = 0);

        map<string, UniValue> GetSubscribesAddresses(const vector<string>& addresses,
            const vector<PocketTxType>& types = { ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE });

        map<string, UniValue> GetSubscribersAddresses(const vector<string>& addresses,
            const vector<PocketTxType>& types = { ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE });

        map<string, UniValue> GetBlockingToAddresses(const vector<string>& addresses);

        map<string, UniValue> GetContentsData(const vector<string>& txids);
        //map<string, UniValue> GetContents(map<string, param>& conditions, optional<int> &counttotal);
        //map<string, UniValue> GetContents(map<string, param>& conditions);
        map<string, UniValue> GetContents(int nHeight, const string& start_txid, int countOut, const string& lang,
            const vector<string>& tags, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
            const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address);
            
        UniValue GetUnspents(vector<string>& addresses, int height);

        tuple<int, UniValue> GetContentLanguages(int height);
        tuple<int, UniValue> GetLastAddressContent(const string& address, int height, int count);

    private:
        UniValue ParseCommentRow(sqlite3_stmt* stmt);
    };

    typedef shared_ptr<WebRepository> WebRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEBREPOSITORY_H

