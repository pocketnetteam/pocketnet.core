// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BARTERON_OFFER_HPP
#define POCKETCONSENSUS_BARTERON_OFFER_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/barteron/Offer.h"

namespace PocketConsensus
{
    typedef shared_ptr<BarteronOffer> BarteronOfferRef;

    /*******************************************************************************************************************
    *  BarteronOffer consensus base class
    *******************************************************************************************************************/
    class BarteronOfferConsensus : public SocialConsensus<BarteronOffer>
    {
    public:
        BarteronOfferConsensus() : SocialConsensus<BarteronOffer>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            if (auto[ok, code] = SocialConsensus::Validate(tx, ptx, block); !ok)
                return {false, code};

            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            // TODO (barteron):
            // if (ptx->IsEdit())
            //     return ValidateEdit(ptx);

            // TODO (barteron): max count active offers

            return Success;
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronOfferRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // TODO (barteron): checks

            return Success;
        }

    protected:
    
        ConsensusValidateResult ValidateBlock(const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // TODO (barteron): implement

            // int count = GetChainCount(ptx);

            // // Get count from block
            // for (auto& blockTx : *block)
            // {
            //     if (!TransactionHelper::IsIn(*blockTx->GetType(), {BARTERON_OFFER}))
            //         continue;

            //     auto blockPtx = static_pointer_cast<BarteronOffer>(blockTx);

            //     if (*ptx->GetAddress() != *blockPtx->GetAddress())
            //         continue;

            //     if (*blockPtx->GetHash() == *ptx->GetHash())
            //         continue;

            //     if (blockPtx->IsEdit())
            //         continue;

            //     count += 1;
            // }

            // return ValidateLimit(ptx, count);

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const BarteronOfferRef& ptx) override
        {
            // TODO (barteron): implement

            // // Get count from chain
            // int count = GetChainCount(ptx);

            // // and from mempool
            // count += ConsensusRepoInst.CountMempoolBarteronOffer(*ptx->GetAddress());

            // return ValidateLimit(ptx, count);

            return Success;
        }

    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class BarteronOfferConsensusFactory : public BaseConsensusFactory<BarteronOfferConsensus>
    {
    public:
        BarteronOfferConsensusFactory()
        {
            // TODO (release): set height
            Checkpoint({ 99999999, 99999999, 0, make_shared<BarteronOfferConsensus>() });
        }
    };

    static BarteronOfferConsensusFactory ConsensusFactoryInst_BarteronOffer;
}

#endif // POCKETCONSENSUS_BARTERON_OFFER_HPP
