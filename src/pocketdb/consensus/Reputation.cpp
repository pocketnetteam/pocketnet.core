// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Reputation.h"

namespace PocketConsensus
{
    using namespace std;

    // -------------------------------
    // Consensus checkpoint at 0 block
    int64_t ReputationConsensus::GetMinLikers(int addressId)
    {
        return GetConsensusLimit(ConsensusLimit_threshold_likers_count);
    }

    bool ReputationConsensus::AllowModifyReputation(int addressId)
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

    tuple<int, string> ReputationConsensus::SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
    {
        if (lottery)
            return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);

        return make_tuple(scoreData->ContentAddressId, scoreData->ContentAddressHash);
    }

    bool ReputationConsensus::AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData, const CTransactionRef& tx, bool lottery)
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

        auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreContentCount(Height, scoreData, tx, values, _scores_one_to_one_depth);
        if (scores_one_to_one_count >= _max_scores_one_to_one)
            return false;

        // All its Ok!
        return true;
    }

    bool ReputationConsensus::AllowModifyReputationOverComment(shared_ptr<ScoreDataDto>& scoreData, const CTransactionRef& tx, bool lottery)
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

        auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreCommentCount(Height, scoreData, tx, values, _scores_one_to_one_depth);
        if (scores_one_to_one_count >= _max_scores_one_to_one)
            return false;

        // All its Ok!
        return true;
    }

    AccountMode ReputationConsensus::GetAccountMode(int reputation, int64_t balance)
    {
        if (reputation >= GetConsensusLimit(ConsensusLimit_threshold_reputation) ||
            balance >= GetConsensusLimit(ConsensusLimit_threshold_balance))
            return AccountMode_Full;
        else
            return AccountMode_Trial;
    }
    tuple<AccountMode, int, int64_t> ReputationConsensus::GetAccountMode(string& address)
    {
        auto reputation = PocketDb::ConsensusRepoInst.GetUserReputation(address);
        auto balance = PocketDb::ConsensusRepoInst.GetUserBalance(address);

        return {GetAccountMode(reputation, balance), reputation, balance};
    }

    bool ReputationConsensus::AllowModifyReputation(shared_ptr<ScoreDataDto>& scoreData, const CTransactionRef& tx, bool lottery)
    {
        if (scoreData->ScoreType == TxType::ACTION_SCORE_CONTENT)
            return AllowModifyReputationOverPost(scoreData, tx, lottery);

        if (scoreData->ScoreType == TxType::ACTION_SCORE_COMMENT)
            return AllowModifyReputationOverComment(scoreData, tx, lottery);

        return false;
    }

    bool ReputationConsensus::AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, TxType contentType)
    {
        if (contentType == TxType::CONTENT_POST)
            return (scoreTime - contentTime) < GetConsensusLimit(ConsensusLimit_scores_depth_modify_reputation);

        return true;
    }

    void ReputationConsensus::PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers)
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

    // ------------------------------------
    // Consensus checkpoint at 151600 block
    tuple<int, string> ReputationConsensus_checkpoint_151600::SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
    {
        return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);
    }

    // -------------------------------------
    // Consensus checkpoint at 1180000 block
    int64_t ReputationConsensus_checkpoint_1180000::GetMinLikers(int addressId)
    {
        auto minLikersCount = GetConsensusLimit(ConsensusLimit_threshold_likers_count);
        auto accountRegistrationHeight = PocketDb::ConsensusRepoInst.GetAccountRegistrationHeight(addressId);
        if (Height - accountRegistrationHeight > GetConsensusLimit(ConsensusLimit_threshold_low_likers_depth))
            minLikersCount = GetConsensusLimit(ConsensusLimit_threshold_low_likers_count);

        return minLikersCount;
    }

    // -------------------------------------
    // Consensus checkpoint at 1324655 block
    AccountMode ReputationConsensus_checkpoint_1324655::GetAccountMode(int reputation, int64_t balance)
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

    // ---------------------------------------
    // Consensus checkpoint at 1324655_2 block
    void ReputationConsensus_checkpoint_1324655_2::PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc,
        map<int, vector<int>>& accountLikers)
    {
        for (const auto& account : accountLikersSrc)
            for (const auto& likerId : account.second)
                if (!PocketDb::RatingsRepoInst.ExistsLiker(account.first, likerId, Height))
                    accountLikers[account.first].emplace_back(likerId);
    }

    // --------------------------------------
    ReputationConsensusFactory ReputationConsensusFactoryInst;

}