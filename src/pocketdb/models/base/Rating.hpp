// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_RATING_HPP
#define POCKETTX_RATING_HPP

#include "pocketdb/models/base/PocketTypes.hpp"
#include "pocketdb/models/base/Base.hpp"

namespace PocketTx
{
    class Rating : public Base
    {
    public:

        Rating() {}
        ~Rating() {}

        shared_ptr<RatingType> GetType() const { return m_type; }
        shared_ptr<int> GetTypeInt() const { return make_shared<int>((int) *m_type); }
        void SetType(RatingType value) { m_type = make_shared<RatingType>(value); }

        shared_ptr <int> GetHeight() const { return m_height; }
        void SetHeight(int value) { m_height = make_shared<int>(value); }

        shared_ptr <string> GetHash() const { return m_hash; }
        void SetHash(string value) { m_hash = make_shared<string>(value); }

        shared_ptr <int64_t> GetValue() const { return m_value; }
        void SetValue(int64_t value) { m_value = make_shared<int64_t>(value); }

    protected:

        shared_ptr <RatingType> m_type = nullptr;
        shared_ptr <int> m_height = nullptr;
        shared_ptr <string> m_hash = nullptr;
        shared_ptr <int64_t> m_value = nullptr;

    private:

    };

} // namespace PocketTx

#endif // POCKETTX_RATING_HPP