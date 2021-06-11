// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_HPP
#define POCKETCONSENSUS_SOCIAL_HPP

#include "pocketdb/models/base/Transaction.hpp"

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/consensus/social/User.hpp"
#include "pocketdb/consensus/social/Video.hpp"
#include "pocketdb/consensus/social/Blocking.hpp"
#include "pocketdb/consensus/social/BlockingCancel.hpp"
#include "pocketdb/consensus/social/Comment.hpp"
#include "pocketdb/consensus/social/Complain.hpp"
#include "pocketdb/consensus/social/Post.hpp"
#include "pocketdb/consensus/social/ScoreComment.hpp"
#include "pocketdb/consensus/social/ScorePost.hpp"
#include "pocketdb/consensus/social/Subscribe.hpp"
#include "pocketdb/consensus/social/SubscribeCancel.hpp"
#include "pocketdb/consensus/social/SubscribePrivate.hpp"

namespace PocketConsensus
{
    using std::string;
    using std::shared_ptr;
    using std::make_shared;
    using std::map;
    using std::make_tuple;
    using std::tuple;

    using namespace PocketTx;
    using namespace PocketDb;

    // This helper need for hide selector Consensus rules
    class SocialConsensusHelper
    {
    public:
        // All social models
        static bool Validate(PocketBlock& pBlock, int height)
        {
            map<PocketTxType, shared_ptr<SocialBaseConsensus>> mConsensus;

            // Build all needed consensus instances for this height
            for (auto tx : pBlock)
            {
                auto txType = tx->GetType();
                if (mConsensus.find(txType) != mConsensus.end())
                    continue;

                switch (txType)
                {
                    case ACCOUNT_USER:
                        mConsensus[txType] = PocketConsensus::UserConsensusFactoryInst.Instance(height);
                        break;
                    case ACCOUNT_VIDEO_SERVER:
                        // TODO (brangr): implement
                        break;
                    case ACCOUNT_MESSAGE_SERVER:
                        // TODO (brangr): implement
                        break;
                    case CONTENT_POST:
                        mConsensus[txType] = PocketConsensus::PostConsensusFactoryInst.Instance(height);
                        break;
                    case CONTENT_VIDEO:
                        mConsensus[txType] = PocketConsensus::VideoConsensusFactoryInst.Instance(height);
                        break;
                    case CONTENT_TRANSLATE:
                        // TODO (brangr): implement
                        break;
                    case CONTENT_SERVERPING:
                        // TODO (brangr): implement
                        break;
                    case CONTENT_COMMENT:
                        mConsensus[txType] = PocketConsensus::CommentConsensusFactoryInst.Instance(height);
                        break;
                    case CONTENT_COMMENT_DELETE:
                        mConsensus[txType] = PocketConsensus::CommentDeleteConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SCORE_POST:
                        mConsensus[txType] = PocketConsensus::ScorePostConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SCORE_COMMENT:
                        mConsensus[txType] = PocketConsensus::ScoreCommentConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SUBSCRIBE:
                        mConsensus[txType] = PocketConsensus::SubscribeConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SUBSCRIBE_PRIVATE:
                        mConsensus[txType] = PocketConsensus::SubscribePrivateConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SUBSCRIBE_CANCEL:
                        mConsensus[txType] = PocketConsensus::SubscribeCancelConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_BLOCKING:
                        mConsensus[txType] = PocketConsensus::BlockingConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_BLOCKING_CANCEL:
                        mConsensus[txType] = PocketConsensus::BlockingCancelConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_COMPLAIN:
                        mConsensus[txType] = PocketConsensus::ComplainConsensusFactoryInst.Instance(height);
                        break;
                    default:
                        return make_tuple<true, SocialConsensusResult::Success>();
                }
            }
        
            // Activate validation logic for all consensus instances
            bool validateResult = true;
            for (auto consensus : mConsensus)
            {
                if (auto[ok, result] = consensus.second.Validate(pBlock); !ok || result != SocialConsensusResult::Success)
                {
                    LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)consensus.first, (int)result, height);
                    validateResult &= result;
                }
            }

            return validateResult;
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_HPP
