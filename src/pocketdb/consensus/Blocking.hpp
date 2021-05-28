// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKING_HPP
#define POCKETCONSENSUS_BLOCKING_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
        *
        *  Blocking consensus base class
        *
        *******************************************************************************************************************/
    class BlockingConsensus : public BaseConsensus
    {
    protected:
    public:
        BlockingConsensus(int height) : BaseConsensus(height) {}
    };


    /*******************************************************************************************************************
        *
        *  Start checkpoint
        *
        *******************************************************************************************************************/
    class BlockingConsensus_checkpoint_0 : public BlockingConsensus
    {
    protected:
    public:
        BlockingConsensus_checkpoint_0(int height) : BlockingConsensus(height) {}
    }; // class BlockingConsensus_checkpoint_0


    /*******************************************************************************************************************
        *
        *  Consensus checkpoint at 1 block
        *
        *******************************************************************************************************************/
    class BlockingConsensus_checkpoint_1 : public BlockingConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }

    public:
        BlockingConsensus_checkpoint_1(int height) : BlockingConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
        *
        *  Factory for select actual rules version
        *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
        *
        *******************************************************************************************************************/
    class BlockingConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<BlockingConsensus*(int height)>>> m_rules{
            {1, [](int height) { return new BlockingConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new BlockingConsensus_checkpoint_0(height); }},
        };

    public:
        shared_ptr <BlockingConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<BlockingConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKING_HPP