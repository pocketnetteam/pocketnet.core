// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEBREPOSITORY_H
#define POCKETDB_WEBREPOSITORY_H

#include "pocketdb/helpers/TypesHelper.hpp"
#include "pocketdb/helpers/TransactionHelper.hpp"

#include "pocketdb/repositories/BaseRepository.hpp"

#include <timedata.h>

namespace PocketDb
{
using std::runtime_error;

using namespace PocketTx;
using namespace PocketHelpers;

class WebRepository : public BaseRepository
{
public:
    explicit WebRepository(SQLiteDatabase& db) : BaseRepository(db) {}

    void Init() override;
    void Destroy() override;

    UniValue GetLastComments(int count, int height, std::string lang = "");
};

} // namespace PocketDb

#endif // POCKETDB_WEBREPOSITORY_H

