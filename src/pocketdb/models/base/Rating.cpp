// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/Rating.h"

namespace PocketTx
{
    shared_ptr <RatingType> Rating::GetType() const { return m_type; }
    void Rating::SetType(RatingType value) { m_type = make_shared<RatingType>(value); }

    shared_ptr<int> Rating::GetHeight() const { return m_height; }
    void Rating::SetHeight(int value) { m_height = make_shared<int>(value); }

    shared_ptr <int64_t> Rating::GetId() const { return m_id; }
    void Rating::SetId(int64_t value) { m_id = make_shared<int64_t>(value); }

    shared_ptr <int64_t> Rating::GetValue() const { return m_value; }
    void Rating::SetValue(int64_t value) { m_value = make_shared<int64_t>(value); }
} // namespace PocketTx