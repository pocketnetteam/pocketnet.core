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

    template<class T>
    class SocialConsensus : public BaseConsensus
    {
    public:
        SocialConsensus() : BaseConsensus()
        {
            Limits.Set("payload_size", 2048, 2048, 2048);
        }

        // Validate transaction in block for miner & network full block sync
        virtual ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block)
        {
            Result(ConsensusResult_NotRegistered, [&]()
            {
                // TODO (aok): optimize algorithm
                // Account must be registered
                vector<string> addressesForCheck;
                vector<string> addresses = GetAddressesForCheckRegistration(ptx);
                if (!addresses.empty())
                {
                    // First check block - maybe user registration this?
                    if (block)
                    {
                        for (const string& address : addresses)
                        {
                            bool inBlock = false;
                            for (auto& blockTx: *block)
                            {
                                if (!TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                                    continue;

                                if (*blockTx->GetString1() == address)
                                {
                                    // TODO (brangr): delete - в один блок пусть с удалением пролазят - проверитЬ!
                                    // if (*blockTx->GetType() == ACCOUNT_DELETE)
                                    //     return {false, ConsensusResult_AccountDeleted};

                                    inBlock = true;
                                    break;
                                }
                            }

                            if (!inBlock)
                                addressesForCheck.push_back(address);
                        }
                    }
                    else
                    {
                        addressesForCheck = addresses;
                    }
                }

                // Check registrations in DB
                return (!addressesForCheck.empty() && !PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addressesForCheck));
            });

            // Check active account ban
            Result(ConsensusResult_AccountBanned, [&]()
            {
                return PocketDb::ConsensusRepoInst.ExistsAccountBan(*ptx->GetString1(), Height);
            });
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode}; // TODO (aok): remove when all consensus classes support Result

            // Check limits
            return ValidateLimits(ptx, block);
        }

        // Generic transactions validating
        virtual ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx)
        {
            // All social transactions must have an address
            if (IsEmpty(ptx->GetAddress()))
                return {false, ConsensusResult_Failed};

            if (auto[ok, result] = CheckOpReturnHash(tx, ptx); !ok)
                return {false, result};

            return Success;
        }

    protected:

        virtual ConsensusValidateResult ValidateLimits(const shared_ptr<T>& ptx, const PocketBlockRef& block)
        {
            if (block)
                return ValidateBlock(ptx, block);
            else
                return ValidateMempool(ptx);
        }

        virtual ConsensusValidateResult ValidateBlock(const shared_ptr<T>& ptx, const PocketBlockRef& block) = 0;

        virtual ConsensusValidateResult ValidateMempool(const shared_ptr<T>& ptx) = 0;

        virtual ConsensusValidateResult ValidateLimit(ConsensusLimit limit, int count)
        {
            if (count >= GetConsensusLimit(limit))
                return {false, ConsensusResult_ExceededLimit};

            return Success;
        }
        
        // Generic check consistence Transaction and Payload
        virtual ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const shared_ptr<T>& ptx)
        {
            auto ptxORHash = ptx->BuildHash();
            auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);

            if (ptxORHash != txORHash)
            {
               if (!CheckpointRepoInst.IsOpReturnCheckpoint(*ptx->GetHash(), ptxORHash))
                   return {false, ConsensusResult_FailedOpReturn};
            }

            return Success;
        }

        // Get addresses from transaction for check registration
        virtual vector<string> GetAddressesForCheckRegistration(const shared_ptr<T>& ptx)
        {
            return { *ptx->GetAddress() };
        }

        // Collect all string fields size
        virtual size_t PayloadSize(const shared_ptr<T>& ptx)
        {
            size_t dataSize =
                (ptx->GetString1() ? ptx->GetString1()->size() : 0) +
                (ptx->GetString2() ? ptx->GetString2()->size() : 0) +
                (ptx->GetString3() ? ptx->GetString3()->size() : 0) +
                (ptx->GetString4() ? ptx->GetString4()->size() : 0) +
                (ptx->GetString5() ? ptx->GetString5()->size() : 0);
            
            if (ptx->GetPayload())
            {
                dataSize +=
                    (ptx->GetPayload()->GetString1() ? ptx->GetPayload()->GetString1()->size() : 0) +
                    (ptx->GetPayload()->GetString2() ? ptx->GetPayload()->GetString2()->size() : 0) +
                    (ptx->GetPayload()->GetString3() ? ptx->GetPayload()->GetString3()->size() : 0) +
                    (ptx->GetPayload()->GetString4() ? ptx->GetPayload()->GetString4()->size() : 0) +
                    (ptx->GetPayload()->GetString5() ? ptx->GetPayload()->GetString5()->size() : 0) +
                    (ptx->GetPayload()->GetString6() ? ptx->GetPayload()->GetString6()->size() : 0) +
                    (ptx->GetPayload()->GetString7() ? ptx->GetPayload()->GetString7()->size() : 0);
            }

            return dataSize;
        }

        // Find transactions in block
        virtual vector<SocialTransactionRef> ExtractBlockPtxs(const PocketBlockRef& block, const shared_ptr<T>& ptx, const vector<TxType>& inclTypes)
        {
            vector<SocialTransactionRef> lst;

            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), inclTypes))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<SocialTransaction>(blockTx);
                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                lst.push_back(blockPtx);
            }

            return lst;
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
