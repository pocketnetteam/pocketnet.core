// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  User consensus base class
    *
    *******************************************************************************************************************/
    class UserConsensus : public BaseConsensus
    {
    protected:
    public:
        UserConsensus() = default;
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_0 : public UserConsensus
    {
    protected:
    public:

        UserConsensus_checkpoint_0() = default;

    }; // class UserConsensus_checkpoint_0

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1 : public UserConsensus_checkpoint_0
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
    class UserConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<UserConsensus *()>>> m_rules
        {
            {1, []() { return new UserConsensus_checkpoint_1(); }},
            {0, []() { return new UserConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <UserConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<UserConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_USER_HPP
