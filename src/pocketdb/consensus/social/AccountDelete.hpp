// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ACCOUNT_DELETE_HPP
#define POCKETCONSENSUS_ACCOUNT_DELETE_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/account/Delete.h"

namespace PocketConsensus
{
    typedef shared_ptr<AccountDelete> AccountDeleteRef;

    /*******************************************************************************************************************
    *  AccountDelete consensus base class
    *******************************************************************************************************************/
    class AccountDeleteConsensus : public SocialConsensus<AccountDelete>
    {
    public:
        AccountDeleteConsensus() : SocialConsensus<AccountDelete>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const AccountDeleteRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            if (IsEmpty(ptx->GetAddress()))
                return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:

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
                    return {false, ConsensusResult_ManyTransactions};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const AccountDeleteRef& ptx) override
        {
            if (ConsensusRepoInst.Exists_MS1T(*ptx->GetAddress(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class AccountDeleteConsensusFactory : public BaseConsensusFactory<AccountDeleteConsensus>
    {
    public:
        AccountDeleteConsensusFactory()
        {
            Checkpoint({ 0, 0, 0, make_shared<AccountDeleteConsensus>() });
        }
    };

    static AccountDeleteConsensusFactory ConsensusFactoryInst_AccountDelete;
}

#endif // POCKETCONSENSUS_ACCOUNT_DELETE_HPP
