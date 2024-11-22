// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COLLECTION_H
#define POCKETCONSENSUS_COLLECTION_H

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Collection.h"

namespace PocketConsensus
{
    typedef shared_ptr<Collection> CollectionRef;
    typedef shared_ptr<Content> ContentRef;

    /*******************************************************************************************************************
    *  Collection consensus base class
    *******************************************************************************************************************/
    class CollectionConsensus : public SocialConsensus<Collection>
    {
    public:
        CollectionConsensus() : SocialConsensus<Collection>()
        {
            // TODO (limits): set limits
            Limits.Set("payload_size", 2000, 2000, 2000);
        }

        tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const CollectionRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (!block) {
                if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                    return {false, baseValidateCode};
            }

            if(auto[contentIdsOk, contentIds] = ptx->GetContentIdsVector(); contentIdsOk)
            {
                // Check count of content ids
                if (contentIds.size() > (size_t)GetConsensusLimit(ConsensusLimit_collection_ids_count))
                   return {false, ConsensusResult_Failed};

                // Contents should be exists in chain
                int count = PocketDb::ConsensusRepoInst.GetLastContentsCount(contentIds, { PocketTx::TxType(*ptx->GetContentTypes()) });
                if((size_t)count != contentIds.size())
                    return {false, ConsensusResult_Failed};
            }
            else
            {
                return {false, ConsensusResult_Failed};
            }

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        
        tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const CollectionRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTypes())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentIds())) return {false, ConsensusResult_Failed};

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
                return {false, ConsensusResult_NotAllowed};

            // First get original collection transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            const auto originalPtx = static_pointer_cast<Content>(originalTx);

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, ConsensusResult_NotAllowed};

            // Change conteents type not allowed
            if (*originalTx->GetInt1() != *ptx->GetContentTypes())
                return {false, ConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const CollectionRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetLimit(mode))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_ContentLimit))
                    return {false, ConsensusResult_ContentLimit};
            }

            return Success;
        }
        virtual bool AllowBlockLimitTime(const CollectionRef& ptx, const CollectionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }
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
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_COLLECTION, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<Collection>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditMempool(const CollectionRef& ptx)
        {
            if (ConsensusRepoInst.CountMempoolCollectionEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditOneLimit(const CollectionRef& ptx)
        {
            int count = ConsensusRepoInst.CountChainCollectionEdit(*ptx->GetAddress(), *ptx->GetRootTxHash(), Height, GetConsensusLimit(ConsensusLimit_edit_collection_depth));
            if (count >= GetConsensusLimit(ConsensusLimit_collection_edit_count))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
    };

    class CollectionConsensus_fix_check : public CollectionConsensus
    {
    public:
        CollectionConsensus_fix_check() : CollectionConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const CollectionRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if(auto[contentIdsOk, contentIds] = ptx->GetContentIdsVector(); contentIdsOk)
            {
                // Check count of content ids
                if (contentIds.size() > (size_t)GetConsensusLimit(ConsensusLimit_collection_ids_count))
                   return {false, ConsensusResult_Failed};

                // Contents should be exists in chain
                int count = PocketDb::ConsensusRepoInst.GetLastContentsCount(contentIds, { PocketTx::TxType(*ptx->GetContentTypes()) });
                if((size_t)count != contentIds.size())
                    return {false, ConsensusResult_Failed};
            }
            else
            {
                return {false, ConsensusResult_Failed};
            }

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }
    };

    class CollectionConsensus_pip109 : public CollectionConsensus_fix_check
    {
    public:
        CollectionConsensus_pip109() : CollectionConsensus_fix_check() {}

        tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const CollectionRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if(auto[contentIdsOk, contentIds] = ptx->GetContentIdsVector(); contentIdsOk)
            {
                // Check count of content ids
                if (contentIds.size() > (size_t)GetConsensusLimit(ConsensusLimit_collection_ids_count))
                   return {false, ConsensusResult_Failed};
            }
            else
            {
                return {false, ConsensusResult_Failed};
            }

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }
        
        tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const CollectionRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentIds())) return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:
    
        tuple<bool, SocialConsensusResult> ValidateEdit(const CollectionRef& ptx) override
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                    *ptx->GetRootTxHash(),
                    { CONTENT_COLLECTION }
            );
            if (lastContentOk && *lastContent->GetType() != CONTENT_COLLECTION)
                return {false, ConsensusResult_NotAllowed};

            // First get original collection transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            const auto originalPtx = static_pointer_cast<Content>(originalTx);

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, ConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
    };


    class CollectionConsensusFactory : public BaseConsensusFactory<CollectionConsensus>
    {
    public:
        CollectionConsensusFactory()
        {
            Checkpoint({ 2162400, 1531000, -1, make_shared<CollectionConsensus>() });
            Checkpoint({ 2583000, 2280000, -1, make_shared<CollectionConsensus_fix_check>() });
            Checkpoint({ 3291900, 3373650,  0, make_shared<CollectionConsensus_pip109>() });
        }
    };

    static CollectionConsensusFactory ConsensusFactoryInst_Collection;
}

#endif // POCKETCONSENSUS_COLLECTION_H