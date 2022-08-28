// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_MODERATOR_REQUEST_HPP
#define POCKETCONSENSUS_MODERATOR_REQUEST_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketConsensus;

    /*******************************************************************************************************************
    *  ModeratorRequest consensus base class
    *******************************************************************************************************************/
    template<class T = Moderator>
    class ModeratorRequestConsensus : public SocialConsensus<T>
    {
    private:
        using Base = SocialConsensus<T>;

    public:
        ModeratorRequestConsensus(int height) : SocialConsensus<T>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = Base::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            return Base::Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx) override
        {
            if (Base::IsEmpty(ptx->GetModeratorAddress()))
                return {false, SocialConsensusResult_Failed};

            return Base::Check(tx, ptx);
        }

    protected:

        vector<string> GetAddressesForCheckRegistration(const shared_ptr<T>& ptx) override
        {
            return { *ptx->GetAddress(), *ptx->GetModeratorAddress() };
        }

    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_HPP