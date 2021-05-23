// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBE_HPP
#define POCKETCONSENSUS_SUBSCRIBE_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Subscribe consensus base class
    *
    *******************************************************************************************************************/
    class SubscribeConsensus : public BaseConsensus
    {
    protected:
    public:
        SubscribeConsensus() = default;
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class SubscribeConsensus_checkpoint_0 : public SubscribeConsensus
    {
    protected:
    public:

        SubscribeConsensus_checkpoint_0() = default;

    }; // class SubscribeConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribeConsensus_checkpoint_1 : public SubscribeConsensus_checkpoint_0
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
    class SubscribeConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<SubscribeConsensus *()>>> m_rules
        {
            {1, []() { return new SubscribeConsensus_checkpoint_1(); }},
            {0, []() { return new SubscribeConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <SubscribeConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<SubscribeConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBE_HPP