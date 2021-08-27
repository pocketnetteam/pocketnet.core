// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKING_HPP
#define POCKETCONSENSUS_BLOCKING_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/Blocking.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<Blocking> BlockingRef;

    /*******************************************************************************************************************
    *  Blocking consensus base class
    *******************************************************************************************************************/
    class BlockingConsensus : public SocialConsensus<Blocking>
    {
    public:
        BlockingConsensus(int height) : SocialConsensus<Blocking>(height) {}
        ConsensusValidateResult Validate(const BlockingRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Double blocking in chain
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *ptx->GetAddress(),
                    *ptx->GetAddressTo()
                ); existsBlocking && blockingType == ACTION_BLOCKING)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_DoubleBlocking))
                    return {false, SocialConsensusResult_DoubleBlocking};
            }

            return Success;
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BlockingRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfBlocking};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const BlockingRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_BLOCKING, ACTION_BLOCKING_CANCEL}))
                    continue;

                auto blockPtx = static_pointer_cast<Blocking>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const BlockingRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolBlocking(*ptx->GetAddress(), *ptx->GetAddressTo()) > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const BlockingRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class BlockingConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < BlockingConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<BlockingConsensus>(height); }},
        };
    public:
        shared_ptr<BlockingConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<BlockingConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKING_HPP