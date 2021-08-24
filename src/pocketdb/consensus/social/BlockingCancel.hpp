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
    /*******************************************************************************************************************
    *  BlockingCancel consensus base class
    *******************************************************************************************************************/
    class BlockingCancelConsensus : public SocialConsensus
    {
    public:
        BlockingCancelConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<BlockingCancel>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *_ptx->GetAddress(),
                    *_ptx->GetAddressTo()
                ); !existsBlocking || blockingType != ACTION_BLOCKING)
                return {false, SocialConsensusResult_InvalidBlocking};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<BlockingCancel>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*_ptx->GetAddress() == *_ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfBlocking};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<BlockingCancel>(ptx);

            // Only one transaction (address -> addressTo) allowed in block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_BLOCKING, ACTION_BLOCKING_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<BlockingCancel>(blockTx);
                if (*_ptx->GetAddress() == *blockPtx->GetAddress() && *_ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<BlockingCancel>(ptx);
            if (ConsensusRepoInst.CountMempoolBlocking(*_ptx->GetAddress(), *_ptx->GetAddressTo()) > 0)
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<BlockingCancel>(ptx);
            return {*_ptx->GetAddress(), *_ptx->GetAddressTo()};
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class BlockingCancelConsensusFactory : public BaseConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0, 0, [](int height) { return make_shared<BlockingCancelConsensus>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    public:
        shared_ptr<BlockingCancelConsensus> Instance(int height)
        {
            return static_pointer_cast<BlockingCancelConsensus>(
                BaseConsensusFactory::m_instance(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKINGCANCEL_HPP