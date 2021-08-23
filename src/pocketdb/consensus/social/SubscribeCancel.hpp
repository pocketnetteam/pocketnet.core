// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBECANCEL_HPP
#define POCKETCONSENSUS_SUBSCRIBECANCEL_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/SubscribeCancel.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<SubscribeCancel> SubscribeCancelRef;

    /*******************************************************************************************************************
    *  SubscribeCancel consensus base class
    *******************************************************************************************************************/
    class SubscribeCancelConsensus : public SocialConsensus<SubscribeCancelRef>
    {
    public:
        SubscribeCancelConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const SubscribeCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Last record not valid subscribe
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (!subscribeExists || subscribeType == ACTION_SUBSCRIBE_CANCEL)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_InvalideSubscribe))
                    return {false, SocialConsensusResult_InvalideSubscribe};
            }

            return Success;
        }

        ConsensusValidateResult Check(const SubscribeCancelRef& ptx) override
        {
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
                if (!IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockTx->GetString1() && *ptx->GetAddressTo() == *blockTx->GetString2())
                {
                    PocketHelpers::SocialCheckpoints socialCheckpoints;
                    if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_DoubleSubscribe))
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
    class SubscribeCancelConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0, 0, [](int height) { return make_shared<SubscribeCancelConsensus>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBECANCEL_HPP