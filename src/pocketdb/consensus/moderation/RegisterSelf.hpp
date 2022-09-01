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
        ModeratorRegisterSelfConsensus(int height) : ModeratorRegisterConsensus<ModeratorRegisterSelf>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRegisterSelfRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = ModeratorRegisterConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Only `Whale` account can register moderator badge
            if (!reputationConsensus->GetBadges(*ptx->GetAddress()).Whale)
                return {false, SocialConsensusResult_LowReputation};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRegisterSelfRef& ptx) override
        {
            return ModeratorRegisterConsensus::Check(tx, ptx);
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModeratorRegisterSelfRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATOR_REGISTER_SELF, MODERATOR_REGISTER_REQUEST, MODERATOR_REGISTER_CANCEL }) || *blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Moderator>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetModeratorAddress() == *blockPtx->GetModeratorAddress())
                    return {false, SocialConsensusResult_Duplicate};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const ModeratorRegisterSelfRef& ptx) override
        {
            // TODO (moderation): implement
            // if (ConsensusRepoInst.ExistsModeratorRegister(*ptx->GetAddress(), *ptx->GetModeratorAddress(), true))
            //     return {false, SocialConsensusResult_Duplicate};

            return Success;
        }
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