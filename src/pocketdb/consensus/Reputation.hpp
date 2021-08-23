// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_REPUTATION_HPP
#define POCKETCONSENSUS_REPUTATION_HPP

#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/consensus/Social.hpp"

namespace PocketConsensus
{
    using namespace std;

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 0 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus : public BaseConsensus
    {
    protected:
        virtual int64_t ThresholdLikersCount() { return 0; }
        virtual int64_t GetMinBalanceFull() { return 50 * COIN; }
        virtual int64_t GetMinBalancePro() { return 250 * COIN; }
        virtual int64_t GetMinReputationFull() { return 500; }
        virtual int64_t GetThresholdReputationScore() { return -10000; }
        virtual int64_t GetScoresOneToOneOverComment() { return 20; }
        virtual int64_t GetScoresOneToOne() { return 99999; }
        virtual int64_t GetScoresOneToOneDepth() { return 336 * 24 * 3600; }
        virtual int64_t GetScoresToPostModifyReputationDepth() { return 336 * 24 * 3600; }


        virtual int64_t GetMinLikers(int addressId)
        {
            return ThresholdLikersCount();
        }

        virtual bool AllowModifyReputation(int addressId)
        {
            // Ignore scores from users with rating < Antibot::Limit::threshold_reputation_score
            auto minUserReputation = GetThresholdReputationScore();
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

        virtual bool AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData,
            const CTransactionRef& tx, bool lottery)
        {
            auto[checkScoreAddressId, checkScoreAddressHash] = SelectAddressScoreContent(scoreData, lottery);

            // Check user reputation
            if (!AllowModifyReputation(checkScoreAddressId))
                return false;

            // Disable reputation increment if from one address to one address > 2 scores over day
            int64_t _max_scores_one_to_one = GetScoresOneToOne();
            int64_t _scores_one_to_one_depth = GetScoresOneToOneDepth();

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

        virtual bool AllowModifyReputationOverComment(shared_ptr<ScoreDataDto>& scoreData,
            const CTransactionRef& tx, bool lottery)
        {
            // Check user reputation
            if (!AllowModifyReputation(scoreData->ScoreAddressId))
                return false;

            // Disable reputation increment if from one address to one address > Limit::scores_one_to_one scores over Limit::scores_one_to_one_depth
            int64_t _max_scores_one_to_one = GetScoresOneToOneOverComment();
            int64_t _scores_one_to_one_depth = GetScoresOneToOneDepth();

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

    public:
        ReputationConsensus(int height) : BaseConsensus(height) {}

        virtual tuple<AccountMode, int, int64_t> GetAccountInfo(string& address)
        {
            auto reputation = PocketDb::ConsensusRepoInst.GetUserReputation(address);
            auto balance = PocketDb::ConsensusRepoInst.GetUserBalance(address);

            if (reputation >= GetMinReputationFull() || balance >= GetMinBalanceFull())
                return {AccountMode_Full, reputation, balance};
            else
                return {AccountMode_Trial, reputation, balance};
        }

        virtual AccountMode GetAccountMode(string& address)
        {
            auto[mode, reputation, balance] = GetAccountInfo(address);
            return mode;
        }

        virtual bool AllowModifyReputation(shared_ptr<ScoreDataDto>& scoreData, const CTransactionRef& tx,
            bool lottery)
        {
            if (scoreData->ScoreType == PocketTxType::ACTION_SCORE_CONTENT)
                return AllowModifyReputationOverPost(scoreData, tx, lottery);

            if (scoreData->ScoreType == PocketTxType::ACTION_SCORE_COMMENT)
                return AllowModifyReputationOverComment(scoreData, tx, lottery);

            return false;
        }

        // TODO (brangr) v0.21.0: we need to limit the depth for all content types
        virtual bool AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, PocketTxType contentType)
        {
            if (contentType == PocketTxType::CONTENT_POST)
                return (scoreTime - contentTime) < GetScoresToPostModifyReputationDepth();

            return true;
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

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 108300 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_108300 : public ReputationConsensus
    {
    protected:
        int64_t GetThresholdReputationScore() override { return 500; }
    public:
        ReputationConsensus_checkpoint_108300(int height) : ReputationConsensus(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 151600 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_151600 : public ReputationConsensus_checkpoint_108300
    {
    protected:
        tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery) override
        {
            return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);
        }

    public:
        ReputationConsensus_checkpoint_151600(int height) : ReputationConsensus_checkpoint_108300(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 225000 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_225000 : public ReputationConsensus_checkpoint_151600
    {
    protected:
        int64_t GetScoresOneToOne() override { return 2; }
        int64_t GetScoresOneToOneDepth() override { return 1 * 24 * 3600; }
    public:
        ReputationConsensus_checkpoint_225000(int height) : ReputationConsensus_checkpoint_151600(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 292800 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_292800 : public ReputationConsensus_checkpoint_225000
    {
    protected:
        int64_t GetThresholdReputationScore() override { return 1000; }
        int64_t GetMinReputationFull() override { return 1000; }
        int64_t GetScoresOneToOneDepth() override { return 7 * 24 * 3600; }
    public:
        ReputationConsensus_checkpoint_292800(int height) : ReputationConsensus_checkpoint_225000(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 322700 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_322700 : public ReputationConsensus_checkpoint_292800
    {
    protected:
        int64_t GetScoresOneToOneDepth() override { return 2 * 24 * 3600; }
        int64_t GetScoresToPostModifyReputationDepth() override { return 30 * 24 * 3600; }
    public:
        ReputationConsensus_checkpoint_322700(int height) : ReputationConsensus_checkpoint_292800(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 1124000 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_1124000 : public ReputationConsensus_checkpoint_322700
    {
    protected:
        int64_t ThresholdLikersCount() override { return 100; }
    public:
        ReputationConsensus_checkpoint_1124000(int height) : ReputationConsensus_checkpoint_322700(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 1180000 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_1180000 : public ReputationConsensus_checkpoint_1124000
    {
    protected:
        virtual int LowThresholdLikersDepth() { return 250'000; }
        virtual int LowThresholdLikersCount() { return 30; }

        int64_t GetMinLikers(int addressId) override
        {
            auto minLikersCount = ThresholdLikersCount();
            auto accountRegistrationHeight = PocketDb::ConsensusRepoInst.GetAccountRegistrationHeight(addressId);
            if (Height - accountRegistrationHeight > LowThresholdLikersDepth())
                minLikersCount = LowThresholdLikersCount();

            return minLikersCount;
        }
    public:
        ReputationConsensus_checkpoint_1180000(int height) : ReputationConsensus_checkpoint_1124000(height) {}
    };

    /******************************************************************************************************************/
    /*                                Consensus checkpoint at 1324655 block                                           */
    /******************************************************************************************************************/
    class ReputationConsensus_checkpoint_1324655 : public ReputationConsensus_checkpoint_1180000
    {
    protected:
    public:
        ReputationConsensus_checkpoint_1324655(int height) : ReputationConsensus_checkpoint_1180000(height) {}

        void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers) override
        {
            for (const auto& account : accountLikersSrc)
                for (const auto& likerId : account.second)
                    if (!PocketDb::RatingsRepoInst.ExistsLiker(account.first, likerId, Height))
                        accountLikers[account.first].emplace_back(likerId);
        }

        tuple<AccountMode, int, int64_t> GetAccountInfo(string& address) override
        {
            auto reputation = PocketDb::ConsensusRepoInst.GetUserReputation(address);
            auto balance = PocketDb::ConsensusRepoInst.GetUserBalance(address);

            if (reputation >= GetMinReputationFull() || balance >= GetMinBalanceFull())
            {
                if (balance >= GetMinBalancePro())
                {
                    return {AccountMode_Pro, reputation, balance};
                }
                else
                {
                    return {AccountMode_Full, reputation, balance};
                }
            }
            else
            {
                return {AccountMode_Trial, reputation, balance};
            }
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class ReputationConsensusFactory : public BaseConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<ReputationConsensus>(height); }},
            {108300,  -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_108300>(height); }},
            {151600,  -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_151600>(height); }},
            {225000,  -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_225000>(height); }},
            {292800,  -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_292800>(height); }},
            {322700,  -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_322700>(height); }},
            {1124000, -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_1124000>(height); }},
            {1180000, -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_1180000>(height); }},
            {1324655,  0, [](int height) { return make_shared<ReputationConsensus_checkpoint_1324655>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    public:
        shared_ptr<ReputationConsensus> Instance(int height)
        {
            return static_pointer_cast<ReputationConsensus>(
                BaseConsensusFactory::m_instance(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_REPUTATION_HPP