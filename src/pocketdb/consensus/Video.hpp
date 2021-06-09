// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/pocketnet.h"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  User consensus base class
    *
    *******************************************************************************************************************/
    class VideoConsensus : public BaseConsensus
    {
    protected:
    public:
        VideoConsensus(int height) : BaseConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class VideoConsensus_checkpoint_0 : public VideoConsensus
    {
    protected:
    public:
        VideoConsensus_checkpoint_0(int height) : VideoConsensus(height) {}
    }; // class VideoConsensus_checkpoint_0

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class VideoConsensus_checkpoint_1 : public VideoConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        VideoConsensus_checkpoint_1(int height) : VideoConsensus_checkpoint_0(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class VideoConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<VideoConsensus*(int height)>>> m_rules{
            {1, [](int height) { return new VideoConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new VideoConsensus_checkpoint_0(height); }},
        };

    public:
        shared_ptr<VideoConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<VideoConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_USER_HPP
