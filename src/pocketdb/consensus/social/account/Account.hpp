// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ACCOUNT_BASE_HPP
#define POCKETCONSENSUS_ACCOUNT_BASE_HPP

#include <boost/algorithm/string.hpp>
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/account/User.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;  

    /*******************************************************************************************************************
    *  AccountUser consensus base class
    *******************************************************************************************************************/
    template<class T>
    class AccountConsensus : public SocialConsensus<T>
    {
    private:
        using Base = SocialConsensus<T>;
        typedef shared_ptr<T> TRef;

    public:
        AccountConsensus() : SocialConsensus<T>() {}
        
        ConsensusValidateResult Validate(const CTransactionRef& tx, const TRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            if (auto[ok, code] = Base::ValidatePayloadSize(ptx); !ok)
                return {false, code};

            return Base::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const TRef& ptx) override
        {
            if (auto[ok, code] = Base::Check(tx, ptx); !ok)
                return {false, code};

            // Check payload
            if (!ptx->GetPayload())
                return {false, ConsensusResult_Failed};

            return Base::Success;
        }

        // TODO (optimization): remove after setting checkpoints
        ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const TRef& ptx) override
        {
            auto ptxORHash = ptx->BuildHash();
            auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);
            if (ptxORHash == txORHash)
                return Base::Success;

            if (CheckpointRepoInst.IsOpReturnCheckpoint(*ptx->GetHash(), ptxORHash))
                return Base::Success;

            // TODO (optimization): DEBUG!
            LogPrintf("DEBUG! SocialConsensusResult_FailedOpReturn - %s\n", *ptx->GetHash());
            return Base::Success;
            // return {false, SocialConsensusResult_FailedOpReturn};
        }

    protected:

        ConsensusValidateResult ValidateBlock(const TRef& ptx, const PocketBlockRef& block) override
        {
            return Base::Success;
        }

        ConsensusValidateResult ValidateMempool(const TRef& ptx) override
        {
            return Base::Success;
        }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_ACCOUNT_BASE_HPP
