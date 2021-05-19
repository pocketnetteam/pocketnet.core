// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "streams.h"
#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/services/TransactionSerializer.hpp"

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
    *  Lottery start checkpoint
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
    *  User consensus checkpoint at ... block
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
    *  Lottery factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class UserConsensusFactory
    {
    private:
    public:
        shared_ptr <UserConsensus> Instance(int height)
        {
            // TODO (brangr): достать подходящий чекпойнт реализацию
            return nullptr;
        }
    };

}

#endif // POCKETCONSENSUS_USER_HPP
