// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMPLAIN_HPP
#define POCKETCONSENSUS_COMPLAIN_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Complain consensus base class
    *
    *******************************************************************************************************************/
    class ComplainConsensus : public BaseConsensus
    {
    protected:
    public:
        ComplainConsensus() = default;
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_0 : public ComplainConsensus
    {
    protected:
    public:

        ComplainConsensus_checkpoint_0() = default;

    }; // class ComplainConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_1 : public ComplainConsensus_checkpoint_0
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
    class ComplainConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<ComplainConsensus *()>>> m_rules
        {
            {1, []() { return new ComplainConsensus_checkpoint_1(); }},
            {0, []() { return new ComplainConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <ComplainConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<ComplainConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_COMPLAIN_HPP