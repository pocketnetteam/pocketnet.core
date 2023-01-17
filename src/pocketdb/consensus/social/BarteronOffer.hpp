
// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BARTERON_OFFER_HPP
#define POCKETCONSENSUS_BARTERON_OFFER_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/BarteronOffer.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<BarteronOffer> BarteronOfferRef;

    /*******************************************************************************************************************
    *  BarteronOffer consensus base class
    *******************************************************************************************************************/
    class BarteronOfferConsensus : public SocialConsensus<BarteronOffer>
    {
    public:
        BarteronOfferConsensus(int height) : SocialConsensus<BarteronOffer>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronOfferRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(ptx->GetRelayTxHash())) return {false, SocialConsensusResult_NotAllowed};

            return Success;
        }

    protected:
        virtual int64_t GetLimit(AccountMode mode) {
            return mode == AccountMode_Full
                     ? GetConsensusLimit(ConsensusLimit_full_barteron_offer)
                     : GetConsensusLimit(ConsensusLimit_trial_barteron_offer);
        }

        ConsensusValidateResult ValidateBlock(const BarteronOfferRef& ptx, const PocketBlockRef& block) override
        {
            // Edit
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_BARTERON_OFFER}))
                    continue;

                auto blockPtx = static_pointer_cast<BarteronOffer>(blockTx);

                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (blockPtx->IsEdit())
                    continue;

                count += 1;
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const BarteronOfferRef& ptx) override
        {

            // Edit
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolBarteronOffer(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const BarteronOfferRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual ConsensusValidateResult ValidateEdit(const BarteronOfferRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                    *ptx->GetRootTxHash(),
                    { CONTENT_POST, CONTENT_VIDEO, CONTENT_DELETE, CONTENT_STREAM, CONTENT_BARTERON_OFFER }
            );
            if (lastContentOk && *lastContent->GetType() != CONTENT_BARTERON_OFFER)
                return {false, SocialConsensusResult_NotAllowed};

            // First get original post transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, SocialConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<BarteronOffer>(originalTx);

            // Change type not allowed
            if (*originalPtx->GetType() != *ptx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }
        // TODO (aok): move to base Social class
        virtual ConsensusValidateResult ValidateLimit(const BarteronOfferRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }
        virtual int GetChainCount(const BarteronOfferRef& ptx)
        {

            return ConsensusRepoInst.CountChainBarteronOffer(
                    *ptx->GetAddress(),
                    Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
        virtual ConsensusValidateResult ValidateEditBlock(const BarteronOfferRef& ptx, const PocketBlockRef& block)
        {

            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_BARTERON_OFFER, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<BarteronOffer>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditMempool(const BarteronOfferRef& ptx)
        {

            if (ConsensusRepoInst.CountMempoolBarteronOfferEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditOneLimit(const BarteronOfferRef& ptx)
        {

            int count = ConsensusRepoInst.CountChainBarteronOfferEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_barteron_offer_edit_count))
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }
        virtual bool AllowEditWindow(const BarteronOfferRef& ptx, const BarteronOfferRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_barteron_offer_depth);
        }
        virtual ConsensusValidateResult ValidatePayloadSize(const BarteronOfferRef& ptx)
        {
            size_t dataSize =
                    (ptx->GetPayloadUrl() ? ptx->GetPayloadUrl()->size() : 0) +
                    (ptx->GetPayloadCaption() ? ptx->GetPayloadCaption()->size() : 0) +
                    (ptx->GetPayloadMessage() ? ptx->GetPayloadMessage()->size() : 0) +
                    (ptx->GetRelayTxHash() ? ptx->GetRelayTxHash()->size() : 0) +
                    (ptx->GetPayloadSettings() ? ptx->GetPayloadSettings()->size() : 0) +
                    (ptx->GetPayloadLang() ? ptx->GetPayloadLang()->size() : 0);

            if (ptx->GetRootTxHash() && *ptx->GetRootTxHash() != *ptx->GetHash())
                dataSize += ptx->GetRootTxHash()->size();

            if (!IsEmpty(ptx->GetPayloadTags()))
            {
                UniValue tags(UniValue::VARR);
                tags.read(*ptx->GetPayloadTags());
                for (size_t i = 0; i < tags.size(); ++i)
                    dataSize += tags[i].get_str().size();
            }

            if (!IsEmpty(ptx->GetPayloadImages()))
            {
                UniValue images(UniValue::VARR);
                images.read(*ptx->GetPayloadImages());
                for (size_t i = 0; i < images.size(); ++i)
                    dataSize += images[i].get_str().size();
            }

            if (dataSize > (size_t)GetConsensusLimit(ConsensusLimit_max_barteron_offer_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class BarteronOfferConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < BarteronOfferConsensus>> m_rules = {
                { 99999999, 99999999, 0, [](int height) { return make_shared<BarteronOfferConsensus>(height); }},
        };
    public:
        shared_ptr<BarteronOfferConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                                  [&](int target, const ConsensusCheckpoint<BarteronOfferConsensus>& itm)
                                  {
                                      return target < itm.Height(Params().NetworkID());
                                  }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_BARTERON_OFFER_HPP
