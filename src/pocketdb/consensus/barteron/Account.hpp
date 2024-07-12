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
            // Get all the necessary data for transaction validation
            consensusData = ConsensusRepoInst.BarteronAccount(
                *ptx->GetAddress()
            );

            return SocialConsensus::Validate(tx, ptx, block);
        }

    protected:
        ConsensusData_BarteronAccount consensusData;
    
        ConsensusValidateResult ValidateBlock(const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            auto blockPtxs = SocialConsensus::ExtractBlockPtxs(block, ptx, { BARTERON_ACCOUNT });
            if (blockPtxs.size() > 0)
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        
        ConsensusValidateResult ValidateMempool(const BarteronAccountRef& ptx) override
        {
            return { consensusData.MempoolCount <= 0, ConsensusResult_ManyTransactions };
        }

    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class BarteronAccountConsensusFactory : public BaseConsensusFactory<BarteronAccountConsensus>
    {
    public:
        BarteronAccountConsensusFactory()
        {
            Checkpoint({ 2930000, 0, 0, make_shared<BarteronAccountConsensus>() });
        }
    };

    static BarteronAccountConsensusFactory ConsensusFactoryInst_BarteronAccount;
}

#endif // POCKETCONSENSUS_BARTERON_ACCOUNT_HPP
