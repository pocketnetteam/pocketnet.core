// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COLLECTION_H
#define POCKETCONSENSUS_COLLECTION_H

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Collection.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<Collection> CollectionRef;
    typedef shared_ptr<Content> ContentRef;

    /*******************************************************************************************************************
    *  Collection consensus base class
    *******************************************************************************************************************/
    class CollectionConsensus : public SocialConsensus<Collection>
    {
    public:
        CollectionConsensus(int height) : SocialConsensus<Collection>(height) {}
        tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const CollectionRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const CollectionRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTypes())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentIds())) return {false, SocialConsensusResult_Failed};
//            if (!IsEmpty(ptx->GetContentIds()) )
//            {
//                UniValue ids(UniValue::VARR);
//                if (!ids.read(*ptx->GetContentIds()))
//                    return {false, SocialConsensusResult_Failed};
//                if (ids.size() > (size_t)GetConsensusLimit(ConsensusLimit_collection_ids_count))
//                    return {false, SocialConsensusResult_Failed};
//            }
            if(auto[contentIdsOk, contentIds] = ptx->GetContentIdsVector(); !contentIdsOk)
                return {false, SocialConsensusResult_Failed};
            else
            {
                // Contents should be exists in chain
                if(auto[lastContentsOk, lastContents] =
                        PocketDb::ConsensusRepoInst.GetLastContents(contentIds, { PocketTx::TxType(*ptx->GetContentTypes()) });
                !lastContentsOk && lastContents.size() != contentIds.size())
                    return {false, SocialConsensusResult_Failed};
            }

            return Success;
        }

    protected:
        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetConsensusLimit(ConsensusLimit_full_collection) : GetConsensusLimit(ConsensusLimit_trial_collection);
        }

        tuple<bool, SocialConsensusResult> ValidateBlock(const CollectionRef& ptx, const PocketBlockRef& block) override
        {
            // Edit collections
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New collections

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (const auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_COLLECTION}))
                    continue;

                const auto blockPtx = static_pointer_cast<Collection>(blockTx);

                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                if (blockPtx->IsEdit())
                    continue;

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (AllowBlockLimitTime(ptx, blockPtx))
                    count += 1;
            }

            return ValidateLimit(ptx, count);
        }
        tuple<bool, SocialConsensusResult> ValidateMempool(const CollectionRef& ptx) override
        {
            // Edit collections
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New collections

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolCollection(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const CollectionRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEdit(const CollectionRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                    *ptx->GetRootTxHash(),
                    { CONTENT_COLLECTION }
            );
            if (lastContentOk && *lastContent->GetType() != CONTENT_COLLECTION)
                return {false, SocialConsensusResult_NotAllowed};

            // First get original collection transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, SocialConsensusResult_NotFound};

            const auto originalPtx = static_pointer_cast<Content>(originalTx);

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original collection edit only 24 hours
//            if (!AllowEditWindow(ptx, originalPtx))
//                return {false, SocialConsensusResult_ContentEditLimit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const CollectionRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetLimit(mode))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_ContentLimit))
                    return {false, SocialConsensusResult_ContentLimit};
            }

            return Success;
        }
        virtual bool AllowBlockLimitTime(const CollectionRef& ptx, const CollectionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }
//        virtual bool AllowEditWindow(const CollectionRef& ptx, const ContentRef& originalTx)
//        {
//            return (*ptx->GetTime() - *originalTx->GetTime()) <= GetConsensusLimit(ConsensusLimit_edit_collection_depth);
//        }
        virtual int GetChainCount(const CollectionRef& ptx)
        {
            return ConsensusRepoInst.CountChainCollection(
                    *ptx->GetAddress(),
                    *ptx->GetTime() - GetConsensusLimit(ConsensusLimit_depth)
            );
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditBlock(const CollectionRef& ptx, const PocketBlockRef& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_POST, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<Collection>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditMempool(const CollectionRef& ptx)
        {
            if (ConsensusRepoInst.CountMempoolCollectionEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditOneLimit(const CollectionRef& ptx)
        {
            int count = ConsensusRepoInst.CountChainCollectionEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_collection_edit_count))
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }
        virtual ConsensusValidateResult ValidatePayloadSize(const CollectionRef& ptx)
        {
            size_t dataSize =
                    (ptx->GetPayloadCaption() ? ptx->GetPayloadCaption()->size() : 0) +
//                    (ptx->GetPayloadMessage() ? ptx->GetPayloadMessage()->size() : 0) +
                    (ptx->GetPayloadImage() ? ptx->GetPayloadImage()->size() : 0) +
                    (ptx->GetPayloadSettings() ? ptx->GetPayloadSettings()->size() : 0) +
                    (ptx->GetPayloadLang() ? ptx->GetPayloadLang()->size() : 0);

            if (ptx->GetRootTxHash() && *ptx->GetRootTxHash() != *ptx->GetHash())
                dataSize += ptx->GetRootTxHash()->size();

            if (dataSize > (size_t)GetConsensusLimit(ConsensusLimit_max_collection_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
        virtual ConsensusValidateResult ValidateBlocking(const string& contentAddress, const CollectionRef& ptx)
        {
            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CollectionConsensusFactory
    {
    protected:
        const vector<ConsensusCheckpoint<CollectionConsensus>> m_rules = {
                { 9999999, 1531000, -1, [](int height) { return make_shared<CollectionConsensus>(height); }},
        };
    public:
        shared_ptr<CollectionConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                                  [&](int target, const ConsensusCheckpoint<CollectionConsensus>& itm)
                                  {
                                      return target < itm.Height(Params().NetworkID());
                                  }
            ))->m_func(m_height);
        }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_COLLECTION_H