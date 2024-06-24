// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMPLAIN_HPP
#define POCKETCONSENSUS_COMPLAIN_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/action/Complain.h"

namespace PocketConsensus
{
    typedef shared_ptr<Complain> ComplainRef;

    /*******************************************************************************************************************
    *  Complain consensus base class
    *******************************************************************************************************************/
    class ComplainConsensus : public SocialConsensus<Complain>
    {
    public:
        ComplainConsensus() : SocialConsensus<Complain>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ComplainRef& ptx, const PocketBlockRef& block) override
        {
            // Author or post must be exists
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetPostTxHash(),
                {CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, APP, CONTENT_DELETE}
            );

            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, APP, CONTENT_DELETE}))
                        continue;

                    // TODO (aok): convert to Content base class
                    if (*blockTx->GetString2() == *ptx->GetPostTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, ConsensusResult_NotFound};

            // Complain to self
            if (*lastContent->GetString1() == *ptx->GetAddress())
                return {false, ConsensusResult_SelfComplain};

            if (*lastContent->GetType() == CONTENT_DELETE)
                return {false, ConsensusResult_ComplainDeletedContent};

            // Check double complain
            if (PocketDb::ConsensusRepoInst.ExistsComplain(*ptx->GetPostTxHash(), *ptx->GetAddress(), false))
                return {false, ConsensusResult_DoubleComplain};

            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ComplainRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetReason())) return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const ComplainRef& ptx, const PocketBlockRef& block) override
        {
            int count = GetChainCount(ptx);

            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_COMPLAIN}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Complain>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;

                    // Maybe in block
                    if (*ptx->GetPostTxHash() == *blockPtx->GetPostTxHash())
                    {
                        if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleComplain))
                            return {false, ConsensusResult_DoubleComplain};
                    }
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const ComplainRef& ptx) override
        {
            // Check double complain
            if (PocketDb::ConsensusRepoInst.ExistsComplain(*ptx->GetPostTxHash(), *ptx->GetAddress(), true))
                return {false, ConsensusResult_DoubleComplain};

            int count = GetChainCount(ptx);
            count += ConsensusRepoInst.CountMempoolComplain(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        vector<string> GetAddressesForCheckRegistration(const ComplainRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual int64_t GetComplainsLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? 
            GetConsensusLimit(ConsensusLimit_full_complain) : 
            GetConsensusLimit(ConsensusLimit_trial_complain);
        }

        virtual ConsensusValidateResult ValidateLimit(const ComplainRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetComplainsLimit(mode))
                return {false, ConsensusResult_ComplainLimit};

            auto minimumReputation = GetConsensusLimit(ConsensusLimit_threshold_reputation);
            if (reputation < minimumReputation)
                return {false, ConsensusResult_ComplainLowReputation};

            return Success;
        }

        virtual bool CheckBlockLimitTime(const ComplainRef& ptx, const ComplainRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        virtual int GetChainCount(const ComplainRef& ptx)
        {
            return 0;
        }
    };


    class ComplainConsensus_checkpoint_1124000 : public ComplainConsensus
    {
    public:
        ComplainConsensus_checkpoint_1124000() : ComplainConsensus() {}
    protected:
        bool CheckBlockLimitTime(const ComplainRef& ptx, const ComplainRef& blockPtx) override
        {
            return true;
        }
    };


    class ComplainConsensus_checkpoint_pip_102 : public ComplainConsensus_checkpoint_1124000
    {
    public:
        ComplainConsensus_checkpoint_pip_102() : ComplainConsensus_checkpoint_1124000() {}
    protected:
        int GetChainCount(const ComplainRef& ptx) override
        {
            return ConsensusRepoInst.CountChainHeight(*ptx->GetType(), *ptx->GetAddress());
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class ComplainConsensusFactory : public BaseConsensusFactory<ComplainConsensus>
    {
    public:
        ComplainConsensusFactory()
        {
            Checkpoint({       0, -1, -1, make_shared<ComplainConsensus>() });
            Checkpoint({ 1124000, -1, -1, make_shared<ComplainConsensus_checkpoint_1124000>() });
            // TODO (aok, team): set fork height for enable this limit
            Checkpoint({ 9999999,  0,  0, make_shared<ComplainConsensus_checkpoint_pip_102>() });
        }
    };

    static ComplainConsensusFactory ConsensusFactoryInst_Complain;
}

#endif // POCKETCONSENSUS_COMPLAIN_HPP