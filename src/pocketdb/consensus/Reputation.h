// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_REPUTATION_H
#define POCKETCONSENSUS_REPUTATION_H

#include "pocketdb/consensus/Base.h"

namespace PocketConsensus
{
    using namespace std;

    // Consensus checkpoint at 0 block
    class ReputationConsensus : public BaseConsensus
    {
    protected:
        virtual int64_t GetMinLikers(int64_t registrationHeight)
        {
            return GetConsensusLimit(ConsensusLimit_threshold_likers_count);
        }
        
        virtual string SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            if (lottery)
                return scoreData->ScoreAddressHash;

            return scoreData->ContentAddressHash;
        }
        
        virtual bool AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            // Check user reputation
            if (!IsShark(SelectAddressScoreContent(scoreData, lottery), ConsensusLimit_threshold_reputation_score))
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
            if (!IsShark(scoreData->ScoreAddressHash, ConsensusLimit_threshold_reputation_score))
                return false;

            // Disable reputation increment if from one address to one address > Limit::scores_one_to_one scores over Limit::scores_one_to_one_depth
            int64_t _max_scores_one_to_one = GetConsensusLimit(ConsensusLimit_scores_one_to_one_over_comment);
            int64_t _scores_one_to_one_depth = GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth);

            vector<int> values;
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

        virtual void ExtendLikersList(vector<int>& lkrs, int likerId)
        {
            lkrs.clear();
            lkrs.emplace_back(likerId);
        }

        virtual void ExtendLikersList(map<RatingType, map<int, vector<int>>>& lkrs, const ScoreDataDtoRef& scoreData)
        {
            for (const auto& type : lkrs)
                if (type.first != RatingType::RATING_ACCOUNT_LIKERS)
                    lkrs[type.first][scoreData->ContentAddressId].clear();

            lkrs[scoreData->LikerType()][scoreData->ContentAddressId].emplace_back(scoreData->ScoreAddressId);
        }

        // TODO (brangr): implement new logic for only comments and dislikers
        virtual bool ValidateScoreValue(const ScoreDataDtoRef& scoreData)
        {
            // For scores to posts allowed only 4 and 5 values
            if (scoreData->ScoreType == ACTION_SCORE_CONTENT)
                if (scoreData->ScoreValue == 4 || scoreData->ScoreValue == 5)
                    return true;
            
            // For scores to comments allowed only 1 value
            if (scoreData->ScoreType == ACTION_SCORE_COMMENT)
                if (scoreData->ScoreValue == 1)
                    return true;
                    
            // Another types not allowed
            return false;
        }

    public:
        explicit ReputationConsensus(int height) : BaseConsensus(height) {}

        virtual bool IsShark(const AccountData& accountData, ConsensusLimit limit = ConsensusLimit_threshold_reputation)
        {
            auto minReputation = GetConsensusLimit(limit);
            auto minLikersCount = GetMinLikers(accountData.RegistrationHeight);

            return accountData.Reputation >= minReputation && accountData.LikersCount >= minLikersCount;
        }

        virtual bool IsShark(const string& address, ConsensusLimit limit = ConsensusLimit_threshold_reputation)
        {
            auto accountData = ConsensusRepoInst.GetAccountData(address);
            return IsShark(accountData, limit);
        }

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
                    return (scoreTime - contentTime) < GetConsensusLimit(ConsensusLimit_scores_depth_modify_reputation);
                default:
                    return true;
            }
        }
    
        virtual int GetScoreContentAuthorValue(int scoreValue)
        {
            return (scoreValue - 3) * 10;
        }
    
        virtual int GetScoreCommentAuthorValue(int scoreValue)
        {
            return scoreValue;
        }

        virtual void ValidateAccountLiker(const ScoreDataDtoRef& scoreData, map<RatingType, map<int, vector<int>>>& likersValues)
        {
            // Check general score data
            if (!ValidateScoreValue(scoreData))
                return;

            // Check already added to list and exists in DB
            auto& lkrs = likersValues[RATING_ACCOUNT_LIKERS][scoreData->ContentAddressId];
            if (find(lkrs.begin(), lkrs.end(), scoreData->ScoreAddressId) == lkrs.end())
            {
                if (!PocketDb::RatingsRepoInst.ExistsLiker(scoreData->ContentAddressId, scoreData->ScoreAddressId))
                {
                    ExtendLikersList(
                        lkrs,
                        scoreData->ScoreAddressId
                    );
                }
            }

            //
            // Split likers types
            // 
            auto& lkrs_post = likersValues[RATING_ACCOUNT_LIKERS_POST][scoreData->ContentAddressId];
            if ((find(lkrs_post.begin(), lkrs_post.end(), scoreData->ScoreAddressId) != lkrs_post.end()))
                return;
            
            auto& lkrs_cmnt_root = likersValues[RATING_ACCOUNT_LIKERS_COMMENT_ROOT][scoreData->ContentAddressId];
            if ((find(lkrs_cmnt_root.begin(), lkrs_cmnt_root.end(), scoreData->ScoreAddressId) != lkrs_cmnt_root.end()))
                return;

            auto& lkrs_cmnt_answer = likersValues[RATING_ACCOUNT_LIKERS_COMMENT_ANSWER][scoreData->ContentAddressId];
            if ((find(lkrs_cmnt_answer.begin(), lkrs_cmnt_answer.end(), scoreData->ScoreAddressId) != lkrs_cmnt_answer.end()))
                return;
                
            if (!PocketDb::RatingsRepoInst.ExistsLiker(scoreData->ContentAddressId, scoreData->ScoreAddressId, Height))
            {
                ExtendLikersList(
                    likersValues,
                    scoreData
                );
            }
        }
    };

    // Consensus checkpoint at 151600 block
    class ReputationConsensus_checkpoint_151600 : public ReputationConsensus
    {
    public:
        explicit ReputationConsensus_checkpoint_151600(int height) : ReputationConsensus(height) {}
    protected:
        string SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery) override
        {
            return scoreData->ScoreAddressHash;
        }
    };

    // Consensus checkpoint at 1180000 block
    class ReputationConsensus_checkpoint_1180000 : public ReputationConsensus_checkpoint_151600
    {
    public:
        explicit ReputationConsensus_checkpoint_1180000(int height) : ReputationConsensus_checkpoint_151600(height) {}
    protected:
        int64_t GetMinLikers(int64_t registrationHeight) override
        {
            if (Height - registrationHeight > GetConsensusLimit(ConsensusLimit_threshold_low_likers_depth))
                return GetConsensusLimit(ConsensusLimit_threshold_low_likers_count);

            return GetConsensusLimit(ConsensusLimit_threshold_likers_count);
        }
    };

    class ReputationConsensus_checkpoint_1324655 : public ReputationConsensus_checkpoint_1180000
    {
    protected:
        void ExtendLikersList(vector<int>& lkrs, int likerId) override
        {
            lkrs.emplace_back(likerId);
        }
        void ExtendLikersList(map<RatingType, map<int, vector<int>>>& lkrs, const ScoreDataDtoRef& scoreData) override
        {
            lkrs[scoreData->LikerType()][scoreData->ContentAddressId].emplace_back(scoreData->ScoreAddressId);
        }
    public:
        explicit ReputationConsensus_checkpoint_1324655(int height) : ReputationConsensus_checkpoint_1180000(height) {}
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

    // Consensus checkpoint: reducing the impact on the reputation of scores 1,2 for content
    class ReputationConsensus_checkpoint_scores_content_author_reducing_impact : public ReputationConsensus_checkpoint_1324655
    {
    public:
        explicit ReputationConsensus_checkpoint_scores_content_author_reducing_impact(int height) : ReputationConsensus_checkpoint_1324655(height) {}
        int GetScoreContentAuthorValue(int scoreValue) override
        {
            int multiplier = 10;
            if (scoreValue == 1 || scoreValue == 2)
                multiplier = 1;
            
            return (scoreValue - 3) * multiplier;
        }
        int GetScoreCommentAuthorValue(int scoreValue) override
        {
            if (scoreValue == -1)
                return 0;
            
            return scoreValue;
        }
    };


    //  Factory for select actual rules version
    class ReputationConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ReputationConsensus>> m_rules = {
            { 0,           -1, [](int height) { return make_shared<ReputationConsensus>(height); }},
            { 151600,      -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_151600>(height); }},
            { 1180000,      0, [](int height) { return make_shared<ReputationConsensus_checkpoint_1180000>(height); }},
            { 1324655,  65000, [](int height) { return make_shared<ReputationConsensus_checkpoint_1324655>(height); }},
            { 1700000, 761000, [](int height) { return make_shared<ReputationConsensus_checkpoint_scores_content_author_reducing_impact>(height); }},
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