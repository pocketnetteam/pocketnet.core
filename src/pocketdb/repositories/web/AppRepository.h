// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RPC_APP_REPOSITORY_H
#define SRC_RPC_APP_REPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using namespace std;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    struct AppListDto
    {
        Pagination Page;
        vector<string> Tags;
        string Search = "";
    };

    class AppRepository : public BaseRepository
    {
    public:
        explicit AppRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        vector<string> List(const AppListDto& args);
        map<string, UniValue> AdditionalInfo(const vector<string>& txs);

    };

    typedef std::shared_ptr<AppRepository> AppRepositoryRef;
}

#endif //SRC_RPC_APP_REPOSITORY_H
