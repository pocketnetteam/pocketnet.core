// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POSTT_H
#define POCKETCONSENSUS_POSTT_H

#include "pocketdb/consensus/SocialT.h"
#include "pocketdb/models/dto/Post.h"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Post consensus base class
    *******************************************************************************************************************/
    class PostConsensusT : public SocialConsensusT<Post>
    {
    public:
        PostConsensusT(int height);
        tuple<bool, SocialConsensusResult> Validate(const shared_ptr<Post>& ptx, const PocketBlockRef& block) override;
        tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const shared_ptr<Post>& ptx) override;

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetEditWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetEditLimit() { return 5; }
        virtual int64_t GetLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }


        virtual tuple<bool, SocialConsensusResult> ValidateEdit(const shared_ptr<Post>& ptx);

        tuple<bool, SocialConsensusResult> ValidateBlock(const shared_ptr<Post>& ptx, const PocketBlockRef& block) override;

        tuple<bool, SocialConsensusResult> ValidateMempool(const shared_ptr<Post>& ptx) override;

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Post>& ptx, int count);

        virtual bool AllowBlockLimitTime(const shared_ptr<Post>& ptx, const shared_ptr<Post>& blockPtx);

        virtual bool AllowEditWindow(const shared_ptr<Post>& ptx, const shared_ptr<Post>& originalTx);

        virtual int GetChainCount(const shared_ptr<Post>& ptx);

        virtual tuple<bool, SocialConsensusResult> ValidateEditBlock(const shared_ptr<Post>& ptx, const PocketBlockRef& block);

        virtual tuple<bool, SocialConsensusResult> ValidateEditMempool(const shared_ptr<Post>& ptx);

        virtual tuple<bool, SocialConsensusResult> ValidateEditOneLimit(const shared_ptr<Post>& ptx);

        vector<string> GetAddressesForCheckRegistration(const shared_ptr<Post>& ptx) override;
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class PostConsensusT_checkpoint_1124000 : public PostConsensusT
    {
    public:
        PostConsensusT_checkpoint_1124000(int height) : PostConsensusT(height) {}
    protected:
        bool AllowBlockLimitTime(const shared_ptr<Post>& ptx, const shared_ptr<Post>& blockPtx) override;
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class PostConsensusT_checkpoint_1180000 : public PostConsensusT_checkpoint_1124000
    {
    public:
        PostConsensusT_checkpoint_1180000(int height) : PostConsensusT_checkpoint_1124000(height) {}
    protected:
        int64_t GetEditWindow() override { return 1440; }
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const shared_ptr<Post>& ptx) override;
        bool AllowEditWindow(const shared_ptr<Post>& ptx, const shared_ptr<Post>& originalTx) override;
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/
    class PostConsensusT_checkpoint_1324655 : public PostConsensusT_checkpoint_1180000
    {
    public:
        PostConsensusT_checkpoint_1324655(int height) : PostConsensusT_checkpoint_1180000(height) {}
    protected:
        int64_t GetTrialLimit() override { return 5; }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class PostConsensusFactoryT
    {
    protected:
        const vector<ConsensusCheckpointT<PostConsensusT>> m_rules =
        {
            {0,       -1, [](int height) { return make_shared<PostConsensusT>(height); }},
            {1124000, -1, [](int height) { return make_shared<PostConsensusT_checkpoint_1124000>(height); }},
            {1180000, -1, [](int height) { return make_shared<PostConsensusT_checkpoint_1180000>(height); }},
            {1324655, 0,  [](int height) { return make_shared<PostConsensusT_checkpoint_1324655>(height); }},
        };
    public:
        shared_ptr<PostConsensusT> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpointT<PostConsensusT>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_POSTT_H