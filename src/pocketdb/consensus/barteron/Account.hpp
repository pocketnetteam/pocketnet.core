// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BARTERON_ACCOUNT_HPP
#define POCKETCONSENSUS_BARTERON_ACCOUNT_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/barteron/Account.h"

namespace PocketConsensus
{
    typedef shared_ptr<BarteronAccount> BarteronAccountRef;

    /*******************************************************************************************************************
    *  BarteronAccount consensus base class
    *******************************************************************************************************************/
    class BarteronAccountConsensus : public SocialConsensus<BarteronAccount>
    {
    public:
        BarteronAccountConsensus() : SocialConsensus<BarteronAccount>()
        {
            Limits.Set("payload_size", 2048, 2048, 2048);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            if (auto[ok, code] = SocialConsensus::Validate(tx, ptx, block); !ok)
                return {false, code};

            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};
            
            // TODO (barteron): implement validate lists size

            return Success;
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronAccountRef& ptx) override
        {
            if (auto[ok, code] = SocialConsensus::Check(tx, ptx); !ok)
                return {false, code};

            // Add or Del tags list must be exists
            if (IsEmpty(ptx->GetPayloadTagsAdd()) && IsEmpty(ptx->GetPayloadTagsDel()))
                return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
    
        ConsensusValidateResult ValidateBlock(const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            // TODO (barteron): implement

            // Get count from chain
            // int count = GetChainCount(ptx);

            // // Get count from block
            // for (auto& blockTx : *block)
            // {
            //     if (!TransactionHelper::IsIn(*blockTx->GetType(), {BARTERON_ACCOUNT}))
            //         continue;

            //     auto blockPtx = static_pointer_cast<BarteronAccount>(blockTx);

            //     if (*ptx->GetAddress() != *blockPtx->GetAddress())
            //         continue;

            //     if (*blockPtx->GetHash() == *ptx->GetHash())
            //         continue;

            //     // if (blockPtx->IsEdit())
            //     //     continue;

            //     count += 1;
            // }

            // return ValidateLimit(ptx, count);

            return Success;
        }
        
        ConsensusValidateResult ValidateMempool(const BarteronAccountRef& ptx) override
        {
            // TODO (barteron): implement
            
            // Get count from chain
            // int count = GetChainCount(ptx);

            // // and from mempool
            // count += ConsensusRepoInst.CountMempoolBarteronAccount(*ptx->GetAddress());

            // return ValidateLimit(ptx, count);

            return Success;
        }
        
        size_t CollectStringsSize(const BarteronAccountRef& ptx) override
        {
            return SocialConsensus::CollectStringsSize(ptx) -
                (ptx->GetPayloadTagsAdd() ? ptx->GetPayloadTagsAdd()->size() : 0) -
                (ptx->GetPayloadTagsDel() ? ptx->GetPayloadTagsDel()->size() : 0);
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class BarteronAccountConsensusFactory : public BaseConsensusFactory<BarteronAccountConsensus>
    {
    public:
        BarteronAccountConsensusFactory()
        {
            // TODO (release): set height
            Checkpoint({ 99999999, 99999999, 0, make_shared<BarteronAccountConsensus>() });
        }
    };

    static BarteronAccountConsensusFactory ConsensusFactoryInst_BarteronAccount;
}

#endif // POCKETCONSENSUS_BARTERON_ACCOUNT_HPP
