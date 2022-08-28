// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REGISTER_HPP
#define POCKETCONSENSUS_MODERATOR_REGISTER_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;

    /*******************************************************************************************************************
    *  ModeratorRegister consensus base class
    *******************************************************************************************************************/
    template<class T>
    class ModeratorRegisterConsensus : public SocialConsensus<T>
    {
    private:
        using Base = SocialConsensus<T>;

    public:

        ModeratorRegisterConsensus(int height) : SocialConsensus<T>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
            return Base::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = Base::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (Base::IsEmpty(ptx->GetModeratorAddress())) return {false, SocialConsensusResult_Failed};

            return Base::Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
        }

        ConsensusValidateResult ValidateMempool(const shared_ptr<T>& ptx) override
        {
        }
        
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REGISTER_HPP