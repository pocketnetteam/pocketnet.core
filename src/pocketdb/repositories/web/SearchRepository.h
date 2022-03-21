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
        explicit SearchRepository(SQLiteDatabase& db) : BaseRepository(db) {}
        void Init() override;
        void Destroy() override;

        UniValue SearchTags(const SearchRequest& request);
        vector<int64_t> SearchIds(const SearchRequest& request);

        vector<int64_t> SearchUsersOld(const SearchRequest& request);
        vector<int64_t> SearchUsers(const string& keyword);

        UniValue GetRecomendedAccountsBySubscriptions(const string& address, int cntOut = 10);
        UniValue GetRecomendedAccountsByScoresOnSimilarAccounts(const string& address, const vector<int>& contentTypes, int nHeight, int depth = 1000, int cntOut = 10);
        UniValue GetRecomendedAccountsByScoresFromAddress(const string& address, const vector<int>& contentTypes, int nHeight, int depth = 1000, int cntOut = 10);
        UniValue GetRecomendedAccountsByTags(const vector<string>& tags, int nHeight, int depth = 1000, int cntOut = 10);
        UniValue GetRecomendedContentsByScoresOnSimilarContents(const string& contentid, const vector<int>& contentTypes, int depth = 1000, int cntOut = 10);
        UniValue GetRecomendedContentsByScoresFromAddress(const string& address, const vector<int>& contentTypes, int nHeight, int depth = 1000, int cntOut = 10);
    };

    typedef shared_ptr<SearchRepository> SearchRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_SEARCH_REPOSITORY_H

