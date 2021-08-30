// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Base.h"

namespace PocketConsensus
{
    BaseConsensus::BaseConsensus() = default;

    BaseConsensus::BaseConsensus(int height) : BaseConsensus()
    {
        Height = height;
    }

    int64_t BaseConsensus::GetConsensusLimit(ConsensusLimit type) const
    {
        return (--m_consensus_limits[type][Params().NetworkID()].upper_bound(Height))->second;
    }
}