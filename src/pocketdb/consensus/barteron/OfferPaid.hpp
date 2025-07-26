// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BARTERON_OFFER_PAID_HPP
#define POCKETCONSENSUS_BARTERON_OFFER_PAID_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/barteron/OfferPaid.h"

namespace PocketConsensus
{
    typedef shared_ptr<BarteronOfferPaid> BarteronOfferPaidRef;

    /*******************************************************************************************************************
    *  BarteronOfferPaid consensus base class
    *******************************************************************************************************************/
    class BarteronOfferPaidConsensus : public SocialConsensus<BarteronOfferPaid>
    {
    public:
        BarteronOfferPaidConsensus() : SocialConsensus<BarteronOfferPaid>()
        {
            Limits.Set("payload_size", 60000, 30000, 1024);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronOfferPaidRef& ptx, const PocketBlockRef& block) override
        {
            consensusData = GetConsensusData(ptx);

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

        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronOfferPaidRef& ptx) override
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

        virtual ConsensusData_BarteronOffer GetConsensusData(const BarteronOfferPaidRef& ptx)
        {
            return ConsensusRepoInst.BarteronOfferFixed(
                *ptx->GetAddress(),
                *ptx->GetRootTxHash()
            );
        }

        void ValidateNew(const BarteronOfferPaidRef& ptx)
        {
            // Limits are not used for paid offers
        }

        void ValidateEdit(const BarteronOfferPaidRef& ptx)
        {
            // Edited transaction must be same type and not deleted
            Result(ConsensusResult_ExceededLimit, [&]() {
                return (TxType)consensusData.LastTxType != BARTERON_OFFER_PAID;
            });
        }
    
        ConsensusValidateResult ValidateBlock(const BarteronOfferPaidRef& ptx, const PocketBlockRef& block) override
        {
            // Limits are not used for paid offers

            return { ResultCode == ConsensusResult_Success, ResultCode }; // TODO (aok): remove when all consensus classes support Result
        }

        ConsensusValidateResult ValidateMempool(const BarteronOfferPaidRef& ptx) override
        {
            // Limits are not used for paid offers

            return { ResultCode == ConsensusResult_Success, ResultCode }; // TODO (aok): remove when all consensus classes support Result
        }

    };

    // Factory for select actual rules version
    class BarteronOfferPaidConsensusFactory : public BaseConsensusFactory<BarteronOfferPaidConsensus>
    {
    public:
        BarteronOfferPaidConsensusFactory()
        {
            Checkpoint({ 999999999, 999999999, -1, make_shared<BarteronOfferPaidConsensus>() });
        }
    };

    static BarteronOfferPaidConsensusFactory ConsensusFactoryInst_BarteronOfferPaid;
}

#endif // POCKETCONSENSUS_BARTERON_OFFER_PAID_HPP
