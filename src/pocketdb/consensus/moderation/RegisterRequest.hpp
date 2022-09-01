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
            // Registration of a moderator by invitation is allowed only if there is an actual and not canceled invitation
            // TODO (moderation): implement exists ModeratorRequestCoin or ModeratorRequestSubs transactions
            
            return ModeratorRegisterRequestConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRegisterRequestRef& ptx) override
        {
            if (IsEmpty(ptx->GetRequestTxHash()))
                return {false, SocialConsensusResult_Failed};

            return ModeratorRegisterConsensus::Check(tx, ptx);
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModeratorRegisterRequestRef& ptx, const PocketBlockRef& block) override
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

        ConsensusValidateResult ValidateMempool(const ModeratorRegisterRequestRef& ptx) override
        {
            // TODO (moderation): implement
            // if (ConsensusRepoInst.ExistsModeratorRegister(*ptx->GetAddress(), true))
            //     return {false, SocialConsensusResult_Duplicate};

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
            { 0, 0, [](int height) { return make_shared<ModeratorRegisterRequestConsensus>(height); }},
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