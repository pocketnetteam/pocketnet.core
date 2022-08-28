// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REGISTER_CANCEL_HPP
#define POCKETCONSENSUS_MODERATOR_REGISTER_CANCEL_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Register.hpp"
#include "pocketdb/models/dto/moderation/RegisterCancel.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModeratorRegisterCancel> ModeratorRegisterCancelRef;

    /*******************************************************************************************************************
    *  ModeratorRegisterCancel consensus base class
    *******************************************************************************************************************/
    class ModeratorRegisterCancelConsensus : public ModeratorRegisterConsensus<ModeratorRegisterCancel>
    {
    public:
        ModeratorRegisterCancelConsensus(int height) : ModeratorRegisterConsensus<ModeratorRegisterCancel>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRegisterCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = ModeratorRegisterConsensus::Validate(tx, ptx, block); !baseValidate)
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

    protected:

        ConsensusValidateResult ValidateBlock(const ModeratorRegisterCancelRef& ptx, const PocketBlockRef& block) override
        {
            // // Check flag from one to one
            // if (ConsensusRepoInst.CountModeratorFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), false) > 0)
            //     return {false, SocialConsensusResult_Duplicate};

            // // Count flags in chain
            // int count = ConsensusRepoInst.CountModeratorFlag(*ptx->GetAddress(), Height - (int)GetConsensusLimit(ConsensusLimit_depth), false);

            // // Count flags in block
            // for (auto& blockTx : *block)
            // {
            //     if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATor_FLAG }) || *blockTx->GetHash() == *ptx->GetHash())
            //         continue;

            //     auto blockPtx = static_pointer_cast<ModeratorFlag>(blockTx);
            //     if (*ptx->GetAddress() == *blockPtx->GetAddress())
            //         if (*ptx->GetContentTxHash() == *blockPtx->GetContentTxHash())
            //             return {false, SocialConsensusResult_Duplicate};
            //         else
            //             count += 1;
            // }

            // // Check limit
            // return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const ModeratorRegisterCancelRef& ptx) override
        {
            // // Check flag from one to one
            // if (ConsensusRepoInst.CountModeratorFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), true) > 0)
            //     return {false, SocialConsensusResult_Duplicate};

            // // Check limit
            // return ValidateLimit(ptx, ConsensusRepoInst.CountModeratorFlag(*ptx->GetAddress(), Height - (int)GetConsensusLimit(ConsensusLimit_depth), true));
        }
    };

    /*******************************************************************************************************************
    *  Enable ModeratorRegisterSelf consensus rules
    *******************************************************************************************************************/
    class ModeratorRegisterCancelConsensus_checkpoint_enable : public ModeratorRegisterCancelConsensus
    {
    public:
        ModeratorRegisterCancelConsensus_checkpoint_enable(int height) : ModeratorRegisterCancelConsensus(height) {}
    protected:
        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRegisterCancelRef& ptx) override
        {
            return ModeratorRegisterConsensus::Check(tx, ptx);
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRegisterCancelConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRegisterCancelConsensus>> m_rules = {
            {       0,      -1, [](int height) { return make_shared<ModeratorRegisterCancelConsensus>(height); }},
            { 9999999, 9999999, [](int height) { return make_shared<ModeratorRegisterCancelConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModeratorRegisterCancelConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRegisterCancelConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REGISTER_CANCEL_HPP