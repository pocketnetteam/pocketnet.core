// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REQUEST_SUBS_HPP
#define POCKETCONSENSUS_MODERATOR_REQUEST_SUBS_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Request.hpp"
#include "pocketdb/models/dto/moderation/RequestSubs.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModeratorRequestSubs> ModeratorRequestSubsRef;

    /*******************************************************************************************************************
    *  ModeratorRequestSubs consensus base class
    *******************************************************************************************************************/
    class ModeratorRequestSubsConsensus : public ModeratorRequestConsensus<ModeratorRequestSubs>
    {
    public:
        ModeratorRequestSubsConsensus(int height) : ModeratorRequestConsensus<ModeratorRequestSubs>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRequestSubsRef& ptx, const PocketBlockRef& block) override
        {
            // Only `Author` can invite moderators
            if (!reputationConsensus->GetBadges(*ptx->GetAddress()).Author)
                return {false, SocialConsensusResult_LowReputation};

            // TODO (moderation): implement check allowed requests count > 0

            return ModeratorRequestConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRequestSubsRef& ptx) override
        {
            return ModeratorRequestConsensus::Check(tx, ptx);
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModeratorRequestSubsRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATOR_REQUEST_COIN, MODERATOR_REQUEST_COIN }) || *blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Moderator>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const ModeratorRequestSubsRef& ptx) override
        {
            // TODO (moderation): implement
            // if (ConsensusRepoInst.ExistsModeratorRequest(*ptx->GetAddress(), true))
            //     return {false, SocialConsensusResult_Duplicate};

            return Success;
        }
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRequestSubsConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRequestSubsConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<ModeratorRequestSubsConsensus>(height); }},
        };
    public:
        shared_ptr<ModeratorRequestSubsConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRequestSubsConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_SUBS_HPP