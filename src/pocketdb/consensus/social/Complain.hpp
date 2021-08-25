// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMPLAIN_HPP
#define POCKETCONSENSUS_COMPLAIN_HPP

#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/Complain.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Complain consensus base class
    *******************************************************************************************************************/
    class ComplainConsensus : public SocialConsensus
    {
    public:
        ComplainConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // Author or post must be exists
            auto postAddress = PocketDb::ConsensusRepoInst.GetContentAddress(*_ptx->GetPostTxHash());
            if (postAddress == nullptr)
                return {false, SocialConsensusResult_NotFound};

            // Complain to self
            if (*postAddress == *_ptx->GetAddress())
                return {false, SocialConsensusResult_SelfComplain};

            // Check double complain
            if (PocketDb::ConsensusRepoInst.ExistsComplain(*_ptx->GetHash(), *_ptx->GetPostTxHash(), *_ptx->GetAddress()))
                return {false, SocialConsensusResult_DoubleComplain};

            return Success;
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetReason())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetFullAccountComplainsLimit() { return 12; }
        virtual int64_t GetTrialAccountComplainsLimit() { return 6; }
        virtual int64_t GetThresholdReputation() { return 500; }
        virtual int64_t GetComplainsLimit(AccountMode mode) {
            return mode >= AccountMode_Full ? GetFullAccountComplainsLimit() : GetTrialAccountComplainsLimit();
        }


        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);
            int count = GetChainCount(ptx);

            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_COMPLAIN}))
                    continue;

                if (*blockTx->GetHash() == *_ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Complain>(blockTx);
                if (*_ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(_ptx, blockPtx))
                        count += 1;

                    // Maybe in block
                    if (*_ptx->GetPostTxHash() == *blockPtx->GetPostTxHash())
                    {
                        PocketHelpers::SocialCheckpoints socialCheckpoints;
                        if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_DoubleComplain))
                            return {false, SocialConsensusResult_DoubleComplain};
                    }
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);

            int count = GetChainCount(_ptx);
            count += ConsensusRepoInst.CountMempoolComplain(*_ptx->GetAddress());
            return ValidateLimit(ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const PTransactionRef& ptx, int count)
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);

            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*_ptx->GetAddress());
            if (count >= GetComplainsLimit(mode))
                return {false, SocialConsensusResult_ComplainLimit};

            auto minimumReputation = GetThresholdReputation();
            if (reputation < minimumReputation)
                return {false, SocialConsensusResult_LowReputation};

            return Success;
        }

        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        virtual int GetChainCount(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);

            return ConsensusRepoInst.CountChainComplainTime(
                *_ptx->GetAddress(),
                *_ptx->GetTime() - GetLimitWindow()
            );
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Complain>(ptx);
            return {*ptx->GetString1()};
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 292800 block
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_292800 : public ComplainConsensus
    {
    public:
        ComplainConsensus_checkpoint_292800(int height) : ComplainConsensus(height) {}
    protected:
        int64_t GetThresholdReputation() override { return 1000; }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_1124000 : public ComplainConsensus_checkpoint_292800
    {
    public:
        ComplainConsensus_checkpoint_1124000(int height) : ComplainConsensus_checkpoint_292800(height) {}
    protected:
        bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_1180000 : public ComplainConsensus_checkpoint_1124000
    {
    public:
        ComplainConsensus_checkpoint_1180000(int height) : ComplainConsensus_checkpoint_1124000(height) {}
    protected:
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const PTransactionRef& ptx) override
        {
            return ConsensusRepoInst.CountChainComplainHeight(*ptx->GetString1(), Height - (int) GetLimitWindow());
        }
    };


    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ComplainConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<ComplainConsensus>(height); }},
            {292800,  -1, [](int height) { return make_shared<ComplainConsensus_checkpoint_292800>(height); }},
            {1124000, -1, [](int height) { return make_shared<ComplainConsensus_checkpoint_1124000>(height); }},
            {1180000, 0,  [](int height) { return make_shared<ComplainConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_COMPLAIN_HPP