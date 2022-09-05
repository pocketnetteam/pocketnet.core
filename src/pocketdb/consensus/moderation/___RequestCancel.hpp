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
            // TODO (moderation): check not registered with canceled request

            return ModeratorRequestConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRequestCancelRef& ptx) override
        {
            if (IsEmpty(ptx->GetRequestTxHash()))
                return {false, SocialConsensusResult_Failed};

            return ModeratorRequestConsensus::Check(tx, ptx);
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
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRequestCancelConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRequestCancelConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<ModeratorRequestCancelConsensus>(height); }},
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