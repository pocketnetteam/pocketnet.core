// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REQUEST_CANCEL_HPP
#define POCKETCONSENSUS_MODERATOR_REQUEST_CANCEL_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Request.hpp"
#include "pocketdb/models/dto/moderation/Moderator.h"
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
    class ModeratorRequestCancelConsensus : public ModeratorRequestConsensus<ModeratorRequestCancel>
    {
    public:
        ModeratorRequestCancelConsensus(int height) : ModeratorRequestConsensus<ModeratorRequestCancel>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRequestCancelRef& ptx, const PocketBlockRef& block) override
        {
            // Source request exists and address and moderator address equals
            if (!ConsensusRepoInst.Exists_HS1S2T(*ptx->GetRequestTxHash(), *ptx->GetAddress(), *ptx->GetModeratorAddress(), { MODERATOR_REQUEST_SUBS, MODERATOR_REQUEST_COIN }, false))
                return {false, SocialConsensusResult_NotFound};

            // Request already canceled
            if (ConsensusRepoInst.Exists_LS1S2T(*ptx->GetAddress(), *ptx->GetModeratorAddress(), { MODERATOR_REQUEST_CANCEL }))
                return {false, SocialConsensusResult_ManyTransactions};

            return ModeratorRequestConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRequestCancelRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = ModeratorRequestConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            if (IsEmpty(ptx->GetRequestTxHash()))
                return {false, SocialConsensusResult_Failed};

            return EnableTransaction();
        }

    protected:

        virtual ConsensusValidateResult EnableTransaction()
        {
            return { false, SocialConsensusResult_NotAllowed };
        }

    };


    // TODO (moderation): remove after fork enabled
    class ModeratorRequestCancelConsensus_checkpoint_enable : public ModeratorRequestCancelConsensus
    {
    public:
        ModeratorRequestCancelConsensus_checkpoint_enable(int height) : ModeratorRequestCancelConsensus(height) {}
    protected:
        ConsensusValidateResult EnableTransaction() override
        {
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
            {       0,       0, -1, [](int height) { return make_shared<ModeratorRequestCancelConsensus>(height); }},
            // TODO (moderation): set height
            { 9999999, 9999999,  0, [](int height) { return make_shared<ModeratorRequestCancelConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModeratorRequestCancelConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRequestCancelConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkID());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_CANCEL_HPP