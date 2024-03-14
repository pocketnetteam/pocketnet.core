// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RATINGSREPOSITORY_H
#define SRC_RATINGSREPOSITORY_H

#include <util/system.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/base/Rating.h"
#include "pocketdb/models/base/DtoModels.h"

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;
    
    using namespace PocketTx;

    class RatingsRepository : public BaseRepository
    {
    public:
        explicit RatingsRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        // Accumulate new rating records
        void InsertRatings(shared_ptr<vector<Rating>> ratings);
        bool ExistsLiker(int addressId, int likerId);
        bool ExistsLiker(int addressId, int likerId, const vector<RatingType>& types);

    private:

        void InsertRating(const Rating& rating);

        void InsertLiker(const Rating& rating);

    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_H
