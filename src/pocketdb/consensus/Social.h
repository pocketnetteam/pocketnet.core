// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_H
#define POCKETCONSENSUS_SOCIAL_H

#include <utility>

#include "univalue/include/univalue.h"
#include "core_io.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.h"
#include "pocketdb/consensus/Base.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using PocketTx::TxType;

    typedef tuple<bool, SocialConsensusResult> ConsensusValidateResult;

    template<class T>
    class SocialConsensus : public BaseConsensus
    {
    private:
        typedef shared_ptr<T> TRef;

    public:
        SocialConsensus(int height) : BaseConsensus(height) {}

        // Validate transaction in block for miner & network full block sync
        virtual ConsensusValidateResult Validate(const CTransactionRef& tx, const TRef& ptx, const PocketBlockRef& block)
        {
            if (block)
                return ValidateBlock(ptx, block);
            else
                return ValidateMempool(ptx);
            
            // TODO (merge)
            // // Check active account ban
            // if (PocketDb::ConsensusRepoInst.ExistsAccountBan(*ptx->GetString1(), Height))
            //     return {false, SocialConsensusResult_AccountBanned};
            // TODO (brangr): delete - в один блок пусть с удалением пролазят - проверитЬ!
            // if (*blockTx->GetType() == ACCOUNT_DELETE)
            //     return {false, SocialConsensusResult_AccountDeleted};
        }

        // Generic transactions validating
        virtual ConsensusValidateResult Check(const CTransactionRef& tx, const TRef& ptx)
        {
            // All social transactions must have an address
            if (IsEmpty(ptx->GetAddress()))
                return {false, SocialConsensusResult_Failed};

            if (auto[ok, result] = CheckOpReturnHash(tx, ptx); !ok)
                return {false, result};

            return Success;
        }

    protected:
        ConsensusValidateResult Success{true, SocialConsensusResult_Success};

        virtual ConsensusValidateResult ValidateBlock(const TRef& ptx, const PocketBlockRef& block) = 0;

        virtual ConsensusValidateResult ValidateMempool(const TRef& ptx) = 0;

        virtual ConsensusValidateResult ValidateLimit(ConsensusLimit limit, int count)
        {
            if (count >= GetConsensusLimit(limit))
                return {false, SocialConsensusResult_ExceededLimit};

            return Success;
        }
        
        virtual ConsensusValidateResult ValidatePayloadSize(const TRef& ptx)
        {
            return Success;
        }

        // Generic check consistence Transaction and Payload
        virtual ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const TRef& ptx)
        {
            auto ptxORHash = ptx->BuildHash();
            auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);

            if (ptxORHash != txORHash)
            {
               if (!CheckpointRepoInst.IsOpReturnCheckpoint(*ptx->GetHash(), ptxORHash))
                   return {false, SocialConsensusResult_FailedOpReturn};
            }

            return Success;
        }

        // Get addresses from transaction for check registration
        virtual vector<string> GetAddressesForCheckRegistration(const TRef& ptx)
        {
            return { *ptx->GetAddress() };
        }

        // Check empty pointer
        bool IsEmpty(const optional<string>& ptr) const
        {
            return !ptr || (*ptr).empty();
        }

        bool IsEmpty(const optional<int>& ptr) const
        {
            return !ptr;
        }

        bool IsEmpty(const optional<int64_t>& ptr) const
        {
            return !ptr;
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_H
