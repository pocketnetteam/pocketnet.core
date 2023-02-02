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
            Limits.Set("max_active_count", 30, 10, 5);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            Result(ConsensusResult_Size, [&]() {
                return PayloadSize(ptx) > (size_t)Limits.Get("payload_size");
            });

            // Validate new or edited transaction
            if (ptx->IsEdit())
                ValidateEdit(ptx);
            else
                ValidateNew(ptx);

            // TODO (aok): remove when all consensus classes support Result
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode};

            return SocialConsensus::Validate(tx, ptx, block);
        }

    protected:

        void ValidateNew(const BarteronOfferRef& ptx)
        {
            // Allowed only N active offers per account
            Result(ConsensusResult_ExceededLimit, [&]() {
                int count = 0;

                ExternalRepoInst.TryTransactionStep(__func__, [&]()
                {
                    auto stmt = ExternalRepoInst.SetupSqlStatement(R"sql(
                        select
                            count()
                        from
                            Transactions t indexed by Transactions_Type_Last_String1_Height_Id
                        where
                            t.Type in (211) and
                            t.Last = 1 and
                            t.String1 = ? and
                            t.Height is not null
                    )sql");
                    ExternalRepoInst.TryBindStatementText(stmt, 1, *ptx->GetAddress());

                    if (ExternalRepoInst.Step(stmt) == SQLITE_ROW)
                        if (auto[ok, value] = ExternalRepoInst.TryGetColumnInt(*stmt, 0); ok)
                            count = value;

                    ExternalRepoInst.FinalizeSqlStatement(*stmt);
                });

                return (count >= Limits.Get("max_active_count"));
            });
        }

        void ValidateEdit(const BarteronOfferRef& ptx)
        {
            // Edited transaction must be same type and not deleted
            Result(ConsensusResult_ExceededLimit, [&]() {
                bool allow = true;

                ExternalRepoInst.TryTransactionStep(__func__, [&]()
                {
                    auto stmt = ExternalRepoInst.SetupSqlStatement(R"sql(
                        select
                            1
                        from
                            Transactions t indexed by Transactions_Type_Last_String1_String2_Height
                        where
                            t.Type in (211) and
                            t.Last = 1 and
                            t.String1 = ? and
                            t.String2 = ? and
                            t.Height is not null
                    )sql");
                    ExternalRepoInst.TryBindStatementText(stmt, 1, *ptx->GetAddress());
                    ExternalRepoInst.TryBindStatementText(stmt, 2, *ptx->GetRootTxHash());

                    allow = (ExternalRepoInst.Step(stmt) == SQLITE_ROW);

                    ExternalRepoInst.FinalizeSqlStatement(*stmt);
                });

                return allow;
            });
        }
    
        ConsensusValidateResult ValidateBlock(const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction change barteron offer allowed in block
            Result(ConsensusResult_ManyTransactions, [&]() {
                auto blockPtxs = SocialConsensus::ExtractBlockPtxs(block, ptx, { BARTERON_OFFER });
                return blockPtxs.size() > 0;
            });

            return {ResultCode == ConsensusResult_Success, ResultCode};
        }

        ConsensusValidateResult ValidateMempool(const BarteronOfferRef& ptx) override
        {
            // Only one transaction change barteron offer allowed in mempool
            Result(ConsensusResult_ManyTransactions, [&]() {
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

                return exists;
            });

            return {ResultCode == ConsensusResult_Success, ResultCode};
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
