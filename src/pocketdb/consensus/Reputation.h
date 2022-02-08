// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_REPUTATION_H
#define POCKETCONSENSUS_REPUTATION_H

#include "pocketdb/consensus/Base.h"

namespace PocketConsensus
{
    using namespace std;

    // ------------------------------------------
    // General rules
    class ReputationConsensus : public BaseConsensus
    {
    protected:
        virtual int64_t GetMinLikers(int addressId);
        virtual bool AllowModifyReputation(int addressId);
        virtual tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery);
        virtual bool AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData, bool lottery);
        virtual bool AllowModifyReputationOverComment(shared_ptr<ScoreDataDto>& scoreData, bool lottery);

    public:
        explicit ReputationConsensus(int height) : BaseConsensus(height) {}

        virtual AccountMode GetAccountMode(int reputation, int64_t balance);
        virtual tuple<AccountMode, int, int64_t> GetAccountMode(string& address);
        virtual int GetAccountUserBadges(string& address);

        virtual bool AllowModifyReputation(shared_ptr<ScoreDataDto>& scoreData, bool lottery);
        virtual bool AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, TxType contentType);

        virtual void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers);
    };

    // ------------------------------------------
    // Fix select Address of Score
    class ReputationConsensus_checkpoint_SelectAddressScoreContent : public ReputationConsensus
    {
    public:
        explicit ReputationConsensus_checkpoint_SelectAddressScoreContent(int height) : ReputationConsensus(height) {}
    protected:
        tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery) override;
    };

    // ------------------------------------------
    // Fix minimum likers count
    class ReputationConsensus_checkpoint_GetMinLikers : public ReputationConsensus_checkpoint_SelectAddressScoreContent
    {
    public:
        explicit ReputationConsensus_checkpoint_GetMinLikers(int height) : ReputationConsensus_checkpoint_SelectAddressScoreContent(height) {}
    protected:
        int64_t GetMinLikers(int addressId) override;
    };

    // ------------------------------------------
    // Fix account mode select
    class ReputationConsensus_checkpoint_GetAccountMode : public ReputationConsensus_checkpoint_GetMinLikers
    {
    public:
        explicit ReputationConsensus_checkpoint_GetAccountMode(int height) : ReputationConsensus_checkpoint_GetMinLikers(height) {}
        AccountMode GetAccountMode(int reputation, int64_t balance) override;
    };

    // ------------------------------------------
    // Fix calculate account likers
    class ReputationConsensus_checkpoint_PrepareAccountLikers : public ReputationConsensus_checkpoint_GetAccountMode
    {
    public:
        explicit ReputationConsensus_checkpoint_PrepareAccountLikers(int height) : ReputationConsensus_checkpoint_GetAccountMode(height) {}
        void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers) override;
    };

    // ------------------------------------------
    // New AccountUser badges - https://github.com/pocketnetteam/pocketnet.core/issues/186
    class ReputationConsensus_checkpoint_AccountUserBadges : public ReputationConsensus_checkpoint_PrepareAccountLikers
    {
    public:
        explicit ReputationConsensus_checkpoint_AccountUserBadges(int height) : ReputationConsensus_checkpoint_PrepareAccountLikers(height) {}
        int GetAccountUserBadges(string& address) override;
    };


    // ------------------------------------------
    //  Factory for select actual rules version
    class ReputationConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ReputationConsensus>> m_rules = {
            {0,       -1,    [](int height) { return make_shared<ReputationConsensus>(height); }},
            {151600,  -1,    [](int height) { return make_shared<ReputationConsensus_checkpoint_SelectAddressScoreContent>(height); }},
            {1180000, 0,     [](int height) { return make_shared<ReputationConsensus_checkpoint_GetMinLikers>(height); }},
            {1324655, 65000, [](int height) { return make_shared<ReputationConsensus_checkpoint_GetAccountMode>(height); }},
            {1324655, 75000, [](int height) { return make_shared<ReputationConsensus_checkpoint_PrepareAccountLikers>(height); }},
            // TODO (brangr) !!! : set height
            {9999999999, 9999999999, [](int height) { return make_shared<ReputationConsensus_checkpoint_AccountUserBadges>(height); }},
        };
    public:
        shared_ptr<ReputationConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ReputationConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };

    extern ReputationConsensusFactory ReputationConsensusFactoryInst;
}

#endif // POCKETCONSENSUS_REPUTATION_H