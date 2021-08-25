// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_HELPER_H
#define POCKETCONSENSUS_HELPER_H

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/ReputationConsensus.h"

#include "pocketdb/consensus/social/Blocking.hpp"
#include "pocketdb/consensus/social/BlockingCancel.hpp"
#include "pocketdb/consensus/social/Comment.hpp"
#include "pocketdb/consensus/social/CommentEdit.hpp"
#include "pocketdb/consensus/social/CommentDelete.hpp"
#include "pocketdb/consensus/social/Complain.hpp"
#include "pocketdb/consensus/social/Post.hpp"
#include "pocketdb/consensus/social/ScoreComment.hpp"
#include "pocketdb/consensus/social/ScoreContent.hpp"
#include "pocketdb/consensus/social/Subscribe.hpp"
#include "pocketdb/consensus/social/SubscribeCancel.hpp"
#include "pocketdb/consensus/social/SubscribePrivate.hpp"
#include "pocketdb/consensus/social/User.hpp"
#include "pocketdb/consensus/social/Video.hpp"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    // This helper need for hide selector Consensus rules
    class SocialConsensusHelper
    {
    public:
        static bool Validate(const PocketBlockRef& block, int height);
        static tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, int height);
        static tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, PocketBlockRef& block, int height);
        // Проверяет блок транзакций без привязки к цепи
        static bool Check(const CBlock& block, const PocketBlockRef& pBlock);
        // Проверяет транзакцию без привязки к цепи
        static tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const PTransactionRef& ptx);
    protected:
        static tuple<bool, SocialConsensusResult> validate(const PTransactionRef& ptx, const PocketBlockRef& block, int height);
        static tuple<bool, SocialConsensusResult> check(const CTransactionRef& tx, const PTransactionRef& ptx);
        static bool isConsensusable(PocketTxType txType);
    private:
        static PostConsensusFactory m_postFactory;
        static UserConsensusFactory m_userFactory;
        static VideoConsensusFactory m_videoFactory;
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
    };
}

#endif // POCKETCONSENSUS_HELPER_H
