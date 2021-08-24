// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_BASE_HPP
#define POCKETCONSENSUS_SOCIAL_BASE_HPP

#include "univalue/include/univalue.h"
#include "core_io.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.hpp"
#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/helpers/CheckpointHelper.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;

    using std::static_pointer_cast;
    using PocketTx::PocketTxType;

    typedef tuple<bool, SocialConsensusResult> ConsensusValidateResult;

    template<class T>
    class SocialConsensus : public BaseConsensus
    {
    public:
        SocialConsensus(int height) : BaseConsensus(height) {}

        // Validate transaction in block for miner & network full block sync
        virtual ConsensusValidateResult Validate(const T& tx, const PocketBlockRef& block)
        {
            // Account must be registered
            {
                vector<string> addresses = GetAddressesForCheckRegistration(tx);

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
            }

            if (block)
                return ValidateBlock(tx, block);
            else
                return ValidateMempool(tx);
        }

        // Generic transactions validating
        virtual ConsensusValidateResult Check(const CTransactionRef& tx, const T& ptx)
        {
            if (AlreadyExists(ptx))
               return {true, SocialConsensusResult_AlreadyExists};

            if (auto[ok, result] = CheckOpReturnHash(tx, ptx); !ok)
                return {false, result};

            return Success;
        }

    protected:
        ConsensusValidateResult Success{true, SocialConsensusResult_Success};

        virtual ConsensusValidateResult ValidateBlock(const T& tx, const PocketBlockRef& block) = 0;

        virtual ConsensusValidateResult ValidateMempool(const T& tx) = 0;

        // Generic check consistence Transaction and Payload
        virtual ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const T& ptx)
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
        virtual bool AlreadyExists(const T& ptx)
        {
            return TransRepoInst.ExistsByHash(*ptx->GetHash());
        }

        // Get addresses from transaction for check registration
        virtual vector<string> GetAddressesForCheckRegistration(const T& tx) = 0;


        // Check empty pointer
        bool IsEmpty(const shared_ptr<string>& ptr) const { return !ptr || (*ptr).empty(); }

        bool IsEmpty(const shared_ptr<int>& ptr) const { return !ptr; }

        bool IsEmpty(const shared_ptr<int64_t>& ptr) const { return !ptr; }

        // Helpers
        bool IsIn(PocketTxType txType, const vector<PocketTxType>& inTypes)
        {
            for (auto inType : inTypes)
                if (inType == txType)
                    return true;

            return false;
        }
    };


    class SocialConsensusFactory : public BaseConsensusFactory
    {
    public:
        shared_ptr<SocialConsensus<PTransactionRef>> Instance(int height)
        {
            return static_pointer_cast<SocialConsensus<PTransactionRef>>(
                BaseConsensusFactory::m_instance(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_BASE_HPP
