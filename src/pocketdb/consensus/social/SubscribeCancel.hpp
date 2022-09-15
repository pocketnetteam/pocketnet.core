// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBECANCEL_HPP
#define POCKETCONSENSUS_SUBSCRIBECANCEL_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/models/dto/action/SubscribeCancel.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<SubscribeCancel> SubscribeCancelRef;

    /*******************************************************************************************************************
    *  SubscribeCancel consensus base class
    *******************************************************************************************************************/
    class SubscribeCancelConsensus : public SocialConsensus<SubscribeCancel>
    {
    public:
        SubscribeCancelConsensus(int height) : SocialConsensus<SubscribeCancel>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const SubscribeCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Last record not valid subscribe
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (!subscribeExists || subscribeType == ACTION_SUBSCRIBE_CANCEL)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_InvalideSubscribe))
                    return {false, SocialConsensusResult_InvalideSubscribe};
            }

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const SubscribeCancelRef& ptx) override
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
        ConsensusValidateResult ValidateBlock(const SubscribeCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                auto blockPtx = static_pointer_cast<SubscribeCancel>(blockTx);

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
        ConsensusValidateResult ValidateMempool(const SubscribeCancelRef& ptx) override
        {
            int mempoolCount = ConsensusRepoInst.CountMempoolSubscribe(
                *ptx->GetAddress(),
                *ptx->GetAddressTo()
            );

            if (mempoolCount > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const SubscribeCancelRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class SubscribeCancelConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<SubscribeCancelConsensus>> m_rules = {
            {0, 0, [](int height) { return make_shared<SubscribeCancelConsensus>(height); }},
        };
    public:
        shared_ptr<SubscribeCancelConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<SubscribeCancelConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBECANCEL_HPP