// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BOOSTCONTENT_HPP
#define POCKETCONSENSUS_BOOSTCONTENT_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/BoostContent.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<BoostContent> BoostContentRef;

    /*******************************************************************************************************************
    *  BoostContent consensus base class
    *******************************************************************************************************************/
    class BoostContentConsensus : public SocialConsensus<BoostContent>
    {
    public:
        BoostContentConsensus(int height) : SocialConsensus<BoostContent>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BoostContentRef& ptx, const PocketBlockRef& block) override
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
        ConsensusValidateResult Check(const CTransactionRef& tx, const BoostContentRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const BoostContentRef& ptx, const PocketBlockRef& block) override
        {
            return Success;
        }
        ConsensusValidateResult ValidateMempool(const BoostContentRef& ptx) override
        {
            return Success;
        }
    };

    class BoostContentConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < BoostContentConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<BoostContentConsensus>(height); }},
        };
    public:
        shared_ptr<BoostContentConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<BoostContentConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
}

#endif //POCKETCONSENSUS_BOOSTCONTENT_HPP
