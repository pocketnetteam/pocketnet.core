// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_H
#define POCKETCONSENSUS_SOCIAL_H

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

    using std::static_pointer_cast;
    using PocketTx::PocketTxType;
    typedef tuple<bool, SocialConsensusResult> ConsensusValidateResult;

    class SocialConsensus : public BaseConsensus
    {
    public:
        SocialConsensus(int height) : BaseConsensus(height) {}

        // Validate transaction in block for miner & network full block sync
        virtual ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block);

        // Generic transactions validating
        virtual ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx);

    protected:
        ConsensusValidateResult Success{true, SocialConsensusResult_Success};

        virtual ConsensusValidateResult ValidateLimits(const PTransactionRef& ptx, const PocketBlockRef& block);

        virtual ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) = 0;

        virtual ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) = 0;

        // Generic check consistence Transaction and Payload
        virtual ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const PTransactionRef& ptx);

        // If transaction already in DB - skip next checks
        virtual bool AlreadyExists(const PTransactionRef& ptx);

        // Get addresses from transaction for check registration
        virtual vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) = 0;


        // Check empty pointer
        bool IsEmpty(const shared_ptr<string>& ptr) const { return !ptr || (*ptr).empty(); }

        bool IsEmpty(const shared_ptr<int>& ptr) const { return !ptr; }

        bool IsEmpty(const shared_ptr<int64_t>& ptr) const { return !ptr; }

        // Helpers
        bool IsIn(PocketTxType txType, const vector<PocketTxType>& inTypes) const
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
        shared_ptr<SocialConsensus> Instance(int height);
    };
}

#endif // POCKETCONSENSUS_SOCIAL_H
