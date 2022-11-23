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
            if (ConsensusRepoInst.Exists_LS1S2T(*ptx->GetAddress(), *ptx->GetModeratorAddress(), { MODERATOR_REQUEST_SUBS, MODERATOR_REQUEST_COIN }))
                return {false, SocialConsensusResult_ManyTransactions};

            // TODO (moderation): check exists old free outputs

            return ModeratorRequestConsensus::Validate(tx, ptx, block);
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRequestCoinRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = ModeratorRequestConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            return EnableTransaction();
        }

    protected:

        virtual ConsensusValidateResult EnableTransaction()
        {
            return { false, SocialConsensusResult_NotAllowed };
        }

    };


    // TODO (moderation): remove after fork enabled
    class ModeratorRequestCoinConsensus_checkpoint_enable : public ModeratorRequestCoinConsensus
    {
    public:
        ModeratorRequestCoinConsensus_checkpoint_enable(int height) : ModeratorRequestCoinConsensus(height) {}
    protected:
        ConsensusValidateResult EnableTransaction() override
        {
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
            {       0,       0, -1, [](int height) { return make_shared<ModeratorRequestCoinConsensus>(height); }},
            // TODO (moderation): set height
            { 9999999, 9999999,  0, [](int height) { return make_shared<ModeratorRequestCoinConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModeratorRequestCoinConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRequestCoinConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkID());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_COIN_HPP