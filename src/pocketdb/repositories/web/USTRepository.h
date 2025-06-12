// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_UST_REPOSITORY_H
#define POCKETDB_UST_REPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    using namespace PocketTx;


    struct USTRequest : public Pagination
    {

    };


    class USTRepository : public BaseRepository
    {
    public:
        explicit USTRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        UniValue ustlist(const USTRequest& request);

    private:
    
    };

    typedef shared_ptr<USTRepository> USTRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_UST_REPOSITORY_H

