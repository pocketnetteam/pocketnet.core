#ifndef SRC_CONSENSUS_H
#define SRC_CONSENSUS_H

#include "pocketdb/consensus/Blocking.hpp"
#include "pocketdb/consensus/BlockingCancel.hpp"
#include "pocketdb/consensus/Comment.hpp"
#include "pocketdb/consensus/CommentDelete.hpp"
#include "pocketdb/consensus/Complain.hpp"
#include "pocketdb/consensus/Post.hpp"
#include "pocketdb/consensus/ScoreComment.hpp"
#include "pocketdb/consensus/ScorePost.hpp"
#include "pocketdb/consensus/Subscribe.hpp"
#include "pocketdb/consensus/SubscribeCancel.hpp"
#include "pocketdb/consensus/SubscribePrivate.hpp"
#include "pocketdb/consensus/User.hpp"
#include "pocketdb/consensus/Video.hpp"
#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Lottery.hpp"

namespace PocketConsensus
{
    extern BlockingConsensusFactory BlockingConsensusFactoryInst;
    extern BlockingCancelConsensusFactory BlockingCancelConsensusFactoryInst;
    extern CommentConsensusFactory CommentConsensusFactoryInst;
    extern CommentDeleteConsensusFactory CommentDeleteConsensusFactoryInst;
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
