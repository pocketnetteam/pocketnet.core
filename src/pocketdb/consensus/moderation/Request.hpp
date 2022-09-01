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

    typedef shared_ptr<PocketConsensus::ReputationConsensus> ReputationConsensusRef;

    /*******************************************************************************************************************
    *  ModeratorRequest consensus base class
    *******************************************************************************************************************/
    template<class T = Moderator>
    class ModeratorRequestConsensus : public SocialConsensus<T>
    {
    private:
        using Base = SocialConsensus<T>;

    public:
        ModeratorRequestConsensus(int height) : SocialConsensus<T>(height)
        {
            reputationConsensus = ReputationConsensusFactoryInst.Instance(height);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
            // TODO (moderation): check exists already request
            
            return Base::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx) override
        {
            if (Base::IsEmpty(ptx->GetModeratorAddress()))
                return {false, SocialConsensusResult_Failed};

            return Base::Check(tx, ptx);
        }

    protected:
        ReputationConsensusRef reputationConsensus;

        vector<string> GetAddressesForCheckRegistration(const shared_ptr<T>& ptx) override
        {
            return { *ptx->GetAddress(), *ptx->GetModeratorAddress() };
        }

    };
}

#endif // POCKETCONSENSUS_MODERATOR_REQUEST_HPP