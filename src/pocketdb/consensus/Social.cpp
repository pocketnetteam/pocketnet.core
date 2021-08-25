// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Social.h"

namespace PocketConsensus
{
    // Validate transaction in block for miner & network full block sync
    ConsensusValidateResult SocialConsensus::Validate(const PTransactionRef& ptx, const PocketBlockRef& block)
    {
        // Account must be registered
        vector <string> addresses = GetAddressesForCheckRegistration(ptx);
        if (!addresses.empty())
        {
            // First check block - maybe user registration this?
            if (block)
            {
                for (auto& blockTx : *block)
                {
                    if (!IsIn(*blockTx->GetType(), {ACCOUNT_USER}))
                        continue;

                    auto blockAddress = *blockTx->GetString1();
                    if (find(addresses.begin(), addresses.end(), blockAddress) != addresses.end())
                        addresses.erase(remove(addresses.begin(), addresses.end(), blockAddress), addresses.end());
                }
            }

            if (!addresses.empty() && !PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses, false))
                return {false, SocialConsensusResult_NotRegistered};
        }

        return ValidateLimits(ptx, block);
    }

    // Generic transactions validating
    ConsensusValidateResult SocialConsensus::Check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        // TODO (brangr): DEBUG!
        // if (AlreadyExists(ptx))
        //    return {true, SocialConsensusResult_AlreadyExists};

        if (auto[ok, result] = CheckOpReturnHash(tx, ptx); !ok)
            return {false, result};

        return Success;
    }

    ConsensusValidateResult SocialConsensus::ValidateLimits(const PTransactionRef& ptx, const PocketBlockRef& block)
    {
        if (block)
            return ValidateBlock(ptx, block);
        else
            return ValidateMempool(ptx);
    }

    // Generic check consistence Transaction and Payload
    ConsensusValidateResult SocialConsensus::CheckOpReturnHash(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        // TODO (brangr): implement check opreturn hash
        // if (IsEmpty(tx->GetOpReturnPayload()))
        //     return {false, SocialConsensusResult_PayloadORNotFound};
        //
        // if (IsEmpty(tx->GetOpReturnTx()))
        //     return {false, SocialConsensusResult_TxORNotFound};
        //
        // if (*tx->GetOpReturnTx() != *tx->GetOpReturnPayload())
        // {
        //     // TODO (brangr): DEBUG!!
        //    PocketHelpers::OpReturnCheckpoints opReturnCheckpoints;
        //    if (!opReturnCheckpoints.IsCheckpoint(*tx->GetHash(), *tx->GetOpReturnPayload()))
        //        return {false, SocialConsensusResult_FailedOpReturn};
        // }

        return Success;
    }

    // If transaction already in DB - skip next checks
    bool SocialConsensus::AlreadyExists(const PTransactionRef& ptx)
    {
        return TransRepoInst.ExistsByHash(*ptx->GetHash());
    }


    shared_ptr <SocialConsensus> SocialConsensusFactory::Instance(int height)
    {
        return static_pointer_cast<SocialConsensus>(
            BaseConsensusFactory::m_instance(height)
        );
    }
}
