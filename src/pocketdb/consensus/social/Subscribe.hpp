// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBE_HPP
#define POCKETCONSENSUS_SUBSCRIBE_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<Subscribe> SubscribeRef;

    /*******************************************************************************************************************
    *  Subscribe consensus base class
    *******************************************************************************************************************/
    class SubscribeConsensus : public SocialConsensus<SubscribeRef>
    {
    public:
        SubscribeConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const SubscribeRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_DoubleSubscribe))
                    return {false, SocialConsensusResult_DoubleSubscribe};
            }

            return Success;
        }

        ConsensusValidateResult Check(const SubscribeRef& ptx) override
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

        ConsensusValidateResult ValidateBlock(const SubscribeRef& ptx, const PocketBlockRef& block) override
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
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class SubscribeConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0, 0, [](int height) { return make_shared<SubscribeConsensus>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBE_HPP