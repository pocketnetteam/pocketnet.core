// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP
#define POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  SubscribePrivate consensus base class
    *******************************************************************************************************************/
    class SubscribePrivateConsensus : public SocialConsensus
    {
    public:
        SubscribePrivateConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<SubscribePrivate>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Check double subscribe
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *_ptx->GetAddress(),
                *_ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE_PRIVATE)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_DoubleSubscribe))
                    return {false, SocialConsensusResult_DoubleSubscribe};
            }

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<SubscribePrivate>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*_ptx->GetAddress() == *_ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfSubscribe};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<SubscribePrivate>(ptx);

            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *_ptx->GetHash())
                    continue;

                if (*_ptx->GetAddress() == *blockTx->GetString1() && *_ptx->GetAddressTo() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleSubscribe};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<SubscribePrivate>(ptx);

            int mempoolCount = ConsensusRepoInst.CountMempoolSubscribe(
                *_ptx->GetAddress(),
                *_ptx->GetAddressTo()
            );

            if (mempoolCount > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<SubscribePrivate>(ptx);
            return {*_ptx->GetAddress(), *_ptx->GetAddressTo()};
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class SubscribePrivateConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0, 0, [](int height) { return make_shared<SubscribePrivateConsensus>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP