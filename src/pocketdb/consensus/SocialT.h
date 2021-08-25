// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIALT_H
#define POCKETCONSENSUS_SOCIALT_H

#include <utility>

#include "univalue/include/univalue.h"
#include "core_io.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.h"
#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/helpers/CheckpointHelper.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using PocketTx::PocketTxType;

    template<class T>
    class SocialConsensusT : public BaseConsensus
    {
    public:
        SocialConsensusT(int height) : BaseConsensus(height) {}

        // Validate transaction in block for miner & network full block sync
        virtual tuple<bool, SocialConsensusResult> Validate(const shared_ptr<T>& ptx, const PocketBlockRef& block)
        {
            // Account must be registered
            vector<string> addresses = GetAddressesForCheckRegistration(ptx);
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
        virtual tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const shared_ptr<T>& ptx)
        {
            // TODO (brangr): DEBUG!
            // if (AlreadyExists(ptx))
            //    return {true, SocialConsensusResult_AlreadyExists};

            if (auto[ok, result] = CheckOpReturnHash(tx, ptx); !ok)
                return {false, result};

            return Success;
        }

    protected:
        tuple<bool, SocialConsensusResult> Success{true, SocialConsensusResult_Success};

        virtual tuple<bool, SocialConsensusResult> ValidateLimits(const shared_ptr<T>& ptx, const PocketBlockRef& block)
        {
            if (block)
                return ValidateBlock(ptx, block);
            else
                return ValidateMempool(ptx);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateBlock(const shared_ptr<T>& ptx, const PocketBlockRef& block) = 0;

        virtual tuple<bool, SocialConsensusResult> ValidateMempool(const shared_ptr<T>& ptx) = 0;

        // Generic check consistence Transaction and Payload
        virtual tuple<bool, SocialConsensusResult> CheckOpReturnHash(const CTransactionRef& tx, const shared_ptr<T>& ptx)
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
        virtual bool AlreadyExists(const shared_ptr<T>& ptx)
        {
            return TransRepoInst.ExistsByHash(*ptx->GetHash());
        }

        // Get addresses from transaction for check registration
        virtual vector<string> GetAddressesForCheckRegistration(const shared_ptr<T>& tx) = 0;

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

        // Helpers
        bool IsIn(PocketTxType txType, const vector<PocketTxType>& inTypes) const
        {
            for (auto inType : inTypes)
                if (inType == txType)
                    return true;

            return false;
        }
    };

    template<class T>
    struct ConsensusCheckpointT
    {
        int m_main_height;
        int m_test_height;
        function<shared_ptr<T>(int height)> m_func;

        int Height(const string& networkId) const
        {
            if (networkId == CBaseChainParams::MAIN)
                return m_main_height;

            if (networkId == CBaseChainParams::TESTNET)
                return m_test_height;

            return m_main_height;
        }
    };
    //
    // template<class T>
    // class ConsensusFactoryT
    // {
    // protected:
    //     virtual vector<ConsensusCheckpointT<T>> m_rules() = 0;
    // public:
    //     virtual shared_ptr<SocialConsensusT<T>> Instance(int height)
    //     {
    //         int m_height = (height > 0 ? height : 0);
    //         return (--upper_bound(m_rules().begin(), m_rules().end(), m_height,
    //             [&](int target, const ConsensusCheckpointT<T>& itm)
    //             {
    //                 return target < itm.Height(Params().NetworkIDString());
    //             }
    //         ))->m_func(height);
    //     }
    // };
}

#endif // POCKETCONSENSUS_SOCIALT_H
