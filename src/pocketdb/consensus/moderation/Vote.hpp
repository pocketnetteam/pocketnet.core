// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATION_VOTE_HPP
#define POCKETCONSENSUS_MODERATION_VOTE_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/Vote.h"

namespace PocketConsensus
{
    typedef shared_ptr<ModerationVote> ModerationVoteRef;

    /*******************************************************************************************************************
    *  ModerationVote consensus base class
    *******************************************************************************************************************/
    class ModerationVoteConsensus : public SocialConsensus<ModerationVote>
    {
    public:
        ModerationVoteConsensus() : SocialConsensus<ModerationVote>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModerationVoteRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(Height);
            auto accountData = ConsensusRepoInst.GetAccountsData({ *ptx->GetAddress() });
            auto badges = reputationConsensus->GetBadges(accountData[*ptx->GetAddress()]);

            // Only moderator can set votes
            if (!badges.Moderator)
                return {false, ConsensusResult_NotAllowed};

            // Double vote to one jury not allowed
            if (ConsensusRepoInst.Exists_S1S2T(*ptx->GetAddress(), *ptx->GetJuryId(), { MODERATION_VOTE }))
                return {false, ConsensusResult_Duplicate};

            // The jury must be exists
            if (!ConsensusRepoInst.ExistsActiveJury(*ptx->GetJuryId()))
                return {false, ConsensusResult_NotFound};

            // The moderators' votes should be accepted with a delay, in case the jury gets into the orphan block
            auto juryFlag = ConsensusRepoInst.Get(*ptx->GetJuryId());
            if (!juryFlag || *juryFlag->GetType() != MODERATION_FLAG
                || !juryFlag->GetHeight() || (Height - *juryFlag->GetHeight() < 10))
                return {false, ConsensusResult_NotAllowed};

            // Votes allowed if moderator requested by system
            if (!ConsensusRepoInst.AllowJuryModerate(*ptx->GetAddress(), *ptx->GetJuryId()))
                return {false, ConsensusResult_NotAllowed};

            return Success;
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const ModerationVoteRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetJuryId())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetVerdict()) || *ptx->GetVerdict() < 0 || *ptx->GetVerdict() > 1) return {false, ConsensusResult_Failed};
            
            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModerationVoteRef& ptx, const PocketBlockRef& block) override
        {
            // Count flags in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATION_VOTE })
                    || *blockTx->GetHash() == *ptx->GetHash()
                    || *ptx->GetAddress() != *blockTx->GetString1())
                    continue;

                auto blockPtx = static_pointer_cast<ModerationVote>(blockTx);
                if (*ptx->GetJuryId() == *blockPtx->GetJuryId())
                    return {false, ConsensusResult_Duplicate};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const ModerationVoteRef& ptx) override
        {
            if (ConsensusRepoInst.Exists_MS1S2T(*ptx->GetAddress(), *ptx->GetJuryId(), { MODERATION_VOTE }))
                return {false, ConsensusResult_Duplicate};

            return Success;
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class ModerationVoteConsensusFactory : public BaseConsensusFactory<ModerationVoteConsensus>
    {
    public:
        ModerationVoteConsensusFactory()
        {
            Checkpoint({ 2162400, 1531000, 0, make_shared<ModerationVoteConsensus>() });
        }
    };

    static ModerationVoteConsensusFactory ConsensusFactoryInst_ModerationVote;
}

#endif // POCKETCONSENSUS_MODERATION_VOTE_HPP
