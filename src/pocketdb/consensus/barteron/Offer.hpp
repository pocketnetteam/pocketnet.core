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
            Limits.Set("list_max_size", 10, 10, 5);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            Result(ConsensusResult_Size, [&]() {
                return CollectStringsSize(ptx) > (size_t)Limits.Get("payload_size");
            });

            // Lists must be <= max size
            Result(ConsensusResult_Size, [&]() {
                auto lst = ptx->GetPayloadTagsIds();
                return (lst && lst->size() > (size_t)Limits.Get("list_max_size"));
            });

            // TODO (barteron):
            if (ptx->IsEdit())
            {

            }
            else
            {

            }

            // TODO (barteron): max count active offers

            // TODO (aok): remove when all consensus classes support Result
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode};

            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronOfferRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // TODO (barteron): checks

            // Tags list must be exists and all elements must be numbers
            if (!ptx->GetPayloadTagsIds())
                return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateNew(const BarteronOfferRef& ptx)
        {

        }

        ConsensusValidateResult ValidateEdit(const BarteronOfferRef& ptx)
        {

        }
    
        ConsensusValidateResult ValidateBlock(const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction change barteron offer allowed in block
            auto blockPtxs = SocialConsensus::ExtractBlockPtxs(block, ptx, { BARTERON_OFFER });
            if (blockPtxs.size() > 0)
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const BarteronOfferRef& ptx) override
        {
            // Only one transaction change barteron offer allowed in mempool
            bool exists = false;

            ExternalRepoInst.TryTransactionStep(__func__, [&]()
            {
                auto stmt = ExternalRepoInst.SetupSqlStatement(R"sql(
                    select
                        1
                    from
                        Transactions t indexed by Transactions_Type_String1_Height_Time_Int1
                    where
                        t.Type in (211) and
                        t.String1 = ? and
                        t.Height is null
                )sql");
                ExternalRepoInst.TryBindStatementText(stmt, 1, *ptx->GetAddress());
                exists = (ExternalRepoInst.Step(stmt) == SQLITE_ROW);
                ExternalRepoInst.FinalizeSqlStatement(*stmt);
            });

            return { !exists, ConsensusResult_ManyTransactions };
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
