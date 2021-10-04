// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEB_REPOSITORY_H
#define POCKETDB_WEB_REPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <timedata.h>
#include "core_io.h"

namespace PocketDb
{
    class WebRepository : public BaseRepository
    {
    public:
        explicit WebRepository(SQLiteDatabase& db) : BaseRepository(db) {}
        void Init() override;
        void Destroy() override;
    private:
    };

    typedef shared_ptr<WebRepository> WebRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEB_REPOSITORY_H

