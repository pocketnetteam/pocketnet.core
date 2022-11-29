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
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModerationVote> ModerationVoteRef;

    /*******************************************************************************************************************
    *  ModerationVote consensus base class
    *******************************************************************************************************************/
    class ModerationVoteConsensus : public SocialConsensus<ModerationVote>
    {
    public:
        ModerationVoteConsensus(int height) : SocialConsensus<ModerationVote>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModerationVoteRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            auto reputationConsensus = ReputationConsensusFactoryInst.Instance(Height);
            auto badges = reputationConsensus->GetBadges(*ptx->GetAddress());

            // Only moderator can set votes
            if (!badges.Moderator)
                return {false, SocialConsensusResult_NotAllowed};

            // Double vote to one jury not allowed
            if (ConsensusRepoInst.Exists_S1S2T(*ptx->GetAddress(), *ptx->GetJuryId(), { MODERATION_VOTE }))
                return {false, SocialConsensusResult_Duplicate};

            // The jury must be convened
            if (!ConsensusRepoInst.ExistsJury(*ptx->GetJuryId()))
                return {false, SocialConsensusResult_Duplicate};

            // The moderators' votes should be accepted with a delay, in case the jury gets into the orphan block
            auto juryFlag = ConsensusRepoInst.Get(*ptx->GetJuryId());
            if (!juryFlag || *juryFlag->GetType() != MODERATION_FLAG
                || !juryFlag->GetHeight() || (Height - *juryFlag->GetHeight() < 10))
                return {false, SocialConsensusResult_NotAllowed};

            // TODO (moderation): votes allowed if moderator requested by system
            // algotihm for select moderators

            return Success;
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const ModerationVoteRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetJuryId())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetVerdict()) || *ptx->GetVerdict() < 0 || *ptx->GetVerdict() > 2) return {false, SocialConsensusResult_Failed};
            
            return EnableTransaction();
        }

    protected:

        virtual ConsensusValidateResult EnableTransaction()
        {
            return { false, SocialConsensusResult_NotAllowed };
        }

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
                    return {false, SocialConsensusResult_Duplicate};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const ModerationVoteRef& ptx) override
        {
            if (ConsensusRepoInst.Exists_MS1S2T(*ptx->GetAddress(), *ptx->GetJuryId(), { MODERATION_VOTE }))
                return {false, SocialConsensusResult_Duplicate};

            return Success;
        }
    };


    // TODO (moderation): remove after fork enabled
    class ModerationVoteConsensus_checkpoint_enable : public ModerationVoteConsensus
    {
    public:
        ModerationVoteConsensus_checkpoint_enable(int height) : ModerationVoteConsensus(height) {}
    protected:
        ConsensusValidateResult EnableTransaction() override
        {
            return Success;
        }
    };


    class ModerationVoteConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModerationVoteConsensus>> m_rules = {
            {       0,      -1, -1, [](int height) { return make_shared<ModerationVoteConsensus>(height); }},
            // TODO (moderation): set height
            { 9999999, 9999999,  0, [](int height) { return make_shared<ModerationVoteConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModerationVoteConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModerationVoteConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkID());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATION_VOTE_HPP