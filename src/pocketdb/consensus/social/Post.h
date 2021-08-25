// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_H
#define POCKETCONSENSUS_POST_H

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/Post.h"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Post consensus base class
    *******************************************************************************************************************/
    class PostConsensus : public SocialConsensus
    {
    public:
        PostConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override;

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override;

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetEditWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetEditLimit() { return 5; }
        virtual int64_t GetLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }


        virtual ConsensusValidateResult ValidateEdit(const PTransactionRef& ptx);

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override;

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override;

        virtual ConsensusValidateResult ValidateLimit(const PTransactionRef& ptx, int count);

        virtual bool AllowBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx);

        virtual bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx);

        virtual int GetChainCount(const PTransactionRef& ptx);

        virtual ConsensusValidateResult ValidateEditBlock(const PTransactionRef& ptx, const PocketBlockRef& block);

        virtual ConsensusValidateResult ValidateEditMempool(const PTransactionRef& ptx);

        virtual ConsensusValidateResult ValidateEditOneLimit(const PTransactionRef& ptx);

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override;
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1124000 : public PostConsensus
    {
    public:
        PostConsensus_checkpoint_1124000(int height) : PostConsensus(height) {}
    protected:
        bool AllowBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override;
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1180000 : public PostConsensus_checkpoint_1124000
    {
    public:
        PostConsensus_checkpoint_1180000(int height) : PostConsensus_checkpoint_1124000(height) {}
    protected:
        int64_t GetEditWindow() override { return 1440; }
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const PTransactionRef& ptx) override;
        bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx) override;
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1324655 : public PostConsensus_checkpoint_1180000
    {
    public:
        PostConsensus_checkpoint_1324655(int height) : PostConsensus_checkpoint_1180000(height) {}
    protected:
        int64_t GetTrialLimit() override { return 5; }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class PostConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<PostConsensus>(height); }},
            {1124000, -1, [](int height) { return make_shared<PostConsensus_checkpoint_1124000>(height); }},
            {1180000, -1, [](int height) { return make_shared<PostConsensus_checkpoint_1180000>(height); }},
            {1324655, 0,  [](int height) { return make_shared<PostConsensus_checkpoint_1324655>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_POST_H