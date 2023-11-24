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
    typedef shared_ptr<SubscribeCancel> SubscribeCancelRef;

    /*******************************************************************************************************************
    *  SubscribeCancel consensus base class
    *******************************************************************************************************************/
    class SubscribeCancelConsensus : public SocialConsensus<SubscribeCancel>
    {
    public:
        SubscribeCancelConsensus() : SocialConsensus<SubscribeCancel>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const SubscribeCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Last record not valid subscribe
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (!subscribeExists || subscribeType == ACTION_SUBSCRIBE_CANCEL)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_InvalideSubscribe))
                    return {false, ConsensusResult_InvalideSubscribe};
            }

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const SubscribeCancelRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, ConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, ConsensusResult_SelfSubscribe};

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
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleSubscribe))
                        return {false, ConsensusResult_DoubleSubscribe};
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
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const SubscribeCancelRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
    };

    class SubscribeCancelConsensus_checkpoint_pip104 : public SubscribeCancelConsensus
    {
    public:
        SubscribeCancelConsensus_checkpoint_pip104() : SubscribeCancelConsensus() {}
    protected:
        vector<string> GetAddressesForCheckRegistration(const SubscribeCancelRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }
    };


    class SubscribeCancelConsensusFactory : public BaseConsensusFactory<SubscribeCancelConsensus>
    {
    public:
        SubscribeCancelConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<SubscribeCancelConsensus>() });
            Checkpoint({ 2552000, 2340000,  0, make_shared<SubscribeCancelConsensus_checkpoint_pip104>() });
        }
    };

    static SubscribeCancelConsensusFactory ConsensusFactoryInst_SubscribeCancel;
}

#endif // POCKETCONSENSUS_SUBSCRIBECANCEL_HPP