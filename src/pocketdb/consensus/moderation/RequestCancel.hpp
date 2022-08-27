// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REQUEST_CANCEL_HPP
#define POCKETCONSENSUS_MODERATOR_REQUEST_CANCEL_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/RequestCancel.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModeratorRequestCancel> ModeratorRequestCancelRef;

    /*******************************************************************************************************************
    *  ModeratorRequestCancel consensus base class
    *******************************************************************************************************************/
    class ModeratorRequestCancelConsensus : public ModeratorRequestConsensus<Moderator>
    {
    public:
        ModeratorRequestCancelConsensus(int height) : ModeratorRequestConsensus<Moderator>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRequestCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = ModeratorRequestConsensus::Validate(tx, ptx, block); !baseValidate)
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

        ConsensusValidateResult ValidateBlock(const ModeratorRequestCancelRef& ptx, const PocketBlockRef& block) override
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

        ConsensusValidateResult ValidateMempool(const ModeratorRequestCancelRef& ptx) override
        {
            // // Check flag from one to one
            // if (ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), true) > 0)
            //     return {false, SocialConsensusResult_Duplicate};

            // // Check limit
            // return ValidateLimit(ptx, ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), Height - (int)GetConsensusLimit(ConsensusLimit_depth), true));
        }

        vector<string> GetAddressesForCheckRegistration(const ModeratorRequestCancelRef& ptx) override
        {
            return { *ptx->GetAddress(), *ptx->GetDestinationAddress() };
        }

        virtual ConsensusValidateResult ValidateLimit(const ModeratorRequestCancelRef& ptx, int count)
        {
            // if (count >= GetConsensusLimit(ConsensusLimit_moderation_flag_count))
            //     return {false, SocialConsensusResult_ExceededLimit};

            return Success;
        }
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRequestCancelConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRequestCancelConsensus>> m_rules = {
            { 9999999, 9999999, [](int height) { return make_shared<ModeratorRequestCancelConsensus>(height); }},
        };
    public:
        shared_ptr<ModeratorRequestCancelConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRequestCancelConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_CANCEL_HPP