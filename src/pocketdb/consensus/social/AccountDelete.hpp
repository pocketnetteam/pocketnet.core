// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ACCOUNT_DELETE_HPP
#define POCKETCONSENSUS_ACCOUNT_DELETE_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/account/Delete.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<AccountDelete> AccountDeleteRef;

    /*******************************************************************************************************************
    *  AccountDelete consensus base class
    *******************************************************************************************************************/
    class AccountDeleteConsensus : public SocialConsensus<AccountDelete>
    {
    public:
        AccountDeleteConsensus(int height) : SocialConsensus<AccountDelete>(height) {}

        ConsensusValidateResult Check(const CTransactionRef& tx, const AccountDeleteRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            if (IsEmpty(ptx->GetAddress()))
                return {false, SocialConsensusResult_Failed};

            return EnableTransaction();
        }

    protected:

        virtual ConsensusValidateResult EnableTransaction()
        {
            return { false, SocialConsensusResult_NotAllowed };
        }

        ConsensusValidateResult ValidateBlock(const AccountDeleteRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                    continue;

                if (*ptx->GetHash() == *blockTx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<SocialTransaction>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const AccountDeleteRef& ptx) override
        {
            if (ConsensusRepoInst.ExistsInMempool(*ptx->GetAddress(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                return {false, SocialConsensusResult_ManyTransactions};

            return Success;
        }
    };

    // TODO (brangr): remove after fork enabled
    class AccountDeleteConsensus_checkpoint_enable : public AccountDeleteConsensus
    {
    public:
        AccountDeleteConsensus_checkpoint_enable(int height) : AccountDeleteConsensus(height) {}
    protected:
        ConsensusValidateResult EnableTransaction() override
        {
            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class AccountDeleteConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<AccountDeleteConsensus>> m_rules = {
            {       0, -1, [](int height) { return make_shared<AccountDeleteConsensus>(height); }},
            { 1914000,  0, [](int height) { return make_shared<AccountDeleteConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<AccountDeleteConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<AccountDeleteConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };

} // namespace PocketConsensus

#endif // POCKETCONSENSUS_ACCOUNT_DELETE_HPP
