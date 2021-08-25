// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
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

        shared_ptr<RatingType> GetType() const;
        shared_ptr<int> GetTypeInt() const;
        void SetType(RatingType value);

        shared_ptr <int> GetHeight() const;
        void SetHeight(int value);

        shared_ptr <int64_t> GetId() const;
        void SetId(int64_t value);

        shared_ptr <int64_t> GetValue() const;
        void SetValue(int64_t value);

    protected:
        shared_ptr <RatingType> m_type = nullptr;
        shared_ptr <int> m_height = nullptr;
        shared_ptr <int64_t> m_id = nullptr;
        shared_ptr <int64_t> m_value = nullptr;
    };

} // namespace PocketTx

#endif // POCKETTX_RATING_HPP