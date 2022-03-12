// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ADPOST_HPP
#define POCKETCONSENSUS_ADPOST_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/AdPost.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<AdPost> AdPostRef;

    /*******************************************************************************************************************
    *  AdPost consensus base class
    *******************************************************************************************************************/
    class AdPostConsensus : public SocialConsensus<AdPost>
    {
    public:
        AdPostConsensus(int height) : SocialConsensus<AdPost>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const AdPostRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(*ptx->GetContentTxHash(), { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_DELETE });
            if (!contentOk)
                return {false, SocialConsensusResult_NotFound};

            if (*contentTx->GetType() == CONTENT_DELETE)
                return {false, SocialConsensusResult_CommentDeletedContent};

            return Success;
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const AdPostRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
        virtual ConsensusValidateResult EnableAccept()
        {
            return {false, SocialConsensusResult_NotAllowed};
        }
        ConsensusValidateResult ValidateBlock(const AdPostRef& ptx, const PocketBlockRef& block) override
        {
            return Success;
        }
        ConsensusValidateResult ValidateMempool(const AdPostRef& ptx) override
        {
            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const AdPostRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }
    };

    /*******************************************************************************************************************
    *  Enable accept transaction
    *******************************************************************************************************************/
    class AdPostConsensus_checkpoint_accept : public AdPostConsensus
    {
    public:
        AdPostConsensus_checkpoint_accept(int height) : AdPostConsensus(height) {}
    protected:
        ConsensusValidateResult EnableAccept()
        {
            return Success;
        }
    };

    class AdPostConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<AdPostConsensus>> m_rules = {
            {       0,      0, [](int height) { return make_shared<AdPostConsensus>(height); }},
            { 1586000, 528100, [](int height) { return make_shared<AdPostConsensus_checkpoint_accept>(height); }},
        };
    public:
        shared_ptr<AdPostConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<AdPostConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
}

#endif //POCKETCONSENSUS_ADPOST_HPP
