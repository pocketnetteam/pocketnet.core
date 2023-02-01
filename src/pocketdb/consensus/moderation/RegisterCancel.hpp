// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REGISTER_CANCEL_HPP
#define POCKETCONSENSUS_MODERATOR_REGISTER_CANCEL_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/moderation/Register.hpp"
#include "pocketdb/models/dto/moderation/RegisterCancel.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;
    typedef shared_ptr<ModeratorRegisterCancel> ModeratorRegisterCancelRef;

    /*******************************************************************************************************************
    *  ModeratorRegisterCancel consensus base class
    *******************************************************************************************************************/
    class ModeratorRegisterCancelConsensus : public ModeratorRegisterConsensus<ModeratorRegisterCancel>
    {
    public:
        ModeratorRegisterCancelConsensus(int height) : ModeratorRegisterConsensus<ModeratorRegisterCancel>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ModeratorRegisterCancelRef& ptx, const PocketBlockRef& block) override
        {
            // TODO (moderation): check if you unregister to another and this register is your request

            return ModeratorRegisterConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ModeratorRegisterCancelRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = ModeratorRegisterConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            return EnableTransaction();
        }

    protected:

        virtual ConsensusValidateResult EnableTransaction()
        {
            return { false, ConsensusResult_NotAllowed };
        }

    };


    // TODO (moderation): remove after fork enabled
    class ModeratorRegisterCancelConsensus_checkpoint_enable : public ModeratorRegisterCancelConsensus
    {
    public:
        ModeratorRegisterCancelConsensus_checkpoint_enable(int height) : ModeratorRegisterCancelConsensus(height) {}
    protected:
        ConsensusValidateResult EnableTransaction() override
        {
            return Success;
        }
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ModeratorRegisterCancelConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ModeratorRegisterCancelConsensus>> m_rules = {
            {       0,      -1, -1, [](int height) { return make_shared<ModeratorRegisterCancelConsensus>(height); }},
            // TODO (moderation): set height
            { 9999999, 9999999,  0, [](int height) { return make_shared<ModeratorRegisterCancelConsensus_checkpoint_enable>(height); }},
        };
    public:
        shared_ptr<ModeratorRegisterCancelConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ModeratorRegisterCancelConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkID());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REGISTER_CANCEL_HPP