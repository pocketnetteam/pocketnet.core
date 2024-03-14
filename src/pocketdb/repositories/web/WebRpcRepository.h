// Copyright (c) 2018-2022 The Pocketnet developers
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
#include "util/html.h"

namespace PocketDb
{
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;
    using namespace std::literals::string_literals;

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
        explicit WebRpcRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        UniValue GetAddressId(const string& address);
        UniValue GetAddressId(int64_t id);
        UniValue GetUserAddress(const string& name);
        UniValue GetAddressesRegistrationDates(const vector<string>& addresses);
        UniValue GetTopAddresses(int count);
        UniValue GetAccountState(const string& address, int heightWindow);
        UniValue GetAccountSetting(const string& address);

        UniValue GetUserStatistic(const vector<string>& addresses, const int nHeight = 0, const int depthR = 0, const int depthC = 0, const int cntC = 1);

        vector<string> GetContentComments(const vector<string>& contentHashes);
        vector<string> GetContentScores(const vector<string>& contentHashes);
        vector<string> GetLastContentHashesByRootTxHashes(const vector<string>& rootHashes);
        vector<string> GetCommentScores(const vector<string>& commentHashes);
        // Get account tx id for signer of specified transactions (using RegId1)
        vector<string> GetAddresses(const vector<string>& txHashes);
        vector<string> GetAccountsIds(const vector<string>& addresses);
        UniValue GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash);
        map<string, UniValue> GetCommentsByHashes(const vector<string>& cmntHashes, const string& addressHash);
        map<int64_t, UniValue> GetCommentsByIds(const vector<int64_t>& cmntIds, const string& addressHash);

        UniValue GetLastComments(int count, int height, const string& lang);
        map<int64_t, UniValue> GetLastComments(const vector<int64_t>& ids, const string& address);

        UniValue GetPagesScores(const vector<string>& postHashes, const vector<string>& commentHashes, const string& address);
        UniValue GetPostScores(const string& postTxHash);

        UniValue GetAddressScores(const vector<string>& postHashes, const string& address);
        UniValue GetAccountRaters(const string& address);

        map<string, UniValue> GetAccountProfiles(const vector<string>& addresses, bool shortForm = true, int firstFlagsDepth = 14);
        map<int64_t, UniValue> GetAccountProfiles(const vector<int64_t>& ids, bool shortForm = true, int firstFlagsDepth = 14);

        UniValue GetSubscribesAddresses(
            const string& address, const vector<TxType>& types = {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE },
            const string& orderBy = "height", bool orderDesc = true, int offset = 0, int limit = 10);
        UniValue GetSubscribersAddresses(
            const string& address, const vector<TxType>& types = {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE },
            const string& orderBy = "height", bool orderDesc = true, int offset = 0, int limit = 10);
        UniValue GetBlockings(const string& address, bool useAddresses);
        UniValue GetBlockers(const string& address, bool useAddresses);

        vector<string> GetTopAccounts(int topHeight, int countOut, const string& lang,
        const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, int depth,
        int badReputationLimit);

        UniValue GetTags(const string& lang, int pageSize, int pageStart);

        vector<int64_t> GetContentIds(const vector<string>& txHashes);
        map<string,string> GetContentsAddresses(const vector<string>& txHashes);

        UniValue GetUnspents(const vector<string>& addresses, int height, int confirmations, vector<pair<string, uint32_t>>& mempoolInputs);

        UniValue GetAccountEarning(const string& address, int height, int depth);

        tuple<int, UniValue> GetContentLanguages(int height);
        tuple<int, UniValue> GetLastAddressContent(const string& address, int height, int count);
        UniValue GetContentsForAddress(const string& address);

        vector<UniValue> GetMissedRelayedContent(const string& address, int height);
        vector<UniValue> GetMissedContentsScores(const string& address, int height, int limit);
        vector<UniValue> GetMissedCommentsScores(const string& address, int height, int limit);
        map<string, UniValue> GetMissedTransactions(const string& address, int height, int count);
        vector<UniValue> GetMissedCommentAnswers(const string& address, int height, int count);
        vector<UniValue> GetMissedPostComments(const string& address, const vector<string>& excludePosts, int height, int count);
        vector<UniValue> GetMissedSubscribers(const string& address, int height, int count);
        vector<UniValue> GetMissedBoosts(const string& address, int height, int count);

        UniValue SearchLinks(const vector<string>& links, const vector<int>& contentTypes, const int nHeight, const int countOut);

        map<string, UniValue> GetContentsData(const vector<string>& hashes, const string& address = "");
        map<int64_t, UniValue> GetContentsData(const vector<int64_t>& ids, const string& address = "");
        vector<UniValue> GetContentsData(const vector<string>& hashes, const vector<int64_t>& ids, const string& address);
        vector<UniValue> GetCollectionsData(const vector<int64_t>& ids);

        UniValue GetHotPosts(int countOut, const int depth, const int nHeight, const string& lang, const vector<int>& contentTypes, const string& address, int badReputationLimit);

        UniValue GetTopFeed(int countOut, const int64_t& topContentId, int topHeight, const string& lang,
        const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
        const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address, int depth,
        int badReputationLimit);

        UniValue GetMostCommentedFeed(int countOut, const int64_t& topContentId, int topHeight, const string& lang,
        const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
        const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address, int depth,
        int badReputationLimit);
        
        UniValue GetProfileFeed(const string& addressFeed, int countOut, int pageNumber, const int64_t& topContentId, int topHeight, const string& lang,
            const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded, const vector<string>& adrsExcluded,
            const vector<string>& tagsExcluded, const string& address, const string& orderby, const string& ascdesc);
        
        UniValue GetSubscribesFeed(const string& addressFeed, int countOut, const int64_t& topContentId, int topHeight, const string& lang,
            const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
            const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address, const vector<string>& addresses_extended);

        UniValue GetHistoricalFeed(int countOut, const int64_t& topContentId, int topHeight, const string& lang,
            const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
            const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address,
            int badReputationLimit);

        UniValue GetHierarchicalFeed(int countOut, const int64_t& topContentId, int topHeight, const string& lang,
            const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
            const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address,
            int badReputationLimit);

        UniValue GetBoostFeed(int topHeight, const string& lang,
            const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
            const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
            int badReputationLimit);

        UniValue GetContentsStatistic(const vector<string>& addresses, const vector<int>& contentTypes);

        vector<int64_t> GetRandomContentIds(const string& lang, int count, int height);

        UniValue GetContentActions(const string& postTxHash);

        UniValue GetProfileCollections(const string& addressFeed, int countOut, int pageNumber, const int64_t& topContentId, int topHeight, const string& lang,
                                       const vector<string>& tags, const vector<int>& contentTypes, const vector<string>& txidsExcluded,
                                       const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, const string& address,
                                       const string& keyword, const string& orderby, const string& ascdesc);

        UniValue GetsubsciptionsGroupedByAuthors(const string& address, const string& addressPagination, int nHeight, int countOutOfUsers, int countOutOfcontents, int badReputationLimit);

    private:
        int cntBlocksForResult = 300;
        int cntPrevPosts = 5;
        int durationBlocksForPrevPosts = 30 * 24 * 60; // about 1 month
        double dekayRep = 0.82;
        double dekayVideo = 0.99;
        double dekayContent =  0.96;

        vector<tuple<string, int64_t, UniValue>> GetAccountProfiles(const vector<string>& addresses, const vector<int64_t>& ids, bool shortForm, int firstFlagsDepth);
        string GetAccountProfilesSqlFull();
        string GetAccountProfilesSqlShort();
        vector<tuple<string, int64_t, UniValue>> GetComments(const vector<string>& cmntHashes, const vector<int64_t>& cmntIds, const string& addressHash);
    };

    typedef shared_ptr<WebRpcRepository> WebRpcRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEB_RPC_REPOSITORY_H
