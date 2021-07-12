// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBE_HPP
#define POCKETCONSENSUS_SUBSCRIBE_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Subscribe consensus base class
    *
    *******************************************************************************************************************/
    class SubscribeConsensus : public SocialBaseConsensus
    {
    public:
        SubscribeConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Subscribe>(tx);

            // Check registration
            vector<string> addresses = {*ptx->GetAddress(), *ptx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE)
            {
                // TODO (brangr): я думаю все чекпойнты прямо тут и сложить
                if (!IsCheckpointTransaction(*tx->GetHash()))
                    return {false, SocialConsensusResult_DoubleBlocking};
                else
                    LogPrintf("Found checkpoint transaction %s\n", *tx->GetHash());
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Subscribe>(tx);

            // Only one transaction (address -> addressTo) allowed in block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE, ACTION_SUBSCRIBE_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Subscribe>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Subscribe>(tx);

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
            auto ptx = static_pointer_cast<Subscribe>(tx);

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
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class SubscribeConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<SubscribeConsensus*(int height)>> m_rules =
            {
                {0, [](int height) { return new SubscribeConsensus(height); }},
            };
    public:
        shared_ptr <SubscribeConsensus> Instance(int height)
        {
            return shared_ptr<SubscribeConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBE_HPP