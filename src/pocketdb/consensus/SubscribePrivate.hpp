// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP
#define POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  SubscribePrivate consensus base class
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus : public BaseConsensus
    {
    protected:
    public:
        SubscribePrivateConsensus() = default;
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus_checkpoint_0 : public SubscribePrivateConsensus
    {
    protected:
    public:

        SubscribePrivateConsensus_checkpoint_0() = default;

    }; // class SubscribePrivateConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus_checkpoint_1 : public SubscribePrivateConsensus_checkpoint_0
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
    class SubscribePrivateConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<SubscribePrivateConsensus *()>>> m_rules
        {
            {1, []() { return new SubscribePrivateConsensus_checkpoint_1(); }},
            {0, []() { return new SubscribePrivateConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <SubscribePrivateConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<SubscribePrivateConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP