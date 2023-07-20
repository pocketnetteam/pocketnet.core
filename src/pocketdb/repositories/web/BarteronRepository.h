// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RPC_BARTERON_REPOSITORY_H
#define SRC_RPC_BARTERON_REPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    using namespace std;

    class BarteronRepository : public BaseRepository
    {
    public:
        explicit BarteronRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        UniValue GetAccount(const string& address);

    };

    typedef std::shared_ptr<BarteronRepository> BarteronRepositoryRef;
}

#endif //SRC_RPC_BARTERON_REPOSITORY_H
