// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMPLAIN_HPP
#define POCKETCONSENSUS_COMPLAIN_HPP

#include <pocketdb/consensus/Reputation.hpp>
#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Complain.hpp"

namespace PocketConsensus
{
    using namespace std;

    /*******************************************************************************************************************
    *
    *  Complain consensus base class
    *
    *******************************************************************************************************************/
    class ComplainConsensus : public SocialBaseConsensus
    {
    public:
        ComplainConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        virtual int64_t GetLimitWindow() { return 86400; }

        virtual int64_t GetFullAccountComplainsLimit() { return 12; }

        virtual int64_t GetTrialAccountComplainsLimit() { return 6; }

        virtual int64_t GetThresholdReputation() { return 500; }

        virtual int64_t GetComplainsLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullAccountComplainsLimit() : GetTrialAccountComplainsLimit();
        }

        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Complain>(tx);

            // Check registration
            vector<string> addresses = {*ptx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            // Author or post must be exists
            auto postAddress = PocketDb::ConsensusRepoInst.GetPostAddress(*ptx->GetPostTxHash());
            if (postAddress == nullptr)
                return {false, SocialConsensusResult_NotFound};

            // Complain to self
            if (*postAddress == *ptx->GetAddress())
                return {false, SocialConsensusResult_SelfComplain};

            // Check double complain
            if (PocketDb::ConsensusRepoInst.ExistsComplain(*ptx->GetHash(), *ptx->GetPostTxHash(), *ptx->GetAddress()))
                return {false, SocialConsensusResult_DoubleComplain};

            return Success;
        }


        virtual bool CheckBlockLimitTime(shared_ptr<Complain> ptx, shared_ptr<Complain> blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        tuple<bool, SocialConsensusResult>
        ValidateLimit(const shared_ptr<Transaction>& tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Complain>(tx);

            // from chain
            int count = GetChainCount(ptx);

            // from block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_COMPLAIN}))
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
                        return {false, SocialConsensusResult_DoubleComplain};
                }
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Complain>(tx);

            // from chain
            int count = GetChainCount(ptx);

            // from mempool
            count += ConsensusRepoInst.CountMempoolComplain(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Complain> tx, int count)
        {
            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetComplainsLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ComplainLimit};

            auto minimumReputation = GetThresholdReputation();
            if (reputation < minimumReputation)
                return {false, SocialConsensusResult_LowReputation};

            return Success;
        }

        virtual int GetChainCount(const shared_ptr<Complain>& ptx)
        {
            return ConsensusRepoInst.CountChainComplainTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Complain>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetReason())) return {false, SocialConsensusResult_Failed};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 292800 block
    *
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_292800 : public ComplainConsensus
    {
    public:
        ComplainConsensus_checkpoint_292800(int height) : ComplainConsensus(height) {}

    protected:
        int CheckpointHeight() override { return 292800; }

        int64_t GetThresholdReputation() override { return 1000; }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_1124000 : public ComplainConsensus_checkpoint_292800
    {
    public:
        ComplainConsensus_checkpoint_1124000(int height) : ComplainConsensus_checkpoint_292800(height) {}

    protected:
        int CheckpointHeight() override { return 1124000; }

        bool CheckBlockLimitTime(shared_ptr<Complain> ptx, shared_ptr<Complain> blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class ComplainConsensus_checkpoint_1180000 : public ComplainConsensus_checkpoint_1124000
    {
    public:
        ComplainConsensus_checkpoint_1180000(int height) : ComplainConsensus_checkpoint_1124000(height) {}

    protected:
        int CheckpointHeight() override { return 1180000; }

        int64_t GetLimitWindow() override { return 1440; }

        int GetChainCount(const shared_ptr<Complain>& ptx) override
        {
            return ConsensusRepoInst.CountChainComplainHeight(
                *ptx->GetAddress(),
                Height - (int) GetLimitWindow()
            );
        }
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class ComplainConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ComplainConsensus*(int height)>> m_rules =
            {
                {1180000, [](int height) { return new ComplainConsensus_checkpoint_1180000(height); }},
                {1124000, [](int height) { return new ComplainConsensus_checkpoint_1124000(height); }},
                {292800,  [](int height) { return new ComplainConsensus_checkpoint_292800(height); }},
                {0,       [](int height) { return new ComplainConsensus(height); }},
            };
    public:
        shared_ptr<ComplainConsensus> Instance(int height)
        {
            return shared_ptr<ComplainConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMPLAIN_HPP