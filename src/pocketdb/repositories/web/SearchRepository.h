// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SEARCH_REPOSITORY_H
#define POCKETDB_SEARCH_REPOSITORY_H

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <timedata.h>
#include "core_io.h"

#include "pocketdb/models/web/SearchRequest.h"
#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    using namespace PocketTx;
    using namespace PocketDbWeb;

    using boost::algorithm::join;
    using boost::adaptors::transformed;

    class SearchRepository : public BaseRepository
    {
    public:
        explicit SearchRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        UniValue SearchTags(const SearchRequest& request);
        vector<int64_t> SearchIds(const SearchRequest& request);

        vector<int64_t> SearchUsers(const string& keyword);

        vector<string> GetRecommendedAccountByAddressSubscriptions(const string& address, string& addressExclude, const vector<int>& contentTypes, const string& lang, int cntOut, int nHeight, int depth = 129600 /* about 3 month */);
        vector<int64_t> GetRecommendedContentByAddressSubscriptions(const string& contentAddress, string& addressExclude, const vector<int>& contentTypes, const string& lang, int cntOut, int nHeight, int depth = 129600 /* about 3 month */);
        vector<int64_t> GetRandomContentByAddress(const string& contentAddress, const vector<int>& contentTypes, const string& lang, int cntOut);
        vector<int64_t> GetContentFromAddressSubscriptions(const string& address, const vector<int>& contentTypes, const string& lang, int cntOut, bool rest = false);
    };

    typedef shared_ptr<SearchRepository> SearchRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_SEARCH_REPOSITORY_H

