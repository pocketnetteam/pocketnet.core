// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_APP_HPP
#define POCKETCONSENSUS_APP_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/App.h"

namespace PocketConsensus
{
    typedef shared_ptr<App> AppRef;

    /*******************************************************************************************************************
    *  App consensus base class
    *******************************************************************************************************************/
    class AppConsensus : public SocialConsensus<App>
    {
    public:
        AppConsensus() : SocialConsensus<App>()
        {
            // TODO app : set limits
            Limits.Set("payload_size", 60000, 60000, 60000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const AppRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            // Get count from chain
            int count = ConsensusRepoInst.CountChainHeight(*ptx->GetType(), *ptx->GetAddress());
            if (count >= GetConsensusLimit(ConsensusLimit_app))
                return { false, ConsensusResult_ContentLimit };

            // Check ID for unique
            if (ConsensusRepoInst.ExistsAnotherByName("", *ptx->GetId(), TxType::APP))
                return {false, ConsensusResult_NicknameDouble};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const AppRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress()))
                return {false, ConsensusResult_Failed};

            // Payload must be filled
            if (!ptx->GetPayload() || IsEmpty(ptx->GetName()) || IsEmpty(ptx->GetId()))
                return {false, ConsensusResult_Failed};

            // Check app name length and symbols aka account name
            if (!CheckIdContent(ptx))
                return {false, ConsensusResult_Failed};

            if (IsEmpty(ptx->GetDescription()))
                return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const AppRef& ptx, const PocketBlockRef& block) override
        {
            // Multiple in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { APP }))
                    continue;

                auto blockPtx = static_pointer_cast<App>(blockTx);

                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                return { false, ConsensusResult_ContentLimit };
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const AppRef& ptx) override
        {
            // Do not allowed multiple txs in mempool
            if (ConsensusRepoInst.Exists_MS1T(*ptx->GetAddress(), { APP }))
                return { false, ConsensusResult_ContentLimit };

            return Success;
        }
        
        virtual ConsensusValidateResult ValidateEdit(const AppRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetRootTxHash(),
                { APP }
            );

            // First get original transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, ConsensusResult_NotAllowed};

            auto originalPtx = static_pointer_cast<App>(originalTx);

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};

            return Success;
        }
        
        virtual bool CheckIdContent(const AppRef& ptx)
        {
            auto id = *ptx->GetId();
            boost::algorithm::to_lower(id);

            if (id.size() > 20)
                return false;
            
            if (!all_of(id.begin(), id.end(), [](unsigned char ch) { return ::isalnum(ch) || ch == '_'; }))
                return false;

            return true;
        }

    };

    class AppConsensusFactory : public BaseConsensusFactory<AppConsensus>
    {
    public:
        AppConsensusFactory()
        {
            // TODO app : set height
            Checkpoint({ 9999999, 0, 0, make_shared<AppConsensus>() });
        }
    };

    static AppConsensusFactory ConsensusFactoryInst_App;
}

#endif // POCKETCONSENSUS_APP_HPP