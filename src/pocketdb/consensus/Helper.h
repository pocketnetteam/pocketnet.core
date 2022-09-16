// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_HELPER_H
#define POCKETCONSENSUS_HELPER_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/consensus/Reputation.h"

#include "pocketdb/consensus/social/Blocking.hpp"
#include "pocketdb/consensus/social/BlockingCancel.hpp"
#include "pocketdb/consensus/social/BoostContent.hpp"
#include "pocketdb/consensus/social/Comment.hpp"
#include "pocketdb/consensus/social/CommentEdit.hpp"
#include "pocketdb/consensus/social/CommentDelete.hpp"
#include "pocketdb/consensus/social/Complain.hpp"
#include "pocketdb/consensus/social/Post.hpp"
#include "pocketdb/consensus/social/Video.hpp"
#include "pocketdb/consensus/social/Article.hpp"
#include "pocketdb/consensus/social/ScoreComment.hpp"
#include "pocketdb/consensus/social/ScoreContent.hpp"
#include "pocketdb/consensus/social/Subscribe.hpp"
#include "pocketdb/consensus/social/SubscribeCancel.hpp"
#include "pocketdb/consensus/social/SubscribePrivate.hpp"
#include "pocketdb/consensus/social/AccountUser.hpp"
#include "pocketdb/consensus/social/AccountSetting.hpp"
#include "pocketdb/consensus/social/AccountDelete.hpp"
#include "pocketdb/consensus/social/ContentDelete.hpp"

#include "pocketdb/consensus/moderation/Flag.hpp"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    // This helper need for hide selector Consensus rules
    class SocialConsensusHelper
    {
    public:
        static tuple<bool, SocialConsensusResult> Validate(const CBlock& block, const PocketBlockRef& pBlock, int height);
        static tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const PTransactionRef& ptx, int height);
        static tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const PTransactionRef& ptx, PocketBlockRef& pBlock, int height);
        static tuple<bool, SocialConsensusResult> Check(const CBlock& block, const PocketBlockRef& pBlock, int height);
        static tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const PTransactionRef& ptx, int height);
    protected:
        static tuple<bool, SocialConsensusResult> validate(const CTransactionRef& tx, const PTransactionRef& ptx, const PocketBlockRef& pBlock, int height);
        static tuple<bool, SocialConsensusResult> check(const CTransactionRef& tx, const PTransactionRef& ptx, int height);
        static bool isConsensusable(TxType txType);
    private:
        static AccountUserConsensusFactory m_accountUserFactory;
        static AccountSettingConsensusFactory m_accountSettingFactory;
        static AccountDeleteConsensusFactory m_accountDeleteFactory;
        static PostConsensusFactory m_postFactory;
        static VideoConsensusFactory m_videoFactory;
        static ArticleConsensusFactory m_articleFactory;
        static CommentConsensusFactory m_commentFactory;
        static CommentEditConsensusFactory m_commentEditFactory;
        static CommentDeleteConsensusFactory m_commentDeleteFactory;
        static ScoreContentConsensusFactory m_scoreContentFactory;
        static ScoreCommentConsensusFactory m_scoreCommentFactory;
        static SubscribeConsensusFactory m_subscribeFactory;
        static SubscribePrivateConsensusFactory m_subscribePrivateFactory;
        static SubscribeCancelConsensusFactory m_subscribeCancelFactory;
        static BlockingConsensusFactory m_blockingFactory;
        static BlockingCancelConsensusFactory m_blockingCancelFactory;
        static ComplainConsensusFactory m_complainFactory;
        static ContentDeleteConsensusFactory m_contentDeleteFactory;
        static BoostContentConsensusFactory m_boostContentFactory;
        
        static ModerationFlagConsensusFactory m_moderationFlagFactory;
    };
}

#endif // POCKETCONSENSUS_HELPER_H
