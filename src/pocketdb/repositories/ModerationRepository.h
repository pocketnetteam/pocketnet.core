// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_MODERATION_REPOSITORY_H
#define SRC_MODERATION_REPOSITORY_H

#include <util/system.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/base/DtoModels.h"
#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;
    
    using namespace PocketTx;

    class ModerationRepository : public BaseRepository
    {
    public:
        explicit ModerationRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}

        void Destroy() override {}

    private:

    }; // namespace PocketDb
}
#endif //SRC_MODERATION_REPOSITORY_H
