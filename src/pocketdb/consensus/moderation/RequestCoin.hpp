// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REQUEST_COIN_HPP
#define POCKETCONSENSUS_MODERATOR_REQUEST_COIN_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Request.hpp"
#include "pocketdb/models/dto/moderation/RequestCoin.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModeratorRequestCoin> ModeratorRequestCoinRef;

    /*******************************************************************************************************************
    *  ModeratorRequestCoin consensus base class
    *******************************************************************************************************************/
    class ModeratorRequestCoinConsensus : public ModeratorRequestConsensus<ModeratorRequestCoin>
    {
    public:
        ModeratorRequestCoinConsensus(int height) : ModeratorRequestConsensus<ModeratorRequestCoin>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRequestCoinRef& ptx, const PocketBlockRef& block) override
        {
            // TODO (moderation): check exists old free outputs

            return ModeratorRequestConsensus::Validate(tx, ptx, block);
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ModeratorRequestCoinRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATOR_REQUEST_COIN, MODERATOR_REQUEST_COIN, MODERATOR_REQUEST_CANCEL }) || *blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Moderator>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    return {false, SocialConsensusResult_ManyTransactions};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const ModeratorRequestCoinRef& ptx) override
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
    class ModeratorRequestCoinConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRequestCoinConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<ModeratorRequestCoinConsensus>(height); }},
        };
    public:
        shared_ptr<ModeratorRequestCoinConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRequestCoinConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_COIN_HPP