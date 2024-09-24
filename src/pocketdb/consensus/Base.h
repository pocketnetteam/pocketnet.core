// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BASE_H
#define POCKETCONSENSUS_BASE_H

#include "univalue/include/univalue.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/models/base/Base.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    enum SocialConsensusResult
    {
        ConsensusResult_Success = 0,
        ConsensusResult_NotRegistered = 1,
        ConsensusResult_ContentLimit = 2,
        ConsensusResult_ScoreLimit = 3,
        ConsensusResult_DoubleScore = 4,
        ConsensusResult_SelfScore = 5,
        ConsensusResult_ChangeInfoLimit = 6,
        ConsensusResult_InvalideSubscribe = 7,
        ConsensusResult_DoubleSubscribe = 8,
        ConsensusResult_SelfSubscribe = 9,
        ConsensusResult_Unknown = 10,
        ConsensusResult_Failed = 11,
        ConsensusResult_NotFound = 12,
        ConsensusResult_DoubleComplain = 13,
        ConsensusResult_SelfComplain = 14,
        ConsensusResult_ComplainLimit = 15,
        ConsensusResult_ComplainLowReputation = 16,
        ConsensusResult_ContentSizeLimit = 17,
        ConsensusResult_NicknameDouble = 18,
        ConsensusResult_NicknameLong = 19,
        ConsensusResult_ReferrerSelf = 20,
        ConsensusResult_FailedOpReturn = 21,
        ConsensusResult_InvalidBlocking = 22,
        ConsensusResult_DoubleBlocking = 23,
        ConsensusResult_SelfBlocking = 24,
        ConsensusResult_DoubleContentEdit = 25,
        ConsensusResult_ContentEditLimit = 26,
        ConsensusResult_ContentEditUnauthorized = 27,
        ConsensusResult_ManyTransactions = 28,
        ConsensusResult_CommentLimit = 29,
        ConsensusResult_CommentEditLimit = 30,
        ConsensusResult_CommentScoreLimit = 31,
        ConsensusResult_Blocking = 32,
        ConsensusResult_Size = 33,
        ConsensusResult_InvalidParentComment = 34,
        ConsensusResult_InvalidAnswerComment = 35,
        ConsensusResult_DoubleCommentEdit = 37,
        ConsensusResult_SelfCommentScore = 38,
        ConsensusResult_DoubleCommentDelete = 39,
        ConsensusResult_DoubleCommentScore = 40,
        ConsensusResult_CommentDeletedEdit = 42,
        ConsensusResult_NotAllowed = 44,
        ConsensusResult_ChangeTxType = 45,
        ConsensusResult_ContentDeleteUnauthorized = 46,
        ConsensusResult_ContentDeleteDouble = 47,
        ConsensusResult_AccountSettingsDouble = 48,
        ConsensusResult_AccountSettingsLimit = 49,
        ConsensusResult_ChangeInfoDoubleInBlock = 50,
        ConsensusResult_CommentDeletedContent = 51,
        ConsensusResult_RepostDeletedContent = 52,
        ConsensusResult_AlreadyExists = 53,
        ConsensusResult_PocketDataNotFound = 54,
        ConsensusResult_TxORNotFound = 55,
        ConsensusResult_ComplainDeletedContent = 56,
        ConsensusResult_ScoreDeletedContent = 57,
        ConsensusResult_RelayContentNotFound = 58,
        ConsensusResult_BadPayload = 59,
        ConsensusResult_ScoreLowReputation = 60,
        ConsensusResult_ChangeInfoDoubleInMempool = 61,
        ConsensusResult_Duplicate = 62,
        ConsensusResult_NotImplemeted = 63,
        ConsensusResult_SelfFlag = 64,
        ConsensusResult_ExceededLimit = 65,
        ConsensusResult_LowReputation = 66,
        ConsensusResult_AccountDeleted = 67,
        ConsensusResult_AccountBanned = 68,
    };

    static inline string SocialConsensusResultString(SocialConsensusResult code)
    {
        switch (code)
        {
            case (ConsensusResult_Success): return "Success";
            case (ConsensusResult_NotRegistered): return "NotRegistered";
            case (ConsensusResult_ContentLimit): return "ContentLimit";
            case (ConsensusResult_ScoreLimit): return "ScoreLimit";
            case (ConsensusResult_DoubleScore): return "DoubleScore";
            case (ConsensusResult_SelfScore): return "SelfScore";
            case (ConsensusResult_ChangeInfoLimit): return "ChangeInfoLimit";
            case (ConsensusResult_InvalideSubscribe): return "InvalideSubscribe";
            case (ConsensusResult_DoubleSubscribe): return "DoubleSubscribe";
            case (ConsensusResult_SelfSubscribe): return "SelfSubscribe";
            case (ConsensusResult_Unknown): return "Unknown";
            case (ConsensusResult_Failed): return "Failed";
            case (ConsensusResult_NotFound): return "NotFound";
            case (ConsensusResult_DoubleComplain): return "DoubleComplain";
            case (ConsensusResult_SelfComplain): return "SelfComplain";
            case (ConsensusResult_ComplainLimit): return "ComplainLimit";
            case (ConsensusResult_ComplainLowReputation): return "ComplainLowReputation";
            case (ConsensusResult_ScoreLowReputation): return "ScoreLowReputation";
            case (ConsensusResult_ContentSizeLimit): return "ContentSizeLimit";
            case (ConsensusResult_NicknameDouble): return "NicknameDouble";
            case (ConsensusResult_NicknameLong): return "NicknameLong";
            case (ConsensusResult_ReferrerSelf): return "ReferrerSelf";
            case (ConsensusResult_FailedOpReturn): return "FailedOpReturn";
            case (ConsensusResult_InvalidBlocking): return "InvalidBlocking";
            case (ConsensusResult_DoubleBlocking): return "DoubleBlocking";
            case (ConsensusResult_SelfBlocking): return "SelfBlocking";
            case (ConsensusResult_DoubleContentEdit): return "DoubleContentEdit";
            case (ConsensusResult_ContentEditLimit): return "ContentEditLimit";
            case (ConsensusResult_ContentEditUnauthorized): return "ContentEditUnauthorized";
            case (ConsensusResult_ManyTransactions): return "ManyTransactions";
            case (ConsensusResult_CommentLimit): return "CommentLimit";
            case (ConsensusResult_CommentEditLimit): return "CommentEditLimit";
            case (ConsensusResult_CommentScoreLimit): return "CommentScoreLimit";
            case (ConsensusResult_Blocking): return "Blocking";
            case (ConsensusResult_Size): return "Size";
            case (ConsensusResult_InvalidParentComment): return "InvalidParentComment";
            case (ConsensusResult_InvalidAnswerComment): return "InvalidAnswerComment";
            case (ConsensusResult_DoubleCommentEdit): return "DoubleCommentEdit";
            case (ConsensusResult_SelfCommentScore): return "SelfCommentScore";
            case (ConsensusResult_DoubleCommentDelete): return "DoubleCommentDelete";
            case (ConsensusResult_DoubleCommentScore): return "DoubleCommentScore";
            case (ConsensusResult_CommentDeletedEdit): return "CommentDeletedEdit";
            case (ConsensusResult_NotAllowed): return "NotAllowed";
            case (ConsensusResult_ChangeTxType): return "ChangeTxType";
            case (ConsensusResult_ContentDeleteUnauthorized): return "ContentDeleteUnauthorized";
            case (ConsensusResult_ContentDeleteDouble): return "ContentDeleteDouble";
            case (ConsensusResult_AccountSettingsDouble): return "AccountSettingsDouble";
            case (ConsensusResult_AccountSettingsLimit): return "AccountSettingsLimit";
            case (ConsensusResult_ChangeInfoDoubleInBlock): return "ChangeInfoDoubleInBlock";
            case (ConsensusResult_CommentDeletedContent): return "CommentDeletedContent";
            case (ConsensusResult_RepostDeletedContent): return "RepostDeletedContent";
            case (ConsensusResult_AlreadyExists): return "AlreadyExists";
            case (ConsensusResult_PocketDataNotFound): return "PocketDataNotFound";
            case (ConsensusResult_TxORNotFound): return "TxORNotFound";
            case (ConsensusResult_ComplainDeletedContent): return "ComplainDeletedContent";
            case (ConsensusResult_ScoreDeletedContent): return "ScoreDeletedContent";
            case (ConsensusResult_RelayContentNotFound): return "RelayContentNotFound";
            case (ConsensusResult_BadPayload): return "BadPayload";
            case (ConsensusResult_ChangeInfoDoubleInMempool): return "ChangeInfoDoubleInMempool";
            case (ConsensusResult_Duplicate): return "Duplicate";
            case (ConsensusResult_NotImplemeted): return "NotImplemeted";
            case (ConsensusResult_SelfFlag): return "SelfFlag";
            case (ConsensusResult_ExceededLimit): return "ExceededLimit";
            case (ConsensusResult_LowReputation): return "LowReputation";
            case (ConsensusResult_AccountDeleted): return "AccountDeleted";
            case (ConsensusResult_AccountBanned): return "AccountBanned";

            default: return "Unknown";
        }
    }

    enum AccountMode
    {
        AccountMode_Trial = 0,
        AccountMode_Full = 1,
        AccountMode_Pro = 2
    };

    enum ConsensusLimit
    {
        ConsensusLimit_depth,

        ConsensusLimit_threshold_reputation,
        ConsensusLimit_threshold_reputation_score,
        ConsensusLimit_threshold_balance,
        ConsensusLimit_threshold_balance_pro,
        ConsensusLimit_threshold_likers_count,
        ConsensusLimit_threshold_low_likers_count,
        ConsensusLimit_threshold_low_likers_depth,

        // Thresholds for obtaining badges - SHARK
        threshold_shark_reg_depth,
        threshold_shark_likers_all,
        threshold_shark_likers_content,
        threshold_shark_likers_comment,
        threshold_shark_likers_comment_answer,

        // Thresholds for obtaining badges - WHALE
        threshold_whale_reg_depth,
        threshold_whale_likers_all,
        threshold_whale_likers_content,
        threshold_whale_likers_comment,
        threshold_whale_likers_comment_answer,

        // Thresholds for obtaining badges - AUTHOR
        threshold_author_reg_depth,
        threshold_author_likers_all,
        threshold_author_likers_content,
        threshold_author_likers_comment,
        threshold_author_likers_comment_answer,

        ConsensusLimit_trial_post,
        ConsensusLimit_trial_video,
        ConsensusLimit_trial_article,
        ConsensusLimit_trial_stream,
        ConsensusLimit_trial_audio,
        ConsensusLimit_trial_collection,
        ConsensusLimit_trial_score,
        ConsensusLimit_trial_complain,
        ConsensusLimit_trial_comment,
        ConsensusLimit_trial_comment_score,

        ConsensusLimit_full_post,
        ConsensusLimit_full_video,
        ConsensusLimit_full_article,
        ConsensusLimit_full_stream,
        ConsensusLimit_full_audio,
        ConsensusLimit_full_collection,
        ConsensusLimit_full_score,
        ConsensusLimit_full_complain,
        ConsensusLimit_full_comment,
        ConsensusLimit_full_comment_score,

        ConsensusLimit_pro_video,
        ConsensusLimit_app,

        ConsensusLimit_post_edit_count,
        ConsensusLimit_video_edit_count,
        ConsensusLimit_article_edit_count,
        ConsensusLimit_stream_edit_count,
        ConsensusLimit_audio_edit_count,
        ConsensusLimit_collection_edit_count,
        ConsensusLimit_comment_edit_count,

        ConsensusLimit_edit_post_depth,
        ConsensusLimit_edit_video_depth,
        ConsensusLimit_edit_article_depth,
        ConsensusLimit_edit_stream_depth,
        ConsensusLimit_edit_audio_depth,
        ConsensusLimit_edit_collection_depth,
        ConsensusLimit_edit_comment_depth,

        account_settings_daily_count,

        ConsensusLimit_multiple_lock_addresses_count,
        ConsensusLimit_collection_ids_count,

        ConsensusLimit_scores_one_to_one,
        ConsensusLimit_scores_one_to_one_over_comment,
        ConsensusLimit_scores_one_to_one_depth,

        ConsensusLimit_scores_depth_modify_reputation,
        ConsensusLimit_lottery_referral_depth,

        ConsensusLimit_bad_reputation,

        moderation_flag_count,
        moderation_flag_max_value,
        moderation_jury_flag_depth,

        moderation_jury_likers_cat1,
        moderation_jury_likers_cat2,
        moderation_jury_likers_cat3,

        moderation_jury_flag_cat1_count,
        moderation_jury_flag_cat2_count,
        moderation_jury_flag_cat3_count,
        moderation_jury_flag_cat4_count,
        
        moderation_jury_moders_cat1_count,
        moderation_jury_moders_cat2_count,
        moderation_jury_moders_cat3_count,
        moderation_jury_moders_cat4_count,
        
        moderation_jury_vote_cat1_count,
        moderation_jury_vote_cat2_count,
        moderation_jury_vote_cat3_count,
        moderation_jury_vote_cat4_count,

        moderation_jury_ban_1_time,
        moderation_jury_ban_2_time,
        moderation_jury_ban_3_time,
    };

    /*********************************************************************************************/
    // Consensus limits

    // Reputation - double value in integer
    // i.e. 213 = 21.3
    // i.e. 45  = 4.5
    typedef map<ConsensusLimit, map<NetworkId, map<int, int64_t>>> ConsensusLimitsMap;

    static inline ConsensusLimitsMap m_consensus_limits = {
        { ConsensusLimit_bad_reputation, {
            { NetworkMain,    { {0, -500} } },
            { NetworkTest,    { {0, -50} } },
            { NetworkRegTest, { {0, -50} } }
        } },
        { ConsensusLimit_threshold_reputation, {
            { NetworkMain,    { {0, 500}, {292800, 1000} } },
            { NetworkTest,    { {0, 100}, {761000, 10} } },
            { NetworkRegTest, { {0, 0} } }
        } },
        { ConsensusLimit_threshold_reputation_score, {
            { NetworkMain,    { {0, -10000}, {108300, 500}, {292800, 1000} } },
            { NetworkTest,    { {0, 0}, {100000, 100} } },
            { NetworkRegTest, { {0, 0} } }
        } },
        { ConsensusLimit_threshold_balance, {
            { NetworkMain,    { {0, 50 * COIN} } },
            { NetworkTest,    { {0, 5 * COIN} } },
            { NetworkRegTest, { {0, 5 * COIN} } }
        } },
        { ConsensusLimit_threshold_balance_pro, {
            { NetworkMain,    { {0, INT64_MAX}, {65000, 250 * COIN} } },
            { NetworkTest,    { {0, INT64_MAX}, {65000,  25 * COIN} } },
            { NetworkRegTest, { {0, 25 * COIN} } }
        } },
        { ConsensusLimit_threshold_likers_count, {
            { NetworkMain,    { {0, 0}, {1124000, 100} }},
            { NetworkTest,    { {0, 0}, {100000, 10}, {1470000, 1} }},
            { NetworkRegTest, { {0, 1} } }
        }},
        { ConsensusLimit_threshold_low_likers_count, {
            { NetworkMain,    { {0, 30} }},
            { NetworkTest,    { {0, 30}, {761000, 0} }},
            { NetworkRegTest, { {0, 0} } }
        }},
        { ConsensusLimit_threshold_low_likers_depth, {
            { NetworkMain,    { {1180000, 250'000} } },
            { NetworkTest,    { {0, 250'000} } },
            { NetworkRegTest, { {0, 50} } }
        } },
        { ConsensusLimit_depth, {
            { NetworkMain,    { {0, 86400}, {1180000, 1440} } },
            { NetworkTest,    { {0, 1440} } },
            { NetworkRegTest, { {0, 1440} } }
        } },

        // Thresholds for obtaining badges - SHARK
        { threshold_shark_reg_depth, {
            { NetworkMain,    { {0, 129600} } },
            { NetworkTest,    { {0, 1} } },
            { NetworkRegTest, { {0, 1} } }
        } },
        { threshold_shark_likers_all, {
            { NetworkMain,    { {0, 100} } },
            { NetworkTest,    { {0, 1} } },
            { NetworkRegTest, { {0, 0}, {1100, 1}, {1150, 2} } }
        } },
        { threshold_shark_likers_content, {
            { NetworkMain,    { {0, 0} } },
            { NetworkTest,    { {0, 0} } },
            { NetworkRegTest, { {0, 0} } }
        } },
        { threshold_shark_likers_comment, {
            { NetworkMain,    { {0, 15}, {1873500, 25} } },
            { NetworkTest,    { {0, 1} } },
            { NetworkRegTest, { {0, 0}, {1100, 1} } }
        } },
        { threshold_shark_likers_comment_answer, {
            { NetworkMain,    { {0, 0} } },
            { NetworkTest,    { {0, 0} } },
            { NetworkRegTest, { {0, 0} } }
        } },
        
        // Thresholds for obtaining badges - WHALE
        { threshold_whale_reg_depth, {
            { NetworkMain,    { {0, 207360} } },
            { NetworkTest,    { {0, 1} } },
            { NetworkRegTest, { {0, 1} } }
        } },
        { threshold_whale_likers_all, {
            { NetworkMain,    { {0, 1000} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },
        { threshold_whale_likers_content, {
            { NetworkMain,    { {0, 100} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },
        { threshold_whale_likers_comment, {
            { NetworkMain,    { {0, 100} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },
        { threshold_whale_likers_comment_answer, {
            { NetworkMain,    { {0, 100} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },

        // Other
        { ConsensusLimit_edit_post_depth, {
            { NetworkMain,    { {0, 86400}, {1180000, 1440}, {2930000, 43200 } } },
            { NetworkTest,    { {0, 1440} } },
            { NetworkRegTest, { {0, 1440} } }
        } },
        { ConsensusLimit_edit_video_depth, {
            { NetworkMain,    { {0, 1440}, {2930000, 43200 } } },
            { NetworkTest,    { {0, 1440} } },
            { NetworkRegTest, { {0, 1440} } }
        } },
        { ConsensusLimit_edit_article_depth, {
            { NetworkMain,    { {0, 4320}, {2930000, 43200 } } },
            { NetworkTest,    { {0, 4320} } },
            { NetworkRegTest, { {0, 4320} } }
        } },
        { ConsensusLimit_edit_stream_depth, {
            { NetworkMain, { {0, 1440}, {2930000, 43200 } } },
            { NetworkTest, { {0, 1440} } },
            { NetworkRegTest, { {0, 1440} } }
        } },
        { ConsensusLimit_edit_audio_depth, {
            { NetworkMain, { {0, 1440}, {2930000, 43200 } } },
            { NetworkTest, { {0, 1440} } },
            { NetworkRegTest, { {0, 1440} } }
        } },
        { ConsensusLimit_edit_collection_depth, {
                { NetworkMain, { {0, 1440}, {2930000, 43200 } } },
                { NetworkTest, { {0, 1440} } },
                { NetworkRegTest, { {0, 100} } }
        } },
        { ConsensusLimit_edit_comment_depth, {
            { NetworkMain,    { {0, 86400}, {1180000, 1440}, {2930000, 43200 } } },
            { NetworkTest,    { {0, 1440} } },
            { NetworkRegTest, { {0, 1440} } }
        } },
        { ConsensusLimit_scores_one_to_one, {
            { NetworkMain,    { {0, 99999}, {225000,  2} } },
            { NetworkTest,    { {0, 2} } },
            { NetworkRegTest, { {0, 2} } }
        } },
        { ConsensusLimit_scores_one_to_one_over_comment, {
            { NetworkMain,    { {0, 20} } },
            { NetworkTest,    { {0, 20} } },
            { NetworkRegTest, { {0, 20} } }
        } },
        { ConsensusLimit_scores_one_to_one_depth, {
            { NetworkMain,    { {0, 336 * 24 * 3600}, {225000, 1 * 24 * 3600}, {292800, 7 * 24 * 3600},  {322700, 2 * 24 * 3600} } },
            { NetworkTest,    { {0, 2 * 24 * 3600} } },
            { NetworkRegTest, { {0, 2 * 24 * 3600} } }
        } },
        { ConsensusLimit_scores_depth_modify_reputation, {
            { NetworkMain,    { {0, 336 * 24 * 3600}, {322700, 30 * 24 * 3600} } },
            { NetworkTest,    { {0, 30 * 24 * 3600} } },
            { NetworkRegTest, { {0, 30 * 24 * 3600} } }
        } },
        // TODO (aok) (v0.21.0): change GetLotteryReferralDepth Time to Height
        { ConsensusLimit_lottery_referral_depth, {
            { NetworkMain,    { {0, 30 * 24 * 3600} } },
            { NetworkTest,    { {0, 30 * 24 * 3600} } },
            { NetworkRegTest, { {0, 30 * 24 * 3600} } }
        } },

        // Limits
        { ConsensusLimit_trial_post, {
            { NetworkMain,    { {0, 15}, {1324655, 5} } },
            { NetworkTest,    { {0, 15} } },
            { NetworkRegTest, { {0, 15} } }
        } },
        { ConsensusLimit_trial_video, {
            { NetworkMain,    { {0, 15}, {1324655, 5} } },
            { NetworkTest,    { {0, 15} } },
            { NetworkRegTest, { {0, 15} } }
        } },
        { ConsensusLimit_trial_article, {
            { NetworkMain,    { {0, 1} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },
        { ConsensusLimit_trial_stream, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 15} } },
            { NetworkRegTest, { {0, 15} } }
        } },
        { ConsensusLimit_trial_audio, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 15} } },
            { NetworkRegTest, { {0, 15} } }
        } },
        { ConsensusLimit_trial_collection, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },
        { ConsensusLimit_trial_score, {
            { NetworkMain,    { {0, 45}, {175600, 100}, {1324655, 15} } },
            { NetworkTest,    { {0, 100} } },
            { NetworkRegTest, { {0, 100} } }
        } },
        { ConsensusLimit_trial_complain, {
            { NetworkMain,    { {0, 6} } },
            { NetworkTest,    { {0, 6} } },
            { NetworkRegTest, { {0, 6} } }
        } },
        { ConsensusLimit_trial_comment, {
            { NetworkMain,    { {0, 150}, {1757000, 50} } },
            { NetworkTest,    { {0, 150} } },
            { NetworkRegTest, { {0, 150} } }
        } },
        { ConsensusLimit_trial_comment_score, {
            { NetworkMain,    { {0, 300}, {1757000, 100} } },
            { NetworkTest,    { {0, 300} } },
            { NetworkRegTest, { {0, 300} } }
        } },
        
        { ConsensusLimit_full_post, {
            { NetworkMain,    { {0, 30}, {1757000, 10}, {1791787, 30} } },
            { NetworkTest,    { {0, 30} } },
            { NetworkRegTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_video, {
            { NetworkMain,    { {0, 30}, {1757000, 10}, {1791787, 30} } },
            { NetworkTest,    { {0, 30} } },
            { NetworkRegTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_article, {
            { NetworkMain,    { {0, 3} } },
            { NetworkTest,    { {0, 30} } },
            { NetworkRegTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_stream, {
            { NetworkMain, { {0, 30} } },
            { NetworkTest, { {0, 30} } },
            { NetworkRegTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_audio, {
            { NetworkMain, { {0, 30} } },
            { NetworkTest, { {0, 30} } },
            { NetworkRegTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_collection, {
            { NetworkMain, { {0, 30} } },
            { NetworkTest, { {0, 30} } },
            { NetworkRegTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_score, {
            { NetworkMain,    { {0, 90}, {175600, 200}, {1757000, 60}, {1791787, 100}, {1873500, 200} } },
            { NetworkTest,    { {0, 200} } },
            { NetworkRegTest, { {0, 200} } }
        } },
        { ConsensusLimit_full_complain, {
            { NetworkMain,    { {0, 12} } },
            { NetworkTest,    { {0, 12} } },
            { NetworkRegTest, { {0, 12} } }
        } },
        { ConsensusLimit_full_comment, {
            { NetworkMain,    { {0, 300}, {1757000, 100}, {1791787, 200} } },
            { NetworkTest,    { {0, 300} } },
            { NetworkRegTest, { {0, 300} } }
        } },
        { ConsensusLimit_full_comment_score, {
            { NetworkMain,    { {0, 600}, {1757000, 200}, {1873500, 300} } },
            { NetworkTest,    { {0, 600} } },
            { NetworkRegTest, { {0, 600} } }
        } },
        
        { ConsensusLimit_pro_video, {
            { NetworkMain,    { {0, 0}, {1324655, 100} } },
            { NetworkTest,    { {0, 100} } },
            { NetworkRegTest, { {0, 10} } }
        } },
//        { ConsensusLimit_pro_stream, {
//            { NetworkMain, { {0, 100} } },
//            { NetworkTest, { {0, 100} } },
//            { NetworkRegTest, { {0, 100} } }
//        } },
//        { ConsensusLimit_pro_audio, {
//            { NetworkMain, { {0, 100} } },
//            { NetworkTest, { {0, 100} } },
//            { NetworkRegTest, { {0, 100} } }
//        } },

        { ConsensusLimit_app, {
            { NetworkMain,    { {0, 1} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },

        { ConsensusLimit_post_edit_count, {
            { NetworkMain,    { {0, 5} } },
            { NetworkTest,    { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },
        { ConsensusLimit_video_edit_count, {
            { NetworkMain,    { {0, 5} } },
            { NetworkTest,    { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },
        { ConsensusLimit_article_edit_count, {
            { NetworkMain,    { {0, 10} } },
            { NetworkTest,    { {0, 10} } },
            { NetworkRegTest, { {0, 10} } }
        } },
        { ConsensusLimit_stream_edit_count, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },
        { ConsensusLimit_audio_edit_count, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },
        { ConsensusLimit_collection_edit_count, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },
        { ConsensusLimit_comment_edit_count, {
            { NetworkMain,    { {0, 4} } },
            { NetworkTest,    { {0, 4} } },
            { NetworkRegTest, { {0, 4} } }
        } },
        
        { account_settings_daily_count, {
            { NetworkMain,    { {0, 5} } },
            { NetworkTest,    { {0, 5} } },
            { NetworkRegTest, { {0, 5} } }
        } },

        { ConsensusLimit_multiple_lock_addresses_count, {
            { NetworkMain,    { {0, 100} } },
            { NetworkTest,    { {0, 100} } },
            { NetworkRegTest, { {0, 100} } }
        } },

        { ConsensusLimit_collection_ids_count, {
            { NetworkMain,    { {0, 100} } },
            { NetworkTest,    { {0, 100} } },
            { NetworkRegTest, { {0, 10} } }
        } },

        // MODERATION

        { moderation_flag_count, {
            { NetworkMain,    { {0, 30} }},
            { NetworkTest,    { {0, 100} }},
            { NetworkRegTest, { {0, 100} } }
        }},
        { moderation_flag_max_value, {
            { NetworkMain,    { {0, 4}, {2162400, 5} }},
            { NetworkTest,    { {0, 4}, {1531000, 5} }},
            { NetworkRegTest, { {0, 5} } }
        }},

        // JURY
        { moderation_jury_flag_depth, {
            { NetworkMain,    { {0, 43200} }},
            { NetworkTest,    { {0, 4320} }},
            { NetworkRegTest, { {0, 10} } }
        }},

        // Likers limit for detect jury category
        { moderation_jury_likers_cat1, {
            { NetworkMain,    { {0, 3} }},
            { NetworkTest,    { {0, 1} }},
            { NetworkRegTest, { {0, 1} } }
        }},
        { moderation_jury_likers_cat2, {
            { NetworkMain,    { {0, 20} }},
            { NetworkTest,    { {0, 2} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_likers_cat3, {
            { NetworkMain,    { {0, 40} }},
            { NetworkTest,    { {0, 3} }},
            { NetworkRegTest, { {0, 3} } }
        }},
        

        // Flags
        { moderation_jury_flag_cat1_count, {
            { NetworkMain,    { {0, 5} }},
            { NetworkTest,    { {0, 5} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_flag_cat2_count, {
            { NetworkMain,    { {0, 10} }},
            { NetworkTest,    { {0, 5} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_flag_cat3_count, {
            { NetworkMain,    { {0, 15} }},
            { NetworkTest,    { {0, 5} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_flag_cat4_count, {
            { NetworkMain,    { {0, 20} }},
            { NetworkTest,    { {0, 5} }},
            { NetworkRegTest, { {0, 2} } }
        }},

        // Moderators
        { moderation_jury_moders_cat1_count, {
            { NetworkMain,    { {0, 80} }},
            { NetworkTest,    { {0, 6} }},
            { NetworkRegTest, { {0, 4} } }
        }},
        { moderation_jury_moders_cat2_count, {
            { NetworkMain,    { {0, 80} }},
            { NetworkTest,    { {0, 6} }},
            { NetworkRegTest, { {0, 4} } }
        }},
        { moderation_jury_moders_cat3_count, {
            { NetworkMain,    { {0, 80} }},
            { NetworkTest,    { {0, 6} }},
            { NetworkRegTest, { {0, 4} } }
        }},
        { moderation_jury_moders_cat4_count, {
            { NetworkMain,    { {0, 80} }},
            { NetworkTest,    { {0, 6} }},
            { NetworkRegTest, { {0, 4} } }
        }},

        { moderation_jury_vote_cat1_count, {
            { NetworkMain,    { {0, 1} }},
            { NetworkTest,    { {0, 3} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_vote_cat2_count, {
            { NetworkMain,    { {0, 2} }},
            { NetworkTest,    { {0, 3} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_vote_cat3_count, {
            { NetworkMain,    { {0, 4} }},
            { NetworkTest,    { {0, 3} }},
            { NetworkRegTest, { {0, 2} } }
        }},
        { moderation_jury_vote_cat4_count, {
            { NetworkMain,    { {0, 8} }},
            { NetworkTest,    { {0, 3} }},
            { NetworkRegTest, { {0, 2} } }
        }},

        { moderation_jury_ban_1_time, {
            { NetworkMain,    { {0, 43200} }},
            { NetworkTest,    { {0, 5000} }},
            { NetworkRegTest, { {0, 100} } }
        }},
        { moderation_jury_ban_2_time, {
            { NetworkMain,    { {0, 129600} }},
            { NetworkTest,    { {0, 10000} }},
            { NetworkRegTest, { {0, 200} } }
        }},
        { moderation_jury_ban_3_time, {
            { NetworkMain,    { {0, 51840000} }},
            { NetworkTest,    { {0, 15000} }},
            { NetworkRegTest, { {0, 1000} } }
        }},

        // { threshold_moderator_request, {
        //     { NetworkMain, { {0, 10080} }},
        //     { NetworkTest, { {0, 1440} }},
        //     { NetworkRegTest, { {0, 10} } }
        // }},
        // { threshold_moderator_register, {
        //     { NetworkMain, { {0, 129600} }},
        //     { NetworkTest, { {0, 10080} }},
        //     { NetworkRegTest, { {0, 10} } }
        // }},

    };





    /*********************************************************************************************/
    typedef tuple<bool, SocialConsensusResult> ConsensusValidateResult;

    /*********************************************************************************************/
    class ConsensusLimits
    {
    public:
        void Set(const string& type, int64_t mainValue, int64_t testValue, int64_t regValue)
        {
            _limits[type] = {
                {NetworkMain, mainValue},
                {NetworkTest, testValue},
                {NetworkRegTest, regValue}
            };
        }
        int64_t Get(const string& type)
        {
            return _limits.at(type).at(Params().NetworkID());
        }
    private:
        map<string, map<NetworkId, int64_t>> _limits;
    };

    /*********************************************************************************************/
    class BaseConsensus
    {
    public:
        ConsensusLimits Limits;

        BaseConsensus() = default;
        virtual ~BaseConsensus() = default;

        int64_t GetConsensusLimit(ConsensusLimit type) const
        {
            return GetConsensusLimit(type, Height);
        }

        static int64_t GetConsensusLimit(ConsensusLimit type, int height)
        {
            return (--m_consensus_limits[type][Params().NetworkID()].upper_bound(height))->second;
        }

        void Initialize(int height)
        {
            Height = height;
            ResultCode = ConsensusResult_Success;
        }

        int GetHeight() const
        {
            return Height;
        }

    protected:
        int Height = 0;
        ConsensusValidateResult Success{ true, ConsensusResult_Success };
        SocialConsensusResult ResultCode = ConsensusResult_Success;
        
        // Set result tuple if not already negative
        // All checks if already negative skeeped
        void Result(SocialConsensusResult result, const function<bool()>& func)
        {
            if (ResultCode != ConsensusResult_Success)
                return;

            if (func())
                ResultCode = result;
        }
    
    private:
        
    };

    /*********************************************************************************************/
    template<class T>
    struct ConsensusCheckpoint
    {
        int m_main_height;
        int m_test_height;
        int m_regtest_height;
        shared_ptr<T> m_factory;

        [[nodiscard]] int Height(NetworkId networkId) const
        {
            if (networkId == NetworkId::NetworkMain)
                return m_main_height;

            if (networkId == NetworkId::NetworkTest)
                return m_test_height;

            if (networkId == NetworkId::NetworkRegTest)
                return m_regtest_height;

            return m_main_height;
        }
    };

    /*********************************************************************************************/
    template<class T>
    class BaseConsensusFactory
    {
    private:
        vector<ConsensusCheckpoint<T>> m_rules;

    protected:
        void Checkpoint(const ConsensusCheckpoint<T>& inst)
        {
            m_rules.push_back(inst);
        }

    public:
        shared_ptr<T> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            auto func = --upper_bound(
                m_rules.begin(),
                m_rules.end(),
                m_height,
                [&](int target, const ConsensusCheckpoint<T>& itm)
                {
                    return target < itm.Height(Params().NetworkID());
                }
            );
            
            if (func == m_rules.end())
                return nullptr;
            
            func->m_factory->Initialize(height);
            return func->m_factory;
        }
    };
}

#endif // POCKETCONSENSUS_BASE_H
