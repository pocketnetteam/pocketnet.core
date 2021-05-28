// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBECANCEL_HPP
#define POCKETCONSENSUS_SUBSCRIBECANCEL_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  SubscribeCancel consensus base class
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus : public BaseConsensus
    {
    protected:
    public:
        SubscribeCancelConsensus(int height) : BaseConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus_checkpoint_0 : public SubscribeCancelConsensus
    {
    protected:
    public:

        SubscribeCancelConsensus_checkpoint_0(int height) : SubscribeCancelConsensus(height) {}

    }; // class SubscribeCancelConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus_checkpoint_1 : public SubscribeCancelConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        SubscribeCancelConsensus_checkpoint_1(int height) : SubscribeCancelConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<SubscribeCancelConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new SubscribeCancelConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new SubscribeCancelConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <SubscribeCancelConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<SubscribeCancelConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBECANCEL_HPP