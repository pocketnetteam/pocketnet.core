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

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;

    using std::static_pointer_cast;
    using PocketTx::PocketTxType;

    class SocialBaseConsensus : public BaseConsensus
    {
    public:
        SocialBaseConsensus(int height) : BaseConsensus(height) {}

        // Validate transaction in block for miner & network full block sync
        virtual tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, const PocketBlock& block)
        {
            if (auto[ok, result] = ValidateModel(tx); !ok)
                return {false, result};

            if (auto[ok, result] = ValidateLimit(tx, block); !ok)
                return {false, result};

            return Success;
        }

        // Validate new transaction received over RPC or network mempool
        virtual tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx)
        {
            if (auto[ok, result] = ValidateModel(tx); !ok)
                return {false, result};

            if (auto[ok, result] = ValidateLimit(tx); !ok)
                return {false, result};

            return Success;
        }

        // Generic transactions validating
        virtual tuple<bool, SocialConsensusResult> Check(const PTransactionRef& tx)
        {
            // TODO (brangr): commented for debug
            //if (AlreadyExists(tx))
            //    return {true, SocialConsensusResult_AlreadyExists};

            if (auto[ok, result] = CheckModel(tx); !ok)
                return {false, result};

            if (auto[ok, result] = CheckOpReturnHash(tx); !ok)
                return {false, result};

            return Success;
        }

    protected:

        tuple<bool, SocialConsensusResult> Success{true, SocialConsensusResult_Success};


        // Implement consensus rules for model transaction
        virtual tuple<bool, SocialConsensusResult> ValidateModel(const PTransactionRef& tx) = 0;

        // Transaction in block validate in chain and block - not mempool
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx,
            const PocketBlock& block) = 0;

        // Single transactions limits checked chain and mempool
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx) = 0;


        // Implement generic rules for model transaction
        virtual tuple<bool, SocialConsensusResult> CheckModel(const PTransactionRef& tx) = 0;

        // Generic check consistence Transaction and Payload
        virtual tuple<bool, SocialConsensusResult> CheckOpReturnHash(const PTransactionRef& tx)
        {
            if (IsEmpty(tx->GetOpReturnPayload()))
                return {false, SocialConsensusResult_PayloadORNotFound};

            if (IsEmpty(tx->GetOpReturnTx()))
                return {false, SocialConsensusResult_TxORNotFound};

            if (*tx->GetOpReturnTx() != *tx->GetOpReturnPayload())
            {
                // TODO (brangr): DEBUG
                LogPrintf("+++ %s != %s for %s\n", *tx->GetOpReturnTx(), *tx->GetOpReturnPayload(), *tx->GetHash());
                //return {false, SocialConsensusResult_FailedOpReturn};
            }

            return Success;
        }

        // If transaction already in DB - skip next checks
        virtual bool AlreadyExists(const PTransactionRef& tx)
        {
            return TransRepoInst.ExistsByHash(*tx->GetHash());
        }


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
}

#endif // POCKETCONSENSUS_SOCIAL_BASE_HPP
