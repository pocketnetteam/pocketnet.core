// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKINGCANCEL_HPP
#define POCKETCONSENSUS_BLOCKINGCANCEL_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  BlockingCancel consensus base class
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensus : public BaseConsensus
    {
    protected:
    public:
        BlockingCancelConsensus() = default;
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensus_checkpoint_0 : public BlockingCancelConsensus
    {
    protected:
    public:

        BlockingCancelConsensus_checkpoint_0() = default;

    }; // class BlockingCancelConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensus_checkpoint_1 : public BlockingCancelConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<BlockingCancelConsensus *()>>> m_rules
        {
            {1, []() { return new BlockingCancelConsensus_checkpoint_1(); }},
            {0, []() { return new BlockingCancelConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <BlockingCancelConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<BlockingCancelConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKINGCANCEL_HPP