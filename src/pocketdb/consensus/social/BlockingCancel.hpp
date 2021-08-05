// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKINGCANCEL_HPP
#define POCKETCONSENSUS_BLOCKINGCANCEL_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  BlockingCancel consensus base class
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensus : public SocialBaseConsensus
    {
    public:
        BlockingCancelConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);

            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (!existsBlocking || blockingType != ACTION_BLOCKING)
                return {false, SocialConsensusResult_InvalidBlocking};

            return Success;
        }

        tuple<bool, SocialConsensusResult>
        ValidateLimit(const PTransactionRef& tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);

            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_BLOCKING, ACTION_BLOCKING_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<BlockingCancel>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);

            if (ConsensusRepoInst.CountMempoolBlocking(*ptx->GetAddress(), *ptx->GetAddressTo()) > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfBlocking};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<BlockingCancelConsensus*(int height)>> m_rules =
            {
                {0, [](int height) { return new BlockingCancelConsensus(height); }},
            };
    public:
        shared_ptr <BlockingCancelConsensus> Instance(int height)
        {
            return shared_ptr<BlockingCancelConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKINGCANCEL_HPP