// Copyright (c) 2018-2021 Pocketnet developers
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
        SocialConsensusResult_LowReputation = 16,
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
        SocialConsensusResult_OpReturnFailed = 41,
        SocialConsensusResult_CommentDeletedEdit = 42,
        SocialConsensusResult_ReferrerAfterRegistration = 43,
        SocialConsensusResult_NotAllowed = 44,
        SocialConsensusResult_AlreadyExists = 45,
        SocialConsensusResult_PayloadORNotFound = 46,
        SocialConsensusResult_TxORNotFound = 47,

    };

    enum AccountMode
    {
        AccountMode_Trial = 0,
        AccountMode_Full = 1,
        AccountMode_Pro = 2
    };

    enum ConsensusLimit
    {
        ConsensusLimit_threshold_reputation,
        ConsensusLimit_threshold_reputation_score,
        ConsensusLimit_threshold_balance,
        ConsensusLimit_threshold_balance_pro,
        ConsensusLimit_threshold_likers_count,
        ConsensusLimit_threshold_low_likers_count,
        ConsensusLimit_threshold_low_likers_depth,
        ConsensusLimit_depth,

        ConsensusLimit_trial_post,
        ConsensusLimit_trial_video,
        ConsensusLimit_trial_score,
        ConsensusLimit_trial_complain,
        ConsensusLimit_trial_comment,
        ConsensusLimit_trial_comment_score,

        ConsensusLimit_full_post,
        ConsensusLimit_full_video,
        ConsensusLimit_full_score,
        ConsensusLimit_full_complain,
        ConsensusLimit_full_comment,
        ConsensusLimit_full_comment_score,

        ConsensusLimit_pro_video,

        ConsensusLimit_post_edit_count,
        ConsensusLimit_video_edit_count,
        ConsensusLimit_comment_edit_count,

        ConsensusLimit_edit_post_depth,
        ConsensusLimit_edit_video_depth,
        ConsensusLimit_edit_comment_depth,
        ConsensusLimit_edit_user_depth,

        ConsensusLimit_max_user_size,
        ConsensusLimit_max_post_size,
        ConsensusLimit_max_comment_size,
        ConsensusLimit_scores_one_to_one,
        ConsensusLimit_scores_one_to_one_over_comment,
        ConsensusLimit_scores_one_to_one_depth,
        ConsensusLimit_scores_depth_modify_reputation,
        ConsensusLimit_lottery_referral_depth,
    };

    /*********************************************************************************************/
    // @formatter:off
    // Consensus limits

    // Reputation - double value in integer
    // i.e. 213 = 21.3
    // i.e. 45  = 4.5
    typedef map<ConsensusLimit, map<NetworkId, map<int, int64_t>>> ConsensusLimits;

    inline static ConsensusLimits m_consensus_limits = {
        // ConsensusLimit_threshold_reputation
        {
            ConsensusLimit_threshold_reputation,
            {
                {
                    NetworkMain,
                    {
                        {0,       500},
                        {292800,  1000}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 100}
                    }
                }
            }
        },
        // ConsensusLimit_threshold_reputation_score
        {
            ConsensusLimit_threshold_reputation_score,
            {
                {
                    NetworkMain,
                    {
                        {0,       -10000},
                        {108300,  500},
                        {292800,  1000}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 0},
                        {100000, 100}
                    }
                }
            }
        },
        // ConsensusLimit_threshold_balance
        {
            ConsensusLimit_threshold_balance,
            {
                {
                    NetworkMain,
                    {
                        {0,       50 * COIN}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 5 * COIN}
                    }
                }
            }
        },
        // ConsensusLimit_threshold_balance_pro
        {
            ConsensusLimit_threshold_balance_pro,
            {
                {
                    NetworkMain,
                    {
                        {0,       250 * COIN}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 25 * COIN}
                    }
                }
            }
        },
        // ConsensusLimit_threshold_likers_count
        {
            ConsensusLimit_threshold_likers_count,
            {
                {
                    NetworkMain,
                    {
                        {0,       0},
                        {1124000, 100}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 0},
                        {100000, 10}
                    }
                }
            }
        },
        // ConsensusLimit_threshold_low_likers_count
        {
            ConsensusLimit_threshold_low_likers_count,
            {
                {
                    NetworkMain,
                    {
                        {0,       30}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 30}
                    }
                }
            }
        },
        // ConsensusLimit_threshold_low_likers_depth
        {
            ConsensusLimit_threshold_low_likers_depth,
            {
                {
                    NetworkMain,
                    {
                        {1180000, 250'000},
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 250'000},
                    }
                }
            }
        },
        // ConsensusLimit_depth
        {
            ConsensusLimit_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       86400},
                        {1180000, 1440}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 1440}
                    }
                }
            }
        },

        // ConsensusLimit_trial_post
        {
            ConsensusLimit_trial_post,
            {
                {
                    NetworkMain,
                    {
                        {0,       15},
                        {1324655, 5}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 15}
                    }
                }
            }
        },
        // ConsensusLimit_trial_video
        {
            ConsensusLimit_trial_video,
            {
                {
                    NetworkMain,
                    {
                        {0,       15},
                        {1324655, 5}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 15}
                    }
                }
            }
        },
        // ConsensusLimit_trial_score
        {
            ConsensusLimit_trial_score,
            {
                {
                    NetworkMain,
                    {
                        {0,       45},
                        {175600,  100},
                        {1324655, 15}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 100}
                    }
                }
            }
        },
        // ConsensusLimit_trial_complain
        {
            ConsensusLimit_trial_complain,
            {
                {
                    NetworkMain,
                    {
                        {0,       6}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 6}
                    }
                }
            }
        },
        // ConsensusLimit_full_post
        {
            ConsensusLimit_full_post,
            {
                {
                    NetworkMain,
                    {
                        {0,       30}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 30}
                    }
                }
            }
        },
        // ConsensusLimit_full_video
        {
            ConsensusLimit_full_video,
            {
                {
                    NetworkMain,
                    {
                        {0,       30}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 30}
                    }
                }
            }
        },
        // ConsensusLimit_full_score
        {
            ConsensusLimit_full_score,
            {
                {
                    NetworkMain,
                    {
                        {0,       90},
                        {175600,  200}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 200}
                    }
                }
            }
        },
        // ConsensusLimit_full_complain
        {
            ConsensusLimit_full_complain,
            {
                {
                    NetworkMain,
                    {
                        {0,       12}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 12}
                    }
                }
            }
        },
        // ConsensusLimit_pro_video
        {
            ConsensusLimit_pro_video,
            {
                {
                    NetworkMain,
                    {
                        {0,       0},
                        {1324655, 100}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 0},
                        {65000,  100}
                    }
                }
            }
        },

        // ConsensusLimit_post_edit_count
        {
            ConsensusLimit_post_edit_count,
            {
                {
                    NetworkMain,
                    {
                        {0,       5}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 5}
                    }
                }
            }
        },
        // ConsensusLimit_video_edit_count
        {
            ConsensusLimit_video_edit_count,
            {
                {
                    NetworkMain,
                    {
                        {0,       5}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 5}
                    }
                }
            }
        },
        // ConsensusLimit_comment_edit_count
        {
            ConsensusLimit_comment_edit_count,
            {
                {
                    NetworkMain,
                    {
                        {0,       4}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 4}
                    }
                }
            }
        },

        // ConsensusLimit_edit_user_depth
        {
            ConsensusLimit_edit_user_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       3600},
                        {1180000, 60}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 30}
                    }
                }
            }
        },
        // ConsensusLimit_edit_post_depth
        {
            ConsensusLimit_edit_post_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       86400},
                        {1180000, 1440}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 1440}
                    }
                }
            }
        },
        // ConsensusLimit_edit_video_depth
        {
            ConsensusLimit_edit_video_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       1440}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 1440}
                    }
                }
            }
        },
        // ConsensusLimit_edit_comment_depth
        {
            ConsensusLimit_edit_comment_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       86400},
                        {1180000, 1440}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 1440}
                    }
                }
            }
        },

        // ConsensusLimit_max_user_size
        {
            ConsensusLimit_max_user_size,
            {
                {
                    NetworkMain,
                    {
                        {0,       2000}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 2000}
                    }
                }
            }
        },
        // ConsensusLimit_max_post_size
        {
            ConsensusLimit_max_post_size,
            {
                {
                    NetworkMain,
                    {
                        {0,       60000}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 60000}
                    }
                }
            }
        },
        // ConsensusLimit_scores_one_to_one
        {
            ConsensusLimit_scores_one_to_one,
            {
                {
                    NetworkMain,
                    {
                        {0,       99999},
                        {225000,  2}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 2}
                    }
                }
            }
        },
        // ConsensusLimit_scores_one_to_one_over_comment
        {
            ConsensusLimit_scores_one_to_one_over_comment,
            {
                {
                    NetworkMain,
                    {
                        {0,       20}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 20}
                    }
                }
            }
        },
        // ConsensusLimit_scores_one_to_one_depth
        {
            ConsensusLimit_scores_one_to_one_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       336 * 24 * 3600},
                        {225000,  1 * 24 * 3600},
                        {292800,  7 * 24 * 3600},
                        {322700, 2 * 24 * 3600}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 2 * 24 * 3600}
                    }
                }
            }
        },
        // ConsensusLimit_trial_comment
        {
            ConsensusLimit_trial_comment,
            {
                {
                    NetworkMain,
                    {
                        {0,       150}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 150}
                    }
                }
            }
        },
        // ConsensusLimit_trial_comment_score
        {
            ConsensusLimit_trial_comment_score,
            {
                {
                    NetworkMain,
                    {
                        {0,       300}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 300}
                    }
                }
            }
        },
        // ConsensusLimit_full_comment
        {
            ConsensusLimit_full_comment,
            {
                {
                    NetworkMain,
                    {
                        {0,       300}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 300}
                    }
                }
            }
        },
        // ConsensusLimit_full_comment_score
        {
            ConsensusLimit_full_comment_score,
            {
                {
                    NetworkMain,
                    {
                        {0,       600}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 600}
                    }
                }
            }
        },
        // ConsensusLimit_max_comment_size
        {
            ConsensusLimit_max_comment_size,
            {
                {
                    NetworkMain,
                    {
                        {0,       2000}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 2000}
                    }
                }
            }
        },
        // ConsensusLimit_scores_depth_modify_reputation
        {
            ConsensusLimit_scores_depth_modify_reputation,
            {
                {
                    NetworkMain,
                    {
                        {0,       336 * 24 * 3600},
                        {322700,  30 * 24 * 3600}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 30 * 24 * 3600}
                    }
                }
            }
        },
        // TODO (brangr) (v0.21.0): change GetLotteryReferralDepth Time to Height
        // ConsensusLimit_lottery_referral_depth
        {
            ConsensusLimit_lottery_referral_depth,
            {
                {
                    NetworkMain,
                    {
                        {0,       30 * 24 * 3600}
                    }
                },
                {
                    NetworkTest,
                    {
                        {0, 30 * 24 * 3600}
                    }
                }
            }
        },
    };

    // @formatter:on
    /*********************************************************************************************/

    class BaseConsensus
    {
    public:

        BaseConsensus()
        {
        }

        BaseConsensus(int height) : BaseConsensus()
        {
            Height = height;
        }

        virtual ~BaseConsensus() = default;

    protected:
        int Height = 0;

        int64_t GetConsensusLimit(ConsensusLimit type)
        {
            return (--m_consensus_limits[type][Params().NetworkID()].upper_bound(Height))->second;
        }

    private:

    };

    struct BaseConsensusCheckpoint
    {
        int m_main_height;
        int m_test_height;
        function<shared_ptr<BaseConsensus>(int height)> m_func;

        int Height(const string& networkId) const
        {
            if (networkId == CBaseChainParams::MAIN)
                return m_main_height;

            if (networkId == CBaseChainParams::TESTNET)
                return m_test_height;

            return m_main_height;
        }
    };
}

#endif // POCKETCONSENSUS_BASE_H
