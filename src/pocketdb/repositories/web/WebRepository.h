// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEB_REPOSITORY_H
#define POCKETDB_WEB_REPOSITORY_H

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <timedata.h>
#include "core_io.h"

#include "pocketdb/repositories/BaseRepository.h"
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
        explicit WebRepository(SQLiteDatabase& db) : BaseRepository(db) {}
        void Init() override;
        void Destroy() override;

        vector<WebTag> GetContentTags(const string& blockHash);
        void UpsertContentTags(const vector<WebTag>& contentTags);

        vector<WebContent> GetContent(const string& blockHash);
        void UpsertContent(const vector<WebContent>& contentList);
    };

    typedef shared_ptr<WebRepository> WebRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEB_REPOSITORY_H

