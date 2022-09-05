// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATION_REGISTER_SELF_HPP
#define POCKETCONSENSUS_MODERATION_REGISTER_SELF_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Register.hpp"
#include "pocketdb/models/dto/moderation/RegisterSelf.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModeratorRegisterSelf> ModeratorRegisterSelfRef;

    /*******************************************************************************************************************
    *  ModeratorRegister consensus base class
    *******************************************************************************************************************/
    class ModeratorRegisterSelfConsensus : public ModeratorRegisterConsensus<ModeratorRegisterSelf>
    {
    public:
        ModeratorRegisterSelfConsensus(int height) : ModeratorRegisterConsensus<ModeratorRegisterSelf>(height)
        {
            // ReputationConsensusRef reputationConsensus;
            // reputationConsensus = ReputationConsensusFactoryInst.Instance(height);
            // // Already `Moderator` cant't register again
            // if (reputationConsensus->GetBadges(*ptx->GetAddress()).Moderator)
            //     return {false, SocialConsensusResult_AlreadyExists};
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRegisterSelfRef& ptx, const PocketBlockRef& block) override
        {
            // Only `Whale` account can register moderator badge
            auto reputationConsensus = ReputationConsensusFactoryInst.Instance(Height);
            if (!reputationConsensus->GetBadges(*ptx->GetAddress()).Whale)
                return {false, SocialConsensusResult_LowReputation};

            return ModeratorRegisterConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRegisterSelfRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = ModeratorRegisterConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            if (*ptx->GetAddress() != *ptx->GetModeratorAddress())
                return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:

    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRegisterSelfConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRegisterSelfConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<ModeratorRegisterSelfConsensus>(height); }},
        };
    public:
        shared_ptr<ModeratorRegisterSelfConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRegisterSelfConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATION_REGISTER_SELF_HPP