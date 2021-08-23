// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKINGCANCEL_HPP
#define POCKETCONSENSUS_BLOCKINGCANCEL_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<BlockingCancel> BlockingCancelRef;
    /*******************************************************************************************************************
    *  BlockingCancel consensus base class
    *******************************************************************************************************************/
    class BlockingCancelConsensus : public SocialConsensus<BlockingCancelRef>
    {
    public:
        BlockingCancelConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const BlockingCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *ptx->GetAddress(),
                    *ptx->GetAddressTo()
                ); !existsBlocking || blockingType != ACTION_BLOCKING)
                return {false, SocialConsensusResult_InvalidBlocking};

            return Success;
        }

        ConsensusValidateResult Check(const BlockingCancelRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(ptx); !baseCheck)
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

        ConsensusValidateResult ValidateBlock(const BlockingCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : *block)
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

        ConsensusValidateResult ValidateMempool(const BlockingCancelRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolBlocking(*ptx->GetAddress(), *ptx->GetAddressTo()) > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const BlockingCancelRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0, 0, [](int height) { return make_shared<BlockingCancelConsensus>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_BLOCKINGCANCEL_HPP