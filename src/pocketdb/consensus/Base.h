// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BASE_H
#define POCKETCONSENSUS_BASE_H

#include "univalue/include/univalue.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    enum SocialConsensusResult
    {
        SocialConsensusResult_Success = 0,
        SocialConsensusResult_NotRegistered = 1,
        SocialConsensusResult_ContentLimit = 2,
        SocialConsensusResult_ScoreLimit = 3,
        SocialConsensusResult_DoubleScore = 4,
        SocialConsensusResult_SelfScore = 5,
        SocialConsensusResult_ChangeInfoLimit = 6,
        SocialConsensusResult_InvalideSubscribe = 7,
        SocialConsensusResult_DoubleSubscribe = 8,
        SocialConsensusResult_SelfSubscribe = 9,
        SocialConsensusResult_Unknown = 10,
        SocialConsensusResult_Failed = 11,
        SocialConsensusResult_NotFound = 12,
        SocialConsensusResult_DoubleComplain = 13,
        SocialConsensusResult_SelfComplain = 14,
        SocialConsensusResult_ComplainLimit = 15,
        SocialConsensusResult_ComplainLowReputation = 16,
        SocialConsensusResult_ContentSizeLimit = 17,
        SocialConsensusResult_NicknameDouble = 18,
        SocialConsensusResult_NicknameLong = 19,
        SocialConsensusResult_ReferrerSelf = 20,
        SocialConsensusResult_FailedOpReturn = 21,
        SocialConsensusResult_InvalidBlocking = 22,
        SocialConsensusResult_DoubleBlocking = 23,
        SocialConsensusResult_SelfBlocking = 24,
        SocialConsensusResult_DoubleContentEdit = 25,
        SocialConsensusResult_ContentEditLimit = 26,
        SocialConsensusResult_ContentEditUnauthorized = 27,
        SocialConsensusResult_ManyTransactions = 28,
        SocialConsensusResult_CommentLimit = 29,
        SocialConsensusResult_CommentEditLimit = 30,
        SocialConsensusResult_CommentScoreLimit = 31,
        SocialConsensusResult_Blocking = 32,
        SocialConsensusResult_Size = 33,
        SocialConsensusResult_InvalidParentComment = 34,
        SocialConsensusResult_InvalidAnswerComment = 35,
        SocialConsensusResult_DoubleCommentEdit = 37,
        SocialConsensusResult_SelfCommentScore = 38,
        SocialConsensusResult_DoubleCommentDelete = 39,
        SocialConsensusResult_DoubleCommentScore = 40,
        SocialConsensusResult_CommentDeletedEdit = 42,
        SocialConsensusResult_NotAllowed = 44,
        SocialConsensusResult_ChangeTxType = 45,
        SocialConsensusResult_ContentDeleteUnauthorized = 46,
        SocialConsensusResult_ContentDeleteDouble = 47,
        SocialConsensusResult_AccountSettingsDouble = 48,
        SocialConsensusResult_AccountSettingsLimit = 49,
        SocialConsensusResult_ChangeInfoDoubleInBlock = 50,
        SocialConsensusResult_CommentDeletedContent = 51,
        SocialConsensusResult_RepostDeletedContent = 52,
        SocialConsensusResult_AlreadyExists = 53,
        SocialConsensusResult_PocketDataNotFound = 54,
        SocialConsensusResult_TxORNotFound = 55,
        SocialConsensusResult_ComplainDeletedContent = 56,
        SocialConsensusResult_ScoreDeletedContent = 57,
        SocialConsensusResult_RelayContentNotFound = 58,
        SocialConsensusResult_BadPayload = 59,
        SocialConsensusResult_ScoreLowReputation = 60,
        SocialConsensusResult_ChangeInfoDoubleInMempool = 61,
        SocialConsensusResult_Duplicate = 62,
        SocialConsensusResult_NotImplemeted = 63,
        SocialConsensusResult_SelfFlag = 64,
        SocialConsensusResult_ExceededLimit = 65,
        SocialConsensusResult_LowReputation = 66,
        SocialConsensusResult_AccountDeleted = 67,
    };

    static inline string SocialConsensusResultString(SocialConsensusResult code)
    {
        switch (code)
        {
            case (SocialConsensusResult_Success): return "Success";
            case (SocialConsensusResult_NotRegistered): return "NotRegistered";
            case (SocialConsensusResult_ContentLimit): return "ContentLimit";
            case (SocialConsensusResult_ScoreLimit): return "ScoreLimit";
            case (SocialConsensusResult_DoubleScore): return "DoubleScore";
            case (SocialConsensusResult_SelfScore): return "SelfScore";
            case (SocialConsensusResult_ChangeInfoLimit): return "ChangeInfoLimit";
            case (SocialConsensusResult_InvalideSubscribe): return "InvalideSubscribe";
            case (SocialConsensusResult_DoubleSubscribe): return "DoubleSubscribe";
            case (SocialConsensusResult_SelfSubscribe): return "SelfSubscribe";
            case (SocialConsensusResult_Unknown): return "Unknown";
            case (SocialConsensusResult_Failed): return "Failed";
            case (SocialConsensusResult_NotFound): return "NotFound";
            case (SocialConsensusResult_DoubleComplain): return "DoubleComplain";
            case (SocialConsensusResult_SelfComplain): return "SelfComplain";
            case (SocialConsensusResult_ComplainLimit): return "ComplainLimit";
            case (SocialConsensusResult_ComplainLowReputation): return "ComplainLowReputation";
            case (SocialConsensusResult_ScoreLowReputation): return "ScoreLowReputation";
            case (SocialConsensusResult_ContentSizeLimit): return "ContentSizeLimit";
            case (SocialConsensusResult_NicknameDouble): return "NicknameDouble";
            case (SocialConsensusResult_NicknameLong): return "NicknameLong";
            case (SocialConsensusResult_ReferrerSelf): return "ReferrerSelf";
            case (SocialConsensusResult_FailedOpReturn): return "FailedOpReturn";
            case (SocialConsensusResult_InvalidBlocking): return "InvalidBlocking";
            case (SocialConsensusResult_DoubleBlocking): return "DoubleBlocking";
            case (SocialConsensusResult_SelfBlocking): return "SelfBlocking";
            case (SocialConsensusResult_DoubleContentEdit): return "DoubleContentEdit";
            case (SocialConsensusResult_ContentEditLimit): return "ContentEditLimit";
            case (SocialConsensusResult_ContentEditUnauthorized): return "ContentEditUnauthorized";
            case (SocialConsensusResult_ManyTransactions): return "ManyTransactions";
            case (SocialConsensusResult_CommentLimit): return "CommentLimit";
            case (SocialConsensusResult_CommentEditLimit): return "CommentEditLimit";
            case (SocialConsensusResult_CommentScoreLimit): return "CommentScoreLimit";
            case (SocialConsensusResult_Blocking): return "Blocking";
            case (SocialConsensusResult_Size): return "Size";
            case (SocialConsensusResult_InvalidParentComment): return "InvalidParentComment";
            case (SocialConsensusResult_InvalidAnswerComment): return "InvalidAnswerComment";
            case (SocialConsensusResult_DoubleCommentEdit): return "DoubleCommentEdit";
            case (SocialConsensusResult_SelfCommentScore): return "SelfCommentScore";
            case (SocialConsensusResult_DoubleCommentDelete): return "DoubleCommentDelete";
            case (SocialConsensusResult_DoubleCommentScore): return "DoubleCommentScore";
            case (SocialConsensusResult_CommentDeletedEdit): return "CommentDeletedEdit";
            case (SocialConsensusResult_NotAllowed): return "NotAllowed";
            case (SocialConsensusResult_ChangeTxType): return "ChangeTxType";
            case (SocialConsensusResult_ContentDeleteUnauthorized): return "ContentDeleteUnauthorized";
            case (SocialConsensusResult_ContentDeleteDouble): return "ContentDeleteDouble";
            case (SocialConsensusResult_AccountSettingsDouble): return "AccountSettingsDouble";
            case (SocialConsensusResult_AccountSettingsLimit): return "AccountSettingsLimit";
            case (SocialConsensusResult_ChangeInfoDoubleInBlock): return "ChangeInfoDoubleInBlock";
            case (SocialConsensusResult_CommentDeletedContent): return "CommentDeletedContent";
            case (SocialConsensusResult_RepostDeletedContent): return "RepostDeletedContent";
            case (SocialConsensusResult_AlreadyExists): return "AlreadyExists";
            case (SocialConsensusResult_PocketDataNotFound): return "PocketDataNotFound";
            case (SocialConsensusResult_TxORNotFound): return "TxORNotFound";
            case (SocialConsensusResult_ComplainDeletedContent): return "ComplainDeletedContent";
            case (SocialConsensusResult_ScoreDeletedContent): return "ScoreDeletedContent";
            case (SocialConsensusResult_RelayContentNotFound): return "RelayContentNotFound";
            case (SocialConsensusResult_BadPayload): return "BadPayload";
            case (SocialConsensusResult_ChangeInfoDoubleInMempool): return "ChangeInfoDoubleInMempool";
            case (SocialConsensusResult_Duplicate): return "Duplicate";
            case (SocialConsensusResult_NotImplemeted): return "NotImplemeted";
            case (SocialConsensusResult_SelfFlag): return "SelfFlag";
            case (SocialConsensusResult_ExceededLimit): return "ExceededLimit";
            case (SocialConsensusResult_LowReputation): return "LowReputation";

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

        ConsensusLimit_trial_post,
        ConsensusLimit_trial_video,
        ConsensusLimit_trial_article,
        ConsensusLimit_trial_score,
        ConsensusLimit_trial_complain,
        ConsensusLimit_trial_comment,
        ConsensusLimit_trial_comment_score,

        ConsensusLimit_full_post,
        ConsensusLimit_full_video,
        ConsensusLimit_full_article,
        ConsensusLimit_full_score,
        ConsensusLimit_full_complain,
        ConsensusLimit_full_comment,
        ConsensusLimit_full_comment_score,

        ConsensusLimit_pro_video,

        ConsensusLimit_post_edit_count,
        ConsensusLimit_video_edit_count,
        ConsensusLimit_article_edit_count,
        ConsensusLimit_comment_edit_count,

        ConsensusLimit_edit_post_depth,
        ConsensusLimit_edit_video_depth,
        ConsensusLimit_edit_article_depth,
        ConsensusLimit_edit_comment_depth,
        ConsensusLimit_edit_user_depth,

        ConsensusLimit_edit_user_daily_count,
        ConsensusLimit_account_settings_daily_count,

        ConsensusLimit_multiple_lock_addresses_count,

        ConsensusLimit_max_user_size,
        ConsensusLimit_max_post_size,
        ConsensusLimit_max_video_size,
        ConsensusLimit_max_article_size,
        ConsensusLimit_max_comment_size,
        ConsensusLimit_max_account_setting_size,

        ConsensusLimit_scores_one_to_one,
        ConsensusLimit_scores_one_to_one_over_comment,
        ConsensusLimit_scores_one_to_one_depth,

        ConsensusLimit_scores_depth_modify_reputation,
        ConsensusLimit_lottery_referral_depth,

        ConsensusLimit_bad_reputation,

        ConsensusLimit_moderation_flag_count,
    };

    /*********************************************************************************************/
    // Consensus limits

    // Reputation - double value in integer
    // i.e. 213 = 21.3
    // i.e. 45  = 4.5
    typedef map<ConsensusLimit, map<NetworkId, map<int, int64_t>>> ConsensusLimits;

    static inline ConsensusLimits m_consensus_limits = {
        { ConsensusLimit_bad_reputation, {
            { NetworkMain, { {0, -500} } },
            { NetworkTest, { {0, -50} } }
        } },
        { ConsensusLimit_threshold_reputation, {
            { NetworkMain, { {0, 500}, {292800, 1000} } },
            { NetworkTest, { {0, 100}, {761000, 10} } }
        } },
        { ConsensusLimit_threshold_reputation_score, {
            { NetworkMain, { {0, -10000}, {108300, 500}, {292800, 1000} } },
            { NetworkTest, { {0, 0}, {100000, 100} } }
        } },
        { ConsensusLimit_threshold_balance, {
            { NetworkMain, { {0, 50 * COIN} } },
            { NetworkTest, { {0, 5 * COIN} } }
        } },
        { ConsensusLimit_threshold_balance_pro, {
            { NetworkMain, { {0, INT64_MAX}, {65000, 250 * COIN} } },
            { NetworkTest, { {0, INT64_MAX}, {65000,  25 * COIN} } }
        } },
        { ConsensusLimit_threshold_likers_count, {
            { NetworkMain, { {0, 0}, {1124000, 100} }},
            { NetworkTest, { {0, 0}, {100000, 10} }},
            { NetworkTest, { {0, 0}, {930000, 1} }}
        }},
        { ConsensusLimit_threshold_low_likers_count, {
            { NetworkMain, { {0, 30} }},
            { NetworkTest, { {0, 30}, {761000, 0} }}
        }},
        { ConsensusLimit_threshold_low_likers_depth, {
            { NetworkMain, { {1180000, 250'000}, } },
            { NetworkTest, { {0, 250'000},
                    }
                }
        } },
        { ConsensusLimit_depth, {
            { NetworkMain, { {0, 86400}, {1180000, 1440} } },
            { NetworkTest, { {0, 1440} } }
        } },

        // Thresholds for obtaining badges - SHARK
        { threshold_shark_reg_depth, {
            { NetworkMain, { {0, 129600} } },
            { NetworkTest, { {0, 1} } }
        } },
        { threshold_shark_likers_all, {
            { NetworkMain, { {0, 100} } },
            { NetworkTest, { {0, 1} } }
        } },
        { threshold_shark_likers_content, {
            { NetworkMain, { {0, 0} } },
            { NetworkTest, { {0, 0} } }
        } },
        { threshold_shark_likers_comment, {
            { NetworkMain, { {0, 15}, {1873500, 25} } },
            { NetworkTest, { {0, 1} } }
        } },
        { threshold_shark_likers_comment_answer, {
            { NetworkMain, { {0, 0} } },
            { NetworkTest, { {0, 0} } }
        } },
        
        // Thresholds for obtaining badges - WHALE
        { threshold_whale_reg_depth, {
            { NetworkMain, { {0, 207360} } },
            { NetworkTest, { {0, 1} } }
        } },
        { threshold_whale_likers_all, {
            { NetworkMain, { {0, 1000} } },
            { NetworkTest, { {0, 10} } }
        } },
        { threshold_whale_likers_content, {
            { NetworkMain, { {0, 100} } },
            { NetworkTest, { {0, 10} } }
        } },
        { threshold_whale_likers_comment, {
            { NetworkMain, { {0, 100} } },
            { NetworkTest, { {0, 10} } }
        } },
        { threshold_whale_likers_comment_answer, {
            { NetworkMain, { {0, 100} } },
            { NetworkTest, { {0, 10} } }
        } },

        // Other
        { ConsensusLimit_edit_user_depth, {
            { NetworkMain, { {0, 3600}, {1180000, 60} } },
            { NetworkTest, { {0, 30} } }
        } },
        { ConsensusLimit_edit_user_daily_count, {
            { NetworkMain, { {0, 10} } },
            { NetworkTest, { {0, 10} } }
        } },
        { ConsensusLimit_edit_post_depth, {
            { NetworkMain, { {0, 86400}, {1180000, 1440} } },
            { NetworkTest, { {0, 1440} } }
        } },
        { ConsensusLimit_edit_video_depth, {
            { NetworkMain, { {0, 1440} } },
            { NetworkTest, { {0, 1440} } }
        } },
        { ConsensusLimit_edit_article_depth, {
            { NetworkMain, { {0, 4320} } },
            { NetworkTest, { {0, 4320} } }
        } },
        { ConsensusLimit_edit_comment_depth, {
            { NetworkMain, { {0, 86400}, {1180000, 1440} } },
            { NetworkTest, { {0, 1440} } }
        } },
        { ConsensusLimit_scores_one_to_one, {
            { NetworkMain, { {0, 99999}, {225000,  2} } },
            { NetworkTest, { {0, 2} } }
        } },
        { ConsensusLimit_scores_one_to_one_over_comment, {
            { NetworkMain, { {0, 20} } },
            { NetworkTest, { {0, 20} } }
        } },
        { ConsensusLimit_scores_one_to_one_depth, {
            { NetworkMain, { {0, 336 * 24 * 3600}, {225000, 1 * 24 * 3600}, {292800, 7 * 24 * 3600},  {322700, 2 * 24 * 3600} } },
            { NetworkTest, { {0, 2 * 24 * 3600} } }
        } },
        { ConsensusLimit_scores_depth_modify_reputation, {
            { NetworkMain, { {0, 336 * 24 * 3600}, {322700, 30 * 24 * 3600} } },
            { NetworkTest, { {0, 30 * 24 * 3600} } }
        } },
        // TODO (brangr) (v0.21.0): change GetLotteryReferralDepth Time to Height
        { ConsensusLimit_lottery_referral_depth, {
            { NetworkMain, { {0, 30 * 24 * 3600} } },
            { NetworkTest, { {0, 30 * 24 * 3600} } }
        } },

        // Limits
        { ConsensusLimit_trial_post, {
            { NetworkMain, { {0, 15}, {1324655, 5} } },
            { NetworkTest, { {0, 15} } }
        } },
        { ConsensusLimit_trial_video, {
            { NetworkMain, { {0, 15}, {1324655, 5} } },
            { NetworkTest, { {0, 15} } }
        } },
        { ConsensusLimit_trial_article, {
            { NetworkMain, { {0, 1} } },
            { NetworkTest, { {0, 10} } }
        } },
        { ConsensusLimit_trial_score, {
            { NetworkMain, { {0, 45}, {175600, 100}, {1324655, 15} } },
            { NetworkTest, { {0, 100} } }
        } },
        { ConsensusLimit_trial_complain, {
            { NetworkMain, { {0, 6} } },
            { NetworkTest, { {0, 6} } }
        } },
        { ConsensusLimit_trial_comment, {
            { NetworkMain, { {0, 150}, {1757000, 50} } },
            { NetworkTest, { {0, 150} } }
        } },
        { ConsensusLimit_trial_comment_score, {
            { NetworkMain, { {0, 300}, {1757000, 100} } },
            { NetworkTest, { {0, 300} } }
        } },
        
        { ConsensusLimit_full_post, {
            { NetworkMain, { {0, 30}, {1757000, 10}, {1791787, 30} } },
            { NetworkTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_video, {
            { NetworkMain, { {0, 30}, {1757000, 10}, {1791787, 30} } },
            { NetworkTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_article, {
            { NetworkMain, { {0, 3} } },
            { NetworkTest, { {0, 30} } }
        } },
        { ConsensusLimit_full_score, {
            { NetworkMain, { {0, 90}, {175600, 200}, {1757000, 60}, {1791787, 100}, {1873500, 200} } },
            { NetworkTest, { {0, 200} } }
        } },
        { ConsensusLimit_full_complain, {
            { NetworkMain, { {0, 12} } },
            { NetworkTest, { {0, 12} } }
        } },
        { ConsensusLimit_full_comment, {
            { NetworkMain, { {0, 300}, {1757000, 100}, {1791787, 200} } },
            { NetworkTest, { {0, 300} } }
        } },
        { ConsensusLimit_full_comment_score, {
            { NetworkMain, { {0, 600}, {1757000, 200}, {1873500, 300} } },
            { NetworkTest, { {0, 600} } }
        } },
        
        { ConsensusLimit_pro_video, {
            { NetworkMain, { {0, 0}, {1324655, 100} } },
            { NetworkTest, { {0, 100} } }
        } },
        
        { ConsensusLimit_moderation_flag_count, {
            { NetworkMain, { {0, 30} }},
            { NetworkTest, { {0, 100} }}
        }},

        { ConsensusLimit_post_edit_count, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } }
        } },
        { ConsensusLimit_video_edit_count, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } }
        } },
        { ConsensusLimit_article_edit_count, {
            { NetworkMain, { {0, 10} } },
            { NetworkTest, { {0, 10} } }
        } },
        { ConsensusLimit_comment_edit_count, {
            { NetworkMain, { {0, 4} } },
            { NetworkTest, { {0, 4} } }
        } },
        
        { ConsensusLimit_account_settings_daily_count, {
            { NetworkMain, { {0, 5} } },
            { NetworkTest, { {0, 5} } }
        } },

        { ConsensusLimit_multiple_lock_addresses_count, {
            { NetworkMain, { {0, 100} } }, // TODO (o1q): Set multiple lock addresses count
            { NetworkTest, { {0, 100} } }
        } },

        // Size
        { ConsensusLimit_max_user_size, {
            { NetworkMain, { {0, 2000} } },
            { NetworkTest, { {0, 2000} } }
        } },
        { ConsensusLimit_max_post_size, {
            { NetworkMain, { {0, 60000} } },
            { NetworkTest, { {0, 60000} } }
        } },
        { ConsensusLimit_max_video_size, {
            { NetworkMain, { {0, 60000} } },
            { NetworkTest, { {0, 60000} } }
        } },
        { ConsensusLimit_max_article_size, {
            { NetworkMain, { {0, 120000} } },
            { NetworkTest, { {0, 60000} } }
        } },
        { ConsensusLimit_max_comment_size, {
            { NetworkMain, { {0, 2000} } },
            { NetworkTest, { {0, 2000} } }
        } },
        { ConsensusLimit_max_account_setting_size, {
            { NetworkMain, { {0, 2048} } },
            { NetworkTest, { {0, 2048} } }
        } },
        
    };

    /*********************************************************************************************/
    class BaseConsensus
    {
    public:
        BaseConsensus();
        explicit BaseConsensus(int height);
        virtual ~BaseConsensus() = default;
        int64_t GetConsensusLimit(ConsensusLimit type) const;
    protected:
        int Height = 0;
    };

    /*********************************************************************************************/
    template<class T>
    struct ConsensusCheckpoint
    {
        int m_main_height;
        int m_test_height;
        function<shared_ptr<T>(int height)> m_func;

        [[nodiscard]] int Height(const string& networkId) const
        {
            if (networkId == CBaseChainParams::MAIN)
                return m_main_height;

            if (networkId == CBaseChainParams::TESTNET)
                return m_test_height;

            return m_main_height;
        }
    };

    /*********************************************************************************************/
}

#endif // POCKETCONSENSUS_BASE_H
