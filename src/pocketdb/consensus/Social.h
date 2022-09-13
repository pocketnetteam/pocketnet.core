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
    public:
        SocialConsensus(int height) : BaseConsensus(height) {}

        // Validate transaction in block for miner & network full block sync
        virtual ConsensusValidateResult Validate(const CTransactionRef& tx, const shared_ptr<T>& ptx, const PocketBlockRef& block)
        {
            // TODO (team): optimize algorithm
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

                // Check registrations in DB
                if (!addressesForCheck.empty() &&
                    !PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addressesForCheck))
                    return {false, SocialConsensusResult_NotRegistered};
            }

            // Check limits
            return ValidateLimits(ptx, block);
        }

        // Generic transactions validating
        virtual ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<T>& ptx)
        {
            if (auto[ok, result] = CheckOpReturnHash(tx, ptx); !ok)
                return {false, result};

            return Success;
        }

    protected:
        ConsensusValidateResult Success{true, SocialConsensusResult_Success};

        virtual ConsensusValidateResult ValidateLimits(const shared_ptr<T>& ptx, const PocketBlockRef& block)
        {
            if (block)
                return ValidateBlock(ptx, block);
            else
                return ValidateMempool(ptx);
        }

        virtual ConsensusValidateResult ValidateBlock(const shared_ptr<T>& ptx, const PocketBlockRef& block) = 0;

        virtual ConsensusValidateResult ValidateMempool(const shared_ptr<T>& ptx) = 0;

        // Generic check consistence Transaction and Payload
        virtual ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const shared_ptr<T>& ptx)
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
        virtual vector<string> GetAddressesForCheckRegistration(const shared_ptr<T>& tx)
        {
            return { *tx->GetAddress() };
        }

        // Check empty pointer
        bool IsEmpty(const shared_ptr<string>& ptr) const
        {
            return !ptr || (*ptr).empty();
        }

        bool IsEmpty(const shared_ptr<int>& ptr) const
        {
            return !ptr;
        }

        bool IsEmpty(const shared_ptr<int64_t>& ptr) const
        {
            return !ptr;
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_H
