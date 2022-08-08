// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATION_REGISTER_HPP
#define POCKETCONSENSUS_MODERATION_REGISTER_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModerationRegister> ModerationRegisterRef;

    /*******************************************************************************************************************
    *  ModerationRegister consensus base class
    *******************************************************************************************************************/
    class ModerationRegisterConsensus : public SocialConsensus<ModerationRegister>
    {
    public:
        ModerationRegisterConsensus(int height) : SocialConsensus<ModerationRegister>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModerationRegisterRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // // Only `Shark` account can flag content
            // auto reputationConsensus = ReputationConsensusFactoryInst.Instance(Height);
            // if (!reputationConsensus->GetBadges(*ptx->GetAddress()).Shark)
            //     return {false, SocialConsensusResult_LowReputation};

            // // Target transaction must be a exists and is a content and author should be equals ptx->GetContentAddressHash()
            // if (!ConsensusRepoInst.ExistsNotDeleted(
            //     *ptx->GetContentTxHash(),
            //     *ptx->GetContentAddressHash(),
            //     { ACCOUNT_USER, CONTENT_POST, CONTENT_ARTICLE, CONTENT_VIDEO, CONTENT_COMMENT, CONTENT_COMMENT_EDIT }
            // ))
            //     return {false, SocialConsensusResult_NotFound};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModerationRegisterRef& ptx) override
        {
            return {false, SocialConsensusResult_NotAllowed};
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModerationRegisterRef& ptx, const PocketBlockRef& block) override
        {
            // // Check flag from one to one
            // if (ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), false) > 0)
            //     return {false, SocialConsensusResult_Duplicate};

            // // Count flags in chain
            // int count = ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), Height - (int)GetConsensusLimit(ConsensusLimit_depth), false);

            // // Count flags in block
            // for (auto& blockTx : *block)
            // {
            //     if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATION_FLAG }) || *blockTx->GetHash() == *ptx->GetHash())
            //         continue;

            //     auto blockPtx = static_pointer_cast<ModerationFlag>(blockTx);
            //     if (*ptx->GetAddress() == *blockPtx->GetAddress())
            //         if (*ptx->GetContentTxHash() == *blockPtx->GetContentTxHash())
            //             return {false, SocialConsensusResult_Duplicate};
            //         else
            //             count += 1;
            // }

            // // Check limit
            // return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const ModerationRegisterRef& ptx) override
        {
            // // Check flag from one to one
            // if (ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), true) > 0)
            //     return {false, SocialConsensusResult_Duplicate};

            // // Check limit
            // return ValidateLimit(ptx, ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), Height - (int)GetConsensusLimit(ConsensusLimit_depth), true));
        }

        vector<string> GetAddressesForCheckRegistration(const ModerationRegisterRef& ptx) override
        {
            return { *ptx->GetAddress() };
        }

        virtual ConsensusValidateResult ValidateLimit(const ModerationRegisterRef& ptx, int count)
        {
            // if (count >= GetConsensusLimit(ConsensusLimit_moderation_flag_count))
            //     return {false, SocialConsensusResult_ExceededLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Enable ModerationRegister consensus rules
    *******************************************************************************************************************/
    class ModerationRegisterConsensus_checkpoint_enable : public ModerationRegisterConsensus
    {
    public:
        ModerationRegisterConsensus_checkpoint_enable(int height) : ModerationRegisterConsensus(height) {}
    protected:
        ConsensusValidateResult Check(const CTransactionRef& tx, const ModerationRegisterRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            return Success;
        }
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModerationRegisterConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModerationRegisterConsensus>> m_rules = {
            {       0,      -1, [](int height) { return make_shared<ModerationRegisterConsensus>(height); }},
            { 9999999, 9999999, [](int height) { return make_shared<ModerationRegisterConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModerationRegisterConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModerationRegisterConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATION_REGISTER_HPP