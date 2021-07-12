// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_REPUTATION_HPP
#define POCKETCONSENSUS_REPUTATION_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Reputation consensus base class
    *
    *******************************************************************************************************************/
    class ReputationConsensus : public BaseConsensus
    {
    protected:
        virtual int64_t GetThresholdLikersCount() { return 0; }
        virtual int64_t GetThresholdBalance() { return 50 * COIN; }
        virtual int64_t GetThresholdReputation() { return 500; }
        virtual int64_t GetThresholdReputationScore() { return -10000; }
        virtual int64_t GetScoresOneToOneOverComment() { return 20; }
        virtual int64_t GetScoresOneToOne() { return 99999; }
        virtual int64_t GetScoresOneToOneDepth() { return 336 * 24 * 3600; }
        virtual int64_t GetScoresToPostModifyReputationDepth() { return 336 * 24 * 3600; }


        virtual bool AllowModifyReputation(int addressId, int height)
        {
            // Ignore scores from users with rating < Antibot::Limit::threshold_reputation_score
            auto minUserReputation = GetThresholdReputationScore();
            auto minLikersCount = GetThresholdLikersCount();

            int64_t nTime1 = GetTimeMicros();

            auto userReputation = PocketDb::ConsensusRepoInst.GetUserReputation(addressId);

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "        - GetUserReputation: %.2fms _ %d\n",
                0.001 * (nTime2 - nTime1), addressId);

            if (userReputation < minUserReputation)
                return false;

            auto userLikers = PocketDb::ConsensusRepoInst.GetUserLikersCount(addressId);

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "        - GetUserLikersCount: %.2fms _ %d\n",
                0.001 * (nTime3 - nTime2), addressId);

            if (userLikers < minLikersCount)
                return false;

            // All is OK
            return true;
        }

        virtual tuple<int, string> SelectAddressScoreContent(shared_ptr <ScoreDataDto> scoreData, bool lottery)
        {
            if (lottery)
                return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);

            return make_tuple(scoreData->ContentAddressId, scoreData->ContentAddressHash);
        }

        virtual bool AllowModifyReputationOverPost(shared_ptr <ScoreDataDto> scoreData, int height,
            const CTransactionRef& tx, bool lottery)
        {
            auto[checkScoreAddressId, checkScoreAddressHash] = SelectAddressScoreContent(scoreData, lottery);

            // Check user reputation
            if (!AllowModifyReputation(checkScoreAddressId, height))
                return false;

            int64_t nTime1 = GetTimeMicros();

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

            auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreContentCount(
                scoreData->ScoreType, scoreData->ContentType,
                checkScoreAddressHash, scoreData->ContentAddressHash,
                height, tx, values, _scores_one_to_one_depth);

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "        - GetScoreContentCount (Post): %.2fms _ %s\n",
                0.001 * (nTime2 - nTime1), checkScoreAddressHash);

            if (scores_one_to_one_count >= _max_scores_one_to_one)
                return false;

            // All its Ok!
            return true;
        }

        virtual bool AllowModifyReputationOverComment(shared_ptr <ScoreDataDto> scoreData, int height,
            const CTransactionRef& tx, bool lottery)
        {
            // Check user reputation
            if (!AllowModifyReputation(scoreData->ScoreAddressId, height))
                return false;

            int64_t nTime1 = GetTimeMicros();

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

            auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreContentCount(
                scoreData->ScoreType, scoreData->ContentType,
                scoreData->ScoreAddressHash, scoreData->ContentAddressHash,
                height, tx, values, _scores_one_to_one_depth);

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "        - GetScoreContentCount (Comment): %.2fms _ %s\n",
                0.001 * (nTime2 - nTime1), scoreData->ScoreAddressHash);

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

            if (reputation >= GetThresholdReputation() || balance >= GetThresholdBalance())
                return {AccountMode_Full, reputation, balance};
            else
                return {AccountMode_Trial, reputation, balance};
        }

        virtual AccountMode GetAccountMode(string& address)
        {
            auto[mode, reputation, balance] = GetAccountInfo(address);
            return mode;
        }

        virtual bool AllowModifyReputation(shared_ptr <ScoreDataDto> scoreData, const CTransactionRef& tx, int height,
            bool lottery)
        {
            if (scoreData->ScoreType == PocketTxType::ACTION_SCORE_CONTENT)
                return AllowModifyReputationOverPost(scoreData, height, tx, lottery);

            if (scoreData->ScoreType == PocketTxType::ACTION_SCORE_COMMENT)
                return AllowModifyReputationOverComment(scoreData, height, tx, lottery);

            return false;
        }

        // TODO (brangr) v0.21.0: we need to limit the depth for all content types
        virtual bool AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, PocketTxType contentType)
        {
            if (contentType == PocketTxType::CONTENT_POST)
                return (scoreTime - contentTime) < GetScoresToPostModifyReputationDepth();

            return true;
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 108300 block
    *
    *******************************************************************************************************************/
    class ReputationConsensus_checkpoint_108300 : public ReputationConsensus
    {
    protected:
        int64_t GetThresholdReputationScore() override { return 500; }
        int CheckpointHeight() override { return 108300; }
    public:
        ReputationConsensus_checkpoint_108300(int height) : ReputationConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 151600 block
    *
    *******************************************************************************************************************/
    class ReputationConsensus_checkpoint_151600 : public ReputationConsensus_checkpoint_108300
    {
    protected:
        int CheckpointHeight() override { return 151600; }

        tuple<int, string> SelectAddressScoreContent(shared_ptr <ScoreDataDto> scoreData, bool lottery) override
        {
            return make_tuple(scoreData->ScoreAddressId, scoreData->ScoreAddressHash);
        }

    public:
        ReputationConsensus_checkpoint_151600(int height) : ReputationConsensus_checkpoint_108300(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 225000 block
    *
    *******************************************************************************************************************/
    class ReputationConsensus_checkpoint_225000 : public ReputationConsensus_checkpoint_151600
    {
    protected:
        int64_t GetScoresOneToOne() override { return 2; }
        int64_t GetScoresOneToOneDepth() override { return 1 * 24 * 3600; }
        int CheckpointHeight() override { return 225000; }
    public:
        ReputationConsensus_checkpoint_225000(int height) : ReputationConsensus_checkpoint_151600(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 292800 block
    *
    *******************************************************************************************************************/
    class ReputationConsensus_checkpoint_292800 : public ReputationConsensus_checkpoint_225000
    {
    protected:
        int64_t GetThresholdReputationScore() override { return 1000; }
        int64_t GetThresholdReputation() override { return 1000; }
        int64_t GetScoresOneToOneDepth() override { return 7 * 24 * 3600; }
        int CheckpointHeight() override { return 292800; }
    public:
        ReputationConsensus_checkpoint_292800(int height) : ReputationConsensus_checkpoint_225000(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 322700 block
    *
    *******************************************************************************************************************/
    class ReputationConsensus_checkpoint_322700 : public ReputationConsensus_checkpoint_292800
    {
    protected:
        int CheckpointHeight() override { return 322700; }
        int64_t GetScoresOneToOneDepth() override { return 2 * 24 * 3600; }
        int64_t GetScoresToPostModifyReputationDepth() override { return 30 * 24 * 3600; }
    public:
        ReputationConsensus_checkpoint_322700(int height) : ReputationConsensus_checkpoint_292800(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class ReputationConsensus_checkpoint_1124000 : public ReputationConsensus_checkpoint_322700
    {
    protected:
        int64_t GetThresholdLikersCount() override { return 100; }
        int CheckpointHeight() override { return 1124000; }
    public:
        ReputationConsensus_checkpoint_1124000(int height) : ReputationConsensus_checkpoint_322700(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class ReputationConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ReputationConsensus*(int height)>> m_rules =
            {
                {1124000, [](int height) { return new ReputationConsensus_checkpoint_1124000(height); }},
                {322700,  [](int height) { return new ReputationConsensus_checkpoint_322700(height); }},
                {292800,  [](int height) { return new ReputationConsensus_checkpoint_292800(height); }},
                {225000,  [](int height) { return new ReputationConsensus_checkpoint_225000(height); }},
                {151600,  [](int height) { return new ReputationConsensus_checkpoint_151600(height); }},
                {108300,  [](int height) { return new ReputationConsensus_checkpoint_108300(height); }},
                {0,       [](int height) { return new ReputationConsensus(height); }},
            };
    public:
        static shared_ptr <ReputationConsensus> Instance(int height)
        {
            return shared_ptr<ReputationConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_REPUTATION_HPP