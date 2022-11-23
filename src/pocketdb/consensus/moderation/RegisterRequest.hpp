// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATION_REGISTER_REQUEST_HPP
#define POCKETCONSENSUS_MODERATION_REGISTER_REQUEST_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Register.hpp"
#include "pocketdb/models/dto/moderation/RegisterRequest.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;

    typedef shared_ptr<ModeratorRegisterRequest> ModeratorRegisterRequestRef;

    /*******************************************************************************************************************
    *  ModeratorRegisterRequest consensus base class
    *******************************************************************************************************************/
    class ModeratorRegisterRequestConsensus : public ModeratorRegisterConsensus<ModeratorRegisterRequest>
    {
    public:
        ModeratorRegisterRequestConsensus(int height) : ModeratorRegisterConsensus<ModeratorRegisterRequest>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRegisterRequestRef& ptx, const PocketBlockRef& block) override
        {
            auto badges = reputationConsensus->GetBadges(*ptx->GetAddress());

            // Already `Moderator` cant't register again
            if (badges.Moderator)
                return {false, SocialConsensusResult_AlreadyExists};

            if (!ConsensusRepoInst.Exists_HS2T(*ptx->GetRequestTxHash(), *ptx->GetAddress(), { MODERATOR_REQUEST_SUBS, MODERATOR_REQUEST_COIN }, true))
                return {false, SocialConsensusResult_NotFound};
            
            return ModeratorRegisterConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRegisterRequestRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = ModeratorRegisterConsensus::Check(tx, ptx); !baseCheck)
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
    class ModeratorRegisterRequestConsensus_checkpoint_enable : public ModeratorRegisterRequestConsensus
    {
    public:
        ModeratorRegisterRequestConsensus_checkpoint_enable(int height) : ModeratorRegisterRequestConsensus(height) {}
    protected:
        ConsensusValidateResult EnableTransaction() override
        {
            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRegisterRequestConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRegisterRequestConsensus>> m_rules = {
            {       0, -1, [](int height) { return make_shared<ModeratorRegisterRequestConsensus>(height); }},
            // TODO (moderation): set height
            { 9999999,  0, [](int height) { return make_shared<ModeratorRegisterRequestConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModeratorRegisterRequestConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRegisterRequestConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATION_REGISTER_REQUEST_HPP