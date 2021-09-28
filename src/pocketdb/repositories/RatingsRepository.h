// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RATINGSREPOSITORY_H
#define SRC_RATINGSREPOSITORY_H

#include <util.h>

#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/base/Rating.h"
#include "pocketdb/models/base/ReturnDtoModels.h"

namespace PocketDb
{
    using std::runtime_error;
    using namespace PocketTx;

    class RatingsRepository : public BaseRepository
    {
    public:
        explicit RatingsRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}

        void Destroy() override {}

        // Accumulate new rating records
        void InsertRatings(shared_ptr<vector<Rating>> ratings);

        bool ExistsLiker(int addressId, int likerId, int height);

    private:

        void InsertRating(const Rating& rating);

        void InsertLiker(const Rating& rating);

    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_H
