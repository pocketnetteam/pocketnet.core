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
#include "pocketdb/consensus/social/CommentEdit.hpp"
#include "pocketdb/consensus/social/CommentDelete.hpp"
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
        
        // Проверяет все консенсусные правила относительно генерируемой цепи
        static bool Validate(const PocketBlock& pBlock, int height)
        {
            for (auto tx : pBlock)
            {
                shared_ptr<SocialBaseConsensus> consensus;

                switch (*tx->GetType())
                {
                    case ACCOUNT_USER:
                        consensus = PocketConsensus::UserConsensusFactoryInst.Instance(height);
                        break;
                    case ACCOUNT_VIDEO_SERVER:
                        // TODO (brangr): implement
                        continue;
                    case ACCOUNT_MESSAGE_SERVER:
                        // TODO (brangr): implement
                        continue;
                    case CONTENT_POST:
                        consensus = PocketConsensus::PostConsensusFactoryInst.Instance(height);
                        break;
                    case CONTENT_VIDEO:
                        // TODO (brangr): implement
                        continue;
                    case CONTENT_TRANSLATE:
                        // TODO (brangr): implement
                        continue;
                    case CONTENT_SERVERPING:
                        // TODO (brangr): implement
                        continue;
                    case CONTENT_COMMENT:
                        consensus = PocketConsensus::CommentConsensusFactoryInst.Instance(height);
                        break;
                    case CONTENT_COMMENT_EDIT:
                        consensus = PocketConsensus::CommentEditConsensusFactoryInst.Instance(height);
                        break;
                    case CONTENT_COMMENT_DELETE:
                        consensus = PocketConsensus::CommentDeleteConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SCORE_POST:
                        consensus = PocketConsensus::ScorePostConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SCORE_COMMENT:
                        consensus = PocketConsensus::ScoreCommentConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SUBSCRIBE:
                        consensus = PocketConsensus::SubscribeConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SUBSCRIBE_PRIVATE:
                        consensus = PocketConsensus::SubscribePrivateConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_SUBSCRIBE_CANCEL:
                        consensus = PocketConsensus::SubscribeCancelConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_BLOCKING:
                        consensus = PocketConsensus::BlockingConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_BLOCKING_CANCEL:
                        consensus = PocketConsensus::BlockingCancelConsensusFactoryInst.Instance(height);
                        break;
                    case ACTION_COMPLAIN:
                        consensus = PocketConsensus::ComplainConsensusFactoryInst.Instance(height);
                        break;
                    default:
                        continue;
                }

                if (auto[ok, result] = consensus->Validate(tx, pBlock); !ok)
                {
                    LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                    return false;
                }
            }

            return true;
        }
    
        // Проверяет общие правила для транзакций без привязки к цепи
        static bool Check(PocketBlock& pBlock)
        {
            // todo (brangr): impletment
            return true;
        }

    };
}

#endif // POCKETCONSENSUS_SOCIAL_HPP
