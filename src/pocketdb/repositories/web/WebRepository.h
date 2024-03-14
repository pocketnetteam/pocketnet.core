// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEB_REPOSITORY_H
#define POCKETDB_WEB_REPOSITORY_H

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <timedata.h>
#include "core_io.h"

#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/repositories/ConsensusRepository.h"
#include "pocketdb/models/web/WebTag.h"
#include "pocketdb/models/web/WebContent.h"

namespace PocketDb
{
    using namespace PocketDbWeb;
    using namespace PocketTx;

    using boost::algorithm::join;
    using boost::adaptors::transformed;

    class WebRepository : public BaseRepository
    {
    public:
        explicit WebRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        int GetCurrentHeight();
        void SetCurrentHeight(int height);

        vector<WebTag> GetContentTags(int height);
        void UpsertContentTags(const vector<WebTag>& contentTags);

        vector<WebContent> GetContent(int height);
        void UpsertContent(const vector<WebContent>& contentList);

        void UpsertBarteronAccounts(int height);
        void UpsertBarteronOffers(int height);

        void CollectAccountStatistic();
    };

    typedef shared_ptr<WebRepository> WebRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEB_REPOSITORY_H

