// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBE_HPP
#define POCKETCONSENSUS_SUBSCRIBE_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/models/dto/action/Subscribe.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<Subscribe> SubscribeRef;

    /*******************************************************************************************************************
    *  Subscribe consensus base class
    *******************************************************************************************************************/
    class SubscribeConsensus : public SocialConsensus<Subscribe>
    {
    public:
        SubscribeConsensus(int height) : SocialConsensus<Subscribe>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const SubscribeRef& ptx, const PocketBlockRef& block) override
        {
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_DoubleSubscribe))
                    return {false, SocialConsensusResult_DoubleSubscribe};
            }

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(ptx); !ok)
                return {false, result};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const SubscribeRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfSubscribe};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const SubscribeRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                auto blockPtx = static_pointer_cast<Subscribe>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                {
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_DoubleSubscribe))
                        return {false, SocialConsensusResult_DoubleSubscribe};
                }
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const SubscribeRef& ptx) override
        {
            int mempoolCount = ConsensusRepoInst.CountMempoolSubscribe(
                *ptx->GetAddress(),
                *ptx->GetAddressTo()
            );

            if (mempoolCount > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const SubscribeRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
        virtual ConsensusValidateResult ValidateBlocking(const SubscribeRef& ptx)
        {
            return Success;
        }
    };

    // Disable scores for blocked accounts
    class SubscribeConsensus_checkpoint_disable_for_blocked : public SubscribeConsensus
    {
    public:
        SubscribeConsensus_checkpoint_disable_for_blocked(int height) : SubscribeConsensus(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const SubscribeRef& ptx) override
        {
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                *ptx->GetAddressTo(), *ptx->GetAddress()); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class SubscribeConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < SubscribeConsensus>> m_rules = {
            {       0,      0, [](int height) { return make_shared<SubscribeConsensus>(height); }},
            { 1757000, 953000, [](int height) { return make_shared<SubscribeConsensus_checkpoint_disable_for_blocked>(height); }},
        };
    public:
        shared_ptr<SubscribeConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<SubscribeConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBE_HPP