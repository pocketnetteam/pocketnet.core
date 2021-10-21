// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEB_RPC_REPOSITORY_H
#define POCKETDB_WEB_RPC_REPOSITORY_H

#include "pocketdb/helpers/PocketnetHelper.h"
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

    struct HierarchicalRecord
    {
        int64_t Id;
        double LAST5;
        double LAST5R;
        double BOOST;
        double UREP;
        double UREPR;
        double DREP;
        double PREP;
        double PREPR;
        double DPOST;
        double POSTRF;

        bool operator > (const HierarchicalRecord& b) const
        {
            return (POSTRF > b.POSTRF);
        }

        bool operator < (const HierarchicalRecord& b) const
        {
            return (POSTRF < b.POSTRF);
        }
    };

    class WebRpcRepository : public BaseRepository
    {
    public:
        explicit WebRpcRepository(SQLiteDatabase& db) : BaseRepository(db) {}

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
        UniValue GetPageScores(const vector<string>& postHashes, const vector<string>& commentHashes, const string& address, int height);

        UniValue GetAddressScores(const vector<string>& postHashes, const string& address);

        map<string, UniValue> GetAccountProfiles(const vector<string>& addresses, bool shortForm = true);
        map<int64_t, UniValue> GetAccountProfiles(const vector<int64_t>& ids, bool shortForm = true);

        map<string, UniValue> GetSubscribesAddresses(const vector<string>& addresses,
            const vector<TxType>& types = {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE });

        map<string, UniValue> GetSubscribersAddresses(const vector<string>& addresses,
            const vector<TxType>& types = {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE });

        map<string, UniValue> GetBlockingToAddresses(const vector<string>& addresses);

        map<string, UniValue> GetTags(const string& addresses, const int nHeight, const int from, const string& lang);

        vector<int64_t> GetContentIds(const vector<string>& txHashes);

        UniValue GetUnspents(vector<string>& addresses, int height);

        tuple<int, UniValue> GetContentLanguages(int height);
        tuple<int, UniValue> GetLastAddressContent(const string& address, int height, int count);

        vector<UniValue> GetMissedRelayedContent(const string& address, int height);
        vector<UniValue> GetMissedContentsScores(const string& address, int height, int limit);
        vector<UniValue> GetMissedCommentsScores(const string& address, int height, int limit);
        map<string, UniValue> GetMissedTransactions(const string& address, int height, int count);
        vector<UniValue> GetMissedCommentAnswers(const string& address, int height, int count);
        vector<UniValue> GetMissedPostComments(const string& address, const vector<string>& excludePosts, int height, int count);


        vector<UniValue> GetContentsData(const vector<int64_t>& ids, const string& address);
        
        UniValue GetHotPosts(int countOut, const int depth, const int nHeight, const string& lang, const vector<int>& contentTypes, const string& address);
        
        UniValue GetProfileFeed(const string& addressFrom, const string& addressTo, int64_t topContentId, int count, const string& lang, const vector<string>& tags, const vector<int>& contentTypes);
        
        UniValue GetHistoricalFeed(int countOut, const int64_t& topContentId, int topHeight, const string& lang, const vector<string>& tags,
            const vector<int>& contentTypes, const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address);

        UniValue GetHierarchicalFeed(int countOut, const int64_t& topContentId, int topHeight, const string& lang, const vector<string>& tags,
            const vector<int>& contentTypes, const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address);

    private:
        int cntBlocksForResult = 300;
        int cntPrevPosts = 5;
        int durationBlocksForPrevPosts = 30 * 24 * 60; // about 1 month
        double dekayRep = 0.82;
        double dekayVideo = 0.99;
        double dekayContent =  0.96;

        UniValue ParseCommentRow(sqlite3_stmt* stmt);
        vector<tuple<string, int64_t, UniValue>> GetAccountProfiles(const vector<string>& addresses, const vector<int64_t>& ids, bool shortForm);
    };

    typedef shared_ptr<WebRpcRepository> WebRpcRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEB_RPC_REPOSITORY_H

