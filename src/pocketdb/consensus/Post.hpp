// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public BaseConsensus
    {
    protected:
    public:
        PostConsensus(int height) : BaseConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_0 : public PostConsensus
    {
    protected:
    public:

        PostConsensus_checkpoint_0(int height) : PostConsensus(height) {}

    }; // class PostConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1 : public PostConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        PostConsensus_checkpoint_1(int height) : PostConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class PostConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<PostConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new PostConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new PostConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <PostConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<PostConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_POST_HPP