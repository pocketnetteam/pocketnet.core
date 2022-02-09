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
        virtual int64_t GetMinLikers(int addressId)
        {
            return GetConsensusLimit(ConsensusLimit_threshold_likers_count);
        }

        virtual bool AllowModifyReputation(int addressId)
        {
            auto minUserReputation = GetConsensusLimit(ConsensusLimit_threshold_reputation_score);
            auto userReputation = PocketDb::ConsensusRepoInst.GetUserReputation(addressId);
            if (userReputation < minUserReputation)
                return false;

            auto minLikersCount = GetMinLikers(addressId);
            auto userLikers = PocketDb::ConsensusRepoInst.GetUserLikersCount(addressId);
            if (userLikers < minLikersCount)
                return false;

            // All is OK
            return true;
        }

        virtual tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            if (lottery)
                return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);

            return make_tuple(scoreData->ContentAddressId, scoreData->ContentAddressHash);
        }

        virtual bool AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            auto[checkScoreAddressId, checkScoreAddressHash] = SelectAddressScoreContent(scoreData, lottery);

            // Check user reputation
            if (!AllowModifyReputation(checkScoreAddressId))
                return false;

            // Disable reputation increment if from one address to one address > 2 scores over day
            int64_t _max_scores_one_to_one = GetConsensusLimit(ConsensusLimit_scores_one_to_one);
            int64_t _scores_one_to_one_depth = GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth);

            std::vector<int> values;
            if (lottery)
            {
                values.push_back(4);
                values.push_back(5);
            }
            else
            {
                values.push_back(1);
                values.push_back(2);
                values.push_back(3);
                values.push_back(4);
                values.push_back(5);
            }

            auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreContentCount(
                Height, scoreData, values, _scores_one_to_one_depth);

            if (scores_one_to_one_count >= _max_scores_one_to_one)
                return false;

            // All its Ok!
            return true;
        }
        
        virtual bool AllowModifyReputationOverComment(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            // Check user reputation
            if (!AllowModifyReputation(scoreData->ScoreAddressId))
                return false;

            // Disable reputation increment if from one address to one address > Limit::scores_one_to_one scores over Limit::scores_one_to_one_depth
            int64_t _max_scores_one_to_one = GetConsensusLimit(ConsensusLimit_scores_one_to_one_over_comment);
            int64_t _scores_one_to_one_depth = GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth);

            std::vector<int> values;
            if (lottery)
            {
                values.push_back(1);
            }
            else
            {
                values.push_back(-1);
                values.push_back(1);
            }

            auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreCommentCount(
                Height, scoreData, values, _scores_one_to_one_depth);

            if (scores_one_to_one_count >= _max_scores_one_to_one)
                return false;

            // All its Ok!
            return true;
        }

    public:
        explicit ReputationConsensus(int height) : BaseConsensus(height) {}

        virtual AccountMode GetAccountMode(int reputation, int64_t balance)
        {
            if (reputation >= GetConsensusLimit(ConsensusLimit_threshold_reputation) ||
                balance >= GetConsensusLimit(ConsensusLimit_threshold_balance))
                return AccountMode_Full;
            else
                return AccountMode_Trial;
        }

        virtual tuple<AccountMode, int, int64_t> GetAccountMode(string& address)
        {
            auto reputation = PocketDb::ConsensusRepoInst.GetUserReputation(address);
            auto balance = PocketDb::ConsensusRepoInst.GetUserBalance(address);

            return {GetAccountMode(reputation, balance), reputation, balance};
        }

        virtual int GetAccountUserBadges(string& address)
        {
            // TODO (brangr) !!! : implement select user account data and build bit mask

            return 0;
        }

        virtual bool AllowModifyReputation(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            if (scoreData->ScoreType == TxType::ACTION_SCORE_CONTENT)
                return AllowModifyReputationOverPost(scoreData, lottery);

            if (scoreData->ScoreType == TxType::ACTION_SCORE_COMMENT)
                return AllowModifyReputationOverComment(scoreData, lottery);

            return false;
        }

        virtual bool AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, TxType contentType)
        {
            switch (contentType)
            {
                case CONTENT_POST:
                case CONTENT_VIDEO:
                case CONTENT_ARTICLE:
                // case CONTENT_SERVERPING:
                    return (scoreTime - contentTime) < GetConsensusLimit(ConsensusLimit_scores_depth_modify_reputation);
                default:
                    return true;
            }
        }

        virtual void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers)
        {
            for (const auto& account : accountLikersSrc)
            {
                for (const auto& likerId : account.second)
                {
                    if (!PocketDb::RatingsRepoInst.ExistsLiker(account.first, likerId, Height))
                    {
                        accountLikers[account.first].clear();
                        accountLikers[account.first].emplace_back(likerId);
                    }
                }
            }
        }
    };

    // ------------------------------------------
    // Fix select Address of Score
    class ReputationConsensus_checkpoint_SelectAddressScoreContent : public ReputationConsensus
    {
    public:
        explicit ReputationConsensus_checkpoint_SelectAddressScoreContent(int height) : ReputationConsensus(height) {}
    protected:
        tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery) override
        {
            return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);
        }
    };

    // ------------------------------------------
    // Fix minimum likers count
    class ReputationConsensus_checkpoint_GetMinLikers : public ReputationConsensus_checkpoint_SelectAddressScoreContent
    {
    public:
        explicit ReputationConsensus_checkpoint_GetMinLikers(int height) : ReputationConsensus_checkpoint_SelectAddressScoreContent(height) {}
    protected:
        int64_t GetMinLikers(int addressId) override
        {
            auto minLikersCount = GetConsensusLimit(ConsensusLimit_threshold_likers_count);
            auto accountRegistrationHeight = PocketDb::ConsensusRepoInst.GetAccountRegistrationHeight(addressId);
            if (Height - accountRegistrationHeight > GetConsensusLimit(ConsensusLimit_threshold_low_likers_depth))
                minLikersCount = GetConsensusLimit(ConsensusLimit_threshold_low_likers_count);

            return minLikersCount;
        }
    };

    // ------------------------------------------
    // Fix account mode select
    class ReputationConsensus_checkpoint_GetAccountMode : public ReputationConsensus_checkpoint_GetMinLikers
    {
    public:
        explicit ReputationConsensus_checkpoint_GetAccountMode(int height) : ReputationConsensus_checkpoint_GetMinLikers(height) {}
        AccountMode GetAccountMode(int reputation, int64_t balance) override
        {
            if (reputation >= GetConsensusLimit(ConsensusLimit_threshold_reputation)
                || balance >= GetConsensusLimit(ConsensusLimit_threshold_balance))
            {
                if (balance >= GetConsensusLimit(ConsensusLimit_threshold_balance_pro))
                    return AccountMode_Pro;
                else
                    return AccountMode_Full;
            }
            else
            {
                return AccountMode_Trial;
            }
        }
    };

    // ------------------------------------------
    // Fix calculate account likers
    class ReputationConsensus_checkpoint_PrepareAccountLikers : public ReputationConsensus_checkpoint_GetAccountMode
    {
    public:
        explicit ReputationConsensus_checkpoint_PrepareAccountLikers(int height) : ReputationConsensus_checkpoint_GetAccountMode(height) {}
        void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers) override
        {
            for (const auto& account : accountLikersSrc)
                for (const auto& likerId : account.second)
                    if (!PocketDb::RatingsRepoInst.ExistsLiker(account.first, likerId, Height))
                        accountLikers[account.first].emplace_back(likerId);
        }
    };

    // ------------------------------------------
    // New AccountUser badges - https://github.com/pocketnetteam/pocketnet.core/issues/186
    class ReputationConsensus_checkpoint_AccountUserBadges : public ReputationConsensus_checkpoint_PrepareAccountLikers
    {
    public:
        explicit ReputationConsensus_checkpoint_AccountUserBadges(int height) : ReputationConsensus_checkpoint_PrepareAccountLikers(height) {}

        // TODO (brangr): override AllowModifyReputation
    };


    // ------------------------------------------
    //  Factory for select actual rules version
    class ReputationConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ReputationConsensus>> m_rules = {
            {0,          -1, [](int height) { return make_shared<ReputationConsensus>(height); }},
            {151600,     -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_SelectAddressScoreContent>(height); }},
            {1180000,     0, [](int height) { return make_shared<ReputationConsensus_checkpoint_GetMinLikers>(height); }},
            {1324655, 65000, [](int height) { return make_shared<ReputationConsensus_checkpoint_GetAccountMode>(height); }},
            {1324655, 75000, [](int height) { return make_shared<ReputationConsensus_checkpoint_PrepareAccountLikers>(height); }},
            // TODO (brangr) !!! : set height
            {9999999, 9999999, [](int height) { return make_shared<ReputationConsensus_checkpoint_AccountUserBadges>(height); }},
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

    static ReputationConsensusFactory ReputationConsensusFactoryInst;
}

#endif // POCKETCONSENSUS_REPUTATION_H