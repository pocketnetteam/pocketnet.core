// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_AUDIO_HPP
#define POCKETCONSENSUS_AUDIO_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Audio.h"

namespace PocketConsensus
{
    typedef shared_ptr<Audio> AudioRef;
    
    class AudioConsensus : public SocialConsensus<Audio>
    {
    public:
        AudioConsensus() : SocialConsensus<Audio>()
        {
            // TODO (limits): set limits
            Limits.Set("payload_size", 60000, 60000, 60000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const AudioRef& ptx, const PocketBlockRef& block) override
        {
            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const AudioRef& ptx) override
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
                     ? GetConsensusLimit(ConsensusLimit_full_audio)
                     : GetConsensusLimit(ConsensusLimit_trial_audio);
        }

        ConsensusValidateResult ValidateBlock(const AudioRef& ptx, const PocketBlockRef& block) override
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
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_AUDIO}))
                    continue;

                auto blockPtx = static_pointer_cast<Audio>(blockTx);

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
        ConsensusValidateResult ValidateMempool(const AudioRef& ptx) override
        {

            // Edit
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolAudio(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const AudioRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual ConsensusValidateResult ValidateEdit(const AudioRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                    *ptx->GetRootTxHash(),
                    { CONTENT_POST, CONTENT_VIDEO, CONTENT_DELETE, CONTENT_STREAM, CONTENT_AUDIO }
            );
            if (lastContentOk && *lastContent->GetType() != CONTENT_AUDIO)
                return {false, ConsensusResult_NotAllowed};

            // First get original post transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<Audio>(originalTx);

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
        virtual ConsensusValidateResult ValidateLimit(const AudioRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, ConsensusResult_ContentLimit};

            return Success;
        }
        virtual int GetChainCount(const AudioRef& ptx)
        {

            return ConsensusRepoInst.CountChainAudio(
                    *ptx->GetAddress(),
                    Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
        virtual ConsensusValidateResult ValidateEditBlock(const AudioRef& ptx, const PocketBlockRef& block)
        {

            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_AUDIO, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<Audio>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditMempool(const AudioRef& ptx)
        {

            if (ConsensusRepoInst.CountMempoolAudioEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditOneLimit(const AudioRef& ptx)
        {

            int count = ConsensusRepoInst.CountChainAudioEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_audio_edit_count))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
        virtual bool AllowEditWindow(const AudioRef& ptx, const AudioRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_audio_depth);
        }
    };
    
    // Fix general validating
    class AudioConsensus_checkpoint_tmp_fix : public AudioConsensus
    {
    public:
        AudioConsensus_checkpoint_tmp_fix() : AudioConsensus() {}
        
        ConsensusValidateResult Validate(const CTransactionRef& tx, const AudioRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }
    };


    class AudioConsensusFactory : public BaseConsensusFactory<AudioConsensus>
    {
    public:
        AudioConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<AudioConsensus>() });
            Checkpoint({ 2552000, 2280000,  0, make_shared<AudioConsensus_checkpoint_tmp_fix>() });
        }
    };

    static AudioConsensusFactory ConsensusFactoryInst_Audio;
}

#endif // POCKETCONSENSUS_AUDIO_HPP
