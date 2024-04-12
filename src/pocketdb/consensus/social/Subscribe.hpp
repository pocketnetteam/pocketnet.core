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
    typedef shared_ptr<Subscribe> SubscribeRef;

    /*******************************************************************************************************************
    *  Subscribe consensus base class
    *******************************************************************************************************************/
    class SubscribeConsensus : public SocialConsensus<Subscribe>
    {
    public:
        SubscribeConsensus() : SocialConsensus<Subscribe>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const SubscribeRef& ptx, const PocketBlockRef& block) override
        {
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleSubscribe))
                    return {false, ConsensusResult_DoubleSubscribe};
            }

            // Check Blocking
            if (ValidateBlocking(ptx))
                return {false, ConsensusResult_Blocking};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const SubscribeRef& ptx) override
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
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleSubscribe))
                        return {false, ConsensusResult_DoubleSubscribe};
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
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const SubscribeRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
        virtual bool ValidateBlocking(const SubscribeRef& ptx)
        {
            return false;
        }
    };

    // Disable scores for blocked accounts
    class SubscribeConsensus_checkpoint_disable_for_blocked : public SubscribeConsensus
    {
    public:
        SubscribeConsensus_checkpoint_disable_for_blocked() : SubscribeConsensus() {}
    protected:
        bool ValidateBlocking(const SubscribeRef& ptx) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(*ptx->GetAddressTo(), *ptx->GetAddress());
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
    };

    // Disable scores for blocked accounts in any direction
    class SubscribeConsensus_checkpoint_pip_105 : public SubscribeConsensus_checkpoint_disable_for_blocked
    {
    public:
        SubscribeConsensus_checkpoint_pip_105() : SubscribeConsensus_checkpoint_disable_for_blocked() {}
    protected:
        bool ValidateBlocking(const SubscribeRef& ptx) override
        {
            return SocialConsensus::CheckBlocking(*ptx->GetAddressTo(), *ptx->GetAddress());
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class SubscribeConsensusFactory : public BaseConsensusFactory<SubscribeConsensus>
    {
    public:
        SubscribeConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<SubscribeConsensus>() });
            Checkpoint({ 1757000,  953000, -1, make_shared<SubscribeConsensus_checkpoint_disable_for_blocked>() });
            Checkpoint({ 2794500, 2574300,  0, make_shared<SubscribeConsensus_checkpoint_pip_105>() });
        }
    };

    static SubscribeConsensusFactory ConsensusFactoryInst_Subscribe;
}

#endif // POCKETCONSENSUS_SUBSCRIBE_HPP