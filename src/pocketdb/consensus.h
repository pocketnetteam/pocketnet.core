#ifndef SRC_CONSENSUS_H
#define SRC_CONSENSUS_H

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
#include "pocketdb/consensus/social/User.hpp"
#include "pocketdb/consensus/social/Video.hpp"
#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Lottery.hpp"

namespace PocketConsensus
{
    extern BlockingConsensusFactory BlockingConsensusFactoryInst;
    extern BlockingCancelConsensusFactory BlockingCancelConsensusFactoryInst;
    extern CommentConsensusFactory CommentConsensusFactoryInst;
    extern ComplainConsensusFactory ComplainConsensusFactoryInst;
    extern PostConsensusFactory PostConsensusFactoryInst;
    extern ScoreCommentConsensusFactory ScoreCommentConsensusFactoryInst;
    extern ScorePostConsensusFactory ScorePostConsensusFactoryInst;
    extern SubscribeConsensusFactory SubscribeConsensusFactoryInst;
    extern SubscribeCancelConsensusFactory SubscribeCancelConsensusFactoryInst;
    extern SubscribePrivateConsensusFactory SubscribePrivateConsensusFactoryInst;
    extern UserConsensusFactory UserConsensusFactoryInst;
    extern VideoConsensusFactory VideoConsensusFactoryInst;
    extern LotteryConsensusFactory LotteryConsensusFactoryInst;
    extern ReputationConsensusFactory ReputationConsensusFactoryInst;
}; // namespace PocketConsensus


#endif //SRC_CONSENSUS_H
