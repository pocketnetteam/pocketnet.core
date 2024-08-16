// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/ChainPostProcessing.h"

namespace PocketServices
{
    void ChainPostProcessing::Index(const CBlock& block, int height)
    {
        vector<TransactionIndexingInfo> txs;
        PrepareTransactions(block, txs);

        int64_t nTime1 = GetTimeMicros();

        IndexChain(block.GetHash().GetHex(), height, txs);
        int64_t nTime2 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "    - IndexChain: %.2fms _ %d\n", 0.001 * (double)(nTime2 - nTime1), height);

        IndexRatings(height, txs);
        int64_t nTime3 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "    - IndexRatings: %.2fms _ %d\n", 0.001 * (double)(nTime3 - nTime2), height);

        IndexModeration(height, txs);
        int64_t nTime4 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "    - IndexModeration: %.2fms _ %d\n", 0.001 * (double)(nTime4 - nTime3), height);

        IndexBadges(height);
        int64_t nTime5 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "    - IndexBadges: %.2fms _ %d\n", 0.001 * (double)(nTime5 - nTime4), height);
    }

    bool ChainPostProcessing::Rollback(int height)
    {
        try
        {
            // Loop restore
            int curHeight = -1;
            do
            {
                curHeight = ChainRepoInst.CurrentHeight();

                LogPrint(BCLog::SYNC, "Rollback current block to prev at height %d\n", curHeight - 1);
                
                ChainRepoInst.Restore(curHeight);
            }
            while (curHeight > height);

            return true;
        }
        catch (std::exception& ex)
        {
            LogPrintf("Error: Rollback to height %d failed with message: %s\n", height, ex.what());
            return false;
        }
    }

    void ChainPostProcessing::PrepareTransactions(const CBlock& block, vector<TransactionIndexingInfo>& txs)
    {
        for (size_t i = 0; i < block.vtx.size(); i++)
        {
            auto& tx = block.vtx[i];
            auto txType = PocketHelpers::TransactionHelper::ParseType(tx);

            auto hash = tx->GetHash().GetHex();
            TransactionIndexingInfo txInfo;
            txInfo.Hash = hash;
            txInfo.BlockNumber = (int) i;
            txInfo.Time = tx->nTime;
            txInfo.Type = txType;

            if (!tx->IsCoinBase())
            {
                for (const auto& inp : tx->vin)
                    txInfo.Inputs.emplace_back(inp.prevout.hash.GetHex(), inp.prevout.n);
            }

            txs.emplace_back(txInfo);
        }
    }

    // Set block height for all transactions in block
    void ChainPostProcessing::IndexChain(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        ChainRepoInst.IndexBlock(blockHash, height, txs);
    }

    void ChainPostProcessing::IndexRatings(int height, vector<TransactionIndexingInfo>& txs)
    {
        map<RatingType, map<int, int>> ratingValues;
        vector<ScoreDataDtoRef> distinctScores;
        map<RatingType, map<int, vector<int>>> likersValues;

        // Actual consensus checker instance by current height
        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(height);

        // Need select content id for saving rating
        auto scoresData = ConsensusRepoInst.GetScoresData(height, reputationConsensus->GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth));

        // Get all accounts information in one query
        vector<string> accountsAddresses;
        for (auto& scoreData : scoresData)
            accountsAddresses.push_back(reputationConsensus->SelectAddressScoreContent(scoreData.second, false));
        auto accountsData = ConsensusRepoInst.GetAccountsData(accountsAddresses);

        // Loop all transactions for find scores and increase ratings for accounts and contents
        for (const auto& txInfo : txs)
        {
            // Only scores allowed in calculating ratings
            if (!txInfo.IsActionScore())
                continue;

            auto& scoreData = scoresData[txInfo.Hash];
            auto& accountData = accountsData[reputationConsensus->SelectAddressScoreContent(scoreData, false)];

            // Old posts denied change reputation
            auto allowModifyOldPosts = reputationConsensus->AllowModifyOldPosts(
                scoreData->ScoreTime,
                scoreData->ContentTime,
                scoreData->ContentType);

            if (!allowModifyOldPosts)
                continue;

            // Check whether the current rating has the right to change the recipient's reputation
            if (!reputationConsensus->AllowModifyReputation(scoreData, accountData, false))
                continue;

            // Calculate ratings values
            // Rating for users over posts = equals -0.2 and 20 - saved in int 20 = 2.0
            // Rating for users over comments = equals -0.1 or 0.1
            // Rating for posts equals between -2 and 2 - as is
            // Rating for comments equals between -1 and 2 - as is
            switch (scoreData->ScoreType)
            {
                case PocketTx::ACTION_SCORE_CONTENT:
                    ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                        reputationConsensus->GetScoreContentAuthorValue(scoreData->ScoreValue);

                    ratingValues[RatingType::RATING_CONTENT][scoreData->ContentId] +=
                        scoreData->ScoreValue - 3;

                    break;

                case PocketTx::ACTION_SCORE_COMMENT:
                    ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                        reputationConsensus->GetScoreCommentAuthorValue(scoreData->ScoreValue);

                    ratingValues[RatingType::RATING_COMMENT][scoreData->ContentId] +=
                        scoreData->ScoreValue;

                    break;

                // Not supported score type
                default:
                    break;
            }
            
            // Extend list of ratings with likers values
            reputationConsensus->DistinctScores(scoreData, distinctScores);
        }

        // Filter all distinct records
        for (const auto& _scoreData : distinctScores)
        {
            reputationConsensus->ValidateAccountLiker(_scoreData, likersValues, ratingValues);
        }
            
        // Prepare all ratings model records for increase ratings
        shared_ptr<vector<Rating>> ratings = make_shared<vector<Rating>>();
        for (const auto& tp : ratingValues)
        {
            for (const auto& itm : tp.second)
            {
                // Skip not changed reputations - e.g. +2 and -2
                if (itm.second == 0)
                    continue;

                Rating rtg;
                rtg.SetType(tp.first);
                rtg.SetHeight(height);
                rtg.SetId(itm.first);
                rtg.SetValue(itm.second);

                ratings->push_back(rtg);
            }
        }

        // Prepare likers models
        for (const auto& tp : likersValues)
        {
            for (const auto& cnt : tp.second)
            {
                for (const auto& lkr : cnt.second)
                {
                    // Skip not changed likers count
                    if (lkr == 0 && (
                        tp.first == RatingType::ACCOUNT_LIKERS_POST_LAST ||
                        tp.first == RatingType::ACCOUNT_LIKERS_COMMENT_ROOT_LAST ||
                        tp.first == RatingType::ACCOUNT_LIKERS_COMMENT_ANSWER_LAST ||
                        tp.first == RatingType::ACCOUNT_DISLIKERS_COMMENT_ANSWER_LAST
                    ))
                        continue;
                        
                    Rating rtg;
                    rtg.SetType(tp.first);
                    rtg.SetHeight(height);
                    rtg.SetId(cnt.first);
                    rtg.SetValue(lkr);

                    ratings->push_back(rtg);
                }
            }
        }

        if (ratings->empty())
            return;

        // Save all ratings in one transaction
        RatingsRepoInst.InsertRatings(ratings);
    }

    // Indexing of the moderation subsystem includes the creation of "Jury" entries in case of collecting a sufficient number of flags.
    // The verdicts of the moderators within the jury are also taken into account.
    void ChainPostProcessing::IndexModeration(int height, vector<TransactionIndexingInfo>& txs)
    {
        for (const auto& txInfo : txs)
        {
            IndexModerationFlag(txInfo, height);
            IndexModerationVote(txInfo, height);
        }
    }

    ModerationCondition ChainPostProcessing::GetConditions(int height, int accountLikers)
    {
        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(height);
        ModerationCondition cond;

        if (accountLikers < reputationConsensus->GetConsensusLimit(moderation_jury_likers_cat1))
        {
            cond.flag_count = reputationConsensus->GetConsensusLimit(moderation_jury_flag_cat1_count);
            cond.moders_count = reputationConsensus->GetConsensusLimit(moderation_jury_moders_cat1_count);
            cond.vote_count = reputationConsensus->GetConsensusLimit(moderation_jury_vote_cat1_count);
        }
        else if (accountLikers < reputationConsensus->GetConsensusLimit(moderation_jury_likers_cat2))
        {
            cond.flag_count = reputationConsensus->GetConsensusLimit(moderation_jury_flag_cat2_count);
            cond.moders_count = reputationConsensus->GetConsensusLimit(moderation_jury_moders_cat2_count);
            cond.vote_count = reputationConsensus->GetConsensusLimit(moderation_jury_vote_cat2_count);
        }
        else if (accountLikers < reputationConsensus->GetConsensusLimit(moderation_jury_likers_cat3))
        {
            cond.flag_count = reputationConsensus->GetConsensusLimit(moderation_jury_flag_cat3_count);
            cond.moders_count = reputationConsensus->GetConsensusLimit(moderation_jury_moders_cat3_count);
            cond.vote_count = reputationConsensus->GetConsensusLimit(moderation_jury_vote_cat3_count);
        }
        else
        {
            cond.flag_count = reputationConsensus->GetConsensusLimit(moderation_jury_flag_cat4_count);
            cond.moders_count = reputationConsensus->GetConsensusLimit(moderation_jury_moders_cat4_count);
            cond.vote_count = reputationConsensus->GetConsensusLimit(moderation_jury_vote_cat4_count);
        }

        return cond;
    }

    void ChainPostProcessing::IndexModerationFlag(const TransactionIndexingInfo& txInfo, int height)
    {
        if (!txInfo.IsModerationFlag())
            return;

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(height);
        int flag_depth = reputationConsensus->GetConsensusLimit(moderation_jury_flag_depth);
        int likers = ConsensusRepoInst.LikersByFlag(txInfo.Hash);
        auto cond = GetConditions(height, likers);

        ChainRepoInst.IndexModerationJury(
            txInfo.Hash,
            reputationConsensus->GetHeight() - flag_depth,
            cond.flag_count,
            cond.moders_count
        );
    }

    void ChainPostProcessing::IndexModerationVote(const TransactionIndexingInfo& txInfo, int height)
    {
        if (!txInfo.IsModerationVote())
            return;

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(height);
        int likers = ConsensusRepoInst.LikersByVote(txInfo.Hash);
        auto cond = GetConditions(height, likers);
            
        ChainRepoInst.IndexModerationBan(
            txInfo.Hash,
            cond.vote_count,
            height + reputationConsensus->GetConsensusLimit(moderation_jury_ban_1_time),
            height + reputationConsensus->GetConsensusLimit(moderation_jury_ban_2_time),
            height + reputationConsensus->GetConsensusLimit(moderation_jury_ban_3_time)
        );
    }

    void ChainPostProcessing::IndexBadges(int height)
    {
        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(height);
        if (reputationConsensus->UseBadges() && height % BadgePeriod() == 0)
        {
            const BadgeSharkConditions sharkConditions = {
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_all),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_content),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_comment),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_comment_answer),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_reg_depth)
            };
            ChainRepoInst.IndexBadges(height, sharkConditions);

            const BadgeModeratorConditions moderatorConditions = {
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_all),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_content),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_comment),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_likers_comment_answer),
                (int)reputationConsensus->GetConsensusLimit(threshold_shark_reg_depth)
            };
            ChainRepoInst.IndexBadges(height, moderatorConditions);

            // TODO (moderation): get BadgeWhaleConditions
        }
    }

} // namespace PocketServices