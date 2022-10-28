// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_RATING_HPP
#define POCKETTX_RATING_HPP

#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/base/Base.h"

namespace PocketTx
{
    class Rating : public Base
    {
    public:
        Rating() = default;
        ~Rating() = default;

        const optional <RatingType>& GetType() const;
        void SetType(RatingType value);

        const optional <int>& GetHeight() const;
        void SetHeight(int value);

        const optional <int64_t>& GetId() const;
        void SetId(int64_t value);

        const optional <int64_t>& GetValue() const;
        void SetValue(int64_t value);

    protected:
        optional <RatingType> m_type = nullopt;
        optional <int> m_height = nullopt;
        optional <int64_t> m_id = nullopt;
        optional <int64_t> m_value = nullopt;
    };

} // namespace PocketTx

#endif // POCKETTX_RATING_HPP