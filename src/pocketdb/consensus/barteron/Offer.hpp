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
            Limits.Set("payload_size", 60000, 30000, 1024);
            Limits.Set("max_active_count", 30, 50, 5000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            consensusData = ConsensusRepoInst.BarteronOffer(
                *ptx->GetAddress(),
                *ptx->GetRootTxHash()
            );

            if (auto[ok, code] = SocialConsensus::Validate(tx, ptx, block); !ok)
                return {false, code};

            // Get all the necessary data for transaction validation
            // Validate new or edited transaction
            if (ptx->IsEdit())
                ValidateEdit(ptx);
            else
                ValidateNew(ptx);

            if (ResultCode != ConsensusResult_Success) return {false, ResultCode}; // TODO (aok): remove when all consensus classes support Result
            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronOfferRef& ptx) override
        {
            SocialConsensus::Check(tx, ptx);

            // Payload must be exists and not empty
            Result(ConsensusResult_Failed, [&]()
            {
                return !ptx->GetPayload();
            });

            // RootTxHash must not empty
            Result(ConsensusResult_Failed, [&]()
            {
                return !ptx->GetRootTxHash();
            });
             
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode}; // TODO (aok): remove when all consensus classes support Result
            return Success;
        }

    protected:
        ConsensusData_BarteronOffer consensusData;

        void ValidateNew(const BarteronOfferRef& ptx)
        {
            // Allowed only N active offers per account
            Result(ConsensusResult_ExceededLimit, [&]() {
                return consensusData.ActiveCount >= Limits.Get("max_active_count");
            });
        }

        void ValidateEdit(const BarteronOfferRef& ptx)
        {
            // Edited transaction must be same type and not deleted
            Result(ConsensusResult_ExceededLimit, [&]() {
                return (TxType)consensusData.LastTxType != BARTERON_OFFER;
            });
        }
    
        ConsensusValidateResult ValidateBlock(const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction change barteron offer allowed in block
            Result(ConsensusResult_ManyTransactions, [&]() {
                auto blockPtxs = SocialConsensus::ExtractBlockPtxs(block, ptx, { BARTERON_OFFER });
                return blockPtxs.size() > 0;
            });

            return { ResultCode == ConsensusResult_Success, ResultCode }; // TODO (aok): remove when all consensus classes support Result
        }

        ConsensusValidateResult ValidateMempool(const BarteronOfferRef& ptx) override
        {
            // Only one transaction change barteron offer allowed in mempool
            Result(ConsensusResult_ManyTransactions, [&]() {
                return consensusData.MempoolCount > 0;
            });

            return { ResultCode == ConsensusResult_Success, ResultCode }; // TODO (aok): remove when all consensus classes support Result
        }

    };

    // Factory for select actual rules version
    class BarteronOfferConsensusFactory : public BaseConsensusFactory<BarteronOfferConsensus>
    {
    public:
        BarteronOfferConsensusFactory()
        {
            Checkpoint({ 2930000, 0, 0, make_shared<BarteronOfferConsensus>() });
        }
    };

    static BarteronOfferConsensusFactory ConsensusFactoryInst_BarteronOffer;
}

#endif // POCKETCONSENSUS_BARTERON_OFFER_HPP
