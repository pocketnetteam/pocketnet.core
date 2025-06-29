// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_UTS_HPP
#define POCKETCONSENSUS_UTS_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketConsensus
{
    typedef shared_ptr<SocialTransaction> SocialTransactionRef;

    /*******************************************************************************************************************
    *  UTS consensus base class
    *******************************************************************************************************************/
    class UTSConsensus : public SocialConsensus<SocialTransaction>
    {
    public:
        UTSConsensus() : SocialConsensus<SocialTransaction>()
        {
            Limits.Set("payload_size", 120000, 60000, 60000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const SocialTransactionRef& ptx, const PocketBlockRef& block) override
        {
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const SocialTransactionRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const SocialTransactionRef& ptx, const PocketBlockRef& block) override
        {
            return Success;
        }

        ConsensusValidateResult ValidateMempool(const SocialTransactionRef& ptx) override
        {
            return Success;
        }

    };

    class UTSConsensusFactory : public BaseConsensusFactory<UTSConsensus>
    {
    public:
        UTSConsensusFactory()
        {
            Checkpoint({ 9999999, 9999999, -1, make_shared<UTSConsensus>() });
        }
    };

    static UTSConsensusFactory ConsensusFactoryInst_UTS;
}

#endif // POCKETCONSENSUS_UTS_HPP