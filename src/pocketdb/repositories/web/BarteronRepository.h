// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RPC_BARTERON_REPOSITORY_H
#define SRC_RPC_BARTERON_REPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using namespace std;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    struct BarteronOffersFeedDto
    {
        Pagination Page;
        string Language = "";
        vector<int> Tags;
        vector<string> Location;
        int LocationGroup = 1;
        int64_t PriceMax = -1;
        int64_t PriceMin = -1;
        string Search = "";
        string State;
    };

    struct BarteronOffersDealDto : public BarteronOffersFeedDto
    {
        vector<int> MyTags;
        vector<int> TheirTags;
        vector<string> Addresses;
        vector<string> ExcludeAddresses;
    };

    struct BarteronAccountAdditionalInfo
    {
        int64_t RegDate;
        int Rating;
        int Sum;
        int Count;
    };

    struct BarteronOffersComplexDealDto
    {
        Pagination Page;
        int64_t MyTag = 0;
        vector<int64_t> TheirTags;
        vector<string> ExcludeAddresses;
        vector<string> Location;
        string Language = "";
        string State;
    };

    class BarteronRepository : public BaseRepository
    {
    public:
        explicit BarteronRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        vector<string> GetAccountIds(const vector<string>& addresses);
        map<string, BarteronAccountAdditionalInfo> GetAccountsAdditionalInfo(const vector<string>& txids);
        vector<string> GetAccountOffersIds(const string& address);
        vector<string> GetFeed(const BarteronOffersFeedDto& args);
        UniValue GetGroups(const BarteronOffersFeedDto& args);
        vector<string> GetDeals(const BarteronOffersDealDto& args);
        map<string, vector<string>> GetComplexDeal(const BarteronOffersComplexDealDto& args);
    
    protected:
        vector<string> _feed_by_search(const BarteronOffersFeedDto& args, const string& search);
        vector<string> _feed_by_tags(const BarteronOffersFeedDto& args);
        vector<string> _feed(const BarteronOffersFeedDto& args);
    };

    typedef std::shared_ptr<BarteronRepository> BarteronRepositoryRef;
}

#endif //SRC_RPC_BARTERON_REPOSITORY_H
