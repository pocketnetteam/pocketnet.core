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
            for (auto tx : pBlock)
            {
                switch (*tx->GetType())
                {
                    case ACCOUNT_USER:
                    {
                        auto ptx = std::static_pointer_cast<User>(tx);
                        if (auto[ok, result] = PocketConsensus::UserConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACCOUNT_VIDEO_SERVER:
                        // TODO (brangr): implement
                        continue;
                    case ACCOUNT_MESSAGE_SERVER:
                        // TODO (brangr): implement
                        continue;
                    case CONTENT_POST:
                    {
                        auto ptx = std::static_pointer_cast<Post>(tx);
                        if (auto[ok, result] = PocketConsensus::PostConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
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
                    {
                        auto ptx = std::static_pointer_cast<Comment>(tx);
                        if (auto[ok, result] = PocketConsensus::CommentConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case CONTENT_COMMENT_DELETE:
                    {
                        auto ptx = std::static_pointer_cast<CommentDelete>(tx);
                        if (auto[ok, result] = PocketConsensus::CommentDeleteConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_SCORE_POST:
                    {
                        auto ptx = std::static_pointer_cast<ScorePost>(tx);
                        if (auto[ok, result] = PocketConsensus::ScorePostConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_SCORE_COMMENT:
                    {
                        auto ptx = std::static_pointer_cast<ScoreComment>(tx);
                        if (auto[ok, result] = PocketConsensus::ScoreCommentConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_SUBSCRIBE:
                    {
                        auto ptx = std::static_pointer_cast<Subscribe>(tx);
                        if (auto[ok, result] = PocketConsensus::SubscribeConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_SUBSCRIBE_PRIVATE:
                    {
                        auto ptx = std::static_pointer_cast<SubscribePrivate>(tx);
                        if (auto[ok, result] = PocketConsensus::SubscribePrivateConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_SUBSCRIBE_CANCEL:
                    {
                        auto ptx = std::static_pointer_cast<SubscribeCancel>(tx);
                        if (auto[ok, result] = PocketConsensus::SubscribeCancelConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_BLOCKING:
                    {
                        auto ptx = std::static_pointer_cast<Blocking>(tx);
                        if (auto[ok, result] = PocketConsensus::BlockingConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_BLOCKING_CANCEL:
                    {
                        auto ptx = std::static_pointer_cast<BlockingCancel>(tx);
                        if (auto[ok, result] = PocketConsensus::BlockingCancelConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    case ACTION_COMPLAIN:
                    {
                        auto ptx = std::static_pointer_cast<Complain>(tx);
                        if (auto[ok, result] = PocketConsensus::ComplainConsensusFactoryInst.Instance(height)->Validate(ptx, pBlock); !ok) {
                            LogPrintf("SocialConsensus %d failed with result %d for block height %d\n", (int)*tx->GetType(), (int)result, height);
                            return false;
                        }
                        break;
                    }
                    default:
                        continue;
                }
            }
        
            return true;
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_HPP
