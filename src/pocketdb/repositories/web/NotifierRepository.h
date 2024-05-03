// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFIERREPOSITORY_H
#define POCKETNET_CORE_NOTIFIERREPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/shortform/ShortForm.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using namespace std;
    using boost::algorithm::join;

    class NotifierRepository : public BaseRepository
    {
    public:
        explicit NotifierRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        UniValue GetAccountInfoByAddress(const string& address);
        UniValue GetPostLang(const string& postHash);
        UniValue GetPostInfo(const string& postHash);
        UniValue GetBoostInfo(const string& boostHash);
        UniValue GetOriginalPostAddressByRepost(const string& repostHash);
        UniValue GetPrivateSubscribeAddressesByAddressTo(const string& addressTo);
        UniValue GetPostInfoAddressByScore(const string& postScoreHash);
        UniValue GetSubscribeAddressTo(const string& subscribeHash);
        UniValue GetCommentInfoAddressByScore(const string& commentScoreHash);
        UniValue GetFullCommentInfo(const string& commentHash);
        UniValue GetPostCountFromMySubscribes(const string& address, int height);


        /**
         * Returns map where key is address. Value is map, where key - height, value - vector of transactions for this height.
         */
        vector<ShortForm> GetEventsForAddresses(const string& address, int64_t heightMax, int64_t heightMin, int64_t blockNum, const set<ShortTxType>& filters);

        /**
         * Get all possible events for all adresses in concrete block
         * 
         * @param height height of block to search
         * @param filters
         */
        UniValue GetNotifications(int64_t height, const set<ShortTxType>& filters);

        /**
         * Get all activities (posts, comments, etc) created by address
         * 
         * @param address - address to search activities for
         * @param heightMax - height to start search from, including
         * @param heightMin - height untill search, excluding
         * @param blockNumMax - number of tx in block to start search in heightMax, excluding
         * @param filters 
         * @return vector of activities in ShortForm
         */
        vector<ShortForm> GetActivities(const string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const set<ShortTxType>& filters);

        // TODO (losty): convert return type to class of smth. + docs
        map<string, map<ShortTxType, int>> GetNotificationsSummary(int64_t heightMax, int64_t heightMin, const vector<string>& addresses, const set<ShortTxType>& filters);

    };

    typedef shared_ptr<NotifierRepository> NotifierRepositoryRef;
}

#endif //POCKETNET_CORE_NOTIFIERREPOSITORY_H
