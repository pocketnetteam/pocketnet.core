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

    typedef shared_ptr<PocketConsensus::ReputationConsensus> ReputationConsensusRef;

    /*******************************************************************************************************************
    *  ModeratorRegister consensus base class
    *******************************************************************************************************************/
    template<class T>
    class ModeratorRegisterConsensus : public SocialConsensus<T>
    {
    private:
        using Base = SocialConsensus<T>;

    public:

        ModeratorRegisterConsensus(int height) : SocialConsensus<T>(height)
        {
            reputationConsensus = ReputationConsensusFactoryInst.Instance(height);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
            // Already `Moderator` cant't register again
            if (reputationConsensus->GetBadges(*ptx->GetAddress()).Moderator)
                return {false, SocialConsensusResult_AlreadyExists};

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

        ConsensusValidateResult ValidateBlock(const shared_ptr<T>& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { MODERATOR_REGISTER_SELF, MODERATOR_REGISTER_REQUEST, MODERATOR_REGISTER_CANCEL }) || *blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Moderator>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    return {false, SocialConsensusResult_Duplicate};
            }

            return Base::Success;
        }

        ConsensusValidateResult ValidateMempool(const shared_ptr<T>& ptx) override
        {
            if (ConsensusRepoInst.ExistsModeratorRegister(*ptx->GetAddress(), true))
                return {false, SocialConsensusResult_Duplicate};

            return Base::Success;
        }
        
    };
}

#endif // POCKETCONSENSUS_MODERATOR_REGISTER_HPP