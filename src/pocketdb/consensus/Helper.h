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
#include "pocketdb/consensus/social/Stream.hpp"
#include "pocketdb/consensus/social/Audio.hpp"
#include "pocketdb/consensus/social/Collection.hpp"
#include "pocketdb/consensus/social/App.hpp"
#include "pocketdb/consensus/social/ScoreComment.hpp"
#include "pocketdb/consensus/social/ScoreContent.hpp"
#include "pocketdb/consensus/social/Subscribe.hpp"
#include "pocketdb/consensus/social/SubscribeCancel.hpp"
#include "pocketdb/consensus/social/SubscribePrivate.hpp"
#include "pocketdb/consensus/social/account/AccountUser.hpp"
#include "pocketdb/consensus/social/account/AccountSetting.hpp"
#include "pocketdb/consensus/social/account/AccountDelete.hpp"
#include "pocketdb/consensus/social/ContentDelete.hpp"

#include "pocketdb/consensus/moderation/Flag.hpp"
#include "pocketdb/consensus/moderation/Vote.hpp"

#include "pocketdb/consensus/barteron/Offer.hpp"
#include "pocketdb/consensus/barteron/Account.hpp"

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

    };
}

#endif // POCKETCONSENSUS_HELPER_H
