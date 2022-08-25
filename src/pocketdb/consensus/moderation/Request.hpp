// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATION_REQUEST_HPP
#define POCKETCONSENSUS_MODERATION_REQUEST_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;

    /*******************************************************************************************************************
    *  ModerationRequest consensus base class
    *******************************************************************************************************************/
    template<class T>
    class ModerationRequestConsensus : public SocialConsensus<T>
    {
    public:
        ModerationRequestConsensus(int height) : SocialConsensus<T>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx) override
        {
            return {false, SocialConsensusResult_NotAllowed};
        }

    protected:

        ConsensusValidateResult ValidateBlock(const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
        }

        ConsensusValidateResult ValidateMempool(const shared_ptr<T>& ptx) override
        {
        }

        vector<string> GetAddressesForCheckRegistration(const shared_ptr<T>& ptx) override
        {
            return { *ptx->GetAddress(), *ptx->GetDestinationAddress() };
        }

        virtual ConsensusValidateResult ValidateLimit(const shared_ptr<T>& ptx, int count) = 0;
    };

    /*******************************************************************************************************************
    *  Enable ModerationRequest consensus rules
    *******************************************************************************************************************/
    class ModerationRequestConsensus_checkpoint_enable : public ModerationRequestConsensus
    {
    public:
        ModerationRequestConsensus_checkpoint_enable(int height) : ModerationRequestConsensus(height) {}
    protected:
        ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Request transaction must have an destination address - who is invited to become a moderator
            if (IsEmpty(ptx->GetDestinationAddress())) return {false, SocialConsensusResult_Failed};

            return Success;
        }
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModerationRequestConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModerationRequestConsensus>> m_rules = {
            {       0,      -1, [](int height) { return make_shared<ModerationRequestConsensus>(height); }},
            { 9999999, 9999999, [](int height) { return make_shared<ModerationRequestConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModerationRequestConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModerationRequestConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATION_REQUEST_HPP