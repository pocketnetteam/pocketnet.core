
// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BARTERON_REQUEST_HPP
#define POCKETCONSENSUS_BARTERON_REQUEST_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/account/BarteronRequest.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<BarteronRequest> BarteronRequestRef;

    /*******************************************************************************************************************
    *  BarteronRequest consensus base class
    *******************************************************************************************************************/
    class BarteronRequestConsensus : public SocialConsensus<BarteronRequest>
    {
    public:
        BarteronRequestConsensus(int height) : SocialConsensus<BarteronRequest>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronRequestRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            // if (ptx->IsEdit())
            //     return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronRequestRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Repost not allowed
            // if (!IsEmpty(ptx->GetRelayTxHash())) return {false, SocialConsensusResult_NotAllowed};

            return Success;
        }

    protected:
        virtual int64_t GetLimit(AccountMode mode) {
            return mode == AccountMode_Full
                     ? GetConsensusLimit(ConsensusLimit_full_barteron_request)
                     : GetConsensusLimit(ConsensusLimit_trial_barteron_request);
        }

        ConsensusValidateResult ValidateBlock(const BarteronRequestRef& ptx, const PocketBlockRef& block) override
        {
            // Edit
            // if (ptx->IsEdit())
            //     return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACCOUNT_BARTERON_REQUEST}))
                    continue;

                auto blockPtx = static_pointer_cast<BarteronRequest>(blockTx);

                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                // if (blockPtx->IsEdit())
                //     continue;

                count += 1;
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const BarteronRequestRef& ptx) override
        {

            // Edit
            // if (ptx->IsEdit())
            //     return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolBarteronRequest(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const BarteronRequestRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        // TODO (aok): move to base Social class
        virtual ConsensusValidateResult ValidateLimit(const BarteronRequestRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }
        virtual int GetChainCount(const BarteronRequestRef& ptx)
        {

            return ConsensusRepoInst.CountChainBarteronRequest(
                    *ptx->GetAddress(),
                    Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }

        virtual ConsensusValidateResult ValidatePayloadSize(const BarteronRequestRef& ptx)
        {
            // size_t dataSize =
            //         (ptx->GetPayloadUrl() ? ptx->GetPayloadUrl()->size() : 0) +
            //         (ptx->GetPayloadCaption() ? ptx->GetPayloadCaption()->size() : 0) +
            //         (ptx->GetPayloadMessage() ? ptx->GetPayloadMessage()->size() : 0) +
            //         (ptx->GetRelayTxHash() ? ptx->GetRelayTxHash()->size() : 0) +
            //         (ptx->GetPayloadSettings() ? ptx->GetPayloadSettings()->size() : 0) +
            //         (ptx->GetPayloadLang() ? ptx->GetPayloadLang()->size() : 0);

            // if (ptx->GetRootTxHash() && *ptx->GetRootTxHash() != *ptx->GetHash())
            //     dataSize += ptx->GetRootTxHash()->size();

            // if (!IsEmpty(ptx->GetPayloadTags()))
            // {
            //     UniValue tags(UniValue::VARR);
            //     tags.read(*ptx->GetPayloadTags());
            //     for (size_t i = 0; i < tags.size(); ++i)
            //         dataSize += tags[i].get_str().size();
            // }

            // if (!IsEmpty(ptx->GetPayloadImages()))
            // {
            //     UniValue images(UniValue::VARR);
            //     images.read(*ptx->GetPayloadImages());
            //     for (size_t i = 0; i < images.size(); ++i)
            //         dataSize += images[i].get_str().size();
            // }

            // if (dataSize > (size_t)GetConsensusLimit(ConsensusLimit_max_barteron_request_size))
            //     return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class BarteronRequestConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < BarteronRequestConsensus>> m_rules = {
                { 99999999, 99999999, 0, [](int height) { return make_shared<BarteronRequestConsensus>(height); }},
        };
    public:
        shared_ptr<BarteronRequestConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                                  [&](int target, const ConsensusCheckpoint<BarteronRequestConsensus>& itm)
                                  {
                                      return target < itm.Height(Params().NetworkID());
                                  }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_BARTERON_REQUEST_HPP
