// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP
#define POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  SubscribePrivate consensus base class
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus : public SocialBaseConsensus
    {
    public:
        SubscribePrivateConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<SubscribePrivate>(tx);

            vector<string> addresses = {*ptx->GetAddress(), *ptx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            // Check double blocking
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE_PRIVATE)
            {
                // TODO (brangr): чекпойнты сюда
                if (!IsCheckpointTransaction(*tx->GetHash()))
                    return {false, SocialConsensusResult_DoubleBlocking};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<SubscribePrivate>(tx);

            // Only one transaction (address -> addressTo) allowed in block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<SubscribePrivate>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<SubscribePrivate>(tx);

            int mempoolCount = ConsensusRepoInst.CountMempoolSubscribe(
                *ptx->GetAddress(),
                *ptx->GetAddressTo()
            );

            if (mempoolCount > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<SubscribePrivate>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfSubscribe};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus_checkpoint_1 : public SubscribePrivateConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        SubscribePrivateConsensus_checkpoint_1(int height) : SubscribePrivateConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<SubscribePrivateConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new SubscribePrivateConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new SubscribePrivateConsensus(height); }},
        };
    public:
        shared_ptr <SubscribePrivateConsensus> Instance(int height)
        {
            return shared_ptr<SubscribePrivateConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP