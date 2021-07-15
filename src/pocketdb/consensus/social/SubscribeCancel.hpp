// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBECANCEL_HPP
#define POCKETCONSENSUS_SUBSCRIBECANCEL_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/SubscribeCancel.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  SubscribeCancel consensus base class
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus : public SocialBaseConsensus
    {
    public:
        SubscribeCancelConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<SubscribeCancel>(tx);

            // Check registrations
            vector<string> addresses = {*ptx->GetAddress(), *ptx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            // Last record not valid subscribe
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (!subscribeExists || subscribeType == ACTION_SUBSCRIBE_CANCEL)
                return {false, SocialConsensusResult_InvalideSubscribe};

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<SubscribeCancel>(tx);

            // Only one transaction (address -> addressTo) allowed in block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<SubscribeCancel>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, SocialConsensusResult_DoubleSubscribe};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<SubscribeCancel>(tx);

            int mempoolCount = ConsensusRepoInst.CountMempoolSubscribe(
                *ptx->GetAddress(),
                *ptx->GetAddressTo()
            );

            if (mempoolCount > 0)
                return {false, SocialConsensusResult_DoubleSubscribe};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<SubscribeCancel>(tx);

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
    class SubscribeCancelConsensus_checkpoint_1 : public SubscribeCancelConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        SubscribeCancelConsensus_checkpoint_1(int height) : SubscribeCancelConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<SubscribeCancelConsensus*(int height)>> m_rules =
            {
                {1, [](int height) { return new SubscribeCancelConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new SubscribeCancelConsensus(height); }},
            };
    public:
        shared_ptr <SubscribeCancelConsensus> Instance(int height)
        {
            return shared_ptr<SubscribeCancelConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBECANCEL_HPP