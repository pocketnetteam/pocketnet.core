// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP
#define POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/action/SubscribePrivate.h"

namespace PocketConsensus
{
    typedef shared_ptr<SubscribePrivate> SubscribePrivateRef;

    /*******************************************************************************************************************
    *  SubscribePrivate consensus base class
    *******************************************************************************************************************/
    class SubscribePrivateConsensus : public SocialConsensus<SubscribePrivate>
    {
    public:
        SubscribePrivateConsensus() : SocialConsensus<SubscribePrivate>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const SubscribePrivateRef& ptx, const PocketBlockRef& block) override
        {
            // Check double subscribe
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE_PRIVATE)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleSubscribe))
                    return {false, ConsensusResult_DoubleSubscribe};
            }

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(ptx); !ok)
                return {false, result};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const SubscribePrivateRef& ptx) override
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
        ConsensusValidateResult ValidateBlock(const SubscribePrivateRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                auto blockPtx = static_pointer_cast<SubscribePrivate>(blockTx);

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
        ConsensusValidateResult ValidateMempool(const SubscribePrivateRef& ptx) override
        {
            int mempoolCount = ConsensusRepoInst.CountMempoolSubscribe(
                *ptx->GetAddress(),
                *ptx->GetAddressTo()
            );

            if (mempoolCount > 0)
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const SubscribePrivateRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
        virtual ConsensusValidateResult ValidateBlocking(const SubscribePrivateRef& ptx)
        {
            return Success;
        }
    };

    // Disable scores for blocked accounts
    class SubscribePrivateConsensus_checkpoint_disable_for_blocked : public SubscribePrivateConsensus
    {
    public:
        SubscribePrivateConsensus_checkpoint_disable_for_blocked() : SubscribePrivateConsensus() {}

    protected:
        ConsensusValidateResult ValidateBlocking(const SubscribePrivateRef& ptx) override
        {
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                *ptx->GetAddressTo(), *ptx->GetAddress()); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, ConsensusResult_Blocking};

            return Success;
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class SubscribePrivateConsensusFactory : public BaseConsensusFactory<SubscribePrivateConsensus>
    {
    public:
        SubscribePrivateConsensusFactory()
        {
            Checkpoint({       0,      0, -1, make_shared<SubscribePrivateConsensus>() });
            Checkpoint({ 1757000, 953000,  0, make_shared<SubscribePrivateConsensus_checkpoint_disable_for_blocked>() });
        }
    };

    static SubscribePrivateConsensusFactory ConsensusFactoryInst_SubscribePrivate;
}

#endif // POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP