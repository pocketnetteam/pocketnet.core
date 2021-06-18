// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_BASE_HPP
#define POCKETCONSENSUS_SOCIAL_BASE_HPP

#include "univalue/include/univalue.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.hpp"
#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    using std::static_pointer_cast;

    class SocialBaseConsensus : public BaseConsensus
    {
    public:
        SocialBaseConsensus(int height) : BaseConsensus(height) {}
        SocialBaseConsensus() : BaseConsensus() {}

        // Validate transaction in block for miner & network full block sync
        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block)
        {
            if (auto[ok, result] = ValidateModel(tx); !ok)
                return {ok, result};

            if (auto[ok, result] = ValidateLimit(tx, block); !ok)
                return {ok, result};

            return Success;
        }

        // Validate new transaction received over RPC or network mempool
        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx)
        {
            if (auto[ok, result] = ValidateModel(tx); !ok)
                return {ok, result};

            if (auto[ok, result] = ValidateLimit(tx); !ok)
                return {ok, result};

            return Success;
        }

        // Generic transactions validating
        virtual tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx)
        {
            if (auto[ok, result] = CheckModel(tx); !ok)
                return {ok, result};

            if (auto[ok, result] = CheckOpReturnHash(tx); !ok)
                return {ok, result};

            return Success;
        }

    protected:

        tuple<bool, SocialConsensusResult> Success{true, SocialConsensusResult_Success};


        // Implement consensus rules for model transaction
        virtual tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr<Transaction> tx) = 0;

        // Transaction in block validate in chain and block - not mempool
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx, PocketBlock& block) = 0;

        // Single transactions limits checked chain and mempool
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx) = 0;


        // Implement generic rules for model transaction
        virtual tuple<bool, SocialConsensusResult> CheckModel(shared_ptr<Transaction> tx) = 0;

        // Generic check consistence Transaction and Payload
        virtual tuple<bool, SocialConsensusResult> CheckOpReturnHash(shared_ptr<Transaction> tx)
        {
            // TODO (brangr): implement
            // std::vector<std::string> vasm;
            // boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));
            // if (vasm.size() < 3) {
            //     resultCode = ANTIBOTRESULT::FailedOpReturn;
            //     return;
            // }

            // if (vasm[2] != oitm["data_hash"].get_str()) {
            //         resultCode = ANTIBOTRESULT::FailedOpReturn;
            //         return;
            // }
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_BASE_HPP
