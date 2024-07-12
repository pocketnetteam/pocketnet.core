// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATION_FLAG_HPP
#define POCKETCONSENSUS_MODERATION_FLAG_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/Flag.h"

namespace PocketConsensus
{
    typedef shared_ptr<ModerationFlag> ModerationFlagRef;

    /*******************************************************************************************************************
    *  ModerationFlag consensus base class
    *******************************************************************************************************************/
    class ModerationFlagConsensus : public SocialConsensus<ModerationFlag>
    {
    public:
        ModerationFlagConsensus() : SocialConsensus<ModerationFlag>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModerationFlagRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Only `Shark` account can flag content
            auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(Height);
            auto accountData = ConsensusRepoInst.GetAccountsData({ *ptx->GetAddress() });
            if (!reputationConsensus->GetBadges(accountData[*ptx->GetAddress()]).Shark)
                return {false, ConsensusResult_LowReputation};

            // Target transaction must be a exists and is a content and author should be equals ptx->GetContentAddressHash()
            if (!ConsensusRepoInst.ExistsNotDeleted(
                *ptx->GetContentTxHash(),
                *ptx->GetContentAddressHash(),
                { ACCOUNT_USER, CONTENT_POST, CONTENT_ARTICLE, CONTENT_VIDEO, CONTENT_STREAM, CONTENT_AUDIO, APP, CONTENT_COMMENT, CONTENT_COMMENT_EDIT }
            ))
                return {false, ConsensusResult_NotFound};

            return Success;
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const ModerationFlagRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentAddressHash())) return {false, ConsensusResult_Failed};
            if (*ptx->GetAddress() == *ptx->GetContentAddressHash()) return {false, ConsensusResult_SelfFlag};
            if (IsEmpty(ptx->GetReason())) return {false, ConsensusResult_Failed};
            if (*ptx->GetReason() < 1 || *ptx->GetReason() > GetConsensusLimit(moderation_flag_max_value)) return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModerationFlagRef& ptx, const PocketBlockRef& block) override
        {
            // Check flag from one to one
            if (ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), false) > 0)
                return {false, ConsensusResult_Duplicate};

            // Count flags in chain
            int count = ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), Height - (int)GetConsensusLimit(ConsensusLimit_depth), false);

            // Count flags in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATION_FLAG }) || *blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<ModerationFlag>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (*ptx->GetContentTxHash() == *blockPtx->GetContentTxHash())
                        return {false, ConsensusResult_Duplicate};
                    else
                        count += 1;
                }
            }

            // Check limit
            return SocialConsensus::ValidateLimit(moderation_flag_count, count);
        }

        ConsensusValidateResult ValidateMempool(const ModerationFlagRef& ptx) override
        {
            // Check flag from one to one
            if (ConsensusRepoInst.CountModerationFlag(*ptx->GetAddress(), *ptx->GetContentAddressHash(), true) > 0)
                return {false, ConsensusResult_Duplicate};

            // Check limit
            return SocialConsensus::ValidateLimit(
                moderation_flag_count,
                ConsensusRepoInst.CountModerationFlag(
                    *ptx->GetAddress(),
                    Height - (int)GetConsensusLimit(ConsensusLimit_depth),
                    true
                )
            );
        }

        vector<string> GetAddressesForCheckRegistration(const ModerationFlagRef& ptx) override
        {
            return { *ptx->GetAddress(), *ptx->GetContentAddressHash() };
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class ModerationFlagConsensusFactory : public BaseConsensusFactory<ModerationFlagConsensus>
    {
    public:
        ModerationFlagConsensusFactory()
        {
            Checkpoint({ 0, 0, 0, make_shared<ModerationFlagConsensus>() });
        }
    };

    static ModerationFlagConsensusFactory ConsensusFactoryInst_ModerationFlag;
}

#endif // POCKETCONSENSUS_MODERATION_FLAG_HPP