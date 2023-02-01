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
            Limits.Set("list_max_size", 1000, 300, 15);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            Result(ConsensusResult_Size, [&]() {
                return CollectStringsSize(ptx) > (size_t)Limits.Get("payload_size");
            });
            
            // Lists must be <= max size
            Result(ConsensusResult_Size, [&]() {
                auto lst = ptx->GetPayloadTagsAddIds();
                return (lst && lst->size() > (size_t)Limits.Get("list_max_size"));
            });

            // Lists must be <= max size
            Result(ConsensusResult_Size, [&]() {
                auto lst = ptx->GetPayloadTagsDelIds();
                return (lst && lst->size() > (size_t)Limits.Get("list_max_size"));
            });

            // TODO (aok): remove when all consensus classes support Result
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode};
            
            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronAccountRef& ptx) override
        {
            if (auto[ok, code] = SocialConsensus::Check(tx, ptx); !ok)
                return {false, code};

            // Add or Del tags list must be exists and all elements must be numbers
            if (!ptx->GetPayloadTagsAddIds() && !ptx->GetPayloadTagsDelIds())
                return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:
    
        ConsensusValidateResult ValidateBlock(const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction change barteron account allowed in block
            auto blockPtxs = SocialConsensus::ExtractBlockPtxs(block, ptx, { BARTERON_ACCOUNT });
            if (blockPtxs.size() > 0)
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        
        ConsensusValidateResult ValidateMempool(const BarteronAccountRef& ptx) override
        {
            // Only one transaction change barteron account allowed in mempool
            bool exists = false;

            ExternalRepoInst.TryTransactionStep(__func__, [&]()
            {
                auto stmt = ExternalRepoInst.SetupSqlStatement(R"sql(
                    select
                        1
                    from
                        Transactions t indexed by Transactions_Type_String1_Height_Time_Int1
                    where
                        t.Type in (104) and
                        t.String1 = ? and
                        t.Height is null
                )sql");
                ExternalRepoInst.TryBindStatementText(stmt, 1, *ptx->GetAddress());
                exists = (ExternalRepoInst.Step(stmt) == SQLITE_ROW);
                ExternalRepoInst.FinalizeSqlStatement(*stmt);
            });

            return { !exists, ConsensusResult_ManyTransactions };
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
