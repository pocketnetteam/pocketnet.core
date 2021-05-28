// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Comment consensus base class
    *
    *******************************************************************************************************************/
    class CommentConsensus : public BaseConsensus
    {
    protected:
    public:
        CommentConsensus(int height) : BaseConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_0 : public CommentConsensus
    {
    protected:
    public:

        CommentConsensus_checkpoint_0(int height) : CommentConsensus(height) {}

    }; // class CommentConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1 : public CommentConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        CommentConsensus_checkpoint_1(int height) : CommentConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class CommentConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<CommentConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new CommentConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new CommentConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <CommentConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<CommentConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP