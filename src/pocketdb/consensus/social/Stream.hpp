// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_STREAM_HPP
#define POCKETCONSENSUS_STREAM_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Stream.h"

namespace PocketConsensus
{
    typedef shared_ptr<Stream> StreamRef;

    /*******************************************************************************************************************
    *  Stream consensus base class
    *******************************************************************************************************************/
    class StreamConsensus : public SocialConsensus<Stream>
    {
    public:
        StreamConsensus() : SocialConsensus<Stream>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const StreamRef& ptx, const PocketBlockRef& block) override
        {
            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const StreamRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(ptx->GetRelayTxHash())) return {false, ConsensusResult_NotAllowed};

            return Success;
        }

    protected:
        virtual int64_t GetLimit(AccountMode mode) {
            return mode == AccountMode_Full
                     ? GetConsensusLimit(ConsensusLimit_full_stream)
                     : GetConsensusLimit(ConsensusLimit_trial_stream);
        }

        ConsensusValidateResult ValidateBlock(const StreamRef& ptx, const PocketBlockRef& block) override
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
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_STREAM}))
                    continue;

                auto blockPtx = static_pointer_cast<Stream>(blockTx);

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
        ConsensusValidateResult ValidateMempool(const StreamRef& ptx) override
        {

            // Edit
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolStream(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const StreamRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual ConsensusValidateResult ValidateEdit(const StreamRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                    *ptx->GetRootTxHash(),
                    { CONTENT_POST, CONTENT_STREAM, CONTENT_DELETE }
            );
            if (lastContentOk && *lastContent->GetType() != CONTENT_STREAM)
                return {false, ConsensusResult_NotAllowed};

            // First get original post transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<Stream>(originalTx);

            // Change type not allowed
            if (*originalPtx->GetType() != *ptx->GetType())
                return {false, ConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
        // TODO (aok): move to base Social class
        virtual ConsensusValidateResult ValidateLimit(const StreamRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, ConsensusResult_ContentLimit};

            return Success;
        }
        virtual int GetChainCount(const StreamRef& ptx)
        {

            return ConsensusRepoInst.CountChainStream(
                    *ptx->GetAddress(),
                    Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
        virtual ConsensusValidateResult ValidateEditBlock(const StreamRef& ptx, const PocketBlockRef& block)
        {

            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_STREAM, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<Stream>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditMempool(const StreamRef& ptx)
        {

            if (ConsensusRepoInst.CountMempoolStreamEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditOneLimit(const StreamRef& ptx)
        {

            int count = ConsensusRepoInst.CountChainStreamEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_stream_edit_count))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
        virtual bool AllowEditWindow(const StreamRef& ptx, const StreamRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_stream_depth);
        }
        virtual ConsensusValidateResult ValidatePayloadSize(const StreamRef& ptx)
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

            if (dataSize > (size_t)GetConsensusLimit(ConsensusLimit_max_stream_size))
                return {false, ConsensusResult_ContentSizeLimit};

            return Success;
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class StreamConsensusFactory : public BaseConsensusFactory<StreamConsensus>
    {
    public:
        StreamConsensusFactory()
        {
            Checkpoint({ 2162400, 1531000, 0, make_shared<StreamConsensus>() });
        }
    };

    static StreamConsensusFactory ConsensusFactoryInst_Stream;
}

#endif // POCKETCONSENSUS_STREAM_HPP
